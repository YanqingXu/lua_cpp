/**
 * @file stdlib.cpp
 * @brief T027 Lua标准库总实现
 * 
 * 实现标准库的便利函数和初始化逻辑
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#include "stdlib.h"

namespace lua_cpp {
namespace stdlib {

std::unique_ptr<StandardLibrary> CreateCompleteStandardLibrary() {
    auto stdlib = std::make_unique<StandardLibrary>();
    
    // 注册所有核心标准库模块
    stdlib->RegisterModule(std::make_unique<BaseLibrary>());
    stdlib->RegisterModule(std::make_unique<StringLibrary>());
    stdlib->RegisterModule(std::make_unique<TableLibrary>());
    stdlib->RegisterModule(std::make_unique<MathLibrary>());
    
    return stdlib;
}

void InitializeAllStandardLibraries(EnhancedVirtualMachine* vm) {
    if (!vm) {
        throw std::invalid_argument("VM cannot be null");
    }
    
    auto stdlib = CreateCompleteStandardLibrary();
    stdlib->InitializeAll(vm);
}

std::string GetStandardLibraryVersion() {
    return "Lua 5.1.5 Standard Library (lua_cpp T027) v1.0.0";
}

std::vector<std::string> GetSupportedModules() {
    return {
        "base",      // 基础库函数
        "string",    // 字符串库
        "table",     // 表库  
        "math"       // 数学库
        // 将来可扩展: "io", "os", "debug", "package", "coroutine"
    };
}

} // namespace stdlib
} // namespace lua_cpp