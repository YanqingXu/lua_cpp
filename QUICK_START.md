# 🚀 Lua C++ 项目快速参考

## 📅 最后更新: 2025年9月20日

---

## 🎯 当前状态一览

### ✅ 已完成 (13/19 tasks)
- **T001-T004**: 基础设施搭建 ✅
- **T005-T009**: 核心类型契约测试 ✅
- **T010**: Lexer契约测试 ✅
- **T011**: Parser契约测试 ✅  
- **T012**: Compiler契约测试 ✅
- **T013**: 垃圾回收器契约测试 ✅

### 🎯 下一个任务
**T014: 内存管理契约测试**
- 文件: `tests/contract/test_memory_contract.cpp`
- 内容: 对象池、内存统计、泄漏检测等核心功能契约

---

## 📁 项目结构
```
e:\Programming\Lua5.15\lua_cpp\
├── src/core/           # ✅ 基础头文件已创建
├── tests/contract/     # ✅ 5个契约测试已完成
│   ├── test_tvalue_contract.cpp     ✅
│   ├── test_table_contract.cpp      ✅  
│   ├── test_string_contract.cpp     ✅
│   ├── test_function_contract.cpp   ✅
│   ├── test_state_contract.cpp      ✅
│   └── test_lexer_contract.cpp      🎯 下一个目标
├── DEVELOPMENT_PROGRESS.md          ✅ 详细进度文档
└── QUICK_START.md                   ✅ 本文件，快速参考
```

---

## 🔄 立即开始开发

### 1. 进入项目目录
```bash
cd e:\Programming\Lua5.15\lua_cpp\
```

### 2. 检查当前状态
```bash
git status
git log --oneline -3
```

### 3. 开始T015任务
```bash
git checkout -b feature/T015-c-api-contract
# 创建 tests/contract/test_c_api_contract.cpp
```

---

## 📋 T015 任务清单

### 🎯 目标: C API契约测试
- [ ] 定义lua_State和基础API接口
- [ ] 测试栈操作API (push/pop/check/to)
- [ ] 测试表操作API (table/field/metatable)
- [ ] 测试函数调用API (call/pcall/resume)
- [ ] 测试字符串和数值API
- [ ] 测试用户数据API (userdata/lightuserdata)
- [ ] 测试引用和回调API (ref/unref/hook)
- [ ] 测试调试和错误处理API
- [ ] 测试C闭包和upvalue
- [ ] 测试垃圾回收控制API
- [ ] 性能基准测试
- [ ] 🔍lua_c_analysis + 🏗️lua_with_cpp双重验证

### 📚 参考资料
- `tests/contract/test_*_contract.cpp` - 已完成的契约测试
- Lua 5.1.5源码 `llex.c`, `llex.h`
- `DEVELOPMENT_PROGRESS.md` - 详细开发文档

---

## ⚡ 开发模式

### 当前阶段: **契约测试阶段**
1. **契约优先**: 先定义接口和行为规范
2. **TDD方法**: 测试驱动开发
3. **完整覆盖**: 确保所有边界情况
4. **性能契约**: 包含性能基准要求
5. **兼容性**: 100% Lua 5.1.5兼容

### 代码质量
- ✅ C++17/20标准
- ✅ Catch2测试框架
- ✅ 严格编译器警告
- ✅ clang-format格式化
- ✅ 完整错误处理

---

## 📊 进度概览

```
基础设施    ████████████████████████████████ 100% (4/4)
核心类型    ████████████████████████████████ 100% (5/5)  
编译器前端  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% (0/4)
运行时系统  ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% (0/3)
标准库      ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░   0% (0/3)

总体进度:   ██████████████░░░░░░░░░░░░░░░░░░  47% (9/19)
```

---

## 🎯 里程碑

### 🏁 阶段1: 契约测试 (当前)
- **T001-T009**: ✅ 已完成
- **T010-T019**: 🔄 进行中

### 🏁 阶段2: 核心实现
- 实现核心类型系统
- 实现词法分析器
- 实现语法分析器

