---
description: "执行所有开发任务，构建完整的现代C++版Lua解释器"
---

# /implement - 开发任务执行

使用这个提示来执行开发任务并构建完整的现代C++版Lua解释器。

## 实施执行策略

### 前置条件验证
在开始执行之前，确保以下条件已满足：
- ✅ 项目宪法(constitution)已建立
- ✅ 功能规格(specification)已定义
- ✅ 技术方案(plan)已制定
- ✅ 开发任务(tasks)已分解
- ✅ lua_with_cpp基础代码已评估
- ✅ lua_c_analysis技术洞察已整理

### 执行原则和方法

#### 1. 测试驱动开发(TDD)
```cpp
// 每个功能的开发流程：
// 1. 先写失败的测试
TEST(ValueSystem, ShouldHandleModernCppTypes) {
    Value v = std::string("hello");
    EXPECT_EQ(v.type(), ValueType::String);
    EXPECT_EQ(std::get<std::string>(v), "hello");
}

// 2. 编写最小实现让测试通过
class Value {
    std::variant<std::monostate, bool, double, std::string> data_;
public:
    ValueType type() const { /* 实现 */ }
};

// 3. 重构优化代码质量
// 4. 重复循环直到功能完整
```

#### 2. 增量集成策略
- **基于lua_with_cpp**: 在现有80%功能基础上增量改进
- **参考lua_c_analysis**: 集成关键算法和优化技术  
- **保持兼容性**: 每次修改后运行兼容性测试
- **性能验证**: 每次优化后运行性能基准测试

#### 3. 并行开发协调
```
主线程 (Core Thread): VM核心 → 内存管理 → 集成测试
功能线程 (Feature Thread): 标准库 → API接口 → 文档
质量线程 (Quality Thread): 测试用例 → 性能基准 → 代码审查
```

## 分阶段执行计划

### 阶段1：基础架构现代化 (Week 1-2)

#### 执行任务：TASK-001 到 TASK-006

**Step 1: 构建系统升级**
```bash
# 执行CMake现代化
mkdir build && cd build
cmake -DCMAKE_CXX_STANDARD=20 \
      -DCMAKE_BUILD_TYPE=Debug \
      -DENABLE_TESTING=ON \
      -DENABLE_BENCHMARKS=ON ..
make -j$(nproc)
ctest --output-on-failure
```

**Step 2: 依赖管理配置**
```cmake
# CMakeLists.txt 现代化配置
cmake_minimum_required(VERSION 3.20)
project(ModernLuaInterpreter CXX)

# 使用CPM或Conan管理依赖
include(CPM.cmake)
CPMAddPackage("gh:google/googletest@1.14.0")
CPMAddPackage("gh:google/benchmark@1.8.0")

# 现代C++设置
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

**Step 3: 代码质量工具集成**
```yaml
# .github/workflows/ci.yml
name: CI
on: [push, pull_request]
jobs:
  build-and-test:
    runs-on: ${{ matrix.os }}
    strategy:
      matrix:
        os: [ubuntu-latest, windows-latest, macos-latest]
        compiler: [gcc, clang, msvc]
    steps:
      - uses: actions/checkout@v4
      - name: Configure
        run: cmake -B build -DCMAKE_CXX_COMPILER=${{ matrix.compiler }}
      - name: Build  
        run: cmake --build build --parallel
      - name: Test
        run: ctest --test-dir build --output-on-failure
      - name: Coverage
        run: gcov build/**/*.gcno && lcov --capture --directory build
```

**验收标准**:
- [ ] 所有平台编译通过 (Windows/Linux/macOS)
- [ ] 现有测试全部通过
- [ ] 代码覆盖率 > 95%
- [ ] 静态分析无警告
- [ ] CI/CD流水线正常运行

### 阶段2：核心引擎优化 (Week 3-6)

#### 执行任务：TASK-007 到 TASK-014

**Step 1: Value系统现代化**
```cpp
// 目标：创建类型安全、高性能的Value系统
class Value {
private:
    using ValueVariant = std::variant<
        std::monostate,           // nil
        bool,                     // boolean  
        double,                   // number
        std::string,              // string (考虑string_view优化)
        GCRef<Table>,            // table
        GCRef<Function>,         // function
        GCRef<UserData>          // userdata
    >;
    ValueVariant data_;

public:
    // 现代C++构造函数
    template<typename T>
    requires std::constructible_from<ValueVariant, T>
    Value(T&& value) : data_(std::forward<T>(value)) {}
    
