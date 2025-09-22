# lua_cpp/temp 目录说明

## 📁 目录用途

此目录用于存放项目中的临时文件、测试文件和不重要的辅助文件，以保持项目根目录的整洁。

## 📋 目录内容分类

### 🔧 可执行文件 (编译产物)
- `test_brackets.exe` - 括号测试可执行文件
- `test_complete.exe` - 完整测试可执行文件  
- `test_complex.exe` - 复杂测试可执行文件
- `test_debug.exe` - 调试测试可执行文件
- `test_lexer.exe` - 词法分析器测试可执行文件

### 📝 项目报告文件
- `SPECKIT_UNIFIED_REPORT.md` - Spec-Kit指令统一完成报告
- `RESTRUCTURE_REPORT.md` - 项目结构重组完成报告

### 🧪 测试源码文件
- `simple_test.cpp` - 简单测试文件
- `test_debug.cpp` - 调试测试文件
- `test_lexer_*.cpp` - 词法分析器测试系列
  - `test_lexer_brackets.cpp` - 括号测试
  - `test_lexer_complete.cpp` - 完整测试
  - `test_lexer_complex.cpp` - 复杂测试
  - `test_lexer_debug.cpp` - 调试测试
  - `test_lexer_simple.cpp` - 简单测试

### 🔤 Token处理测试文件
- `test_token_simple.cpp` - 简单token测试
- `token_test.cpp` - token测试
- `token_verify.cpp` - token验证

## ⚠️ 重要说明

1. **临时性质**: 此目录中的文件可以安全删除或重新生成
2. **编译产物**: .exe文件是编译产生的，可以通过重新编译生成
3. **测试文件**: 大部分为独立的测试文件，不影响主要功能
4. **报告文件**: 记录了项目整理过程，仅供参考

## 🧹 清理建议

- 可执行文件可以定期清理，通过CMake重新编译即可
- 报告文件可以在项目稳定后删除
- 测试文件如果不再使用可以删除或移至tests目录

---

*此目录由项目整理工具自动创建，用于提升项目根目录的可读性和组织性。*