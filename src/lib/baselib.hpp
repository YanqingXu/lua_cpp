#pragma once

#include "vm/state.hpp"

namespace Lua {
namespace Lib {

// 打开基础库
void openBaseLib(State* state);

// 注册的C函数
int print(State* state);
int type(State* state);
int pairs(State* state);
int ipairs(State* state);
int next(State* state);
int tonumber(State* state);
int tostring(State* state);
int error(State* state);
int assert_(State* state);
int pcall(State* state);

}} // namespace Lua::Lib
