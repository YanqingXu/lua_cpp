#include "stringlib.hpp"
#include "object/table.hpp"
#include <algorithm>
#include <cctype>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>

namespace Lua {
namespace Lib {

void openStringLib(State* state) {
    if (!state) return;
    
    // 创建string表
    Ptr<Table> stringTable = std::make_shared<Table>();
    
    // 注册字符串函数
    stringTable->set("len", Value(state->registerFunction(string_len)));
    stringTable->set("sub", Value(state->registerFunction(string_sub)));
    stringTable->set("upper", Value(state->registerFunction(string_upper)));
    stringTable->set("lower", Value(state->registerFunction(string_lower)));
    stringTable->set("char", Value(state->registerFunction(string_char)));
    stringTable->set("byte", Value(state->registerFunction(string_byte)));
    stringTable->set("rep", Value(state->registerFunction(string_rep)));
    stringTable->set("reverse", Value(state->registerFunction(string_reverse)));
    stringTable->set("format", Value(state->registerFunction(string_format)));
    stringTable->set("find", Value(state->registerFunction(string_find)));
    stringTable->set("match", Value(state->registerFunction(string_match)));
    stringTable->set("gsub", Value(state->registerFunction(string_gsub)));
    stringTable->set("gmatch", Value(state->registerFunction(string_gmatch)));
    
    // 创建字符串元表
    Ptr<Table> stringMeta = std::make_shared<Table>();
    stringMeta->set("__index", Value(stringTable));
    
    // 设置字符串元表
    state->setStringMetatable(stringMeta);
    
    // 将字符串表设置为全局变量
    state->getGlobals()->set("string", Value(stringTable));
}

// 工具函数：获取字符串参数
Str checkString(VM::State* state, int arg) {
    if (state->isString(arg)) {
        return state->toString(arg);
    } else {
        state->error("bad argument #" + std::to_string(arg) + " (string expected)");
        return "";
    }
}

// 处理Lua风格的索引（负数表示从尾部开始）
size_t normalizeIndex(int index, size_t len) {
    if (index > 0) {
        return std::min(static_cast<size_t>(index - 1), len);
    } else if (index < 0) {
        return std::min(len, std::max(static_cast<size_t>(0), len + static_cast<size_t>(index)));
    }
    return 0; // index == 0
}

// 字符串库函数实现
int string_len(VM::State* state) {
    Str s = checkString(state, 1);
    state->pushNumber(static_cast<double>(s.length()));
    return 1;
}

int string_sub(VM::State* state) {
    Str s = checkString(state, 1);
    int i = static_cast<int>(state->toNumber(2));
    int j = state->getTop() >= 3 ? static_cast<int>(state->toNumber(3)) : -1;
    
    size_t len = s.length();
    size_t start = normalizeIndex(i, len);
    size_t end = normalizeIndex(j, len);
    
    if (start > end || start >= len) {
        state->pushString("");
    } else {
        state->pushString(s.substr(start, end - start + 1));
    }
    
    return 1;
}

int string_upper(VM::State* state) {
    Str s = checkString(state, 1);
    Str result = s;
    
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::toupper(c); });
    
    state->pushString(result);
    return 1;
}

int string_lower(VM::State* state) {
    Str s = checkString(state, 1);
    Str result = s;
    
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    state->pushString(result);
    return 1;
}

int string_char(VM::State* state) {
    int n = state->getTop();
    std::string result;
    
    for (int i = 1; i <= n; i++) {
        int c = static_cast<int>(state->toNumber(i));
        if (c < 0 || c > 255) {
            state->error("bad argument #" + std::to_string(i) + " to 'char' (value out of range)");
        }
        result.push_back(static_cast<char>(c));
    }
    
    state->pushString(result);
    return 1;
}

int string_byte(VM::State* state) {
    Str s = checkString(state, 1);
    int i = state->getTop() >= 2 ? static_cast<int>(state->toNumber(2)) : 1;
    int j = state->getTop() >= 3 ? static_cast<int>(state->toNumber(3)) : i;
    
    size_t len = s.length();
    size_t start = normalizeIndex(i, len);
    size_t end = normalizeIndex(j, len);
    
    if (start > end || start >= len) {
        return 0; // 没有返回值
    }
    
    // 计算要返回的字节数
    int n = static_cast<int>(end - start + 1);
    
    // 返回字符的字节值
    for (size_t k = 0; k < static_cast<size_t>(n); k++) {
        state->pushNumber(static_cast<double>(static_cast<unsigned char>(s[start + k])));
    }
    
    return n;
}

int string_rep(VM::State* state) {
    Str s = checkString(state, 1);
    int n = static_cast<int>(state->toNumber(2));
    
    if (n <= 0) {
        state->pushString("");
        return 1;
    }
    
    std::string result;
    result.reserve(s.length() * n);
    
    for (int i = 0; i < n; i++) {
        result += s;
    }
    
    state->pushString(result);
    return 1;
}

