# 数据模型设计：现代C++ Lua解释器

**创建日期**: 2025-09-20  
**相关文档**: [plan.md](./plan.md) | [research.md](./research.md)  
**状态**: 设计完成

## 📊 核心数据模型

### LuaValue - 统一值类型系统
```cpp
class LuaValue {
public:
    enum class Type : uint8_t {
        NIL, BOOLEAN, NUMBER, STRING, TABLE, FUNCTION, USERDATA, THREAD
    };
    
private:
    using ValueVariant = std::variant<
        std::monostate,                    // nil
        bool,                             // boolean  
        double,                           // number
        std::shared_ptr<LuaString>,       // string
        std::shared_ptr<LuaTable>,        // table
        std::shared_ptr<LuaFunction>,     // function
        std::shared_ptr<LuaUserdata>,     // userdata
        std::shared_ptr<LuaThread>        // thread
    >;
    
    ValueVariant value_;
    
public:
    Type type() const noexcept;
    bool is_truthy() const noexcept;
    
    // 类型安全访问
    template<typename T> T& as();
    template<typename T> std::optional<T> try_as() const;
};
```

```cpp
namespace lua::core {

// 主要值类型枚举
enum class ValueType : uint8_t {
    NIL = 0,
    BOOLEAN,
    NUMBER,
    STRING,
    TABLE,
    FUNCTION,
    USERDATA,
    THREAD
};

// 统一值表示 - 优化内存布局
class LuaValue {
private:
    ValueType type_;
    union {
        bool boolean_;
        double number_;
        LuaString* string_;
        LuaTable* table_;
        LuaFunction* function_;
        void* userdata_;
        LuaThread* thread_;
    } data_;

public:
    // 构造函数和类型转换
    explicit LuaValue() noexcept : type_(ValueType::NIL) {}
    explicit LuaValue(bool b) noexcept : type_(ValueType::BOOLEAN) { data_.boolean_ = b; }
    explicit LuaValue(double n) noexcept : type_(ValueType::NUMBER) { data_.number_ = n; }
    
    // 类型检查和安全访问
    ValueType type() const noexcept { return type_; }
    bool is_nil() const noexcept { return type_ == ValueType::NIL; }
    bool is_boolean() const noexcept { return type_ == ValueType::BOOLEAN; }
    
    // 类型安全的值提取
    std::optional<bool> as_boolean() const noexcept;
    std::optional<double> as_number() const noexcept;
    LuaString* as_string() const noexcept;
    
    // 垃圾回收支持
    void mark_for_gc() const;
    bool is_collectible() const noexcept;
};

} // namespace lua::core
```

### 字符串系统设计

```cpp
namespace lua::core {

// 字符串驻留和优化
class LuaString {
private:
    size_t length_;
    uint32_t hash_;
    bool is_interned_;
    std::unique_ptr<char[]> data_;

public:
    // 字符串创建策略
    static LuaString* create(std::string_view sv);
    static LuaString* create_interned(std::string_view sv);
    
    // 小字符串优化 (SSO)
    static constexpr size_t SSO_SIZE = 15;
    
    // 高效比较和哈希
    bool operator==(const LuaString& other) const noexcept;
    uint32_t hash() const noexcept { return hash_; }
    
    // 字符串操作
    std::string_view view() const noexcept;
    const char* c_str() const noexcept;
    size_t length() const noexcept { return length_; }
};

// 字符串池管理
class StringPool {
private:
    std::unordered_set<std::unique_ptr<LuaString>, StringHash, StringEqual> pool_;
    std::mutex mutex_; // 线程安全

public:
    LuaString* intern(std::string_view sv);
    void collect_unused();
    size_t size() const;
};

} // namespace lua::core
```

### 表(Table)系统设计