    // 类型安全访问
    template<typename T>
    constexpr decltype(auto) get() const {
        return std::get<T>(data_);
    }
    
    // 访问者模式支持
    template<typename Visitor>
    constexpr decltype(auto) visit(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), data_);
    }
};
```

**Step 2: VM指令执行优化**
```cpp
// 目标：高效的指令执行引擎
class VirtualMachine {
private:
    std::vector<Value> registers_;
    std::vector<Instruction> instructions_;
    size_t pc_ = 0; // 程序计数器

public:
    // 优化的指令执行循环
    void execute() {
        while (pc_ < instructions_.size()) {
            const auto& instr = instructions_[pc_];
            
            // 使用跳转表优化指令分发
            switch (instr.opcode()) {
                case OpCode::MOVE:
                    registers_[instr.a()] = registers_[instr.b()];
                    break;
                case OpCode::LOADK:
                    registers_[instr.a()] = constants_[instr.bx()];
                    break;
                case OpCode::ADD:
                    executeArithmetic<std::plus<>>(instr);
                    break;
                // ... 其他指令
            }
            ++pc_;
        }
    }
    
private:
    // 模板化算术操作避免代码重复
    template<typename Op>
    void executeArithmetic(const Instruction& instr) {
        auto& ra = registers_[instr.a()];
        const auto& rb = getValue(instr.b());
        const auto& rc = getValue(instr.c());
        
        ra = Op{}(rb, rc); // 使用函数对象执行操作
    }
};
```

**Step 3: 内存管理现代化**
```cpp
// 目标：RAII风格的内存管理
template<typename T>
class GCRef {
private:
    std::shared_ptr<T> ptr_;
    
public:
    // 智能指针语义
    GCRef() = default;
    GCRef(std::shared_ptr<T> ptr) : ptr_(std::move(ptr)) {}
    
    // 自动类型转换
    operator bool() const { return ptr_ != nullptr; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_.get(); }
    
    // GC集成点
    void mark() const {
        if (ptr_) {
            GarbageCollector::instance().mark(ptr_.get());
        }
    }
};

// 现代化的垃圾回收器
class GarbageCollector {
private:
    std::vector<std::weak_ptr<GCObject>> objects_;
    bool collecting_ = false;
    
public:
    // 三色标记算法
    void collect() {
        if (collecting_) return; // 防止递归
        
        collecting_ = true;
        RAII_GUARD([this] { collecting_ = false; });
        
        // 标记阶段
        markRoots();
        
        // 清除阶段  
        sweep();
    }
    
private:
    void markRoots() {
        // 标记VM寄存器中的对象
        // 标记全局变量中的对象
        // 标记调用栈中的对象
    }
    
    void sweep() {
        // 清除未标记的对象
        objects_.erase(
            std::remove_if(objects_.begin(), objects_.end(),
                [](const auto& weak_ptr) { return weak_ptr.expired(); }),
            objects_.end()
        );
    }
};
```

**验收标准**:
- [ ] Value系统性能提升 > 20%
- [ ] VM指令执行性能提升 > 20%  
- [ ] 内存泄漏检测通过 (Valgrind/AddressSanitizer)
- [ ] 所有现有功能保持兼容
- [ ] 新增单元测试覆盖率 100%

### 阶段3：标准库完善 (Week 7-9)

#### 执行任务：TASK-015 到 TASK-020

**Step 1: 模块化标准库架构**
```cpp
// 目标：统一的模块接口和注册机制
class LibraryModule {
public:
    virtual ~LibraryModule() = default;
    virtual std::string_view name() const = 0;
    virtual void registerFunctions(VM& vm) = 0;
    virtual void registerConstants(VM& vm) = 0;
};

// 基础库实现
class BaseLibrary : public LibraryModule {
public:
    std::string_view name() const override { return "base"; }
    
    void registerFunctions(VM& vm) override {
        vm.registerGlobalFunction("print", &BaseLibrary::print);
        vm.registerGlobalFunction("type", &BaseLibrary::type);
        vm.registerGlobalFunction("tostring", &BaseLibrary::tostring);
        vm.registerGlobalFunction("tonumber", &BaseLibrary::tonumber);
        // ... 其他函数
    }
    
private:
    static Value print(VM& vm, std::span<const Value> args) {
        for (const auto& arg : args) {
            std::cout << toString(arg);
            if (&arg != &args.back()) std::cout << "\t";
        }
        std::cout << "\n";
        return Value{}; // nil
    }
    
