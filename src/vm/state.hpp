#pragma once

#include "types.hpp"
#include "object/value.hpp"
#include "object/table.hpp"
#include "object/string.hpp"
#include "object/function.hpp"
#include "object/thread.hpp"
#include "gc/garbage_collector.hpp"
#include "vm/vm.hpp"

#include <functional>
#include <stdexcept>
#include <memory>

namespace Lua {

/**
 * @brief Lua 运行时错误抛出的异常
 */
class LuaException : public std::runtime_error {
public:
    explicit LuaException(const Str& message) : std::runtime_error(message) {}
};

/**
 * @brief 管理 Lua 实例的执行状态
 * 
 * State 类表示一个完整的 Lua 执行环境，包括
 * 值栈、全局变量和运行时配置。它提供了
 * 执行 Lua 代码并与 C++ 交互的主要接口。
 */
class State : public std::enable_shared_from_this<State> {
public:
    // 创建一个新的 Lua 状态
    static std::shared_ptr<State> create();
    
    // 析构函数
    ~State();
    
    // 打开标准库
    void openLibs();
    
    // 执行 Lua 代码
    i32 doString(const Str& code);
    i32 doFile(const Str& filename);
    
    // 栈操作
    void push(const Value& value);              // 将值推入栈中
    void pushNil();                             // 将 nil 推入栈中
    void pushBoolean(bool b);                   // 将布尔值推入栈中
    void pushNumber(double n);                  // 将数字推入栈中
    void pushString(const Str& s);              // 将字符串推入栈中
    void pushTable(Ptr<Table> table);           // 将表推入栈中
    void pushFunction(Ptr<Function> function);  // 将函数推入栈中
    void pushThread(Ptr<Thread> thread);        // 将线程推入栈中
    
    Value pop();                                    // 从栈中弹出值
    void pop(i32 n);                                // 从栈中弹出 n 个值
    Value peek(i32 index) const;                    // 获取栈中指定索引的值
    i32 getTop() const;                             // 获取当前栈大小
    void setTop(i32 index);                         // 设置栈大小
    bool checkStack(i32 n);                         // 确保栈有足够的空间容纳 n 个元素
    i32 absIndex(i32 index) const;                  // 将相对索引转换为绝对索引
    
    // 表操作
    void createTable(i32 narray = 0, i32 nrec = 0); // 创建一个新表并将其推入栈中
    void getTable(i32 index);                     // table[key] 其中 key 位于栈顶
    void setTable(i32 index);                     // table[key] = value 其中 key 和 value 位于栈顶
    void getField(i32 index, const Str& k);       // table[k]
    void setField(i32 index, const Str& k);       // table[k] = v 其中 v 位于栈顶
    void rawGetI(i32 index, i32 i);               // table[i] 不使用元方法
    void rawSetI(i32 index, i32 i);               // table[i] = v 不使用元方法
    
    // 全局变量
    void getGlobal(const Str& name);              // 将 _G[name] 推入栈中
    void setGlobal(const Str& name);              // _G[name] = v 其中 v 位于栈顶
    
    // 栈值类型检查
    bool isNil(i32 index) const;
    bool isBoolean(i32 index) const;
    bool isNumber(i32 index) const;
    bool isString(i32 index) const;
    bool isTable(i32 index) const;
    bool isFunction(i32 index) const;
    bool isUserData(i32 index) const;
    bool isThread(i32 index) const;
    
    // 栈值转换
    bool toBoolean(i32 index) const;
    double toNumber(i32 index) const;
    Str toString(i32 index) const;
    Ptr<Table> toTable(i32 index) const;
    Ptr<Function> toFunction(i32 index) const;
    Ptr<UserData> toUserData(i32 index) const;
    Ptr<Thread> toThread(i32 index) const;
    
    // C++ 函数管理
    using CFunction = std::function<int(State*)>;
    void registerFunction(const Str& name, CFunction func);
    i32 call(i32 nargs, i32 nresults);
    
    // 获取垃圾回收器
    GarbageCollector& gc() { return *m_gc; }
    
    // 错误处理
    void error(const Str& message);
    
    // 注册表（C++ 代码的私有存储）
    Ptr<Table> getRegistry() const { return m_registry; }
    
private:
    // 私有构造函数（请使用 create() 代替）
    State();
    
    // 初始化状态
    void initialize();
    
    // 栈实现
    Vec<Value> m_stack;
    i32 m_stackTop;
    
    // 全局环境和注册表
    Ptr<Table> m_globals;
    Ptr<Table> m_registry;
    
    // 垃圾回收器
    UniquePtr<GarbageCollector> m_gc;
    
    // 虚拟机
    Ptr<VM> m_vm;
    
    // 调用栈跟踪
    i32 m_callDepth;
    
    // 友元类
    friend class VM;
    friend class GarbageCollector;
};

} // namespace Lua