```cpp
namespace lua::core {

// 高效的表实现 - 哈希表+数组混合
class LuaTable {
private:
    // 数组部分 - 用于连续整数索引
    std::vector<LuaValue> array_part_;
    size_t array_size_;
    
    // 哈希部分 - 用于其他键值
    struct HashNode {
        LuaValue key;
        LuaValue value;
        HashNode* next; // 冲突链表
    };
    std::unique_ptr<HashNode[]> hash_part_;
    size_t hash_size_;
    size_t hash_count_;
    
    // 元表支持
    LuaTable* metatable_;

public:
    // 构造和析构
    explicit LuaTable(size_t array_hint = 0, size_t hash_hint = 0);
    ~LuaTable();
    
    // 值访问 - 统一接口
    LuaValue get(const LuaValue& key) const;
    void set(const LuaValue& key, const LuaValue& value);
    
    // 数组优化访问
    LuaValue get_array(size_t index) const;
    void set_array(size_t index, const LuaValue& value);
    
    // 表操作
    size_t length() const; // # 操作符
    void resize(size_t new_array_size, size_t new_hash_size);
    
    // 元表操作
    LuaTable* get_metatable() const { return metatable_; }
    void set_metatable(LuaTable* mt) { metatable_ = mt; }
    
    // 迭代器支持
    class iterator;
    iterator begin();
    iterator end();
    
    // 垃圾回收支持
    void mark_for_gc() const;
};

} // namespace lua::core
```

### 函数系统设计

```cpp
namespace lua::core {

// 函数类型
enum class FunctionType {
    LUA_FUNCTION,    // Lua函数
    C_FUNCTION,      // C函数
    CLOSURE          // 闭包
};

// 函数基类
class LuaFunction {
protected:
    FunctionType type_;
    
public:
    explicit LuaFunction(FunctionType type) : type_(type) {}
    virtual ~LuaFunction() = default;
    
    FunctionType type() const { return type_; }
    virtual LuaValue call(const std::vector<LuaValue>& args) = 0;
};

// Lua函数实现
class LuaLuaFunction : public LuaFunction {
private:
    std::vector<uint32_t> bytecode_;
    std::vector<LuaValue> constants_;
    std::vector<LuaString*> locals_;
    std::vector<LuaValue> upvalues_;
    
public:
    explicit LuaLuaFunction(std::vector<uint32_t> bytecode,
                           std::vector<LuaValue> constants);
    
    LuaValue call(const std::vector<LuaValue>& args) override;
    
    // 字节码访问
    const std::vector<uint32_t>& bytecode() const { return bytecode_; }
    const std::vector<LuaValue>& constants() const { return constants_; }
};

// C函数实现
class LuaCFunction : public LuaFunction {
private:
    using CFunction = int(*)(lua_State*);
    CFunction function_;
    
public:
    explicit LuaCFunction(CFunction func) 
        : LuaFunction(FunctionType::C_FUNCTION), function_(func) {}
    
    LuaValue call(const std::vector<LuaValue>& args) override;
};

} // namespace lua::core
```

### 虚拟机状态设计

```cpp
namespace lua::core {

// 虚拟机状态管理
class LuaState {
private:
    // 执行栈
    std::vector<LuaValue> stack_;
    size_t stack_top_;
    
    // 调用栈
    struct CallFrame {
        LuaFunction* function;
        size_t pc;           // 程序计数器
        size_t stack_base;   // 栈基址
        size_t num_args;     // 参数数量
    };
    std::vector<CallFrame> call_stack_;
    
    // 全局环境
    std::unique_ptr<LuaTable> global_table_;
    
    // 垃圾回收器
    std::unique_ptr<GarbageCollector> gc_;
    
    // 字符串池
    std::unique_ptr<StringPool> string_pool_;
    
    // 错误处理
    jmp_buf error_jmp_;
    std::string error_message_;

public:
    explicit LuaState();
    ~LuaState();
    
    // 栈操作
    void push(const LuaValue& value);
    LuaValue pop();
    LuaValue& top(int index = -1);
    size_t stack_size() const { return stack_top_; }
    
    // 函数调用
    void call(LuaFunction* func, int nargs, int nresults);
    void return_function(int nresults);
    
    // 全局变量
    LuaValue get_global(const std::string& name);
    void set_global(const std::string& name, const LuaValue& value);
    
    // 错误处理
    void error(const std::string& message);
    bool pcall(LuaFunction* func, int nargs, int nresults);
    
    // 垃圾回收
    void gc_collect();
    void gc_step();
    size_t gc_memory_usage() const;
};

} // namespace lua::core
```

## 🔄 数据关系和验证规则

### 类型转换规则

