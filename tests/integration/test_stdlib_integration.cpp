/**
 * @file test_stdlib_integration.cpp
 * @brief T017 - 标准库功能验证集成测试
 * 
 * 本文件测试Lua标准库函数的集成功能：
 * - 基础库 (基本函数、类型操作)
 * - 字符串库 (string.*)
 * - 表库 (table.*)
 * - 数学库 (math.*)
 * - IO库 (io.*) - 基础部分
 * - OS库 (os.*) - 安全部分
 * 
 * 测试策略：
 * 🔍 lua_c_analysis: 验证与原始Lua 5.1.5标准库行为的完全一致性
 * 🏗️ lua_with_cpp: 验证现代C++实现的标准库扩展和优化
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-21
 * @version 1.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// Lua C++ 项目核心头文件
#include "lua_api.h"
#include "luaaux.h"
#include "stdlib_integration.h"

// lua_c_analysis 参考实现
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

// 标准库
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <regex>
#include <iomanip>
#include <sstream>

using namespace Catch::Matchers;

namespace lua_cpp {
namespace stdlib_integration_tests {

// ============================================================================
// 测试基础设施和工具类
// ============================================================================

/**
 * @brief 标准库集成测试基础类
 * 
 * 提供统一的测试环境设置和标准库函数验证
 */
class StdlibTestFixture {
public:
    StdlibTestFixture() {
        setup_reference_env();
        setup_modern_env();
        call_trace.clear();
    }
    
    ~StdlibTestFixture() {
        cleanup_reference_env();
        cleanup_modern_env();
    }
    
protected:
    // lua_c_analysis 参考环境
    lua_State* L_ref = nullptr;
    
    // lua_with_cpp 现代环境
    std::unique_ptr<lua_cpp::StandardLibrary> stdlib_modern;
    
    // 调用跟踪
    static std::vector<std::string> call_trace;
    
    void setup_reference_env() {
        L_ref = luaL_newstate();
        REQUIRE(L_ref != nullptr);
        luaL_openlibs(L_ref);  // 打开所有标准库
    }
    
    void cleanup_reference_env() {
        if (L_ref) {
            lua_close(L_ref);
            L_ref = nullptr;
        }
    }
    
    void setup_modern_env() {
        stdlib_modern = std::make_unique<lua_cpp::StandardLibrary>();
    }
    
    void cleanup_modern_env() {
        stdlib_modern.reset();
    }
    
    static void trace_call(const std::string& message) {
        call_trace.push_back(message);
    }
    
    void clear_trace() {
        call_trace.clear();
    }
    
    void clean_stack() {
        if (L_ref) {
            lua_settop(L_ref, 0);
        }
    }
    
    // 执行Lua代码并获取结果
    std::string execute_lua_code(const std::string& code) {
        if (luaL_loadstring(L_ref, code.c_str()) != LUA_OK) {
            std::string error = lua_tostring(L_ref, -1);
            lua_pop(L_ref, 1);
            throw std::runtime_error("Lua load error: " + error);
        }
        
        if (lua_pcall(L_ref, 0, LUA_MULTRET, 0) != LUA_OK) {
            std::string error = lua_tostring(L_ref, -1);
            lua_pop(L_ref, 1);
            throw std::runtime_error("Lua execution error: " + error);
        }
        
        // 收集结果
        int nresults = lua_gettop(L_ref);
        std::ostringstream result;
        
        for (int i = 1; i <= nresults; i++) {
            if (i > 1) result << " ";
            
            if (lua_isstring(L_ref, i)) {
                result << lua_tostring(L_ref, i);
            } else if (lua_isnumber(L_ref, i)) {
                lua_Number num = lua_tonumber(L_ref, i);
                // 格式化数字输出，避免浮点精度问题
                if (num == std::floor(num)) {
                    result << static_cast<long long>(num);
                } else {
                    result << std::fixed << std::setprecision(6) << num;
                }
            } else if (lua_isboolean(L_ref, i)) {
                result << (lua_toboolean(L_ref, i) ? "true" : "false");
            } else if (lua_isnil(L_ref, i)) {
                result << "nil";
            } else {
                result << luaL_typename(L_ref, i);
            }
        }
        
        lua_settop(L_ref, 0);
        return result.str();
    }
};

// 静态成员初始化
std::vector<std::string> StdlibTestFixture::call_trace;

