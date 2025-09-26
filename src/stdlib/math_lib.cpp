/**
 * @file math_lib.cpp
 * @brief T027 Lua数学库实现
 * 
 * 实现Lua 5.1.5数学库的所有函数
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#include "math_lib.h"
#include <cmath>
#include <algorithm>
#include <chrono>
#include <limits>

namespace lua_cpp {
namespace stdlib {

// ============================================================================
// 静态成员初始化
// ============================================================================

std::mt19937 MathLibrary::random_generator_;
bool MathLibrary::random_seed_set_ = false;

// ============================================================================
// MathLibrary类实现
// ============================================================================

MathLibrary::MathLibrary() = default;

std::vector<LibFunction> MathLibrary::GetFunctions() const {
    std::vector<LibFunction> functions;
    
    // 三角函数
    REGISTER_FUNCTION(functions, "sin", lua_math_sin, "正弦函数");
    REGISTER_FUNCTION(functions, "cos", lua_math_cos, "余弦函数");
    REGISTER_FUNCTION(functions, "tan", lua_math_tan, "正切函数");
    REGISTER_FUNCTION(functions, "asin", lua_math_asin, "反正弦函数");
    REGISTER_FUNCTION(functions, "acos", lua_math_acos, "反余弦函数");
    REGISTER_FUNCTION(functions, "atan", lua_math_atan, "反正切函数");
    REGISTER_FUNCTION(functions, "atan2", lua_math_atan2, "双参数反正切函数");
    
    // 指数和对数函数
    REGISTER_FUNCTION(functions, "exp", lua_math_exp, "指数函数");
    REGISTER_FUNCTION(functions, "log", lua_math_log, "自然对数");
    REGISTER_FUNCTION(functions, "log10", lua_math_log10, "常用对数");
    REGISTER_FUNCTION(functions, "pow", lua_math_pow, "幂函数");
    REGISTER_FUNCTION(functions, "sqrt", lua_math_sqrt, "平方根");
    
    // 取整和绝对值函数
    REGISTER_FUNCTION(functions, "floor", lua_math_floor, "向下取整");
    REGISTER_FUNCTION(functions, "ceil", lua_math_ceil, "向上取整");
    REGISTER_FUNCTION(functions, "abs", lua_math_abs, "绝对值");
    REGISTER_FUNCTION(functions, "fmod", lua_math_fmod, "浮点取模");
    REGISTER_FUNCTION(functions, "modf", lua_math_modf, "分离整数和小数部分");
    
    // 最值函数
    REGISTER_FUNCTION(functions, "max", lua_math_max, "最大值");
    REGISTER_FUNCTION(functions, "min", lua_math_min, "最小值");
    
    // 角度转换
    REGISTER_FUNCTION(functions, "deg", lua_math_deg, "弧度转角度");
    REGISTER_FUNCTION(functions, "rad", lua_math_rad, "角度转弧度");
    
    // 随机数函数
    REGISTER_FUNCTION(functions, "random", lua_math_random, "生成随机数");
    REGISTER_FUNCTION(functions, "randomseed", lua_math_randomseed, "设置随机数种子");
    
    return functions;
}

void MathLibrary::RegisterModule(EnhancedVirtualMachine* vm) {
    if (!vm) return;
    
    auto functions = GetFunctions();
    
    // 创建math表
    LuaTable math_table;
    
    // 注册所有函数
    for (const auto& func : functions) {
        LuaValue func_value = LuaValue::CreateFunction(
            [f = func.func](EnhancedVirtualMachine* vm) -> std::vector<LuaValue> {
                int result_count = f(vm);
                auto& stack = vm->GetStack();
                
                std::vector<LuaValue> results;
                if (result_count > 0 && static_cast<size_t>(result_count) <= stack.size()) {
                    results.reserve(result_count);
                    for (int i = stack.size() - result_count; i < static_cast<int>(stack.size()); i++) {
                        results.push_back(stack[i]);
                    }
                    // 清理栈上的返回值
                    for (int i = 0; i < result_count; i++) {
                        stack.pop_back();
                    }
                }
                
                return results;
            }
        );
        
        math_table.SetField(LuaValue::CreateString(func.name), func_value);
    }
    
    // 注册数学常数
    math_table.SetField(LuaValue::CreateString("pi"), LuaValue::CreateNumber(PI));
    math_table.SetField(LuaValue::CreateString("huge"), LuaValue::CreateNumber(HUGE_VAL_LUA));
    
    // 注册math表到全局环境
    auto& globals = vm->GetGlobalEnvironment();
    globals.SetField("math", LuaValue::CreateTable(math_table));
}

void MathLibrary::Initialize(EnhancedVirtualMachine* vm) {
    (void)vm;
    
    // 初始化随机数生成器
    if (!random_seed_set_) {
        auto now = std::chrono::high_resolution_clock::now();
        auto seed = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
        random_generator_.seed(static_cast<std::mt19937::result_type>(seed));
        random_seed_set_ = true;
    }
}

void MathLibrary::Cleanup(EnhancedVirtualMachine* vm) {
    (void)vm;  // 数学库暂时不需要特殊清理
}

// ============================================================================
// 三角函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_sin) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.sin");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.sin", 1);
    double result = std::sin(x);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 压入结果
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.sin")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_cos) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.cos");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.cos", 1);
    double result = std::cos(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.cos")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_tan) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.tan");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.tan", 1);
    double result = std::tan(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.tan")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_asin) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.asin");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.asin", 1);
    
    if (x < -1.0 || x > 1.0) {
        ErrorHelper::RuntimeError("math.asin", "input out of range [-1, 1]");
    }
    
    double result = std::asin(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.asin")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_acos) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.acos");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.acos", 1);
    
    if (x < -1.0 || x > 1.0) {
        ErrorHelper::RuntimeError("math.acos", "input out of range [-1, 1]");
    }
    
    double result = std::acos(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.acos")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_atan) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.atan");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.atan", 1);
    double result = std::atan(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.atan")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_atan2) {
    StackHelper helper(vm);
    helper.CheckArgCount(2, "math.atan2");
    
    double y = CheckNumberArg(vm->GetStack()[0], "math.atan2", 1);
    double x = CheckNumberArg(vm->GetStack()[1], "math.atan2", 2);
    double result = std::atan2(y, x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.atan2")));
    
    return 1;
}

// ============================================================================
// 指数和对数函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_exp) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.exp");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.exp", 1);
    double result = std::exp(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.exp")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_log) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.log");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.log", 1);
    
    if (x <= 0.0) {
        ErrorHelper::RuntimeError("math.log", "input must be positive");
    }
    
    double result = std::log(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.log")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_log10) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.log10");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.log10", 1);
    
    if (x <= 0.0) {
        ErrorHelper::RuntimeError("math.log10", "input must be positive");
    }
    
    double result = std::log10(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.log10")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_pow) {
    StackHelper helper(vm);
    helper.CheckArgCount(2, "math.pow");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.pow", 1);
    double y = CheckNumberArg(vm->GetStack()[1], "math.pow", 2);
    
    double result = std::pow(x, y);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.pow")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_sqrt) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.sqrt");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.sqrt", 1);
    
    if (x < 0.0) {
        ErrorHelper::RuntimeError("math.sqrt", "input must be non-negative");
    }
    
    double result = std::sqrt(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.sqrt")));
    
    return 1;
}

// ============================================================================
// 取整和绝对值函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_floor) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.floor");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.floor", 1);
    double result = std::floor(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(result));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_ceil) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.ceil");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.ceil", 1);
    double result = std::ceil(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(result));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_abs) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.abs");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.abs", 1);
    double result = std::abs(x);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(result));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_fmod) {
    StackHelper helper(vm);
    helper.CheckArgCount(2, "math.fmod");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.fmod", 1);
    double y = CheckNumberArg(vm->GetStack()[1], "math.fmod", 2);
    
    if (y == 0.0) {
        ErrorHelper::RuntimeError("math.fmod", "division by zero");
    }
    
    double result = std::fmod(x, y);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(CheckMathResult(result, "math.fmod")));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_modf) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.modf");
    
    double x = CheckNumberArg(vm->GetStack()[0], "math.modf", 1);
    
    double integral_part;
    double fractional_part = std::modf(x, &integral_part);
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(integral_part));
    vm->GetStack().push_back(LuaValue::CreateNumber(fractional_part));
    
    return 2;
}

// ============================================================================
// 最值函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_max) {
    auto& stack = vm->GetStack();
    
    if (stack.empty()) {
        ErrorHelper::ArgError("math.max", -1, "at least one argument expected");
    }
    
    double max_val = CheckNumberArg(stack[0], "math.max", 1);
    
    for (size_t i = 1; i < stack.size(); i++) {
        double val = CheckNumberArg(stack[i], "math.max", static_cast<int>(i + 1));
        max_val = std::max(max_val, val);
    }
    
    stack.clear();
    stack.push_back(LuaValue::CreateNumber(max_val));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_min) {
    auto& stack = vm->GetStack();
    
    if (stack.empty()) {
        ErrorHelper::ArgError("math.min", -1, "at least one argument expected");
    }
    
    double min_val = CheckNumberArg(stack[0], "math.min", 1);
    
    for (size_t i = 1; i < stack.size(); i++) {
        double val = CheckNumberArg(stack[i], "math.min", static_cast<int>(i + 1));
        min_val = std::min(min_val, val);
    }
    
    stack.clear();
    stack.push_back(LuaValue::CreateNumber(min_val));
    
    return 1;
}

// ============================================================================
// 角度转换函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_deg) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.deg");
    
    double radians = CheckNumberArg(vm->GetStack()[0], "math.deg", 1);
    double degrees = radians * RAD_TO_DEG;
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(degrees));
    
    return 1;
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_rad) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.rad");
    
    double degrees = CheckNumberArg(vm->GetStack()[0], "math.rad", 1);
    double radians = degrees * DEG_TO_RAD;
    
    vm->GetStack().clear();
    vm->GetStack().push_back(LuaValue::CreateNumber(radians));
    
    return 1;
}

// ============================================================================
// 随机数函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_random) {
    auto& stack = vm->GetStack();
    
    if (stack.empty()) {
        // math.random() - 返回[0,1)的随机数
        std::uniform_real_distribution<double> dist(0.0, 1.0);
        double result = dist(random_generator_);
        
        stack.clear();
        stack.push_back(LuaValue::CreateNumber(result));
        
        return 1;
    } else if (stack.size() == 1) {
        // math.random(m) - 返回[1,m]的随机整数
        int m = static_cast<int>(CheckNumberArg(stack[0], "math.random", 1));
        
        if (m < 1) {
            ErrorHelper::ArgError("math.random", 1, "interval is empty");
        }
        
        std::uniform_int_distribution<int> dist(1, m);
        int result = dist(random_generator_);
        
        stack.clear();
        stack.push_back(LuaValue::CreateNumber(static_cast<double>(result)));
        
        return 1;
    } else if (stack.size() == 2) {
        // math.random(m, n) - 返回[m,n]的随机整数
        int m = static_cast<int>(CheckNumberArg(stack[0], "math.random", 1));
        int n = static_cast<int>(CheckNumberArg(stack[1], "math.random", 2));
        
        if (m > n) {
            ErrorHelper::ArgError("math.random", -1, "interval is empty");
        }
        
        std::uniform_int_distribution<int> dist(m, n);
        int result = dist(random_generator_);
        
        stack.clear();
        stack.push_back(LuaValue::CreateNumber(static_cast<double>(result)));
        
        return 1;
    } else {
        ErrorHelper::ArgError("math.random", -1, "expected 0-2 arguments");
        return 0;
    }
}

LUA_STDLIB_FUNCTION(MathLibrary::lua_math_randomseed) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "math.randomseed");
    
    double seed_val = CheckNumberArg(vm->GetStack()[0], "math.randomseed", 1);
    std::mt19937::result_type seed = static_cast<std::mt19937::result_type>(seed_val);
    
    random_generator_.seed(seed);
    random_seed_set_ = true;
    
    vm->GetStack().clear();
    
    return 0;  // math.randomseed不返回值
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

double MathLibrary::CheckNumberArg(const LuaValue& value, 
                                 const std::string& func_name, 
                                 int arg_index) {
    if (value.GetType() != LuaValueType::NUMBER) {
        ErrorHelper::TypeError(func_name, arg_index, "number", 
                             (value.GetType() == LuaValueType::STRING) ? "string" : "other");
    }
    
    return value.AsNumber();
}

double MathLibrary::CheckMathResult(double result, const std::string& func_name) {
    if (std::isnan(result)) {
        ErrorHelper::RuntimeError(func_name, "result is NaN");
    }
    
    if (std::isinf(result)) {
        // 在Lua中，无穷大是允许的
        return result;
    }
    
    return result;
}

} // namespace stdlib
} // namespace lua_cpp