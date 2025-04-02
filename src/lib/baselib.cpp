#include "baselib.hpp"
#include "types.hpp"
#include "object/table.hpp"
#include "object/userdata.hpp"
#include "object/function.hpp"
#include <iostream>
#include <string>
#include <cmath>

namespace Lua {
namespace Lib {

void openBaseLib(State* state) {
    if (!state) return;
    
    // 创建_G表
    Ptr<Table> globals = std::make_shared<Table>();
    
    // 注册基础函数
    globals->set("print", Value(state->registerFunction("print", print)));
    globals->set("type", Value(state->registerFunction("type", type)));
    globals->set("pairs", Value(state->registerFunction("pairs", pairs)));
    globals->set("ipairs", Value(state->registerFunction("ipairs", ipairs)));
    globals->set("next", Value(state->registerFunction("next", next)));
    globals->set("tonumber", Value(state->registerFunction("tonumber", tonumber)));
    globals->set("tostring", Value(state->registerFunction("tostring", tostring)));
    globals->set("error", Value(state->registerFunction("error", error)));
    globals->set("assert", Value(state->registerFunction("assert", assert_)));
    globals->set("pcall", Value(state->registerFunction("pcall", pcall)));
    
    // 设置全局表
    state->setGlobals(globals);
}

int print(State* state) {
    int nargs = state->getTop();
    
    for (int i = 1; i <= nargs; i++) {
        if (i > 1) std::cout << "\t";
        
        Value val = state->peek(i);
        std::cout << state->toString(i);
    }
    
    std::cout << std::endl;
    return 0;  // 没有返回值
}

int type(State* state) {
    if (state->getTop() < 1) {
        state->pushString("nil");
        return 1;
    }
    
    Value val = state->peek(1);
    switch (val.type()) {
        case ValueType::Nil:
            state->pushString("nil");
            break;
        case ValueType::Boolean:
            state->pushString("boolean");
            break;
        case ValueType::Number:
            state->pushString("number");
            break;
        case ValueType::String:
            state->pushString("string");
            break;
        case ValueType::Table:
            state->pushString("table");
            break;
        case ValueType::Function:
            state->pushString("function");
            break;
        case ValueType::UserData:
            state->pushString("userdata");
            break;
        case ValueType::Thread:
            state->pushString("thread");
            break;
        default:
            state->pushString("unknown");
            break;
    }
    
    return 1;  // 返回类型字符串
}

// 迭代器相关函数
int pairs(State* state) {
    // 检查参数
    if (state->getTop() < 1 || !state->isTable(1)) {
        state->error("bad argument #1 to 'pairs' (table expected)");
        return 0;
    }
    
    // 返回迭代器函数、表和初始索引nil
    state->pushFunction(state->registerFunction(next));  // 迭代器函数
    state->pushValue(1);                                 // 表
    state->pushNil();                                    // 初始索引nil
    
    return 3;  // 返回3个值
}

int next(State* state) {
    // 检查参数
    if (state->getTop() < 1 || !state->isTable(1)) {
        state->error("bad argument #1 to 'next' (table expected)");
        return 0;
    }
    
    // 获取表和键
    Ptr<Table> table = state->toTable(1);
    Value key = (state->getTop() >= 2) ? state->peek(2) : Value();
    
    // 获取下一个键值对
    Value nextKey, value;
    bool found = table->next(key, nextKey, value);
    
    if (!found) {
        state->pushNil();  // 没有下一个元素
        return 1;
    }
    
    // 返回下一个键和值
    state->push(nextKey);
    state->push(value);
    
    return 2;  // 返回键和值
}

int ipairs(State* state) {
    // 检查参数
    if (state->getTop() < 1 || !state->isTable(1)) {
        state->error("bad argument #1 to 'ipairs' (table expected)");
        return 0;
    }
    
    // 返回迭代器函数、表和初始索引0
    state->pushFunction(state->registerFunction([](State* s) -> int {
        // ipairs迭代器函数
        if (s->getTop() < 2 || !s->isTable(1) || !s->isNumber(2)) {
            s->error("bad argument to 'ipairs' iterator");
            return 0;
        }
        
        Ptr<Table> table = s->toTable(1);
        double indexDouble = s->toNumber(2);
        int index = static_cast<int>(indexDouble);
        int nextIndex = index + 1;
        
        // 获取下一个元素
        Value value = table->get(Value(static_cast<double>(nextIndex)));
        
        if (value.type() == ValueType::Nil) {
            return 0;  // 没有下一个元素
        }
        
        // 返回下一个索引和值
        s->pushNumber(nextIndex);
        s->push(value);
        
        return 2;  // 返回索引和值
    }));
    
    state->pushValue(1);         // 表
    state->pushNumber(0);        // 初始索引0
    
    return 3;  // 返回3个值
}

int tonumber(State* state) {
    if (state->getTop() < 1) {
        state->pushNil();
        return 1;
    }
    
    // 检查参数是否是数字或可转换为数字的字符串
    if (state->isNumber(1)) {
        state->pushValue(1);  // 已经是数字，直接返回
        return 1;
    } else if (state->isString(1)) {
        // 尝试将字符串转换为数字
        Str s = state->toString(1);
        try {
            double num = std::stod(s);
            state->pushNumber(num);
            return 1;
        } catch (...) {
            // 转换失败，返回nil
            state->pushNil();
            return 1;
        }
    }
    
    // 不是数字或字符串，返回nil
    state->pushNil();
    return 1;
}

int tostring(State* state) {
    if (state->getTop() < 1) {
        state->pushString("");
        return 1;
    }
    
    // 尝试获取__tostring元方法
    // TODO: 实现元方法调用
    
    // 使用默认字符串表示
    state->pushString(state->toString(1));
    return 1;
}

int error(State* state) {
    // 获取错误信息
    Str msg = (state->getTop() >= 1) ? state->toString(1) : "error";
    
    // 抛出错误
    state->error(msg);
    
    return 0;  // 不会到达这里
}

int assert_(State* state) {
    // 检查条件
    bool condition = state->toBoolean(1);
    
    if (!condition) {
        // 获取错误信息
        Str msg = (state->getTop() >= 2) ? state->toString(2) : "assertion failed!";
        
        // 抛出错误
        state->error(msg);
    }
    
    // 条件为true，返回所有参数
    return state->getTop();
}

int pcall(State* state) {
    // 检查参数
    if (state->getTop() < 1 || !state->isFunction(1)) {
        state->error("bad argument #1 to 'pcall' (function expected)");
        return 0;
    }
    
    // 获取参数数量
    int nargs = state->getTop() - 1;
    
    try {
        // 调用函数
        int nresults = state->call(nargs, -1);  // -1表示所有结果
        
        // 添加成功状态
        state->pushBoolean(true);
        state->insert(1);  // 将状态移到结果的最前面
        
        return nresults + 1;  // 返回状态和所有结果
    } catch (const LuaException& e) {
        // 函数调用出错
        state->setTop(0);  // 清空栈
        
        state->pushBoolean(false);  // 失败状态
        state->pushString(e.what());  // 错误信息
        
        return 2;  // 返回状态和错误信息
    }
}

}} // namespace Lua::Lib
