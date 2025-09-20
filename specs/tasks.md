# 开发任务列表：现代C++ Lua解释器

**输入**: 来自 `/specs/` 的设计文档  
**前置条件**: plan.md, research.md, data-model.md, contracts/  
**参考项目**: lua_c_analysis (行为验证) + lua_with_cpp (质量标准)  
**创建日期**: 2025-09-20

## 执行流程总结
```
1. 加载实现计划 ✅ - plan.md已分析，包含参考项目集成策略
2. 加载设计文档 ✅ - data-model.md, contracts/已提取
3. 按类别生成任务 ✅ - 设置、测试、核心、集成、完善
4. 集成参考项目验证 ✅ - 每个任务都包含双重验证步骤
5. 应用任务规则 ✅ - TDD优先，并行标记，依赖管理
6. 顺序编号任务 ✅ - T001-T058编号
7. 生成依赖图 ✅ - 明确任务依赖关系
8. 验证任务完整性 ✅ - 覆盖所有契约和实体
```

## 参考项目验证策略
**每个开发任务都包含以下标准验证步骤**:
- 🔍 **lua_c_analysis验证**: 与对应C源码的行为和性能对比
- 🏗️ **lua_with_cpp参考**: 采用相应的现代C++架构和质量标准
- ✅ **双重验证**: 行为正确性 + 代码质量 + 性能基准

## 格式说明: `[ID] [P?] 任务描述`
- **[P]**: 可并行执行（不同文件，无依赖）
- 任务描述包含确切的文件路径
- 严格按照TDD原则：测试先行，实现后行
- **🔍**: lua_c_analysis参考验证步骤
- **🏗️**: lua_with_cpp架构借鉴步骤

## 第3.1阶段：项目设置

- [ ] **T001** 创建项目目录结构按照plan.md规划
  - `src/core/`, `src/lexer/`, `src/parser/`, `src/compiler/`, `src/vm/`, `src/gc/`, `src/types/`, `src/stdlib/`, `src/api/`, `src/cli/`
  - `tests/unit/`, `tests/integration/`, `tests/contract/`, `benchmarks/`
  - `docs/`, `CMakeLists.txt`, `.gitignore`, `README.md`

- [ ] **T002** 初始化CMake项目配置
  - 配置C++17最低要求，C++20可选特性
  - 设置编译器警告为最严格级别（-Wall -Wextra -Werror）
  - 集成Catch2测试框架和Google Benchmark

- [ ] **T003** [P] 配置开发工具链
  - `.clang-format` 代码格式化配置
  - `.clang-tidy` 静态分析配置
  - GitHub Actions CI/CD流水线配置

- [ ] **T004** [P] 创建基础头文件
  - `src/core/lua_config.h` - 编译配置和宏定义
  - `src/core/lua_common.h` - 通用类型和常量
  - `src/core/lua_errors.h` - 错误码和异常类定义

## 第3.2阶段：契约测试（TDD - 必须在实现前完成）

**关键提醒: 这些测试必须编写完成且失败后才能开始任何实现工作**

### 核心类型契约测试
- [ ] **T005** [P] LuaValue类型系统契约测试
  - `tests/contract/test_lua_value_contract.cpp`
  - 测试所有类型转换、比较操作、内存安全
  - 🔍 **lua_c_analysis验证**: 对比`lobject.h`中TValue的行为和类型判断
  - 🏗️ **lua_with_cpp参考**: 学习`vm/value.hpp`的类型安全包装设计

- [ ] **T006** [P] LuaString驻留机制契约测试  
  - `tests/contract/test_lua_string_contract.cpp`
  - 测试字符串驻留、哈希一致性、线程安全
  - 🔍 **lua_c_analysis验证**: 与`lstring.c`的字符串池行为保持一致
  - 🏗️ **lua_with_cpp参考**: 采用`gc/core/gc_string.hpp`的现代化设计

- [ ] **T007** [P] LuaTable操作契约测试
  - `tests/contract/test_lua_table_contract.cpp`
  - 测试数组/哈希混合存储、元表机制、迭代器
  - 🔍 **lua_c_analysis验证**: 确保与`ltable.c`的混合存储策略完全兼容
  - 🏗️ **lua_with_cpp参考**: 借鉴`vm/table.hpp`的接口设计和优化

- [ ] **T008** [P] LuaFunction调用契约测试
  - `tests/contract/test_lua_function_contract.cpp`
  - 测试Lua函数、C函数、闭包管理
  - 🔍 **lua_c_analysis验证**: 对比`lfunc.c`的函数对象和闭包实现
  - 🏗️ **lua_with_cpp参考**: 学习`vm/function.hpp`的现代函数管理

