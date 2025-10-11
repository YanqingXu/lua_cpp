# T028 协程标准库 - 快速启动指南

**创建日期**: 2025-10-11  
**用途**: 为开发者提供T028任务的快速启动参考  

---

## 🚀 一、立即开始

### 1.1 阅读顺序（15分钟快速理解）

```bash
# 1. 核心计划文档（必读）
cat specs/T028_COROUTINE_STDLIB_PLAN.md

# 2. 已有协程基础（了解基础设施）
cat src/vm/coroutine_support.h

# 3. T027标准库参考（学习架构模式）
cat T027_COMPLETION_REPORT.md
```

### 1.2 关键技术点速查

| 技术 | C++20特性 | Lua API | 目标性能 |
|------|-----------|---------|----------|
| **协程创建** | `co_await` | `coroutine.create(f)` | < 5μs |
| **协程恢复** | `handle.resume()` | `coroutine.resume(co, ...)` | < 100ns |
| **协程挂起** | `co_yield` | `coroutine.yield(...)` | < 100ns |
| **状态查询** | `promise_type` | `coroutine.status(co)` | < 10ns |
| **当前协程** | TLS | `coroutine.running()` | < 5ns |
| **协程包装** | lambda | `coroutine.wrap(f)` | < 50ns |

---

## 📁 二、文件结构

### 2.1 需要创建的文件

```
lua_cpp/
├── src/stdlib/
│   ├── coroutine_lib.h              # 🆕 协程库头文件（300-400行）
│   └── coroutine_lib.cpp            # 🆕 协程库实现（800-1000行）
├── tests/
│   ├── unit/
│   │   └── test_t028_coroutine_unit.cpp        # 🆕 单元测试（800行）
│   └── integration/
│       └── test_t028_coroutine_integration.cpp # 🆕 集成测试（400行）
├── specs/
│   └── T028_COROUTINE_STDLIB_PLAN.md    # ✅ 已创建 (本次)
└── T028_QUICK_START.md                  # ✅ 已创建 (本文件)
```

### 2.2 需要修改的文件

```
lua_cpp/
├── src/vm/
│   ├── enhanced_virtual_machine.h      # 🔧 添加协程库接口
│   └── enhanced_virtual_machine.cpp    # 🔧 集成协程库
├── src/stdlib/
│   └── stdlib.h                        # 🔧 导出协程库接口
└── CMakeLists.txt                      # 🔧 添加C++20支持检查
```

---

## 🎯 三、核心实现清单

### 3.1 Phase 1: 基础框架（Day 1上午，4-6小时）

#### ✅ Checklist

- [ ] **创建协程库头文件**
  ```bash
  touch src/stdlib/coroutine_lib.h
  code src/stdlib/coroutine_lib.h
  ```
  
  **关键内容**:
  - `LuaCoroutine` 类（C++20协程封装）
  - `promise_type` 定义
  - `YieldAwaiter` 和 `ResumeAwaiter`
  - `CoroutineLibrary` 类接口

- [ ] **CMakeLists.txt C++20支持**
  ```cmake
  # 添加C++20支持
  target_compile_features(lua_cpp PUBLIC cxx_std_20)
  
  # 检查协程支持
  include(CheckCXXSourceCompiles)
  check_cxx_source_compiles("
      #include <coroutine>
      int main() { return 0; }
  " HAS_COROUTINE)
  ```

- [ ] **基础单元测试框架**
  ```bash
  touch tests/unit/test_t028_coroutine_unit.cpp
  ```

### 3.2 Phase 2: 核心API（Day 1下午，6-8小时）

#### ✅ Checklist

- [ ] **`coroutine.create(f)`**
  - [ ] 函数验证
  - [ ] 协程对象创建
  - [ ] ID生成和存储
  - [ ] 单元测试

- [ ] **`coroutine.resume(co, ...)`**
  - [ ] 协程验证
  - [ ] 状态检查
  - [ ] 上下文切换
  - [ ] 错误处理
  - [ ] 单元测试

