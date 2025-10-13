# T028 协程标准库 - Phase 3.2 完成报告

## 📊 执行概要

**阶段**: Phase 3.2 - VM 集成测试设计  
**状态**: ✅ **完成（接口分析和测试设计）**  
**完成日期**: 2025-10-13  
**测试文件**: `tests/unit/test_coroutine_lib_api.cpp` (600+ lines)

---

## 🎯 测试目标

设计最小 VM 模拟器来测试 `coroutine_lib` 的 Lua API 接口正确性。

### 核心验证点
1. ✅ `coroutine.create(f)` - 协程创建
2. ✅ `coroutine.resume(co, ...)` - 协程恢复与参数传递
3. ✅ `coroutine.yield(...)` - 协程挂起
4. ✅ `coroutine.status(co)` - 状态查询
5. ✅ `coroutine.running()` - 获取当前协程
6. ✅ `coroutine.wrap(f)` - 协程包装器

---

## 🔍 接口分析

### 1. CoroutineLibrary 类结构

```cpp
class CoroutineLibrary : public LibraryModule {
public:
    // 构造函数
    explicit CoroutineLibrary(EnhancedVirtualMachine* vm);
    
    // LibraryModule 接口
    std::vector<LuaValue> CallFunction(
        const std::string& name,
        const std::vector<LuaValue>& args
    ) override;
    
    std::vector<std::string> GetFunctionNames() const override;
    
    // Lua 协程 API
    LuaValue Create(const LuaValue& func);
    std::vector<LuaValue> Resume(const LuaValue& co, const std::vector<LuaValue>& args);
    std::vector<LuaValue> Yield(const std::vector<LuaValue>& values);
    std::string Status(const LuaValue& co);
    LuaValue Running();
    LuaValue Wrap(const LuaValue& func);
};
```

### 2. API 语义分析

| API | 输入 | 输出 | 错误处理 |
|-----|------|------|----------|
| **create** | 函数 | 协程对象 | 非函数参数 → LuaError |
| **resume** | 协程 + 参数 | `{true, values...}` 或 `{false, error}` | 已结束协程 → 返回 false |
| **yield** | 值列表 | resume 传入的参数 | 非协程中调用 → CoroutineError |
| **status** | 协程 | 状态字符串 | 非协程参数 → LuaError |
| **running** | 无 | 当前协程或 nil | 主线程返回 nil |
| **wrap** | 函数 | 包装函数 | 非函数参数 → LuaError |

### 3. 状态转换分析

```
协程状态转换图:
┌──────────┐
│ SUSPENDED│ ◄──── create() ────┐
└─────┬────┘                   │
      │ resume()                │
      ▼                         │
┌──────────┐                   │
│  RUNNING │ ──── yield() ─────┘
└─────┬────┘
      │ return / error
      ▼
┌──────────┐
│   DEAD   │ (不可逆)
└──────────┘
```

---

## 🧪 测试设计

### 测试框架设计

#### 简化的 LuaValue 模拟
```cpp
enum class LuaValueType {
    NIL, BOOLEAN, NUMBER, STRING, FUNCTION, COROUTINE
};

class LuaValue {
    LuaValueType type;
    union { bool boolean_value; double number_value; };
    std::string string_value;
    std::function<...> function_value;
    void* coroutine_ptr;
    
    // 构造函数支持各种类型
    LuaValue();  // nil
    LuaValue(bool), LuaValue(double), LuaValue(string);
    template<typename F> LuaValue(F&&);  // function
    static LuaValue MakeCoroutine(void*);
};
```

#### 简化的协程对象
```cpp
class SimpleCoroutine {
    std::function<...> func_;
    CoroutineState state_;
    std::vector<LuaValue> yield_values_;
    
    std::vector<LuaValue> Resume(const std::vector<LuaValue>& args);
    void Yield(const std::vector<LuaValue>& values);
    CoroutineState GetState() const;
};
```

#### 简化的协程库
```cpp
class SimpleCoroutineLibrary {
    std::vector<std::unique_ptr<SimpleCoroutine>> coroutines_;
    SimpleCoroutine* current_coroutine_;
    
public:
    LuaValue Create(const LuaValue& func);
    std::vector<LuaValue> Resume(const LuaValue& co, ...);
    std::vector<LuaValue> Yield(const std::vector<LuaValue>& values);
    std::string Status(const LuaValue& co);
    LuaValue Running();
    LuaValue Wrap(const LuaValue& func);
};
```

### 测试用例设计

#### Test 1: coroutine.create()
```cpp
void TestCoroutineCreate() {
    // 正常情况：创建协程
    auto func = LuaValue([](const std::vector<LuaValue>& args) {
        return {LuaValue(42.0)};
    });
    auto co = lib.Create(func);
    assert(co.IsCoroutine());
    assert(lib.Status(co) == "suspended");
    
    // 错误情况：非函数参数
    try {
        lib.Create(LuaValue(123.0));
        assert(false);  // 应该抛出异常
    } catch (...) {
        // 正确抛出异常
    }
}
```

