#pragma once

#include "vm/state.hpp"

// 导入各个库的头文件
#include "baselib.hpp"
#include "mathlib.hpp"
#include "stringlib.hpp"

namespace Lua {
namespace Lib {

// 打开所有标准库
void openLibs(State* state);

}} // namespace Lua::Lib
