#include "mathlib.hpp"
#include "object/table.hpp"
#include <cmath>
#include <random>
#include <ctime>

namespace Lua {
namespace Lib {

// 随机数生成器
static std::mt19937 rng;
static bool rng_seeded = false;

void openMathLib(State* state) {
    if (!state) return;
    
    // 创建math表
    Ptr<Table> mathTable = std::make_shared<Table>();
    
    // 注册数学函数
    mathTable->set(Value("abs"), Value(state->registerFunction("abs", math_abs)));
    mathTable->set(Value("sin"), Value(state->registerFunction("sin", math_sin)));
    mathTable->set(Value("cos"), Value(state->registerFunction("cos", math_cos)));
    mathTable->set(Value("tan"), Value(state->registerFunction("tan", math_tan)));
    mathTable->set(Value("asin"), Value(state->registerFunction("asin", math_asin)));
    mathTable->set(Value("acos"), Value(state->registerFunction("acos", math_acos)));
    mathTable->set(Value("atan"), Value(state->registerFunction("atan", math_atan)));
    mathTable->set(Value("atan2"), Value(state->registerFunction("atan2", math_atan2)));
    mathTable->set(Value("ceil"), Value(state->registerFunction("ceil", math_ceil)));
    mathTable->set(Value("floor"), Value(state->registerFunction("floor", math_floor)));
    mathTable->set(Value("fmod"), Value(state->registerFunction("fmod", math_fmod)));
    mathTable->set(Value("modf"), Value(state->registerFunction("modf", math_modf)));
    mathTable->set(Value("sqrt"), Value(state->registerFunction("sqrt", math_sqrt)));
    mathTable->set(Value("pow"), Value(state->registerFunction("pow", math_pow)));
    mathTable->set(Value("log"), Value(state->registerFunction("log", math_log)));
    mathTable->set(Value("log10"), Value(state->registerFunction("log10", math_log10)));
    mathTable->set(Value("exp"), Value(state->registerFunction("exp", math_exp)));
    mathTable->set(Value("deg"), Value(state->registerFunction("deg", math_deg)));
    mathTable->set(Value("rad"), Value(state->registerFunction("rad", math_rad)));
    mathTable->set(Value("random"), Value(state->registerFunction("random", math_random)));
    mathTable->set(Value("randomseed"), Value(state->registerFunction("randomseed", math_randomseed)));
    mathTable->set(Value("min"), Value(state->registerFunction("min", math_min)));
    mathTable->set(Value("max"), Value(state->registerFunction("max", math_max)));
    
    // 设置数学常量
    mathTable->set(Value("pi"), Value(3.141592653589793));
    mathTable->set(Value("huge"), Value(INFINITY));
    
    // 将数学表设置为全局变量
    state->getGlobals()->set(Value("math"), Value(mathTable));
    
    // 初始化随机数生成器
    if (!rng_seeded) {
        rng.seed(static_cast<unsigned int>(std::time(nullptr)));
        rng_seeded = true;
    }
}

// 工具函数：检查数字参数
double checkNumber(State* state, int arg) {
    if (state->isNumber(arg)) {
        return state->toNumber(arg);
    } else {
        state->error("bad argument #" + std::to_string(arg) + " (number expected)");
        return 0.0;
    }
}

// 数学函数实现
int math_abs(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::fabs(x));
    return 1;
}

int math_sin(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::sin(x));
    return 1;
}

int math_cos(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::cos(x));
    return 1;
}

int math_tan(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::tan(x));
    return 1;
}

int math_asin(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::asin(x));
    return 1;
}

int math_acos(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::acos(x));
    return 1;
}

int math_atan(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::atan(x));
    return 1;
}

int math_atan2(State* state) {
    double y = checkNumber(state, 1);
    double x = checkNumber(state, 2);
    state->pushNumber(std::atan2(y, x));
    return 1;
}

int math_ceil(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::ceil(x));
    return 1;
}

int math_floor(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::floor(x));
    return 1;
}

int math_fmod(State* state) {
    double x = checkNumber(state, 1);
    double y = checkNumber(state, 2);
    state->pushNumber(std::fmod(x, y));
    return 1;
}

int math_modf(State* state) {
    double x = checkNumber(state, 1);
    double intPart;
    double fracPart = std::modf(x, &intPart);
    
    state->pushNumber(intPart);
    state->pushNumber(fracPart);
    return 2;
}

int math_sqrt(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::sqrt(x));
    return 1;
}

int math_pow(State* state) {
    double x = checkNumber(state, 1);
    double y = checkNumber(state, 2);
    state->pushNumber(std::pow(x, y));
    return 1;
}

int math_log(State* state) {
    double x = checkNumber(state, 1);
    
    if (state->getTop() >= 2) {
        // 使用指定的底数
        double base = checkNumber(state, 2);
        state->pushNumber(std::log(x) / std::log(base));
    } else {
        // 使用自然对数
        state->pushNumber(std::log(x));
    }
    
    return 1;
}

int math_log10(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::log10(x));
    return 1;
}

int math_exp(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(std::exp(x));
    return 1;
}

int math_deg(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(x * 180.0 / 3.141592653589793);
    return 1;
}

int math_rad(State* state) {
    double x = checkNumber(state, 1);
    state->pushNumber(x * 3.141592653589793 / 180.0);
    return 1;
}

int math_random(State* state) {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    if (state->getTop() == 0) {
        // 不带参数：返回[0,1)之间的随机数
        state->pushNumber(dist(rng));
    } else if (state->getTop() == 1) {
        // 一个参数：返回[1,n]之间的整数
        int n = static_cast<int>(checkNumber(state, 1));
        if (n < 1) {
            state->error("bad argument #1 to 'random' (interval is empty)");
        }
        
        std::uniform_int_distribution<int> intDist(1, n);
        state->pushNumber(static_cast<double>(intDist(rng)));
    } else {
        // 两个参数：返回[m,n]之间的整数
        int m = static_cast<int>(checkNumber(state, 1));
        int n = static_cast<int>(checkNumber(state, 2));
        
        if (m > n) {
            state->error("bad argument to 'random' (interval is empty)");
        }
        
        std::uniform_int_distribution<int> intDist(m, n);
        state->pushNumber(static_cast<double>(intDist(rng)));
    }
    
    return 1;
}

int math_randomseed(State* state) {
    unsigned int seed = static_cast<unsigned int>(checkNumber(state, 1));
    rng.seed(seed);
    rng_seeded = true;
    return 0;
}

int math_min(State* state) {
    int n = state->getTop();
    if (n < 1) {
        state->error("bad argument #1 to 'min' (value expected)");
    }
    
    int minIdx = 1;
    double minVal = checkNumber(state, minIdx);
    
    for (int i = 2; i <= n; i++) {
        double val = checkNumber(state, i);
        if (val < minVal) {
            minVal = val;
            minIdx = i;
        }
    }
    
	state->pushNumber(minIdx);
    return 1;
}

int math_max(State* state) {
    int n = state->getTop();
    if (n < 1) {
        state->error("bad argument #1 to 'max' (value expected)");
    }
    
    int maxIdx = 1;
    double maxVal = checkNumber(state, maxIdx);
    
    for (int i = 2; i <= n; i++) {
        double val = checkNumber(state, i);
        if (val > maxVal) {
            maxVal = val;
            maxIdx = i;
        }
    }
    
    state->pushNumber(maxIdx);
    return 1;
}

}} // namespace Lua::Lib