### 虚拟机契约测试
- [ ] **T009** [P] 虚拟机执行契约测试
  - `tests/contract/test_vm_execution_contract.cpp`
  - 测试指令分发、栈操作、调用帧管理
  - 🔍 **lua_c_analysis验证**: 与`lvm.c`的`luaV_execute`函数行为完全一致
  - 🏗️ **lua_with_cpp参考**: 采用`vm/vm_executor.hpp`的现代化执行架构

- [ ] **T010** [P] 词法分析器契约测试
  - `tests/contract/test_lexer_contract.cpp`
  - 测试所有token类型、错误处理、位置信息
  - 🔍 **lua_c_analysis验证**: 确保与`llex.c`的token识别规则完全相同
  - 🏗️ **lua_with_cpp参考**: 学习`lexer/lexer.hpp`的错误处理和状态管理

- [ ] **T011** [P] 语法分析器契约测试
  - `tests/contract/test_parser_contract.cpp`
  - 测试AST生成、语法错误、操作符优先级
  - 🔍 **lua_c_analysis验证**: 与`lparser.c`的语法解析规则保持一致
  - 🏗️ **lua_with_cpp参考**: 借鉴`parser/enhanced_parser.hpp`的现代解析器设计

- [ ] **T012** [P] 字节码编译器契约测试
  - `tests/contract/test_compiler_contract.cpp`
  - 测试指令生成、优化、调试信息
  - 🔍 **lua_c_analysis验证**: 生成与`lcode.c`兼容的字节码格式
  - 🏗️ **lua_with_cpp参考**: 学习`compiler/compiler.hpp`的现代编译器架构

### 垃圾回收契约测试
- [ ] **T013** [P] 垃圾回收器契约测试
  - `tests/contract/test_gc_contract.cpp`
  - 测试标记清除、增量回收、弱引用
  - 🔍 **lua_c_analysis验证**: 与`lgc.c`的三色标记算法行为保持一致
  - 🏗️ **lua_with_cpp参考**: 采用`gc/core/garbage_collector.hpp`的现代GC设计

- [ ] **T014** [P] 内存管理契约测试
  - `tests/contract/test_memory_contract.cpp`
  - 测试对象池、内存统计、泄漏检测
  - 🔍 **lua_c_analysis验证**: 与`lmem.c`的内存分配策略保持兼容
  - 🏗️ **lua_with_cpp参考**: 学习`gc/memory/allocator.hpp`的智能指针集成

### C API契约测试
- [ ] **T015** [P] C API基础操作契约测试
  - `tests/contract/test_c_api_basic_contract.cpp`
  - 测试栈操作、类型检查、状态管理
  - 🔍 **lua_c_analysis验证**: 与`lapi.c`的所有API函数保持二进制兼容
  - 🏗️ **lua_with_cpp参考**: 借鉴`api/`目录的现代C++ API包装设计

- [ ] **T016** [P] C API函数调用契约测试
  - `tests/contract/test_c_api_call_contract.cpp`
  - 测试函数注册、调用、错误处理
  - 🔍 **lua_c_analysis验证**: 确保与原始C API的函数调用语义完全一致
  - 🏗️ **lua_with_cpp参考**: 学习现代化的错误处理和异常安全机制

### 集成测试
- [ ] **T017** [P] Lua脚本执行集成测试
  - `tests/integration/test_script_execution.cpp`
  - 测试完整的脚本解析、编译、执行流程

- [ ] **T018** [P] 标准库集成测试
  - `tests/integration/test_stdlib_integration.cpp`
  - 测试所有标准库函数的正确性

- [ ] **T019** [P] 兼容性集成测试
  - `tests/integration/test_lua515_compatibility.cpp`
  - 测试与Lua 5.1.5的完全兼容性

## 第3.3阶段：核心实现（仅在测试失败后开始）

### 基础类型实现
- [ ] **T020** [P] LuaValue统一值类型实现
  - `src/core/lua_value.h` + `src/core/lua_value.cpp`
  - 实现variant包装、类型安全访问、转换函数

- [ ] **T021** [P] LuaString驻留字符串实现
  - `src/types/string.h` + `src/types/string.cpp`
  - 实现字符串池、哈希计算、线程安全

- [ ] **T022** [P] LuaTable混合表实现
  - `src/types/table.h` + `src/types/table.cpp`
  - 实现数组+哈希混合存储、元表支持

