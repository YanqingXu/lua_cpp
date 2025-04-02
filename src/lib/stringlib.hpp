#pragma once

#include "vm/state.hpp"

namespace Lua {
namespace Lib {

// 打开字符串库
void openStringLib(VM::State* state);

// 字符串库函数
int string_len(VM::State* state);
int string_sub(VM::State* state);
int string_upper(VM::State* state);
int string_lower(VM::State* state);
int string_char(VM::State* state);
int string_byte(VM::State* state);
int string_rep(VM::State* state);
int string_reverse(VM::State* state);
int string_format(VM::State* state);
int string_find(VM::State* state);
int string_match(VM::State* state);
int string_gsub(VM::State* state);
int string_gmatch(VM::State* state);

}} // namespace Lua::Lib