- [ ] **`coroutine.yield(...)`**
  - [ ] 协程上下文检查
  - [ ] `co_yield` 实现
  - [ ] 值传递
  - [ ] 单元测试

- [ ] **`coroutine.status(co)`**
  - [ ] 协程验证
  - [ ] 状态转换
  - [ ] 单元测试

- [ ] **`coroutine.running()`**
  - [ ] 当前协程跟踪
  - [ ] 主线程判断
  - [ ] 单元测试

- [ ] **`coroutine.wrap(f)`**
  - [ ] 创建协程
  - [ ] 包装函数生成
  - [ ] 错误传播
  - [ ] 单元测试

### 3.3 Phase 3: VM集成（Day 2上午，3-4小时）

#### ✅ Checklist

- [ ] **`EnhancedVirtualMachine` 修改**
  - [ ] 添加 `coroutine_lib_` 成员
  - [ ] 构造函数初始化
  - [ ] `GetCoroutineLibrary()` 接口
  - [ ] `RegisterStandardLibraries()` 注册

- [ ] **全局表注册**
  ```cpp
  // 在全局表中注册coroutine表
  global_table_->SetField("coroutine", coroutine_table);
  ```

- [ ] **集成测试编写**
  - [ ] Producer/Consumer模式
  - [ ] 嵌套协程
  - [ ] 协程间通信

### 3.4 Phase 4: 测试验证（Day 2下午，6-8小时）

#### ✅ Checklist

- [ ] **单元测试完成度**
  - [ ] 基础操作测试（10个）
  - [ ] 错误处理测试（5个）
  - [ ] 边界条件测试（5个）
  - [ ] C++20特性测试（3个）
  - [ ] 覆盖率达到 95%+

- [ ] **集成测试完成度**
  - [ ] 简单场景（5个）
  - [ ] 复杂场景（10个）
  - [ ] 性能场景（5个）

- [ ] **Lua兼容性测试**
  - [ ] Lua 5.1.5官方测试套件
  - [ ] 100%通过率

### 3.5 Phase 5: 性能优化（Day 3上午，2-3小时）

#### ✅ Checklist

- [ ] **协程池化**
  - [ ] `CoroutinePool` 实现
  - [ ] 对象复用逻辑
  - [ ] 性能提升验证

- [ ] **内存优化**
  - [ ] 栈空间预分配
  - [ ] 智能指针优化
  - [ ] 内存使用监控

- [ ] **编译优化**
  - [ ] 关键函数内联
  - [ ] 零成本抽象验证
  - [ ] 编译器优化选项

- [ ] **性能基准达标**
  - [ ] 创建 < 5μs ✅
  - [ ] Resume/Yield < 100ns ✅
  - [ ] 内存 < 1KB per coroutine ✅

### 3.6 Phase 6: 文档完善（Day 3下午，2-3小时）

#### ✅ Checklist

- [ ] **API文档**
  - [ ] Doxygen注释完整
  - [ ] 使用示例代码
  - [ ] 参数说明

- [ ] **完成报告**
  - [ ] `T028_COMPLETION_REPORT.md`
  - [ ] 技术亮点总结
  - [ ] 性能指标报告
  - [ ] 集成指南

- [ ] **更新项目文档**
  - [ ] `PROJECT_DASHBOARD.md`
  - [ ] `TODO.md`
  - [ ] `README.md`

---

## 🔧 四、关键代码模板

### 4.1 LuaCoroutine类骨架