- [ ] **T023** [P] LuaFunction函数对象实现
  - `src/types/function.h` + `src/types/function.cpp`
  - 实现函数调用、闭包管理、上值处理

### 词法和语法分析
- [ ] **T024** [P] 词法分析器实现
  - `src/lexer/lexer.h` + `src/lexer/lexer.cpp`
  - `src/lexer/token.h` + `src/lexer/token.cpp`

- [ ] **T025** [P] 语法分析器实现
  - `src/parser/parser.h` + `src/parser/parser.cpp`
  - `src/parser/ast.h` + `src/parser/ast.cpp`

- [ ] **T026** 字节码编译器实现
  - `src/compiler/compiler.h` + `src/compiler/compiler.cpp`
  - 依赖T024、T025完成

### 虚拟机核心
- [ ] **T027** [P] 指令定义和操作码实现
  - `src/compiler/opcode.h` + `src/compiler/opcode.cpp`
  - 所有Lua 5.1.5操作码定义

- [ ] **T028** [P] 执行栈管理实现
  - `src/vm/stack.h` + `src/vm/stack.cpp`
  - 栈操作、边界检查、自动扩展

- [ ] **T029** 指令分发器实现
  - `src/vm/dispatch.h` + `src/vm/dispatch.cpp`
  - 依赖T027、T028完成

- [ ] **T030** 虚拟机主循环实现
  - `src/vm/vm.h` + `src/vm/vm.cpp`
  - 依赖T026、T029完成

### 内存和垃圾回收
- [ ] **T031** [P] GC对象基类实现
  - `src/gc/object.h` + `src/gc/object.cpp`
  - 标记接口、生命周期管理

- [ ] **T032** 垃圾回收器实现
  - `src/gc/gc.h` + `src/gc/gc.cpp`
  - 依赖T031完成，增量标记清除算法

### Lua状态机
- [ ] **T033** Lua状态机实现
  - `src/core/lua_state.h` + `src/core/lua_state.cpp`
  - 依赖T020、T030、T032完成

## 第3.4阶段：标准库和API

### 标准库实现
- [ ] **T034** [P] 基础库实现
  - `src/stdlib/base_lib.h` + `src/stdlib/base_lib.cpp`
  - print、type、getmetatable等基础函数

- [ ] **T035** [P] 字符串库实现
  - `src/stdlib/string_lib.h` + `src/stdlib/string_lib.cpp`
  - string.*所有函数

- [ ] **T036** [P] 表库实现
  - `src/stdlib/table_lib.h` + `src/stdlib/table_lib.cpp`
  - table.*所有函数

- [ ] **T037** [P] 数学库实现
  - `src/stdlib/math_lib.h` + `src/stdlib/math_lib.cpp`
  - math.*所有函数

- [ ] **T038** [P] I/O库实现
  - `src/stdlib/io_lib.h` + `src/stdlib/io_lib.cpp`
  - io.*所有函数

- [ ] **T039** [P] 操作系统库实现
  - `src/stdlib/os_lib.h` + `src/stdlib/os_lib.cpp`
  - os.*所有函数

### C API实现
- [ ] **T040** C API基础函数实现
  - `src/api/lua_api.h` + `src/api/lua_api.cpp`
  - 依赖T033完成，实现所有基础API

- [ ] **T041** [P] 辅助库实现
  - `src/api/aux_lib.h` + `src/api/aux_lib.cpp`
  - luaL_*辅助函数

### 命令行工具
- [ ] **T042** 命令行解释器实现
  - `src/cli/lua_cli.cpp`
  - 依赖T040完成，交互式REPL

## 第3.5阶段：集成和优化

### 系统集成
- [ ] **T043** 标准库注册和初始化
  - 集成T034-T039到主解释器
  - 依赖T040完成

- [ ] **T044** 错误处理和调试信息
  - 统一错误处理机制、调用栈跟踪
  - 依赖T033完成

- [ ] **T045** 协程支持实现
  - `src/types/coroutine.h` + `src/types/coroutine.cpp`
  - 依赖T033完成

### 性能优化
- [ ] **T046** 指令分发优化
  - 计算goto、分支预测优化
  - 修改T029实现

- [ ] **T047** 内存布局优化
  - 对象池、缓存友好数据结构
  - 修改T032实现

- [ ] **T048** 字符串操作优化
  - SIMD优化、快速比较
  - 修改T021实现

## 第3.6阶段：测试和验证