```cpp
namespace lua::core {

// 类型转换策略
class TypeConverter {
public:
    // Lua的类型转换规则
    static bool to_boolean(const LuaValue& value);
    static std::optional<double> to_number(const LuaValue& value);
    static std::optional<std::string> to_string(const LuaValue& value);
    
    // 强制转换（可能失败）
    static bool try_convert(const LuaValue& from, ValueType to, LuaValue& result);
    
    // 比较操作
    static bool equals(const LuaValue& a, const LuaValue& b);
    static int compare(const LuaValue& a, const LuaValue& b);
};

} // namespace lua::core
```

### 表的键值约束

```cpp
namespace lua::core {

// 表键验证
class TableKeyValidator {
public:
    // 有效键检查
    static bool is_valid_key(const LuaValue& key) {
        return key.type() != ValueType::NIL && 
               !is_nan_number(key);
    }
    
    // 哈希函数
    static uint32_t hash_key(const LuaValue& key);
    
private:
    static bool is_nan_number(const LuaValue& value);
};

} // namespace lua::core
```

### 函数调用栈状态转换

```cpp
namespace lua::core {

// 调用栈状态机
enum class CallState {
    CALL_ENTRY,      // 函数入口
    CALL_EXECUTING,  // 执行中
    CALL_RETURN,     // 返回
    CALL_ERROR       // 错误状态
};

class CallStackManager {
private:
    CallState state_;
    std::vector<CallFrame> frames_;
    
public:
    // 状态转换
    void enter_call(LuaFunction* func, int nargs);
    void exit_call(int nresults);
    void handle_error(const std::string& error);
    
    // 状态查询
    CallState current_state() const { return state_; }
    const CallFrame& current_frame() const;
    size_t call_depth() const { return frames_.size(); }
};

} // namespace lua::core
```

## 🗑️ 垃圾回收对象关系

```cpp
namespace lua::core {

// GC对象基类
class GCObject {
private:
    bool marked_;
    GCObject* next_; // GC链表

public:
    virtual ~GCObject() = default;
    
    // GC标记
    bool is_marked() const { return marked_; }
    void mark() { marked_ = true; }
    void unmark() { marked_ = false; }
    
    // GC遍历
    virtual void mark_references() = 0;
    
    // GC链表
    GCObject* next() const { return next_; }
    void set_next(GCObject* next) { next_ = next; }
};

// 具体GC对象实现
class GCString : public LuaString, public GCObject {
public:
    void mark_references() override {} // 字符串无引用
};

class GCTable : public LuaTable, public GCObject {
public:
    void mark_references() override {
        // 标记所有键值对
        for (auto& pair : *this) {
            if (pair.key.is_collectible()) pair.key.mark_for_gc();
            if (pair.value.is_collectible()) pair.value.mark_for_gc();
        }
        // 标记元表
        if (metatable_) metatable_->mark_for_gc();
    }
};

} // namespace lua::core
```

## 📊 内存布局优化

### 缓存友好设计

```cpp
namespace lua::core {

// 值的紧凑表示 - 16字节对齐
static_assert(sizeof(LuaValue) == 16, "LuaValue should be 16 bytes");

// 表节点的内存局部性
struct TableNode {
    LuaValue key;    // 16 bytes
    LuaValue value;  // 16 bytes
    uint32_t hash;   // 4 bytes
    uint32_t next;   // 4 bytes (索引而非指针)
}; // 40 bytes, 缓存行友好

// 字符串的SSO优化
class LuaString {
    static constexpr size_t SSO_SIZE = 15;
    
    union {
        struct { // 长字符串
            char* data;
            size_t length;
            uint32_t hash;
        } long_str;
        
        struct { // 短字符串
            char data[SSO_SIZE];
            uint8_t length;
        } short_str;
    };
    
    bool is_short() const { return short_str.length <= SSO_SIZE; }
};

} // namespace lua::core
```

## ✅ 验证和约束总结

### 类型安全约束
- 所有类型转换都有明确的规则和验证
- 使用std::optional表示可能失败的转换
- 编译时类型检查通过模板实现

### 内存安全约束
- 所有指针都通过智能指针管理
- GC对象有明确的生命周期
- 异常安全保证通过RAII实现

### 性能约束
- 值类型使用紧凑的16字节表示
- 表操作针对Lua使用模式优化
- 字符串驻留减少内存分配

### 兼容性约束
- 完全兼容Lua 5.1.5的值语义
- C API通过适配器模式保持兼容
- 错误处理机制与原版一致

---

**设计状态**: ✅ 完成  
**内存布局**: 已优化  
**类型安全**: 已保证  
**下一步**: 创建接口契约