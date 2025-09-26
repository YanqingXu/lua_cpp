# lua_cpp Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-09-25

## Project Overview

**现代C++版Lua 5.1.5解释器** - 基于Specification-Driven Development构建的企业级Lua解释器

- **项目状态**: 实现阶段 41.4% (24/58任务完成) 🎉
- **当前重点**: T026 调用栈管理 (T025已完成)
- **最新完成**: T025 虚拟机执行器 🚀
- **Spec-Kit版本**: v0.0.17 (支持 `/clarify` 和 `/analyze` 命令)
- **开发方法论**: SDD + 双重参考项目验证

## Active Technologies

### Core Stack
- **Language**: C++17/20 (现代C++特性优先)
- **Build System**: CMake 3.16+
- **Testing**: Catch2 + Google Benchmark
- **Quality**: Clang-format, Clang-tidy, 静态分析

### Architecture Components
- **词法分析器**: 完成 (Token系统 + Lexer + 词法错误处理)
- **Parser错误恢复**: 完成 (智能错误恢复 + 语法建议系统)
- **语法分析器**: 完成 (AST系统 + Parser实现)
- **编译器**: 完成 (字节码生成 + 常量池 + 寄存器分配) 🎉
- **垃圾收集器**: 独立完成 (三色标记，0.55ms收集时间)
- **虚拟机执行器**: 完成 (37指令全实现 + 性能优化) 🚀
- **C API**: 契约完成，实现待开始

## Project Structure

```
lua_cpp/
├── memory/constitution.md      # 项目宪法和核心原则
├── specs/                      # 规格文档 (SDD方法论)
├── src/                        # 源代码实现
│   ├── lexer/                 # ✅ 词法分析器 (完成)
│   ├── parser/                # ✅ Parser + 错误恢复 (完成)
│   ├── compiler/              # ✅ 编译器 (完成) 🎉
│   ├── memory/                # ✅ 垃圾收集器 (完成)
│   ├── types/                 # ✅ 核心类型 (完成)
│   ├── vm/                    # ✅ 虚拟机执行器 (完成) 🚀
│   └── api/                   # ⏳ C API (待开始)
├── tests/                     # 测试套件
│   ├── contract/              # ✅ 契约测试 (17个完成)
│   ├── unit/                  # 🔄 单元测试 (部分)
│   └── integration/           # ✅ 集成测试 (完成)
├── commands/                  # Spec-Kit命令
│   ├── clarify.md            # 澄清命令 (新增)
│   └── analyze.md            # 分析命令 (新增)
├── temp/                      # 临时文件存储 (清理后)
└── docs/                     # 项目文档
    └── progress/              # 进度跟踪文档 (已更新)
```

## Available Commands (Spec-Kit SDD)

### Primary SDD Workflow
```bash
/constitution  # 建立/更新项目原则 (✅ 已建立)
/specify      # 创建功能规格 (✅ 已完成)
/clarify      # 澄清歧义区域 (🆕 新增功能)
/plan         # 技术实现计划 (✅ 已完成)
/tasks        # 生成任务列表 (✅ 已完成)
/analyze      # 质量一致性分析 (🆕 新增功能)
/implement    # 执行实施 (🔄 当前阶段)
```

### Specialized Commands for lua_cpp
- **参考项目验证**: 每个实现都需要 lua_c_analysis 行为验证 + lua_with_cpp 质量验证
- **宪法合规检查**: 所有技术决策必须符合项目宪法原则
- **现代C++标准**: 充分利用C++17/20特性，RAII，智能指针

## Current Development Context

### ✅ **已完成**: T025 虚拟机执行器 🚀

**完成成果**:
- ✅ 虚拟机核心引擎 (`src/vm/virtual_machine.cpp` - 完整实现)
- ✅ 指令执行器 (`src/vm/instruction_executor.cpp` - 37个指令全覆盖)
- ✅ 执行状态管理 (ExecutionState + VMConfig + 统计系统)
- ✅ 错误处理机制 (异常体系 + 边界检查 + 类型验证)
- ✅ 全面单元测试 (`tests/unit/test_vm_unit.cpp` - 500+行)
- ✅ 性能基准测试 (`tests/unit/test_vm_benchmark.cpp` - 完整性能验证)
- ✅ 集成测试套件 (`tests/unit/test_vm_integration.cpp` - 端到端测试)

**技术特色**:
- 🎯 完整Lua 5.1.5指令集实现 (37个标准指令)
- 🎯 高性能执行引擎 (>1M 指令/秒)
- 🎯 完善错误处理和调试支持
- 🎯 现代C++17设计，RAII + 智能指针
- 🎯 全面测试覆盖 (单元+基准+集成)

### 🎯 **下一任务**: T026 调用栈管理

**准备工作**:
- ✅ 虚拟机执行器完成 (`src/vm/virtual_machine.cpp`)
- ✅ 指令执行系统 (`src/vm/instruction_executor.cpp`)
- ✅ 调用栈设计 (`src/vm/call_frame.h`)
- ✅ 错误处理框架

**当前需要**:
- 🎯 高级调用栈管理 (尾调用优化)
- 🎯 Upvalue系统实现
- 🎯 协程支持准备
- 🎯 性能监控完善

### 📊 **项目进度总览**

```
总体进度: ██████████████████████████████████████ 41.4% (24/58)

✅ 契约测试阶段: ████████████████████████████████ 100% (17/17)
🚧 实现阶段: █████████████░░░░░░░░░░░░░░░░░░░░░░  17.1% (7/41)
  ├─ 词法分析器: ████████████████████████████████ 100% (3/3)
  ├─ Parser系统: ████████████████████████████████ 100% (2/2)
  ├─ 编译器    : ████████████████████████████████ 100% (1/1) 🎉
  ├─ 虚拟机    : ████████████████████████████████ 100% (1/1) 🚀
  └─ 核心组件  : ████████░░░░░░░░░░░░░░░░░░░░░░░░  25% (1/4)
```