### 🏁 阶段3: 运行时系统  
- 实现虚拟机
- 实现垃圾收集器
- 实现C API

### 🏁 阶段4: 标准库
- 基础库函数
- 协程支持
- 调试接口

---

## 🔗 重要文件

| 文件 | 用途 | 状态 |
|------|------|------|
| `DEVELOPMENT_PROGRESS.md` | 详细进度跟踪 | ✅ |
| `QUICK_START.md` | 本文件，快速参考 | ✅ |
| `src/core/lua_common.h` | 核心类型定义 | ✅ |
| `tests/contract/` | 契约测试目录 | 🔄 5/10完成 |

---

## 💡 开发提示

### 下次开发时
1. 📖 先读这个文件了解当前状态
2. 📋 检查TODO列表确认任务
3. 🔍 查看已完成的契约测试了解模式
4. 🚀 直接开始T010任务

### 遇到问题时
1. 📚 查看 `DEVELOPMENT_PROGRESS.md` 获取详细信息
2. 🔍 参考已完成的契约测试文件
3. 📖 查阅Lua 5.1.5官方文档

---

**快速启动命令**:
```bash
cd e:\Programming\Lua5.15\lua_cpp\
code tests/contract/test_lexer_contract.cpp  # 开始T010
```

### 第二步：定义功能规格 (宪法建立后)

**如果你使用Claude AI，请执行：**

```
/specify 基于lua_c_analysis和lua_with_cpp项目，定义现代C++版Lua解释器的完整功能规格：

核心需求：
- 完整的Lua 5.1.5语法支持（变量、函数、表、控制流、闭包等）
- 现代化的C++实现（智能指针、RAII、模板元编程、异常安全）
- 高性能虚拟机（基于寄存器的VM、优化的字节码执行）
- 完整标准库（string、math、table、io、os、debug等模块）
- 垃圾回收系统（三色标记、增量回收、写屏障）
- C++ API接口（嵌入式友好、类型安全、异常安全）

技术要求：
- 保持与lua_with_cpp现有架构的兼容性
- 集成lua_c_analysis的核心算法和优化技术
- 支持实时性能监控和调试工具
- 提供全面的错误处理和诊断信息

质量标准：
- 100%自动化测试覆盖
- 微秒级性能基准
- 零内存泄漏
- 完整的Lua兼容性测试套件
```

**如果你使用GitHub Copilot，请在VS Code中执行：**

```
/specify 基于lua_c_analysis和lua_with_cpp项目，定义现代C++版Lua解释器的完整功能规格。请参考项目中的REFERENCE_ANALYSIS.md文件，包含了两个参考项目的深度技术分析。

### 第三步：制定技术方案 (规格定义后)

**如果你使用Claude AI，请执行：**

```
/plan 为现代C++版Lua解释器制定详细的技术实现方案：

技术栈选择：
- C++17/20标准，CMake 3.20+构建系统
- Google Test测试框架，Google Benchmark性能测试
- Clang-Tidy静态分析，Valgrind内存检测
- GitHub Actions CI/CD，集成代码质量门禁

架构设计：
- 保留lua_with_cpp的模块化架构（lexer、parser、compiler、vm、gc、lib）
- 集成lua_c_analysis的核心算法（VM指令集、GC策略、对象系统）
- 采用现代C++设计模式（RAII、智能指针、移动语义、变参模板）
- 实现类型安全的Value系统（std::variant、模板特化、编译时检查）

实施策略：
- 阶段1：基础架构升级（构建系统、测试框架、CI/CD）
- 阶段2：核心引擎完善（VM、编译器、GC优化）
- 阶段3：标准库扩展（完整模块实现、性能优化）
- 阶段4：集成测试验证（兼容性、性能、稳定性）

质量保证：
- 每个模块都有对应的单元测试和集成测试
- 建立性能回归检测机制
- 实施代码审查和静态分析流程
- 建立Lua兼容性基准测试套件
```

**如果你使用GitHub Copilot，请在VS Code中执行：**

```
/plan 为现代C++版Lua解释器制定详细的技术实现方案，采用C++17/20技术栈和CMake构建系统。基于lua_with_cpp的现有架构进行增强，集成lua_c_analysis的核心算法。

