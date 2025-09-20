# 快速开始指南：现代C++ Lua解释器

**文档日期**: 2025-09-20  
**相关文档**: [plan.md](./plan.md) | [data-model.md](./data-model.md)  
**预计完成时间**: 30分钟

## 🎯 快速开始目标

验证现代C++ Lua解释器的核心功能，包括：
- ✅ 基本Lua脚本执行
- ✅ C++ API调用
- ✅ 内存管理和垃圾回收
- ✅ 性能基准达标

## 📋 环境准备

### 系统要求
- **操作系统**: Windows 10+, Ubuntu 20.04+, macOS 10.15+
- **编译器**: GCC 9+, Clang 10+, 或 MSVC 2019+
- **CMake**: 3.16+
- **内存**: 至少4GB RAM
- **存储**: 至少1GB可用空间

### 依赖安装

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake git
sudo apt install libcatch2-dev libbenchmark-dev
```

**macOS (Homebrew):**
```bash
brew install cmake catch2 google-benchmark
```

**Windows (vcpkg):**
```powershell
vcpkg install catch2 benchmark
```

## 🚀 构建和安装

### 1. 克隆和配置
```bash
# 克隆项目
git clone <repository-url> lua_cpp
cd lua_cpp

# 创建构建目录
mkdir build && cd build

# 配置CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
```

### 2. 编译项目
```bash
# 编译（使用所有可用核心）
cmake --build . --parallel

# 验证构建成功
ls bin/    # 应该看到 lua_cpp 可执行文件
```

### 3. 运行测试
```bash
# 运行所有测试
ctest --output-on-failure

# 运行特定测试类别
ctest -R "unit_tests"
ctest -R "integration_tests"  
ctest -R "compatibility_tests"
```

## 🧪 功能验证测试

### 测试1: 基本Lua脚本执行
```bash
# 创建测试脚本
cat > hello.lua << 'EOF'
print("Hello from modern C++ Lua!")

-- 测试基本数据类型
local number = 42
local string = "Lua 5.1.5 compatible"
local table = {1, 2, 3, name = "test"}

print("Number:", number)
print("String:", string)
print("Table size:", #table)

-- 测试函数
function factorial(n)
    if n <= 1 then
        return 1
    else
        return n * factorial(n - 1)
    end
end

print("Factorial of 5:", factorial(5))
EOF

# 执行脚本
./bin/lua_cpp hello.lua
```

**期望输出:**
```
Hello from modern C++ Lua!
Number: 42
String: Lua 5.1.5 compatible
Table size: 3
Factorial of 5: 120
```

### 测试2: C++ API使用
```cpp
// 创建 test_api.cpp
#include "lua_api.h"
#include <iostream>

int main() {
    // 创建Lua状态机
    lua_State* L = luaL_newstate();
    if (!L) {
        std::cerr << "Failed to create Lua state" << std::endl;
        return 1;
    }
    
    // 执行简单计算
    lua_pushnumber(L, 10);
    lua_pushnumber(L, 20);
    lua_pushnumber(L, lua_tonumber(L, -1) + lua_tonumber(L, -2));
    
    double result = lua_tonumber(L, -1);
    std::cout << "10 + 20 = " << result << std::endl;
    
    // 清理
    lua_close(L);
    return 0;
}
```

```bash
# 编译并运行API测试
g++ -I../src/api test_api.cpp -L./lib -llua_cpp -o test_api
./test_api
```

**期望输出:**
```
10 + 20 = 30
```

### 测试3: 兼容性验证
```bash
# 下载官方Lua 5.1.5测试套件（如果可用）
# 或运行内置兼容性测试
./bin/lua_cpp -e "
-- 测试Lua 5.1.5特性
local mt = {__index = function(t, k) return 'default' end}
local t = setmetatable({}, mt)
assert(t.anything == 'default')
print('Metatable test: PASS')

-- 测试协程
local co = coroutine.create(function() 
    coroutine.yield(42)
    return 'done'
end)
local ok, val = coroutine.resume(co)
assert(ok and val == 42)
print('Coroutine test: PASS')

print('All compatibility tests passed!')
"
```

### 测试4: 性能基准
```bash
# 运行性能基准测试
./bin/benchmarks

# 或运行特定基准
./bin/benchmarks --benchmark_filter="LuaTable.*"
./bin/benchmarks --benchmark_filter="FunctionCall.*"
```

**期望结果:**
- 执行性能应达到或超过原版Lua 5.1.5的95%
- 内存使用不超过原版的120%
- 启动时间小于100ms

## 📊 验收标准

### ✅ 功能正确性
- [ ] 所有Lua 5.1.5语法特性正常工作
- [ ] 标准库函数行为一致
- [ ] 错误消息格式兼容
- [ ] 元表和元方法正确实现

### ✅ 性能要求
- [ ] 执行速度 ≥ 原版95%
- [ ] 内存使用 ≤ 原版120%  
- [ ] 启动时间 ≤ 100ms
- [ ] 垃圾回收暂停时间合理

### ✅ 质量指标
- [ ] 所有单元测试通过
- [ ] 集成测试通过
- [ ] 内存泄漏检测清洁
- [ ] 静态分析无警告

### ✅ API兼容性
- [ ] 所有C API函数正常工作
- [ ] 参数和返回值类型正确
- [ ] 错误处理机制一致
- [ ] 栈操作行为兼容

## 🔧 故障排除

### 常见问题

**问题1: 编译错误**
```bash
# 检查编译器版本
gcc --version    # 需要 >= 9.0
clang --version  # 需要 >= 10.0

# 检查CMake版本
cmake --version  # 需要 >= 3.16

# 清理并重新构建
rm -rf build && mkdir build && cd build
cmake .. && make
```

**问题2: 测试失败**
```bash
# 详细测试输出
ctest --verbose

# 运行特定失败的测试
ctest -R "失败的测试名称" --verbose

# 检查内存问题
valgrind --tool=memcheck ./bin/lua_cpp hello.lua
```

**问题3: 性能不达标**
```bash
# 确保Release模式构建
cmake .. -DCMAKE_BUILD_TYPE=Release

# 检查编译器优化
cmake .. -DCMAKE_CXX_FLAGS="-O3 -DNDEBUG"

# 运行性能分析
perf record ./bin/benchmarks
perf report
```

## 📈 下一步

快速开始完成后，可以：

1. **深入学习**: 阅读[架构文档](../docs/architecture/)
2. **贡献代码**: 查看[开发指南](../CONTRIBUTING.md)
3. **性能调优**: 研究[优化技巧](../docs/performance/)
4. **扩展功能**: 开发[自定义模块](../docs/extensions/)

## 🆘 获取帮助

- **文档**: [项目文档](../docs/)
- **问题报告**: [GitHub Issues](../issues)
- **讨论**: [GitHub Discussions](../discussions)
- **邮件**: [项目邮件列表]

---

**验证完成**: 🎉 恭喜！您已成功验证现代C++ Lua解释器的核心功能。

**预计用时**: 实际 _____ 分钟 (目标: 30分钟)  
**所有测试通过**: ✅ 是 / ❌ 否  
**性能达标**: ✅ 是 / ❌ 否