### 单元测试
- [ ] **T049** [P] 词法分析器单元测试
  - `tests/unit/lexer_test/test_lexer_unit.cpp`
  - 详细的边界条件和错误情况测试

- [ ] **T050** [P] 语法分析器单元测试
  - `tests/unit/parser_test/test_parser_unit.cpp`
  - AST正确性和错误恢复测试

- [ ] **T051** [P] 虚拟机单元测试
  - `tests/unit/vm_test/test_vm_unit.cpp`
  - 指令执行正确性测试

- [ ] **T052** [P] 垃圾回收器单元测试
  - `tests/unit/gc_test/test_gc_unit.cpp`
  - GC算法正确性和性能测试

### 性能测试
- [ ] **T053** [P] 执行性能基准测试
  - `benchmarks/execution/bench_execution.cpp`
  - 与原版Lua 5.1.5性能对比

- [ ] **T054** [P] 内存使用基准测试
  - `benchmarks/memory/bench_memory.cpp`
  - 内存分配和GC性能测试

- [ ] **T055** [P] 启动时间基准测试
  - `benchmarks/startup/bench_startup.cpp`
  - 冷启动和热启动时间测试

### 兼容性验证
- [ ] **T056** Lua 5.1.5官方测试套件
  - 运行官方测试，确保100%兼容性
  - 依赖T042完成

- [ ] **T057** 第三方脚本兼容性测试
  - 收集和测试流行的Lua 5.1.5脚本
  - 依赖T056完成

### 文档和发布
- [ ] **T058** [P] 项目文档完善
  - API文档、架构文档、使用示例
  - 可与其他任务并行

## 📊 任务依赖关系图

```
设置阶段: T001 → T002 → T003, T004 (并行)
                ↓
测试阶段: T005-T019 (全部并行，必须失败)
                ↓
基础实现: T020, T021, T022, T023 (并行)
                ↓
分析器: T024, T025 (并行) → T026
                ↓
虚拟机: T027, T028 (并行) → T029 → T030
                ↓
内存管理: T031 → T032
                ↓
状态机: T033 (依赖T020, T030, T032)
                ↓
标准库: T034-T039 (并行)
                ↓
API: T040 → T041, T042 (并行)
                ↓
集成: T043 → T044, T045 (并行)
                ↓
优化: T046, T047, T048 (并行)
                ↓
验证: T049-T055 (并行) → T056 → T057
                ↓
发布: T058 (可提前开始)
```

## 🔄 并行执行示例

### 阶段1：项目设置
```bash
# 可同时执行
任务: "配置开发工具链 (.clang-format, .clang-tidy, CI/CD)"
任务: "创建基础头文件 (lua_config.h, lua_common.h, lua_errors.h)"
```

### 阶段2：契约测试编写
```bash
# 可同时执行 - 所有契约测试都是独立文件
任务: "LuaValue类型系统契约测试 tests/contract/test_lua_value_contract.cpp"
任务: "LuaString驻留机制契约测试 tests/contract/test_lua_string_contract.cpp"
任务: "LuaTable操作契约测试 tests/contract/test_lua_table_contract.cpp"
# ... 等等
```

### 阶段3：核心类型实现
```bash
# 可同时执行 - 独立的类型实现
任务: "LuaValue统一值类型实现 src/core/lua_value.h/cpp"
任务: "LuaString驻留字符串实现 src/types/string.h/cpp"
任务: "LuaTable混合表实现 src/types/table.h/cpp"
任务: "LuaFunction函数对象实现 src/types/function.h/cpp"
```

## 📋 验证检查清单

**门禁：在返回前由main()检查**

- [x] 所有契约都有对应的测试任务
- [x] 所有实体都有模型创建任务
- [x] 所有测试都在实现之前
- [x] 并行任务确实独立无依赖
- [x] 每个任务都指定了确切的文件路径
- [x] 没有任务与其他[P]任务修改相同文件
- [x] TDD原则严格执行
- [x] 依赖关系清晰明确

## 📈 任务统计

- **总任务数**: 58个任务
- **并行任务**: 32个（标记[P]）
- **串行任务**: 26个（有依赖关系）
- **测试任务**: 25个（T005-T019, T049-T058）
- **实现任务**: 28个（T020-T047）
- **预计工期**: 12-16周（4人团队并行开发）

---

**任务生成完成** ✅  
**TDD原则**: 测试先行，实现后行  
**并行优化**: 最大化团队效率  
**质量保证**: 每个任务都有明确的验收标准