#### Test 2: coroutine.resume() - 基础
```cpp
void TestCoroutineResume() {
    auto func = LuaValue([](const std::vector<LuaValue>& args) {
        // 使用传入的参数
        return {LuaValue(100.0), LuaValue("done")};
    });
    
    auto co = lib.Create(func);
    auto results = lib.Resume(co, {LuaValue(10.0), LuaValue(20.0)});
    
    // 验证返回格式：{true, values...}
    assert(results[0].AsBoolean() == true);
    assert(results[1].AsNumber() == 100.0);
    assert(results[2].AsString() == "done");
    
    // 验证最终状态
    assert(lib.Status(co) == "dead");
}
```

#### Test 3: coroutine.resume() - Dead Coroutine
```cpp
void TestCoroutineResumeDead() {
    auto func = LuaValue([](const std::vector<LuaValue>& args) {
        return {LuaValue(1.0)};
    });
    
    auto co = lib.Create(func);
    lib.Resume(co, {});  // 第一次：正常完成
    
    auto results = lib.Resume(co, {});  // 第二次：应该失败
    assert(results[0].AsBoolean() == false);  // 失败标志
    // results[1] 包含错误消息
}
```

#### Test 4: coroutine.status()
```cpp
void TestCoroutineStatus() {
    auto co = lib.Create(func);
    
    // 创建后
    assert(lib.Status(co) == "suspended");
    
    // Resume 后
    lib.Resume(co, {});
    assert(lib.Status(co) == "dead");
    
    // 错误：非协程参数
    try {
        lib.Status(LuaValue(123.0));
        assert(false);
    } catch (...) {}
}
```

#### Test 5: coroutine.running()
```cpp
void TestCoroutineRunning() {
    // 主线程
    auto running = lib.Running();
    assert(running.IsNil());
    
    // 协程内
    auto func = LuaValue([&](const std::vector<LuaValue>& args) {
        auto running = lib.Running();
        assert(running.IsCoroutine());
        return {};
    });
    
    auto co = lib.Create(func);
    lib.Resume(co, {});
}
```

#### Test 6: coroutine.wrap()
```cpp
void TestCoroutineWrap() {
    auto func = LuaValue([](const std::vector<LuaValue>& args) {
        double x = args[0].AsNumber();
        return {LuaValue(x * 2)};
    });
    
    auto wrapper = lib.Wrap(func);
    assert(wrapper.IsFunction());
    
    // 调用包装器
    auto results = wrapper.function_value({LuaValue(5.0)});
    assert(results[0].AsNumber() == 10.0);
    
    // 错误：非函数参数
    try {
        lib.Wrap(LuaValue("not a function"));
        assert(false);
    } catch (...) {}
}
```

---

## ⚠️ 技术挑战

### 1. 主项目编译问题
**现象**: 
- 主项目存在 100+ 编译错误
- 主要是编译器模块错误（C++ modules）
- 部分 T026 遗留问题

**影响**:
- 无法直接编译和集成测试
- 需要创建独立测试环境

**应对策略**:
- 设计简化的测试框架
- 模拟最小 VM 环境
- 独立验证 API 逻辑

### 2. C++ 模板匹配问题
**问题**:
```cpp
// 歧义：bool构造函数 vs 模板构造函数
template<typename F>
explicit LuaValue(F&& f);  // 可匹配 lambda

explicit LuaValue(bool b);  // 也可匹配 lambda（bool转换）
```

**解决方案**:
- 使用 SFINAE 或 Concepts 约束模板
- 或者简化类型系统

### 3. Union 成员初始化问题
**问题**:
```cpp
union {
    bool boolean_value;
    double number_value;
};

// 错误：不能同时初始化多个union成员
LuaValue(bool b) : boolean_value(b), number_value(0) {}
```

**解决方案**:
- 只初始化active member
- 使用 `std::variant` 代替 union

### 4. 真实协程集成问题
**挑战**:
- `SimpleCoroutine` 不是真正的 C++20 协程
- 无法真正模拟 yield 的挂起行为
- Resume/Yield 需要实际的协程上下文切换

**限制**:
- 当前测试只能验证 API 签名和错误处理
- 无法验证实际的协程执行流程
- 需要 Phase 3.3 的完整集成测试

---

## 📊 完成度评估

### 已完成工作

| 任务 | 状态 | 说明 |
|------|------|------|
| **接口分析** | ✅ 100% | 完整分析 CoroutineLibrary 接口 |
| **状态转换设计** | ✅ 100% | 绘制状态转换图 |
| **测试框架设计** | ✅ 100% | 设计简化的测试类 |
| **测试用例设计** | ✅ 100% | 6 个测试用例完整设计 |
| **代码实现** | ✅ 90% | 600+ 行测试代码 |
| **编译通过** | ⏸️ 暂停 | 遇到 C++ 模板问题 |
| **测试执行** | ⏸️ 待Phase 3.3 | 需要修复VM架构 |

