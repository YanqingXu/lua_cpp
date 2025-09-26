/**
 * @file stdlib.h
 * @brief T027 Lua标准库总头文件
 * 
 * 包含所有Lua 5.1.5标准库模块：
 * - base_lib: 基础库函数
 * - string_lib: 字符串库
 * - table_lib: 表库
 * - math_lib: 数学库
 * 
 * 提供一键初始化所有标准库的便利接口
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#pragma once

#include "stdlib_common.h"
#include "base_lib.h"
#include "string_lib.h"
#include "table_lib.h"
#include "math_lib.h"

namespace lua_cpp {
namespace stdlib {

/**
 * @brief 创建并返回包含所有标准库的StandardLibrary实例
 * 
 * 这个函数创建一个完整配置的StandardLibrary实例，
 * 包含Lua 5.1.5的所有核心标准库模块
 * 
 * @return 配置好的StandardLibrary实例
 */
std::unique_ptr<StandardLibrary> CreateCompleteStandardLibrary();

/**
 * @brief 快速初始化VM的所有标准库
 * 
 * 这是一个便利函数，用于快速为VM添加完整的Lua 5.1.5标准库支持
 * 
 * @param vm 目标虚拟机
 */
void InitializeAllStandardLibraries(EnhancedVirtualMachine* vm);

/**
 * @brief 获取标准库版本信息
 * 
 * @return 版本信息字符串
 */
std::string GetStandardLibraryVersion();

/**
 * @brief 获取支持的标准库模块列表
 * 
 * @return 模块名称列表
 */
std::vector<std::string> GetSupportedModules();

} // namespace stdlib
} // namespace lua_cpp