```cpp
// src/stdlib/coroutine_lib.h

#pragma once

#include <coroutine>
#include <vector>
#include <memory>
#include "core/lua_value.h"

namespace lua_cpp {

class LuaCoroutine {
public:
    struct promise_type {
        CoroutineState state_ = CoroutineState::SUSPENDED;
        std::vector<LuaValue> yield_values_;
        std::vector<LuaValue> resume_values_;
        std::exception_ptr exception_;
        
        LuaCoroutine get_return_object();
        std::suspend_always initial_suspend() noexcept;
        std::suspend_always final_suspend() noexcept;
        void return_void();
        void unhandled_exception();
        auto yield_value(std::vector<LuaValue> values);
    };
    
    explicit LuaCoroutine(std::coroutine_handle<promise_type> handle);
    ~LuaCoroutine();
    
    // 禁用拷贝，允许移动
    LuaCoroutine(const LuaCoroutine&) = delete;
    LuaCoroutine& operator=(const LuaCoroutine&) = delete;
    LuaCoroutine(LuaCoroutine&&) noexcept;
    LuaCoroutine& operator=(LuaCoroutine&&) noexcept;
    
    std::vector<LuaValue> Resume(const std::vector<LuaValue>& args);
    CoroutineState GetState() const;
    bool IsDone() const;

private:
    std::coroutine_handle<promise_type> handle_;
};

} // namespace lua_cpp
```

### 4.2 CoroutineLibrary类骨架

```cpp
// src/stdlib/coroutine_lib.h (续)

class CoroutineLibrary : public LibraryModule {
public:
    explicit CoroutineLibrary(EnhancedVirtualMachine* vm);
    ~CoroutineLibrary() override = default;
    
    std::vector<LuaValue> CallFunction(
        const std::string& name,
        const std::vector<LuaValue>& args
    ) override;
    
    std::vector<std::string> GetFunctionNames() const override;
    
    // Lua API
    LuaValue Create(const LuaValue& func);
    std::vector<LuaValue> Resume(const LuaValue& co, const std::vector<LuaValue>& args);
    std::vector<LuaValue> Yield(const std::vector<LuaValue>& values);
    std::string Status(const LuaValue& co);
    LuaValue Running();
    LuaValue Wrap(const LuaValue& func);

private:
    EnhancedVirtualMachine* vm_;
    std::unordered_map<size_t, std::shared_ptr<LuaCoroutine>> coroutines_;
    size_t next_coroutine_id_ = 1;
    std::optional<size_t> current_coroutine_id_;
};
```

### 4.3 基础测试模板

```cpp
// tests/unit/test_t028_coroutine_unit.cpp

#include <catch2/catch_test_macros.hpp>
#include "stdlib/coroutine_lib.h"
#include "vm/enhanced_virtual_machine.h"

TEST_CASE("CoroutineLibrary - coroutine.create", "[coroutine][create]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto& lib = *vm->GetCoroutineLibrary();
    
    SECTION("Create with valid function") {
        auto func = CreateTestFunction();
        auto co = lib.Create(func);
        
        REQUIRE(co.IsCoroutine());
        REQUIRE(lib.Status(co) == "suspended");
    }
    
    SECTION("Create with invalid argument") {
        REQUIRE_THROWS_AS(
            lib.Create(LuaValue(42.0)),
            LuaError
        );
    }
}

TEST_CASE("CoroutineLibrary - coroutine.resume", "[coroutine][resume]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto& lib = *vm->GetCoroutineLibrary();
    
    SECTION("Resume simple coroutine") {
        auto func = CreateSimpleFunction();
        auto co = lib.Create(func);
        
        auto result = lib.Resume(co, {});
        
        REQUIRE(result[0].GetBoolean() == true);
        REQUIRE(lib.Status(co) == "dead");
    }
}

// ... 更多测试用例 ...
```

---

## ⚡ 五、性能基准

### 5.1 基准测试代码

```cpp
// tests/unit/test_t028_coroutine_benchmark.cpp

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>

TEST_CASE("Coroutine Performance Benchmarks", "[coroutine][benchmark]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto& lib = *vm->GetCoroutineLibrary();
    
    BENCHMARK("Coroutine creation") {
        return lib.Create(CreateTestFunction());
    };
    
    BENCHMARK("Resume/Yield cycle") {
        auto co = lib.Create(CreateYieldingFunction());
        return lib.Resume(co, {});
    };
    
    BENCHMARK("Wrap function call") {
        auto wrapped = lib.Wrap(CreateTestFunction());
        return wrapped.Call({});
    };
}
```

