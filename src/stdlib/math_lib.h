/**
 * @file math_lib.h
 * @brief T027 Lua数学库头文件
 * 
 * 实现Lua 5.1.5数学库函数：
 * - 三角函数: sin, cos, tan, asin, acos, atan, atan2
 * - 指数对数: exp, log, log10, pow, sqrt
 * - 取整函数: floor, ceil, abs, fmod
 * - 随机数: random, randomseed
 * - 常数: pi, huge
 * - 其他: max, min, modf, deg, rad
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#pragma once

#include "stdlib_common.h"
#include <random>

namespace lua_cpp {
namespace stdlib {

/**
 * @brief Lua数学库模块
 * 
 * 实现Lua 5.1.5标准的数学库函数
 */
class MathLibrary : public LibraryModule {
public:
    MathLibrary();
    ~MathLibrary() override = default;
    
    // LibraryModule接口实现
    std::string GetModuleName() const override { return "math"; }
    std::string GetModuleVersion() const override { return "1.0.0"; }
    std::vector<LibFunction> GetFunctions() const override;
    void RegisterModule(EnhancedVirtualMachine* vm) override;
    void Initialize(EnhancedVirtualMachine* vm) override;
    void Cleanup(EnhancedVirtualMachine* vm) override;

private:
    // ====================================================================
    // 数学库函数声明
    // ====================================================================
    
    // 三角函数
    static LUA_STDLIB_FUNCTION(lua_math_sin);
    static LUA_STDLIB_FUNCTION(lua_math_cos);
    static LUA_STDLIB_FUNCTION(lua_math_tan);
    static LUA_STDLIB_FUNCTION(lua_math_asin);
    static LUA_STDLIB_FUNCTION(lua_math_acos);
    static LUA_STDLIB_FUNCTION(lua_math_atan);
    static LUA_STDLIB_FUNCTION(lua_math_atan2);
    
    // 指数和对数函数
    static LUA_STDLIB_FUNCTION(lua_math_exp);
    static LUA_STDLIB_FUNCTION(lua_math_log);
    static LUA_STDLIB_FUNCTION(lua_math_log10);
    static LUA_STDLIB_FUNCTION(lua_math_pow);
    static LUA_STDLIB_FUNCTION(lua_math_sqrt);
    
    // 取整和绝对值函数
    static LUA_STDLIB_FUNCTION(lua_math_floor);
    static LUA_STDLIB_FUNCTION(lua_math_ceil);
    static LUA_STDLIB_FUNCTION(lua_math_abs);
    static LUA_STDLIB_FUNCTION(lua_math_fmod);
    static LUA_STDLIB_FUNCTION(lua_math_modf);
    
    // 最值函数
    static LUA_STDLIB_FUNCTION(lua_math_max);
    static LUA_STDLIB_FUNCTION(lua_math_min);
    
    // 角度转换
    static LUA_STDLIB_FUNCTION(lua_math_deg);
    static LUA_STDLIB_FUNCTION(lua_math_rad);
    
    // 随机数函数
    static LUA_STDLIB_FUNCTION(lua_math_random);
    static LUA_STDLIB_FUNCTION(lua_math_randomseed);
    
    // ====================================================================
    // 内部辅助函数和数据
    // ====================================================================
    
    /**
     * @brief 检查数学函数的参数是否为有效数字
     * @param value 要检查的值
     * @param func_name 函数名
     * @param arg_index 参数索引
     * @return 数字值
     */
    static double CheckNumberArg(const LuaValue& value, 
                               const std::string& func_name, 
                               int arg_index);
    
    /**
     * @brief 检查数学运算结果的有效性
     * @param result 运算结果
     * @param func_name 函数名
     * @return 检查后的结果
     */
    static double CheckMathResult(double result, const std::string& func_name);
    
    // 随机数生成器
    static std::mt19937 random_generator_;
    static bool random_seed_set_;
    
    // 数学常数
    static constexpr double PI = 3.14159265358979323846;
    static constexpr double HUGE_VAL_LUA = 1e308;  // Lua的huge值
    static constexpr double DEG_TO_RAD = PI / 180.0;
    static constexpr double RAD_TO_DEG = 180.0 / PI;
};

} // namespace stdlib
} // namespace lua_cpp