int string_reverse(VM::State* state) {
    Str s = checkString(state, 1);
    std::string result = s;
    std::reverse(result.begin(), result.end());
    state->pushString(result);
    return 1;
}

int string_format(VM::State* state) {
    Str fmt = checkString(state, 1);
    int arg = 2;
    
    std::stringstream result;
    
    for (size_t i = 0; i < fmt.length(); i++) {
        if (fmt[i] != '%') {
            result << fmt[i];
            continue;
        }
        
        if (++i >= fmt.length()) {
            result << '%'; // 单独的%符号
            break;
        }
        
        switch (fmt[i]) {
            case '%': // %%
                result << '%';
                break;
                
            case 's': // %s
                if (arg > state->getTop()) {
                    state->error("bad argument #" + std::to_string(arg) + " to 'format' (no value)");
                }
                result << state->toString(arg++);
                break;
                
            case 'd': // %d
            case 'i': // %i
                if (arg > state->getTop()) {
                    state->error("bad argument #" + std::to_string(arg) + " to 'format' (no value)");
                }
                result << static_cast<int>(state->toNumber(arg++));
                break;
                
            case 'f': // %f
                if (arg > state->getTop()) {
                    state->error("bad argument #" + std::to_string(arg) + " to 'format' (no value)");
                }
                result << state->toNumber(arg++);
                break;
                
            case 'c': // %c
                if (arg > state->getTop()) {
                    state->error("bad argument #" + std::to_string(arg) + " to 'format' (no value)");
                }
                result << static_cast<char>(static_cast<int>(state->toNumber(arg++)));
                break;
                
            default:
                // 不支持的格式说明符
                result << '%' << fmt[i];
                break;
        }
    }
    
    state->pushString(result.str());
    return 1;
}

// 简单的字符串查找（不使用正则表达式）
int string_find(VM::State* state) {
    Str s = checkString(state, 1);
    Str pattern = checkString(state, 2);
    int init = state->getTop() >= 3 ? static_cast<int>(state->toNumber(3)) : 1;
    
    if (init < 1) {
        init = 1;
    } else if (init > static_cast<int>(s.length()) + 1) {
        state->pushNil();
        return 1;
    }
    
    init--; // 转换为0-based索引
    
    size_t pos = s.find(pattern, init);
    if (pos == std::string::npos) {
        state->pushNil();
        return 1;
    }
    
    state->pushNumber(static_cast<double>(pos + 1)); // 转换回1-based索引
    state->pushNumber(static_cast<double>(pos + pattern.length()));
    return 2;
}

// 这些函数需要完整的正则表达式支持，暂时提供简化版本
int string_match(VM::State* state) {
    Str s = checkString(state, 1);
    Str pattern = checkString(state, 2);
    
    // 简化版：只支持字面字符串匹配
    size_t pos = s.find(pattern);
    if (pos == std::string::npos) {
        state->pushNil();
        return 1;
    }
    
    state->pushString(pattern);
    return 1;
}

int string_gsub(VM::State* state) {
    Str s = checkString(state, 1);
    Str pattern = checkString(state, 2);
    Str repl = state->isString(3) ? state->toString(3) : "";
    int limit = state->getTop() >= 4 ? static_cast<int>(state->toNumber(4)) : -1;
    
    if (limit == 0) {
        state->pushString(s);
        state->pushNumber(0);
        return 2;
    }
    
    // 简化版：只支持字面字符串替换
    std::string result = s;
    size_t pos = 0;
    int count = 0;
    
    while ((limit < 0 || count < limit) && 
           (pos = result.find(pattern, pos)) != std::string::npos) {
        result.replace(pos, pattern.length(), repl);
        pos += repl.length();
        count++;
    }
    
    state->pushString(result);
    state->pushNumber(static_cast<double>(count));
    return 2;
}

int string_gmatch(VM::State* state) {
    // 简化版：返回一个简单的迭代器函数
    state->pushFunction(state->registerFunction([](VM::State* s) -> int {
        Str str = s->toString(1);
        Str pattern = s->toString(2);
        int pos = s->isNumber(3) ? static_cast<int>(s->toNumber(3)) : 0;
        
        // 查找下一个匹配
        size_t nextPos = str.find(pattern, pos);
        if (nextPos == std::string::npos) {
            return 0; // 没有更多匹配
        }
        
        // 返回匹配和新位置
        s->pushString(pattern);
        s->pushNumber(static_cast<double>(nextPos + pattern.length()));
        
        return 2;
    }));
    
    // 为迭代器传递参数
    state->pushValue(1); // 字符串
    state->pushValue(2); // 模式
    state->pushNumber(0); // 初始位置
    
    return 3;
}

}} // namespace Lua::Lib