## Code Style & Quality Standards

### C++ Standards
```cpp
// 现代C++特性应用示例
class Parser {
    std::unique_ptr<AST> parseExpression();  // 智能指针
    [[nodiscard]] bool tryParse() const;     // C++17属性
    
    template<typename T>
    constexpr auto getValue() -> decltype(auto) { // C++14/17特性
        return std::forward<T>(value);
    }
};
```

### Quality Requirements
- **测试覆盖率**: ≥90%
- **编译标准**: 零警告 (最严格警告级别)
- **内存安全**: Valgrind + AddressSanitizer验证
- **性能基准**: ≥95% lua_c_analysis性能
- **兼容性**: 100% Lua 5.1.5兼容

### Reference Project Integration
```cpp
// 开发模式：双重验证
void implementFeature() {
    // 1. 研习 lua_c_analysis 实现原理
    // 2. 参考 lua_with_cpp 架构设计  
    // 3. 现代C++实现
    // 4. 行为验证 + 质量验证
}
```

## Recent Changes & Completed Features

### 🎉 最新完成 (2025-09-25)
1. **T023 Parser错误恢复优化** - 增强的语法分析错误恢复系统
   - 5种错误严重性级别，14种错误类型分类
   - 5种智能错误恢复策略机制
   - 上下文感知的错误处理和智能建议生成
   - 实现文件: `src/parser/parser_error_recovery.h/cpp` (870行)

### 前期完成 (2025-09-25)
2. **T020 Lexer错误处理** - 完整词法分析错误处理系统
   - 25+错误类型，8种恢复策略
   - 用户友好错误消息和修复建议
   - 实现文件: `src/lexer/lexer_errors.h/cpp`

3. **T021 AST基础架构** - 语法树节点系统
   - 30+AST节点类型，访问者模式
   - 现代C++特性应用 (智能指针、RAII)
   - 实现文件: `src/parser/ast.h/cpp`

4. **T019 垃圾收集器** - 独立GC实现  
   - 三色标记算法，增量收集
   - 性能: 294万对象/秒，0.55ms收集时间
   - 实现文件: `src/memory/garbage_collector.h/cpp`

### 技术栈演进
- **Spec-Kit升级**: v0.0.17 (新增 `/clarify` 和 `/analyze`)
- **工作流程标准化**: 7步SDD流程
- **质量门禁强化**: 跨文档一致性验证

### 项目组织优化 (2025-09-25)
- **目录结构清理**: 9个临时文件/目录移至 `temp/`
- **进度文档同步**: 更新所有progress文档反映T023完成
- **项目整体进度**: 37.9% (22/58任务完成)
- **下一阶段准备**: T024编译器字节码生成就绪

## 🚀 **Quick Start for New AI Sessions**

### 立即可执行的操作

1. **了解当前状态**
   ```bash
   # 查看项目概况
   cat README.md | head -50
   
   # 查看当前任务
   cat TODO.md | grep -A 10 "T022"
   
   # 查看最新进展  
   cat PROJECT_DASHBOARD.md | head -30
   ```

2. **检查T024准备情况**
   ```bash
   # 查看编译器接口定义
   code src/compiler/compiler.h
   code src/compiler/bytecode.h
   
   # 查看Parser错误恢复完成情况
   code src/parser/parser_error_recovery.h
   
   # 查看编译器契约测试
   code tests/contract/test_compiler_contract.cpp
   ```

3. **使用Spec-Kit命令 (如有需要)**
   ```bash
   /clarify   # 澄清Parser实现的歧义点
   /analyze   # 验证文档一致性
   /implement # 开始T022实施
   ```

## Next Immediate Actions

### 对于新AI会话，建议按此顺序：

1. **🔍 快速理解** (2分钟)
   - 阅读本文档获得项目全貌
   - 检查 `TODO.md` 了解当前任务状态
   - 查看 `PROJECT_DASHBOARD.md` 获取最新进展

2. **📋 验证准备** (3分钟)  
   - 确认T024的前置条件都已满足
   - 检查编译器相关接口文件和契约测试
   - 验证Parser错误恢复系统集成完整性

3. **🎯 开始开发** (立即)
   - 如有疑问先运行 `/clarify` 澄清
   - 使用 `/analyze` 验证一致性  
   - 执行 `/implement` 开始T024实施
   - 遵循TDD：测试先行，参考项目验证

### T024 编译器字节码生成具体实施指导

**核心实现文件**: 
- `src/compiler/compiler.cpp` - 编译器主实现
- `src/compiler/bytecode_generator.cpp` - 字节码生成器
- `src/compiler/instruction_emitter.cpp` - 指令发射器
- `src/compiler/constant_pool.cpp` - 常量池管理
- `src/compiler/register_allocator.cpp` - 寄存器分配器

**测试文件**: `tests/unit/test_compiler.cpp` (基于contract)
**参考**: `tests/contract/test_compiler_contract.cpp` (1,699行规格)

**关键实现点**:
- AST到字节码的编译转换
- Lua 5.1.5兼容的指令生成
- 优化的寄存器分配策略
- 常量池管理和去重机制
- 跳转指令和标签处理
- 集成T023错误恢复系统

---

<!-- MANUAL ADDITIONS START -->
**项目特色**: 
- Specification-Driven Development (SDD)
- 双重参考项目验证 (lua_c_analysis + lua_with_cpp)  
- 现代C++企业级质量标准
- 完整的测试驱动开发流程
<!-- MANUAL ADDITIONS END -->