#pragma once

#include "vm/state.hpp"
#include "object/value.hpp"
#include <string>
#include <sstream>

namespace Lua {
namespace Lib {

// 错误处理工具函数
inline void throwTypeError(State* state, int arg, const char* expected) {
    std::stringstream ss;
    ss << "bad argument #" << arg << " (" << expected << " expected)";
    state->error(ss.str());
}

// 检查并获取boolean
inline bool checkBoolean(State* state, int arg) {
    if (state->isBoolean(arg)) {
        return state->toBoolean(arg);
    }
    throwTypeError(state, arg, "boolean");
    return false; // 不会执行到这里
}

// 检查并获取数字
inline double checkNumber(State* state, int arg) {
    if (state->isNumber(arg)) {
        return state->toNumber(arg);
    }
    throwTypeError(state, arg, "number");
    return 0.0; // 不会执行到这里
}

// 检查并获取整数
inline int checkInteger(State* state, int arg) {
    double num = checkNumber(state, arg);
    int intVal = static_cast<int>(num);
    if (intVal != num) {
        throwTypeError(state, arg, "integer");
    }
    return intVal;
}

// 检查并获取字符串
inline Str checkString(State* state, int arg) {
    if (state->isString(arg)) {
        return state->toString(arg);
    }
    throwTypeError(state, arg, "string");
    return ""; // 不会执行到这里
}

// 检查并获取表
inline Ptr<Table> checkTable(State* state, int arg) {
    if (state->isTable(arg)) {
        return state->toTable(arg);
    }
    throwTypeError(state, arg, "table");
    return nullptr; // 不会执行到这里
}

// 检查并获取函数
inline Ptr<Function> checkFunction(State* state, int arg) {
    if (state->isFunction(arg)) {
        return state->toFunction(arg);
    }
    throwTypeError(state, arg, "function");
    return nullptr; // 不会执行到这里
}

// 检查参数数量
inline void checkArgCount(State* state, int expected) {
    int actual = state->getTop();
    if (actual < expected) {
        std::stringstream ss;
        ss << "not enough arguments (expected " << expected << ", got " << actual << ")";
        state->error(ss.str());
    }
}

// 可选boolean参数
inline bool optBoolean(State* state, int arg, bool defaultValue) {
    if (state->getTop() < arg || state->isNil(arg)) {
        return defaultValue;
    }
    return checkBoolean(state, arg);
}

// 可选数字参数
inline double optNumber(State* state, int arg, double defaultValue) {
    if (state->getTop() < arg || state->isNil(arg)) {
        return defaultValue;
    }
    return checkNumber(state, arg);
}

// 可选整数参数
inline int optInteger(State* state, int arg, int defaultValue) {
    if (state->getTop() < arg || state->isNil(arg)) {
        return defaultValue;
    }
    return checkInteger(state, arg);
}

// 可选字符串参数
inline Str optString(State* state, int arg, const Str& defaultValue) {
    if (state->getTop() < arg || state->isNil(arg)) {
        return defaultValue;
    }
    return checkString(state, arg);
}

}} // namespace Lua::Lib
