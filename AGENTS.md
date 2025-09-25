# lua_cpp Development Guidelines

Auto-generated from all feature plans. Last updated: 2025-09-25

## Project Overview

**现代C++版Lua 5.1.5解释器** - 基于Specification-Driven Development构建的企业级Lua解释器

- **项目状态**: 实现阶段 36.2% (21/58任务完成)
- **当前重点**: T022 Parser核心功能实现
- **Spec-Kit版本**: v0.0.17 (支持 `/clarify` 和 `/analyze` 命令)
- **开发方法论**: SDD + 双重参考项目验证

## Active Technologies

### Core Stack
- **Language**: C++17/20 (现代C++特性优先)
- **Build System**: CMake 3.16+
- **Testing**: Catch2 + Google Benchmark
- **Quality**: Clang-format, Clang-tidy, 静态分析

### Architecture Components
- **词法分析器**: 完成 (Token系统 + Lexer + 错误处理)
- **语法分析器**: 进行中 (AST基础完成，Parser实现中)
- **垃圾收集器**: 独立完成 (三色标记，0.55ms收集时间)
- **虚拟机**: 设计阶段
- **C API**: 契约完成，实现待开始

## Project Structure

```
lua_cpp/
├── memory/constitution.md      # 项目宪法和核心原则
├── specs/                      # 规格文档 (SDD方法论)
├── src/                        # 源代码实现
│   ├── lexer/                 # ✅ 词法分析器 (完成)
│   ├── parser/                # 🔄 语法分析器 (进行中)
│   ├── memory/                # ✅ 垃圾收集器 (完成)
│   ├── types/                 # ✅ 核心类型 (完成)
│   ├── vm/                    # ⏳ 虚拟机 (待开始)
│   └── api/                   # ⏳ C API (待开始)
├── tests/                     # 测试套件
│   ├── contract/              # ✅ 契约测试 (17个完成)
│   ├── unit/                  # 🔄 单元测试 (部分)
│   └── integration/           # ✅ 集成测试 (完成)
├── commands/                  # Spec-Kit命令
│   ├── clarify.md            # 澄清命令 (新增)
│   └── analyze.md            # 分析命令 (新增)
└── docs/                     # 项目文档
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

### 🎯 **当前任务**: T022 Parser核心功能实现

**准备工作** (已完成):
- ✅ AST节点定义 (`src/parser/ast.h` - 941行)
- ✅ AST实现 (`src/parser/ast.cpp` - 815行)  
- ✅ Parser接口设计 (`src/parser/parser.h`)
- ✅ Lexer完整实现 (Token + 核心功能 + 错误处理)

**当前需要**:
- 🎯 递归下降解析器实现
- 🎯 表达式解析 (运算符优先级)
- 🎯 语句解析 (控制流、函数定义)
- 🎯 与AST和Lexer的集成

**参考资料**:
- 契约测试: `tests/contract/test_parser_contract.cpp` (1,900+行)
- 原版参考: `lua_c_analysis/src/lparser.c`
- 现代实现: `lua_with_cpp/src/parser/`

### 📊 **项目进度总览**

```
总体进度: ████████████████████████████████░░ 36.2% (21/58)

✅ 契约测试阶段: ████████████████████████████████ 100% (17/17)
🚧 实现阶段: ██████████░░░░░░░░░░░░░░░░░░░░░░░░  9.8% (4/41)
  ├─ 词法分析器: ████████████████████████████████ 100% (3/3)
  ├─ 语法分析器: ████████░░░░░░░░░░░░░░░░░░░░░░░░  25% (1/4)
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

### 最近完成 (2025-09-25)
1. **T020 Lexer错误处理** - 完整错误处理系统
   - 25+错误类型，8种恢复策略
   - 用户友好错误消息和修复建议
   - 实现文件: `src/lexer/lexer_errors.h/cpp`

2. **T021 AST基础架构** - 语法树节点系统
   - 30+AST节点类型，访问者模式
   - 现代C++特性应用 (智能指针、RAII)
   - 实现文件: `src/parser/ast.h/cpp`

3. **T023 垃圾收集器** - 独立GC实现  
   - 三色标记算法，增量收集
   - 性能: 294万对象/秒，0.55ms收集时间
   - 实现文件: `src/memory/garbage_collector.h/cpp`

### 技术栈演进
- **Spec-Kit升级**: v0.0.17 (新增 `/clarify` 和 `/analyze`)
- **工作流程标准化**: 7步SDD流程
- **质量门禁强化**: 跨文档一致性验证

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

2. **检查T022准备情况**
   ```bash
   # 查看AST定义
   code src/parser/ast.h
   
   # 查看Parser接口
   code src/parser/parser.h
   
   # 查看契约测试
   code tests/contract/test_parser_contract.cpp
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
   - 确认T022的前置条件都已满足
   - 检查相关源文件和测试文件存在性
   - 验证开发环境配置完整性

3. **🎯 开始开发** (立即)
   - 如有疑问先运行 `/clarify` 澄清
   - 使用 `/analyze` 验证一致性  
   - 执行 `/implement` 开始T022实施
   - 遵循TDD：测试先行，参考项目验证

### T022 Parser具体实施指导

**实现文件**: `src/parser/parser.cpp`
**测试文件**: `tests/unit/test_parser.cpp` (基于contract)
**参考**: `tests/contract/test_parser_contract.cpp` (1,900行规格)

**关键实现点**:
- 递归下降解析器模式
- 运算符优先级处理
- 错误恢复和报告
- AST节点构建
- 与Lexer无缝集成

---

<!-- MANUAL ADDITIONS START -->
**项目特色**: 
- Specification-Driven Development (SDD)
- 双重参考项目验证 (lua_c_analysis + lua_with_cpp)  
- 现代C++企业级质量标准
- 完整的测试驱动开发流程
<!-- MANUAL ADDITIONS END -->