### 5.2 性能目标

| 操作 | 目标 | 测量方法 |
|------|------|----------|
| 创建协程 | < 5μs | `std::chrono::high_resolution_clock` |
| Resume | < 100ns | 循环1M次取平均 |
| Yield | < 100ns | 循环1M次取平均 |
| Status查询 | < 10ns | 内联优化验证 |

---

## 🐛 六、常见问题

### Q1: C++20协程编译器支持？

**A**: 需要以下编译器版本之一：
- GCC 10+ (`-std=c++20 -fcoroutines`)
- Clang 14+ (`-std=c++20`)
- MSVC 19.29+ (`/std:c++20`)

### Q2: 如何调试C++20协程？

**A**: 使用以下技巧：
```cpp
// 1. 添加调试宏
#define COROUTINE_DEBUG 1

// 2. promise_type中添加日志
struct promise_type {
    promise_type() {
        #ifdef COROUTINE_DEBUG
        std::cout << "Coroutine created\n";
        #endif
    }
};

// 3. 使用GDB协程插件
gdb -ex "py import libstdcxx.v6.printers"
```

### Q3: 如何与T026协程支持集成？

**A**: 两种方式：
1. **独立模式**: T028直接使用C++20协程
2. **集成模式**: T028作为T026的高层封装

建议使用**独立模式**，保持架构清晰。

### Q4: 性能优化的关键点？

**A**: 重点优化以下方面：
1. 避免不必要的内存分配
2. 使用移动语义而非拷贝
3. 内联关键函数
4. 协程对象池化

---

## 📚 七、参考资料快速链接

### 内部文档
- [T028详细计划](specs/T028_COROUTINE_STDLIB_PLAN.md)
- [T026完成报告](T026_COMPLETION_REPORT.md)
- [T027完成报告](T027_COMPLETION_REPORT.md)
- [协程支持头文件](src/vm/coroutine_support.h)

### 外部资源
- [C++20协程 - cppreference](https://en.cppreference.com/w/cpp/language/coroutines)
- [Lua 5.1.5协程文档](https://www.lua.org/manual/5.1/manual.html#2.11)
- [Lewis Baker协程教程](https://lewissbaker.github.io/)

---

## ✅ 八、最终验收清单

在完成T028之前，确保以下所有项目都已勾选：

### 功能完整性
- [ ] `coroutine.create(f)` 实现并测试
- [ ] `coroutine.resume(co, ...)` 实现并测试
- [ ] `coroutine.yield(...)` 实现并测试
- [ ] `coroutine.status(co)` 实现并测试
- [ ] `coroutine.running()` 实现并测试
- [ ] `coroutine.wrap(f)` 实现并测试

### 质量保证
- [ ] 单元测试覆盖率 ≥ 95%
- [ ] 集成测试场景 ≥ 20个
- [ ] Lua 5.1.5兼容性 100%
- [ ] 零编译警告
- [ ] 零内存泄漏（Valgrind验证）

### 性能达标
- [ ] 协程创建 < 5μs
- [ ] Resume/Yield < 100ns
- [ ] 内存开销 < 1KB per coroutine

### 文档完善
- [ ] API文档（Doxygen）
- [ ] 使用示例代码
- [ ] T028_COMPLETION_REPORT.md
- [ ] 更新PROJECT_DASHBOARD.md

---

## 🎯 九、开始开发！

准备好了吗？让我们开始T028协程标准库的开发！

```bash
# 1. 切换到项目目录
cd e:\Programming\spec-kit-lua\lua_cpp\

# 2. 创建功能分支
git checkout -b feature/T028-coroutine-stdlib

# 3. 开始Phase 1: 创建头文件
code src/stdlib/coroutine_lib.h

# 4. 运行测试（TDD模式）
cmake --build build --target test_t028_coroutine_unit
ctest -R coroutine -V
```

**祝开发顺利！🚀**

---

**文档版本**: 1.0  
**最后更新**: 2025-10-11  
**维护者**: lua_cpp项目团队
