#pragma once

#include "vm/state.hpp"

namespace Lua {
namespace Lib {

// 打开数学库
void openMathLib(State* state);

// 数学库函数
int math_abs(State* state);
int math_sin(State* state);
int math_cos(State* state);
int math_tan(State* state);
int math_asin(State* state);
int math_acos(State* state);
int math_atan(State* state);
int math_atan2(State* state);
int math_ceil(State* state);
int math_floor(State* state);
int math_fmod(State* state);
int math_modf(State* state);
int math_sqrt(State* state);
int math_pow(State* state);
int math_log(State* state);
int math_log10(State* state);
int math_exp(State* state);
int math_deg(State* state);
int math_rad(State* state);
int math_random(State* state);
int math_randomseed(State* state);
int math_min(State* state);
int math_max(State* state);

}} // namespace Lua::Lib
