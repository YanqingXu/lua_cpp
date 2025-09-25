# lua_cpp 项目AI助手快速上下文脚本

# 功能：帮助新的AI对话快速建立项目上下文，理解开发进度和下一步行动

## 🚀 使用方式

### 方法1: AI助手直接阅读 (推荐)
```
请阅读文件：./AGENTS.md - 这是为AI助手准备的完整项目上下文文档
```

### 方法2: 三步快速上下文建立
```
步骤1: 阅读 README.md (项目概述)
步骤2: 阅读 TODO.md (当前任务和进度) 
步骤3: 阅读 PROJECT_DASHBOARD.md (最新状态)
```

### 方法3: Spec-Kit命令上下文
```
/constitution - 查看项目宪法和原则
/analyze      - 验证当前文档一致性
/clarify      - 澄清下一步开发的歧义点
```

## 📋 快速状态检查

### 项目基本信息
- **名称**: lua_cpp - 现代C++ Lua 5.1.5解释器
- **方法论**: Specification-Driven Development (SDD)  
- **进度**: 21/58任务完成 (36.2%)
- **当前任务**: T022 Parser核心功能实现

### 开发环境
- **语言**: C++17/20 
- **构建**: CMake 3.16+
- **测试**: Catch2 + Contract tests
- **质量**: 90%+覆盖率，零内存泄露

### 当前状态 (2025-09-25)
- ✅ 词法分析器完成 (Token + Lexer + 错误处理)
- ✅ AST基础架构完成 (30+节点类型)
- ✅ 垃圾收集器完成 (三色标记算法)
- 🔄 Parser实现进行中 (T022)

## 🎯 新AI会话建议工作流

### 立即执行 (2分钟理解项目)
1. 阅读 `./AGENTS.md` - 完整项目上下文
2. 检查 `TODO.md` - 当前任务T022状态
3. 查看 `src/parser/` - 验证已完成的AST和接口

### 开始开发 (如果继续T022)
1. **澄清阶段**: 使用 `/clarify` 命令解决Parser实现歧义
   - 递归下降vs LALR选择
   - 错误恢复策略
   - 性能优化要求

2. **分析阶段**: 使用 `/analyze` 命令验证一致性
   - 契约测试 vs 实现规格一致性
   - 参考项目对比验证
   - 质量标准合规检查

3. **实施阶段**: 使用 `/implement` 开始编码
   - TDD: 测试先行
   - 参考 `tests/contract/test_parser_contract.cpp` 
   - 双重验证：lua_c_analysis + lua_with_cpp

## 📁 关键文件快速索引

### 必读文档
- `AGENTS.md` - AI助手完整上下文 (本项目核心)
- `README.md` - 项目概述和最新进展  
- `TODO.md` - 任务进度和下一步
- `PROJECT_DASHBOARD.md` - 实时开发状态

### 核心规格
- `memory/constitution.md` - 项目宪法 (不可变原则)
- `specs/` - 完整SDD规格文档
- `commands/` - Spec-Kit v0.0.17 命令

### 当前代码
- `src/parser/ast.h` - AST节点定义 (941行)
- `src/parser/parser.h` - Parser接口
- `tests/contract/test_parser_contract.cpp` - Parser契约 (1,900行)

### 参考项目
- `../lua_c_analysis/` - Lua原版C代码分析
- `../lua_with_cpp/` - 现代C++参考实现

## 🛠️ 常用开发命令

### 构建和测试
```bash
# 快速构建
cmake -B build && cmake --build build -j

# 运行测试
cd build && ctest -V

# 运行特定测试 (Parser相关)
./build/tests/test_parser_contract
```

### 代码质量检查
```bash
# 格式检查
find src -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 静态分析
clang-tidy src/**/*.cpp -- -Isrc -std=c++17
```

### Spec-Kit工作流
```bash
# 项目宪法检查
/constitution

# 当前任务澄清 (T022 Parser)
/clarify

# 文档一致性分析
/analyze  

# 开始实施
/implement
```

## 💡 AI助手提示

### 角色理解
你是一个专业的C++开发专家，专注于：
- 现代C++最佳实践 (C++17/20)  
- 编译器和解释器开发
- Specification-Driven Development
- 企业级代码质量标准

### 工作原则  
1. **质量优先**: 90%+测试覆盖率，零内存泄露
2. **规格驱动**: 所有实现必须有明确规格支撑
3. **双重验证**: 参考原版C实现 + 现代C++架构
4. **渐进开发**: 小步快跑，持续集成

### 当前焦点
- **主任务**: T022 Parser核心功能实现
- **技术栈**: 递归下降解析器 + AST + 现代C++
- **质量门禁**: Contract tests通过 + 参考项目验证
- **关键文件**: `src/parser/parser.cpp` (待实现)

---
**更新时间**: 2025-09-25
**Spec-Kit版本**: v0.0.17
**项目状态**: T022 Parser实现准备完成，可以开始编码