    static Value type(VM& vm, std::span<const Value> args) {
        if (args.empty()) return Value{"nil"};
        
        return args[0].visit([](const auto& value) -> Value {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) return Value{"nil"};
            else if constexpr (std::is_same_v<T, bool>) return Value{"boolean"};
            else if constexpr (std::is_same_v<T, double>) return Value{"number"};
            else if constexpr (std::is_same_v<T, std::string>) return Value{"string"};
            else if constexpr (std::is_same_v<T, GCRef<Table>>) return Value{"table"};
            else if constexpr (std::is_same_v<T, GCRef<Function>>) return Value{"function"};
            else return Value{"userdata"};
        });
    }
};
```

**验收标准**:
- [ ] 所有Lua 5.1.5标准库函数实现完成
- [ ] 字符串操作性能提升 > 50%
- [ ] 数学库精度符合IEEE 754标准
- [ ] IO库跨平台兼容性测试通过
- [ ] 标准库功能测试 100%通过

### 阶段4：集成测试和文档 (Week 10-12)

#### 执行任务：TASK-021 到 TASK-026

**Step 1: Lua兼容性测试**
```cpp
// 目标：100% Lua 5.1.5兼容性
class CompatibilityTestSuite {
public:
    void runAllTests() {
        // 语法兼容性测试
        testLuaSyntax();
        
        // 语义兼容性测试  
        testLuaSemantics();
        
        // 标准库兼容性测试
        testStandardLibrary();
        
        // 错误处理兼容性测试
        testErrorHandling();
    }
    
private:
    void testLuaSyntax() {
        // 测试所有Lua 5.1.5语法构造
        const std::vector<std::string> test_scripts = {
            "test_variables.lua",
            "test_functions.lua", 
            "test_tables.lua",
            "test_control_flow.lua",
            "test_metatables.lua",
            "test_coroutines.lua"
        };
        
        for (const auto& script : test_scripts) {
            auto result = interpreter_.executeFile(script);
            EXPECT_TRUE(result.success()) << "Failed: " << script;
        }
    }
    
    void testStandardLibrary() {
        // 对比每个标准库函数的行为
        runCompatibilityTest("string.len('hello')", 5.0);
        runCompatibilityTest("math.abs(-42)", 42.0);
        runCompatibilityTest("table.getn({1,2,3})", 3.0);
        // ... 更多测试
    }
    
    template<typename Expected>
    void runCompatibilityTest(std::string_view code, Expected expected) {
        auto result = interpreter_.execute(code);
        EXPECT_EQ(result.getValue(), Value{expected});
    }
};
```

**验收标准**:
- [ ] Lua 5.1.5兼容性测试 100%通过
- [ ] 性能基准全部达标
- [ ] 内存安全验证无问题  
- [ ] API文档完整性检查通过
- [ ] 使用示例可运行验证通过

## 质量保证和持续集成

### 自动化测试流水线
```yaml
# 完整的CI/CD流水线
stages:
  - build
  - test
  - benchmark
  - compatibility
  - security
  - deploy

build_job:
  script:
    - cmake -B build -DCMAKE_BUILD_TYPE=Release
    - cmake --build build --parallel
    
test_job:
  script:
    - ctest --test-dir build --parallel --output-on-failure
    - gcov build/**/*.gcno
    - genhtml coverage.info -o coverage_report
    
benchmark_job:
  script:
    - ./build/benchmarks --benchmark_format=json > benchmark_results.json
    - python scripts/check_performance_regression.py
    
compatibility_job:
  script:
    - ./build/compatibility_tests
    - python scripts/compare_with_reference_lua.py
```

### 代码质量门禁
- **编译**: 零警告编译通过
- **测试**: 100%测试通过，95%+覆盖率
- **性能**: 无性能回归，基准达标
- **安全**: 静态分析和动态检测通过
- **兼容**: Lua 5.1.5兼容性验证通过

使用 `{SCRIPT}` 来执行实现脚本，跟踪进度和质量指标。

请按照以上执行计划，逐阶段实施现代C++版Lua解释器的开发，重点：
- 充分利用lua_with_cpp的80%现有功能基础
- 深度集成lua_c_analysis的核心算法和优化技术
- 严格执行测试驱动开发和质量保证流程
- 确保每个阶段都达到质量标准和性能目标

开始执行完整的现代C++版Lua解释器构建流程！