### 第四步：生成开发任务 (方案制定后)

**对于Claud AI和GitHub Copilot，命令相同：**

```
/tasks 基于实现计划，生成详细的开发任务列表，重点考虑：
- 从lua_with_cpp项目继承已有代码和架构
- 集成lua_c_analysis项目的技术洞察和算法
- 建立清晰的任务依赖关系和并行开发机会
- 确保每个任务都有明确的验收标准和测试要求
```

### 第五步：开始实施开发 (任务规划后)

**对于Claud AI和GitHub Copilot，命令相同：**

```
/implement 执行所有开发任务，构建完整的现代C++版Lua解释器
```

## 🎛️ AI助手配置

该项目支持多种AI助手，你可以根据你的偏好选择：

### Claude AI
- **配置位置**: `.claude/commands/`
- **优势**: 深度代码理解，复杂架构设计
- **使用方式**: 在Claude界面中直接输入命令

### GitHub Copilot  
- **配置位置**: `.github/prompts/`
- **优势**: VS Code集成，实时代码生成
- **使用方式**: 在VS Code中Ctrl+I开启Copilot Chat，输入命令

### 共同特点
- 所有AI助手都使用相同的五个核心命令
- 所有配置都包含了lua_c_analysis和lua_with_cpp的参考信息
- 所有命令都遵循Spec-Kit规格驱动开发方法论

```
lua_cpp/                           # 你的新项目根目录
├── 📋 PROJECT_STATUS.md            # 项目进度报告  
├── 📊 REFERENCE_ANALYSIS.md        # 参考项目技术分析
├── 🎯 README.md                    # 项目概述和快速开始
├── 📝 QUICK_START.md               # 本文件 - 快速上手指南
├── 💾 memory/constitution.md       # 项目宪法和核心原则
├── 🎛️ .claude/commands/            # Claude AI命令配置
├── 📜 scripts/                     # 开发辅助脚本
└── 📋 templates/                   # 文档和代码模板
```

## 🎯 关键优势

### 1. 规格驱动开发
- **清晰的需求**: 通过规格化避免范围蔓延
- **架构优先**: 设计在实现之前，减少技术债务  
- **质量可控**: 明确的验收标准和测试要求

### 2. 站在巨人肩膀上
- **lua_c_analysis**: 深度技术洞察和实现细节
- **lua_with_cpp**: 现代C++架构和部分实现
- **避免重复造轮子**: 充分利用现有技术积累

### 3. 现代化技术栈
- **C++17/20**: 类型安全、内存安全、性能优化
- **自动化测试**: 100%覆盖率，回归检测
- **持续集成**: 代码质量门禁，自动化验证

## 🚨 重要提示

1. **按顺序执行命令**: 必须先执行`/constitution`，再执行`/specify`，依此类推
2. **充分利用参考项目**: 经常查阅`REFERENCE_ANALYSIS.md`了解技术细节
3. **保持迭代改进**: Spec-Kit支持规格和实现的持续优化
4. **关注兼容性**: 始终以Lua 5.1.5兼容性为最高优先级

## 🎉 恭喜！

你现在拥有了一个**专业级的现代C++版Lua解释器开发项目**！这个项目：

- ✅ **技术先进**: 基于现代C++和Spec-Kit方法论
- ✅ **基础扎实**: 有两个优秀参考项目支撑
- ✅ **流程完善**: 规格驱动开发确保质量
- ✅ **目标明确**: Lua 5.1.5完全兼容性

**立即开始**: 
- **如果使用Claude AI**: 执行 `/constitution` 命令
- **如果使用GitHub Copilot**: 在VS Code中打开Copilot Chat，执行 `/constitution` 命令

开启你的现代Lua解释器开发之旅！** 🚀

---

*创建时间: 2025年9月20日*  
*状态: 准备就绪，等待开发启动*