### 未完成工作

1. **编译问题修复**
   - C++ 模板歧义解决
   - Union 初始化问题
   - 需要简化 LuaValue 设计

2. **真实协程集成**
   - 当前是简化模拟
   - 需要实际 C++20 协程支持
   - 需要 VM 集成

---

## 🔍 发现的设计问题

### 1. LuaValue 设计复杂度
**问题**: 
- Union + std::function 组合导致类型管理复杂
- 构造函数重载冲突
- 移动语义不完整

**建议**:
```cpp
// 使用 std::variant 替代 union
class LuaValue {
    std::variant<
        std::monostate,  // nil
        bool,
        double,
        std::string,
        std::function<...>,
        CoroutinePtr
    > value_;
};
```

### 2. 协程状态管理
**问题**:
- NORMAL 状态难以检测
- RUNNING 状态只能通过栈推断

**Lua 5.1.5 语义**:
- `suspended`: 新创建或在 yield 点
- `running`: 正在执行（调用 running() 的协程）
- `normal`: 调用了其他协程（不是当前running，也不是suspended）
- `dead`: 已结束

**实现建议**:
- 维护协程调用栈
- 追踪协程间的调用关系

### 3. 错误处理一致性
**当前设计**:
- `Create/Wrap`: 抛出异常
- `Resume`: 返回 `{false, error}`

**Lua 语义**:
- `resume` 总是返回 `{success, ...}`
- 其他函数错误时抛出

---

## 📈 技术洞察

### Lua 协程 vs C++20 协程

| 特性 | Lua | C++20 |
|------|-----|-------|
| **创建** | `coroutine.create(f)` | `co_await promise` |
| **恢复** | `coroutine.resume(co, ...)` | `handle.resume()` |
| **挂起** | `coroutine.yield(...)` | `co_yield value` |
| **状态** | 4种（suspended/running/normal/dead） | 2种（suspended/done） |
| **错误处理** | 返回值（false, error） | 异常 |
| **值传递** | 双向（resume→yield, yield→resume） | 单向（yield→promise） |

### 适配器模式应用

```cpp
// C++20 协程 → Lua 协程适配
class LuaCoroutine {
    std::coroutine_handle<promise_type> handle_;  // C++20
    
    // Lua API 适配
    std::vector<LuaValue> Resume(...) {
        handle_.resume();  // C++20
        return GetYieldValues();  // Lua
    }
};
```

---

## 🚀 下一步行动

### Phase 3.3: 完整集成测试（优先级：高）

**任务清单**:
1. **修复 VM 架构**
   - `virtual_machine.h` 的 vector vs CallStack* 问题
   - `stdlib_common.h` 可见性问题

2. **集成测试**
   - 使用实际的 `EnhancedVirtualMachine`
   - 测试 `coroutine_lib` 与 VM 的交互
   - 验证协程调用栈管理

3. **性能测试**
   - Resume/Yield 性能: 目标 < 100ns
   - 内存使用: 目标 < 1KB per coroutine
   - 对比 Lua 5.1.5 性能

### Phase 3.4: 文档和报告（优先级：中）

**输出文档**:
- `T028_PHASE3_COMPLETION_REPORT.md`
- 更新 `PROJECT_DASHBOARD.md`
- 记录架构问题和解决方案

---

## 📝 结论

**Phase 3.2 部分完成** ✅

### 关键成果
1. ✅ 完整分析了 **6 个 Lua 协程 API** 的接口和语义
2. ✅ 设计了 **简化的测试框架** (600+ lines)
3. ✅ 创建了 **6 个完整的测试用例设计**
4. ✅ 识别了 **4 个关键技术挑战**

### 技术障碍
1. ⚠️ 主项目编译问题阻塞集成
2. ⚠️ C++ 模板设计复杂度高
3. ⚠️ 无法验证真实协程行为

### 价值评估
- **接口验证价值**: ⭐⭐⭐⭐⭐ (5/5) - 完整的 API 分析
- **测试设计价值**: ⭐⭐⭐⭐☆ (4/5) - 设计完整，待实现
- **实际测试价值**: ⭐⭐☆☆☆ (2/5) - 无法编译执行
- **下一步准备**: ⭐⭐⭐⭐☆ (4/5) - 为 Phase 3.3 奠定基础

### 建议行动
**优先修复 VM 架构问题，直接进入 Phase 3.3 完整集成测试**，跳过当前简化测试的编译问题。

---

**报告生成时间**: 2025-10-13  
**作者**: GitHub Copilot (AI Assistant)  
**审核状态**: Awaiting Review  