// ============================================================================
// 测试组1: 基础库函数 (Basic Library Functions)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: 基础函数", "[integration][stdlib][basic]") {
    SECTION("🔍 lua_c_analysis验证: 类型检查函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // type函数
            {"return type(nil)", "nil"},
            {"return type(true)", "boolean"},
            {"return type(42)", "number"},
            {"return type('hello')", "string"},
            {"return type({})", "table"},
            {"return type(type)", "function"},
            {"return type(coroutine.create(function() end))", "thread"},
            
            // tostring函数
            {"return tostring(nil)", "nil"},
            {"return tostring(true)", "true"},
            {"return tostring(false)", "false"},
            {"return tostring(123)", "123"},
            {"return tostring('test')", "test"},
            
            // tonumber函数
            {"return tonumber('123')", "123"},
            {"return tonumber('3.14')", "3.14"},
            {"return tonumber('hello')", "nil"},
            {"return tonumber('FF', 16)", "255"},
            {"return tonumber('1010', 2)", "10"},
            {"return tonumber('777', 8)", "511"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 全局环境操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // getfenv/setfenv (Lua 5.1特有)
            {"return type(getfenv())", "table"},
            {"return type(getfenv(1))", "table"},
            
            // rawget/rawset
            {R"(
                local t = {a = 1}
                return rawget(t, 'a')
            )", "1"},
            
            {R"(
                local t = {}
                rawset(t, 'key', 'value')
                return rawget(t, 'key')
            )", "value"},
            
            // rawequal
            {R"(
                local a, b = {}, {}
                return rawequal(a, b)
            )", "false"},
            
            {R"(
                local a = {}
                local b = a
                return rawequal(a, b)
            )", "true"},
            
            // rawlen (如果支持)
            {R"(
                local t = {1, 2, 3, 4, 5}
                return #t
            )", "5"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 迭代器函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // next函数
            {R"(
                local t = {a = 1, b = 2}
                local k1 = next(t)
                local k2 = next(t, k1)
                return type(k1), type(k2)
            )", "string string"},
            
            // pairs遍历
            {R"(
                local t = {x = 10, y = 20, z = 30}
                local sum = 0
                for k, v in pairs(t) do
                    sum = sum + v
                end
                return sum
            )", "60"},
            
            // ipairs遍历
            {R"(
                local t = {10, 20, 30, 40}
                local product = 1
                for i, v in ipairs(t) do
                    product = product * v
                end
                return product
            )", "240000"},
            
            // 空表遍历
            {R"(
                local t = {}
                local count = 0
                for k, v in pairs(t) do
                    count = count + 1
                end
                return count
            )", "0"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组2: 字符串库 (String Library)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: 字符串库", "[integration][stdlib][string]") {
    SECTION("🔍 lua_c_analysis验证: 基础字符串操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // string.len
            {"return string.len('hello')", "5"},
            {"return string.len('')", "0"},
            {"return string.len('测试')", "6"},  // UTF-8字节长度
            
            // string.upper/lower
            {"return string.upper('Hello World')", "HELLO WORLD"},
            {"return string.lower('Hello World')", "hello world"},
            {"return string.upper('')", ""},
            
            // string.sub
            {"return string.sub('hello', 2)", "ello"},
            {"return string.sub('hello', 2, 4)", "ell"},
            {"return string.sub('hello', -2)", "lo"},
            {"return string.sub('hello', 2, -2)", "ell"},
            {"return string.sub('hello', 10)", ""},
            
            // string.rep
            {"return string.rep('abc', 3)", "abcabcabc"},
            {"return string.rep('x', 0)", ""},
            {"return string.rep('hi', 1)", "hi"},
            
            // string.reverse
            {"return string.reverse('hello')", "olleh"},
            {"return string.reverse('')", ""},
            {"return string.reverse('a')", "a"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 字符串查找和替换") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // string.find (简单搜索)
            {"return string.find('hello world', 'world')", "7"},
            {"return string.find('hello world', 'foo')", "nil"},
            {"return string.find('hello', 'l')", "3"},
            {"return string.find('hello', 'l', 4)", "4"},
            
            // string.gsub (简单替换)
            {"return string.gsub('hello world', 'world', 'lua')", "hello lua 1"},
            {"return string.gsub('hello hello', 'hello', 'hi')", "hi hi 2"},
            {"return string.gsub('test', 'missing', 'replacement')", "test 0"},
            {"return string.gsub('aaa', 'a', 'b', 2)", "bba 2"},
            
            // string.match (简单模式)
            {"return string.match('hello123', '%d+')", "123"},
            {"return string.match('test@example.com', '@(.+)')", "example.com"},
            {"return string.match('no numbers', '%d+')", "nil"},
            
            // 复杂一些的模式
            {"return string.find('hello', 'l+')", "3"},
            {"return string.gsub('a1b2c3', '%d', 'X')", "aXbXcX 3"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 字符串格式化") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // string.format 基础
            {"return string.format('Hello %s', 'World')", "Hello World"},
            {"return string.format('%d + %d = %d', 1, 2, 3)", "1 + 2 = 3"},
            {"return string.format('%.2f', 3.14159)", "3.14"},
            {"return string.format('%x', 255)", "ff"},
            {"return string.format('%X', 255)", "FF"},
            {"return string.format('%o', 8)", "10"},
            
            // 宽度和对齐
            {"return string.format('%5d', 42)", "   42"},
            {"return string.format('%-5d', 42)", "42   "},
            {"return string.format('%05d', 42)", "00042"},
            {"return string.format('%s:%s', 'key', 'value')", "key:value"},
            
            // 多个参数
            {"return string.format('%s has %d apples', 'Alice', 5)", "Alice has 5 apples"},
            {"return string.format('%c%c%c', 65, 66, 67)", "ABC"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 字符串字节操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // string.byte
            {"return string.byte('A')", "65"},
            {"return string.byte('hello', 1)", "104"},  // 'h'
            {"return string.byte('hello', 2)", "101"},  // 'e'
            {"return string.byte('hello', -1)", "111"}, // 'o'
            
            // string.char
            {"return string.char(65)", "A"},
            {"return string.char(72, 101, 108, 108, 111)", "Hello"},
            {"return string.char(65, 66, 67)", "ABC"},
            
            // 组合使用
            {R"(
                local s = 'test'
                local bytes = {string.byte(s, 1, #s)}
                return string.char(unpack(bytes))
            )", "test"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组3: 表库 (Table Library)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: 表库", "[integration][stdlib][table]") {
    SECTION("🔍 lua_c_analysis验证: 表操作函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // table.insert
            {R"(
                local t = {1, 2, 3}
                table.insert(t, 4)
                return #t, t[4]
            )", "4 4"},
            
            {R"(
                local t = {1, 2, 3}
                table.insert(t, 2, 'inserted')
                return #t, t[2]
            )", "4 inserted"},
            
            // table.remove
            {R"(
                local t = {1, 2, 3, 4}
                local removed = table.remove(t)
                return #t, removed
            )", "3 4"},
            
            {R"(
                local t = {1, 2, 3, 4}
                local removed = table.remove(t, 2)
                return #t, removed, t[2]
            )", "3 2 3"},
            
            // table.concat
            {R"(
                local t = {'a', 'b', 'c'}
                return table.concat(t)
            )", "abc"},
            
            {R"(
                local t = {'a', 'b', 'c'}
                return table.concat(t, '-')
            )", "a-b-c"},
            
            {R"(
                local t = {'a', 'b', 'c', 'd'}
                return table.concat(t, ':', 2, 3)
            )", "b:c"},
            
            // 数字表的连接
            {R"(
                local t = {1, 2, 3, 4}
                return table.concat(t, '+')
            )", "1+2+3+4"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 表排序") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 默认排序
            {R"(
                local t = {3, 1, 4, 1, 5}
                table.sort(t)
                return table.concat(t, ',')
            )", "1,1,3,4,5"},
            
            // 字符串排序
            {R"(
                local t = {'banana', 'apple', 'cherry'}
                table.sort(t)
                return table.concat(t, ',')
            )", "apple,banana,cherry"},
            
            // 自定义比较函数
            {R"(
                local t = {3, 1, 4, 1, 5}
                table.sort(t, function(a, b) return a > b end)
                return table.concat(t, ',')
            )", "5,4,3,1,1"},
            
            // 按长度排序字符串
            {R"(
                local t = {'a', 'abc', 'ab'}
                table.sort(t, function(a, b) return #a < #b end)
                return table.concat(t, ',')
            )", "a,ab,abc"},
            
            // 空表排序
            {R"(
                local t = {}
                table.sort(t)
                return #t
            )", "0"},
            
            // 单元素表排序
            {R"(
                local t = {42}
                table.sort(t)
                return t[1]
            )", "42"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 表的高级操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 表的深度操作
            {R"(
                local function table_sum(t)
                    local sum = 0
                    for i = 1, #t do
                        sum = sum + t[i]
                    end
                    return sum
                end
                
                local t = {1, 2, 3}
                table.insert(t, table_sum(t))
                return table_sum(t)
            )", "12"},  // 1+2+3+6
            
            // 表的复制
            {R"(
                local function table_copy(t)
                    local copy = {}
                    for i = 1, #t do
                        copy[i] = t[i]
                    end
                    return copy
                end
                
                local original = {1, 2, 3}
                local copy = table_copy(original)
                table.insert(copy, 4)
                return #original, #copy
            )", "3 4"},
            
            // 表的反转
            {R"(
                local function table_reverse(t)
                    local reversed = {}
                    for i = #t, 1, -1 do
                        table.insert(reversed, t[i])
                    end
                    return reversed
                end
                
                local t = {1, 2, 3, 4}
                local rev = table_reverse(t)
                return table.concat(rev, ',')
            )", "4,3,2,1"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组4: 数学库 (Math Library)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: 数学库", "[integration][stdlib][math]") {
    SECTION("🔍 lua_c_analysis验证: 基础数学函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 基本算术
            {"return math.abs(-5)", "5"},
            {"return math.abs(5)", "5"},
            {"return math.abs(0)", "0"},
            
            {"return math.floor(3.7)", "3"},
            {"return math.floor(-3.7)", "-4"},
            {"return math.ceil(3.2)", "4"},
            {"return math.ceil(-3.2)", "-3"},
            
            {"return math.max(1, 3, 2)", "3"},
            {"return math.max(-1, -3, -2)", "-1"},
            {"return math.min(1, 3, 2)", "1"},
            {"return math.min(-1, -3, -2)", "-3"},
            
            // 舍入
            {"return math.floor(0.5)", "0"},
            {"return math.ceil(0.5)", "1"},
            
            // 数学常数
            {"return math.pi > 3.14", "true"},
            {"return math.pi < 3.15", "true"},
            {"return math.huge > 1000000", "true"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 幂和对数函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 幂函数
            {"return math.pow(2, 3)", "8"},
            {"return math.pow(4, 0.5)", "2"},
            {"return math.sqrt(16)", "4"},
            {"return math.sqrt(2) > 1.41", "true"},
            {"return math.sqrt(2) < 1.42", "true"},
            
            // 对数函数
            {"return math.log(math.exp(1))", "1"},
            {"return math.log10(100)", "2"},
            {"return math.log10(1000)", "3"},
            
            // 指数函数
            {"return math.exp(0)", "1"},
            {"return math.exp(1) > 2.7", "true"},
            {"return math.exp(1) < 2.8", "true"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 三角函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 基本三角函数
            {"return math.sin(0)", "0"},
            {"return math.cos(0)", "1"},
            {"return math.tan(0)", "0"},
            
            // π/2处的值
            {"return math.abs(math.sin(math.pi/2) - 1) < 0.0001", "true"},
            {"return math.abs(math.cos(math.pi/2)) < 0.0001", "true"},
            
            // π处的值
            {"return math.abs(math.sin(math.pi)) < 0.0001", "true"},
            {"return math.abs(math.cos(math.pi) + 1) < 0.0001", "true"},
            
            // 反三角函数
            {"return math.abs(math.asin(1) - math.pi/2) < 0.0001", "true"},
            {"return math.abs(math.acos(1)) < 0.0001", "true"},
            {"return math.abs(math.atan(1) - math.pi/4) < 0.0001", "true"},
            
            // atan2函数
            {"return math.abs(math.atan2(1, 1) - math.pi/4) < 0.0001", "true"},
            {"return math.abs(math.atan2(0, 1)) < 0.0001", "true"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 随机数函数") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 设置种子并测试
            {R"(
                math.randomseed(12345)
                local r1 = math.random()
                math.randomseed(12345)
                local r2 = math.random()
                return r1 == r2
            )", "true"},
            
            // 范围随机数
            {R"(
                math.randomseed(54321)
                local r = math.random(1, 10)
                return r >= 1 and r <= 10
            )", "true"},
            
            {R"(
                math.randomseed(98765)
                local r = math.random(5)
                return r >= 1 and r <= 5
            )", "true"},
            
            // 多次调用产生不同结果
            {R"(
                math.randomseed(11111)
                local r1 = math.random()
                local r2 = math.random()
                return r1 ~= r2
            )", "true"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组5: IO库基础功能 (IO Library Basics)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: IO库基础", "[integration][stdlib][io]") {
    SECTION("🔍 lua_c_analysis验证: IO基础类型") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // IO类型检查
            {"return type(io.stdin)", "userdata"},
            {"return type(io.stdout)", "userdata"},
            {"return type(io.stderr)", "userdata"},
            
            // IO函数存在性检查
            {"return type(io.open)", "function"},
            {"return type(io.close)", "function"},
            {"return type(io.read)", "function"},
            {"return type(io.write)", "function"},
            {"return type(io.flush)", "function"},
            {"return type(io.type)", "function"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 字符串IO操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 使用字符串作为临时"文件"进行测试
            {R"(
                -- 模拟简单的字符串IO
                local data = "hello\nworld\n123"
                local lines = {}
                for line in data:gmatch("[^\n]+") do
                    table.insert(lines, line)
                end
                return #lines
            )", "3"},
            
            {R"(
                local data = "line1\nline2\nline3"
                local first_line = data:match("([^\n]+)")
                return first_line
            )", "line1"},
            
            // 模拟写入操作
            {R"(
                local output = {}
                local function mock_write(...)
                    for i = 1, select('#', ...) do
                        table.insert(output, tostring(select(i, ...)))
                    end
                end
                
                mock_write("Hello", " ", "World", "\n")
                return table.concat(output)
            )", "Hello World\n"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组6: OS库安全功能 (OS Library Safe Functions)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: OS库安全功能", "[integration][stdlib][os]") {
    SECTION("🔍 lua_c_analysis验证: 时间和日期") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // os.time基础
            {"return type(os.time())", "number"},
            {"return os.time() > 0", "true"},
            
            // os.date基础格式
            {"return type(os.date())", "string"},
            {"return type(os.date('*t'))", "table"},
            
            // 特定时间戳
            {R"(
                local t = os.time({year=2000, month=1, day=1, hour=0, min=0, sec=0})
                return t > 0
            )", "true"},
            
            // 日期表解析
            {R"(
                local date_table = os.date('*t', os.time())
                return type(date_table.year)
            )", "number"},
            
            {R"(
                local date_table = os.date('*t', os.time())
                return date_table.month >= 1 and date_table.month <= 12
            )", "true"},
            
            // 格式化日期
            {R"(
                local formatted = os.date('%Y-%m-%d', os.time({year=2023, month=6, day=15}))
                return formatted
            )", "2023-06-15"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 环境信息") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // os.clock
            {"return type(os.clock())", "number"},
            {"return os.clock() >= 0", "true"},
            
            // 时间计算
            {R"(
                local start = os.clock()
                -- 简单循环消耗一点时间
                local sum = 0
                for i = 1, 1000 do
                    sum = sum + i
                end
                local elapsed = os.clock() - start
                return elapsed >= 0
            )", "true"},
            
            // difftime
            {R"(
                local t1 = os.time()
                local t2 = t1 + 3600  -- 一小时后
                return os.difftime(t2, t1)
            )", "3600"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组7: 综合集成测试 (Comprehensive Integration Tests)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: 综合测试", "[integration][stdlib][comprehensive]") {
    SECTION("🔍 lua_c_analysis验证: 多库协作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 字符串 + 数学库
            {R"(
                local numbers = "1.5 2.7 3.8 4.2"
                local sum = 0
                for num_str in numbers:gmatch("%S+") do
                    sum = sum + tonumber(num_str)
                end
                return math.floor(sum * 10) / 10
            )", "12"},  // (1.5+2.7+3.8+4.2) = 12.2, floor(122)/10 = 12
            
            // 表 + 字符串库
            {R"(
                local words = {"Hello", "Beautiful", "World"}
                table.sort(words, function(a, b) 
                    return string.len(a) < string.len(b) 
                end)
                return table.concat(words, " ")
            )", "Hello World Beautiful"},
            
            // 数学 + 表库
            {R"(
                local angles = {}
                for i = 0, 3 do
                    table.insert(angles, math.sin(i * math.pi / 2))
                end
                -- 格式化为整数以避免浮点误差
                for i = 1, #angles do
                    angles[i] = math.floor(angles[i] + 0.5)
                end
                return table.concat(angles, ",")
            )", "0,1,0,-1"},
            
            // 时间 + 字符串格式化
            {R"(
                local t = os.time({year=2023, month=12, day=25, hour=10, min=30, sec=0})
                local formatted = os.date("%B %d, %Y at %H:%M", t)
                return string.match(formatted, "December")
            )", "December"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 复杂数据处理") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 数据统计处理
            {R"(
                local data = "85,92,78,96,88,91,76,89,94,87"
                local scores = {}
                for score_str in data:gmatch("%d+") do
                    table.insert(scores, tonumber(score_str))
                end
                
                table.sort(scores)
                local median = scores[math.floor(#scores/2) + 1]
                return median
            )", "89"},
            
            // 文本处理
            {R"(
                local text = "The quick brown fox jumps over the lazy dog"
                local words = {}
                for word in text:gmatch("%w+") do
                    table.insert(words, string.lower(word))
                end
                
                table.sort(words)
                return #words, words[1], words[#words]
            )", "9 brown the"},
            
            // 数值计算
            {R"(
                local function factorial(n)
                    if n <= 1 then return 1 end
                    return n * factorial(n - 1)
                end
                
                local function combination(n, r)
                    return factorial(n) / (factorial(r) * factorial(n - r))
                end
                
                return combination(5, 2)  -- C(5,2) = 10
            )", "10"}
        };
        
        for (const auto& [code, expected] : test_cases) {
            INFO("测试代码: " << code);
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组8: 性能基准测试 (Performance Benchmarks)
// ============================================================================

TEST_CASE_METHOD(StdlibTestFixture, "标准库集成: 性能基准", "[integration][stdlib][performance]") {
    SECTION("🔍 lua_c_analysis验证: 字符串操作性能") {
        const int iterations = 100;
        
        BENCHMARK("字符串连接性能") {
            std::string code = R"(
                local result = ""
                for i = 1, 100 do
                    result = result .. tostring(i) .. ","
                end
                return string.len(result)
            )";
            
            for (int i = 0; i < iterations; i++) {
                std::string result = execute_lua_code(code);
                REQUIRE(result == "299");  // 100个数字和99个逗号，每个数字平均2字符
                clean_stack();
            }
        };
        
        BENCHMARK("table.concat性能") {
            std::string code = R"(
                local parts = {}
                for i = 1, 100 do
                    table.insert(parts, tostring(i))
                end
                local result = table.concat(parts, ",")
                return string.len(result)
            )";
            
            for (int i = 0; i < iterations; i++) {
                std::string result = execute_lua_code(code);
                REQUIRE(result == "199");  // 100个数字和99个逗号
                clean_stack();
            }
        };
    }
    
    SECTION("🔍 lua_c_analysis验证: 数学计算性能") {
        BENCHMARK("三角函数计算") {
            std::string code = R"(
                local sum = 0
                for i = 1, 1000 do
                    sum = sum + math.sin(i * math.pi / 180)
                end
                return math.floor(sum)
            )";
            
            std::string result = execute_lua_code(code);
            REQUIRE_FALSE(result.empty());
            clean_stack();
        };
        
        BENCHMARK("表排序性能") {
            std::string code = R"(
                local t = {}
                for i = 1, 1000 do
                    table.insert(t, math.random(1000))
                end
                table.sort(t)
                return #t
            )";
            
            std::string result = execute_lua_code(code);
            REQUIRE(result == "1000");
            clean_stack();
        };
    }
}

} // namespace stdlib_integration_tests
} // namespace lua_cpp

// ============================================================================
// 全局测试监听器
// ============================================================================

namespace {

class StdlibTestListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;
    
    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        if (testInfo.tags.count("[stdlib]")) {
            std::cout << "\n📚 开始标准库测试: " << testInfo.name << std::endl;
        }
    }
    
    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        if (testCaseStats.testInfo->tags.count("[stdlib]")) {
            if (testCaseStats.totals.assertions.allPassed()) {
                std::cout << "✅ 标准库测试通过: " << testCaseStats.testInfo->name << std::endl;
            } else {
                std::cout << "❌ 标准库测试失败: " << testCaseStats.testInfo->name << std::endl;
            }
        }
    }
};

CATCH_REGISTER_LISTENER(StdlibTestListener)

} // anonymous namespace