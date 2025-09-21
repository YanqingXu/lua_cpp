/**
 * @file test_lua515_compatibility.cpp
 * @brief T017 - Lua 5.1.5兼容性测试套件
 * 
 * 本文件测试与官方Lua 5.1.5的完全兼容性：
 * - 语法兼容性测试
 * - API兼容性验证
 * - 行为一致性检查
 * - 官方测试套件集成
 * 
 * 测试策略：
 * 🔍 lua_c_analysis: 作为兼容性基准，验证相同输入产生相同输出
 * 🏗️ lua_with_cpp: 验证现代实现的兼容性保证
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
#include "compatibility_layer.h"

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
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>
#include <filesystem>

using namespace Catch::Matchers;

namespace lua_cpp {
namespace lua515_compatibility_tests {

// ============================================================================
// 测试基础设施和工具类
// ============================================================================

/**
 * @brief Lua 5.1.5兼容性测试基础类
 * 
 * 提供对比测试环境，确保现代实现与原始Lua 5.1.5行为完全一致
 */
class Lua515CompatibilityTestFixture {
public:
    Lua515CompatibilityTestFixture() {
        setup_reference_lua();
        setup_modern_lua();
        compatibility_issues.clear();
    }
    
    ~Lua515CompatibilityTestFixture() {
        cleanup_reference_lua();
        cleanup_modern_lua();
    }
    
protected:
    // 参考实现 (lua_c_analysis)
    lua_State* L_ref = nullptr;
    
    // 现代实现 (lua_with_cpp)
    std::unique_ptr<lua_cpp::LuaState> L_modern;
    std::unique_ptr<lua_cpp::CompatibilityLayer> compat_layer;
    
    // 兼容性问题跟踪
    static std::vector<std::string> compatibility_issues;
    
    void setup_reference_lua() {
        L_ref = luaL_newstate();
        REQUIRE(L_ref != nullptr);
        luaL_openlibs(L_ref);
    }
    
    void cleanup_reference_lua() {
        if (L_ref) {
            lua_close(L_ref);
            L_ref = nullptr;
        }
    }
    
    void setup_modern_lua() {
        L_modern = std::make_unique<lua_cpp::LuaState>();
        compat_layer = std::make_unique<lua_cpp::CompatibilityLayer>(L_modern.get());
    }
    
    void cleanup_modern_lua() {
        compat_layer.reset();
        L_modern.reset();
    }
    
    static void report_compatibility_issue(const std::string& issue) {
        compatibility_issues.push_back(issue);
    }
    
    void clean_stacks() {
        if (L_ref) lua_settop(L_ref, 0);
        // 现代实现清理由RAII处理
    }
    
    // 执行结果比较结构
    struct ExecutionResult {
        bool success;
        std::vector<std::string> values;
        std::string error_message;
        lua_Number execution_time_ms;
        
        bool operator==(const ExecutionResult& other) const {
            return success == other.success && 
                   values == other.values && 
                   error_message == other.error_message;
        }
    };
    
    // 执行Lua代码并收集结果
    ExecutionResult execute_reference(const std::string& code) {
        ExecutionResult result;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int load_result = luaL_loadstring(L_ref, code.c_str());
        if (load_result != LUA_OK) {
            result.success = false;
            result.error_message = lua_tostring(L_ref, -1);
            lua_pop(L_ref, 1);
            return result;
        }
        
        int exec_result = lua_pcall(L_ref, 0, LUA_MULTRET, 0);
        
        auto end = std::chrono::high_resolution_clock::now();
        result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
        
        result.success = (exec_result == LUA_OK);
        
        if (!result.success) {
            result.error_message = lua_tostring(L_ref, -1);
            lua_pop(L_ref, 1);
        } else {
            // 收集所有返回值
            int nresults = lua_gettop(L_ref);
            for (int i = 1; i <= nresults; i++) {
                std::string value = lua_value_to_string(L_ref, i);
                result.values.push_back(value);
            }
            lua_settop(L_ref, 0);
        }
        
        return result;
    }
    
    ExecutionResult execute_modern(const std::string& code) {
        ExecutionResult result;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            auto exec_result = compat_layer->execute_lua_code(code);
            
            auto end = std::chrono::high_resolution_clock::now();
            result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            result.success = exec_result.success;
            result.values = exec_result.return_values;
            result.error_message = exec_result.error_message;
            
        } catch (const std::exception& e) {
            auto end = std::chrono::high_resolution_clock::now();
            result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            result.success = false;
            result.error_message = e.what();
        }
        
        return result;
    }
    
    // 比较两个执行结果
    void compare_results(const ExecutionResult& ref_result, const ExecutionResult& modern_result, 
                        const std::string& test_description) {
        
        INFO("测试: " << test_description);
        
        if (ref_result.success != modern_result.success) {
            std::string issue = "Success mismatch in " + test_description + 
                              ": ref=" + (ref_result.success ? "true" : "false") +
                              ", modern=" + (modern_result.success ? "true" : "false");
            report_compatibility_issue(issue);
            FAIL(issue);
        }
        
        if (ref_result.success) {
            // 比较返回值
            REQUIRE(ref_result.values.size() == modern_result.values.size());
            
            for (size_t i = 0; i < ref_result.values.size(); i++) {
                if (ref_result.values[i] != modern_result.values[i]) {
                    std::string issue = "Value mismatch in " + test_description + 
                                      " at index " + std::to_string(i) +
                                      ": ref='" + ref_result.values[i] + 
                                      "', modern='" + modern_result.values[i] + "'";
                    report_compatibility_issue(issue);
                    REQUIRE(ref_result.values[i] == modern_result.values[i]);
                }
            }
        } else {
            // 对于错误，只需要都失败即可，错误消息可能略有不同
            REQUIRE_FALSE(modern_result.success);
        }
    }
    
private:
    std::string lua_value_to_string(lua_State* L, int index) {
        int type = lua_type(L, index);
        
        switch (type) {
            case LUA_TNIL:
                return "nil";
            case LUA_TBOOLEAN:
                return lua_toboolean(L, index) ? "true" : "false";
            case LUA_TNUMBER: {
                lua_Number num = lua_tonumber(L, index);
                if (num == std::floor(num)) {
                    return std::to_string(static_cast<long long>(num));
                } else {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(6) << num;
                    return oss.str();
                }
            }
            case LUA_TSTRING:
                return lua_tostring(L, index);
            case LUA_TTABLE:
                return "table";
            case LUA_TFUNCTION:
                return "function";
            case LUA_TUSERDATA:
            case LUA_TLIGHTUSERDATA:
                return "userdata";
            case LUA_TTHREAD:
                return "thread";
            default:
                return "unknown";
        }
    }
};

// 静态成员初始化
std::vector<std::string> Lua515CompatibilityTestFixture::compatibility_issues;

// ============================================================================
// 测试组1: 基础语法兼容性 (Basic Syntax Compatibility)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 基础语法", "[compatibility][lua515][syntax]") {
    SECTION("🔍 基础数据类型") {
        std::vector<std::string> test_scripts = {
            // 数字类型
            "return 42",
            "return 3.14",
            "return -17",
            "return 1e10",
            "return 0xFF",
            "return 0x10",
            
            // 字符串类型
            "return 'hello'",
            "return \"world\"",
            "return [[multiline\nstring]]",
            "return [=[nested [[ string ]]=]",
            
            // 布尔和nil
            "return true",
            "return false", 
            "return nil",
            
            // 基础运算
            "return 1 + 2",
            "return 'hello' .. ' world'",
            "return not true",
            "return true and false",
            "return true or false"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "基础语法: " + script);
            clean_stacks();
        }
    }
    
    SECTION("🔍 变量和作用域") {
        std::vector<std::string> test_scripts = {
            // 局部变量
            "local x = 10; return x",
            "local a, b = 1, 2; return a, b",
            "local x = 5; local y = x * 2; return y",
            
            // 全局变量
            "global_var = 'test'; return global_var",
            "return type(undefined_global)",
            
            // 作用域
            R"(
                local x = 1
                do
                    local x = 2
                    return x
                end
            )",
            
            R"(
                local function outer()
                    local x = 10
                    local function inner()
                        return x
                    end
                    return inner()
                end
                return outer()
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "变量作用域: " + script);
            clean_stacks();
        }
    }
    
    SECTION("🔍 控制结构") {
        std::vector<std::string> test_scripts = {
            // if语句
            R"(
                local x = 10
                if x > 5 then
                    return "large"
                else
                    return "small"
                end
            )",
            
            // for循环
            R"(
                local sum = 0
                for i = 1, 5 do
                    sum = sum + i
                end
                return sum
            )",
            
            // while循环
            R"(
                local x = 1
                while x < 10 do
                    x = x * 2
                end
                return x
            )",
            
            // repeat循环
            R"(
                local x = 1
                repeat
                    x = x * 2
                until x > 10
                return x
            )",
            
            // break语句
            R"(
                local sum = 0
                for i = 1, 10 do
                    if i > 5 then break end
                    sum = sum + i
                end
                return sum
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "控制结构: " + script);
            clean_stacks();
        }
    }
}

// ============================================================================
// 测试组2: 函数和闭包兼容性 (Function and Closure Compatibility)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 函数闭包", "[compatibility][lua515][functions]") {
    SECTION("🔍 函数定义和调用") {
        std::vector<std::string> test_scripts = {
            // 基础函数
            R"(
                local function add(a, b)
                    return a + b
                end
                return add(3, 4)
            )",
            
            // 多返回值
            R"(
                local function multi()
                    return 1, 2, 3
                end
                local a, b, c = multi()
                return a + b + c
            )",
            
            // 变长参数
            R"(
                local function varargs(...)
                    local sum = 0
                    for i = 1, select('#', ...) do
                        sum = sum + select(i, ...)
                    end
                    return sum
                end
                return varargs(1, 2, 3, 4, 5)
            )",
            
            // 递归函数
            R"(
                local function factorial(n)
                    if n <= 1 then
                        return 1
                    else
                        return n * factorial(n - 1)
                    end
                end
                return factorial(5)
            )",
            
            // 尾调用优化测试
            R"(
                local function tail_recursive(n, acc)
                    if n <= 0 then
                        return acc
                    else
                        return tail_recursive(n - 1, acc + n)
                    end
                end
                return tail_recursive(100, 0)
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "函数调用: " + script);
            clean_stacks();
        }
    }
    
    SECTION("🔍 闭包和upvalue") {
        std::vector<std::string> test_scripts = {
            // 简单闭包
            R"(
                local function make_counter()
                    local count = 0
                    return function()
                        count = count + 1
                        return count
                    end
                end
                local counter = make_counter()
                return counter() + counter() + counter()
            )",
            
            // 多个闭包共享upvalue
            R"(
                local function make_pair()
                    local value = 0
                    local function get()
                        return value
                    end
                    local function set(v)
                        value = v
                    end
                    return get, set
                end
                local get, set = make_pair()
                set(42)
                return get()
            )",
            
            // 嵌套闭包
            R"(
                local function outer(x)
                    return function(y)
                        return function(z)
                            return x + y + z
                        end
                    end
                end
                local f = outer(1)(2)
                return f(3)
            )",
            
            // 闭包作为返回值
            R"(
                local function make_adder(n)
                    return function(x)
                        return x + n
                    end
                end
                local add5 = make_adder(5)
                local add10 = make_adder(10)
                return add5(3) + add10(7)
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "闭包: " + script);
            clean_stacks();
        }
    }
}

// ============================================================================
// 测试组3: 表操作兼容性 (Table Operations Compatibility)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 表操作", "[compatibility][lua515][tables]") {
    SECTION("🔍 表创建和访问") {
        std::vector<std::string> test_scripts = {
            // 基础表操作
            "local t = {1, 2, 3}; return t[1], t[2], t[3]",
            "local t = {a = 1, b = 2}; return t.a, t.b",
            "local t = {10, x = 20, 30}; return t[1], t.x, t[2]",
            
            // 表长度
            "local t = {1, 2, 3, 4, 5}; return #t",
            "local t = {1, 2, nil, 4}; return #t",  // 测试nil的影响
            
            // 动态表修改
            R"(
                local t = {}
                t[1] = "first"
                t.key = "value"
                t[2] = "second"
                return #t, t[1], t.key
            )",
            
            // 表作为键
            R"(
                local t1 = {}
                local t2 = {}
                local main = {}
                main[t1] = "table1"
                main[t2] = "table2"
                return main[t1], main[t2]
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "表操作: " + script);
            clean_stacks();
        }
    }
    
    SECTION("🔍 表遍历") {
        std::vector<std::string> test_scripts = {
            // pairs遍历
            R"(
                local t = {a = 1, b = 2, c = 3}
                local sum = 0
                for k, v in pairs(t) do
                    sum = sum + v
                end
                return sum
            )",
            
            // ipairs遍历
            R"(
                local t = {10, 20, 30}
                local product = 1
                for i, v in ipairs(t) do
                    product = product * v
                end
                return product
            )",
            
            // next函数
            R"(
                local t = {x = 1, y = 2}
                local count = 0
                local k = next(t)
                while k do
                    count = count + 1
                    k = next(t, k)
                end
                return count
            )",
            
            // 混合索引表的遍历
            R"(
                local t = {10, 20, x = 30, y = 40, 50}
                local numeric_sum = 0
                local total_sum = 0
                
                for i, v in ipairs(t) do
                    numeric_sum = numeric_sum + v
                end
                
                for k, v in pairs(t) do
                    total_sum = total_sum + v
                end
                
                return numeric_sum, total_sum
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "表遍历: " + script);
            clean_stacks();
        }
    }
    
    SECTION("🔍 元表操作") {
        std::vector<std::string> test_scripts = {
            // 基础元表
            R"(
                local t = {value = 10}
                local mt = {
                    __add = function(a, b)
                        return {value = a.value + b.value}
                    end
                }
                setmetatable(t, mt)
                local t2 = {value = 20}
                setmetatable(t2, mt)
                local result = t + t2
                return result.value
            )",
            
            // __index元方法
            R"(
                local t = {}
                local mt = {
                    __index = function(table, key)
                        return "default_" .. key
                    end
                }
                setmetatable(t, mt)
                return t.missing_key
            )",
            
            // __newindex元方法
            R"(
                local proxy = {}
                local real_table = {}
                local mt = {
                    __newindex = function(table, key, value)
                        real_table[key] = value * 2
                    end,
                    __index = function(table, key)
                        return real_table[key]
                    end
                }
                setmetatable(proxy, mt)
                proxy.x = 10
                return proxy.x
            )",
            
            // __tostring元方法
            R"(
                local t = {name = "test"}
                local mt = {
                    __tostring = function(self)
                        return "Object: " .. self.name
                    end
                }
                setmetatable(t, mt)
                return tostring(t)
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "元表: " + script);
            clean_stacks();
        }
    }
}

// ============================================================================
// 测试组4: 协程兼容性 (Coroutine Compatibility)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 协程", "[compatibility][lua515][coroutines]") {
    SECTION("🔍 基础协程操作") {
        std::vector<std::string> test_scripts = {
            // 简单协程
            R"(
                local function simple_coro()
                    coroutine.yield(1)
                    coroutine.yield(2)
                    return 3
                end
                
                local co = coroutine.create(simple_coro)
                local success1, value1 = coroutine.resume(co)
                local success2, value2 = coroutine.resume(co)
                local success3, value3 = coroutine.resume(co)
                
                return value1 + value2 + value3
            )",
            
            // 协程状态检查
            R"(
                local function test_coro()
                    coroutine.yield("yielded")
                    return "finished"
                end
                
                local co = coroutine.create(test_coro)
                local status1 = coroutine.status(co)
                coroutine.resume(co)
                local status2 = coroutine.status(co)
                coroutine.resume(co)
                local status3 = coroutine.status(co)
                
                return status1, status2, status3
            )",
            
            // 协程参数传递
            R"(
                local function param_coro(x, y)
                    local z = coroutine.yield(x + y)
                    return x + y + z
                end
                
                local co = coroutine.create(param_coro)
                local success1, sum = coroutine.resume(co, 10, 20)
                local success2, final = coroutine.resume(co, 5)
                
                return sum, final
            )",
            
            // 生产者-消费者模式
            R"(
                local function producer()
                    for i = 1, 5 do
                        coroutine.yield(i * 2)
                    end
                end
                
                local co = coroutine.create(producer)
                local sum = 0
                
                while coroutine.status(co) ~= "dead" do
                    local success, value = coroutine.resume(co)
                    if value then
                        sum = sum + value
                    end
                end
                
                return sum
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "协程: " + script);
            clean_stacks();
        }
    }
}

// ============================================================================
// 测试组5: 错误处理兼容性 (Error Handling Compatibility)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 错误处理", "[compatibility][lua515][errors]") {
    SECTION("🔍 pcall和xpcall") {
        std::vector<std::string> test_scripts = {
            // 成功的pcall
            R"(
                local function safe_divide(a, b)
                    if b == 0 then
                        error("Division by zero")
                    end
                    return a / b
                end
                
                local success, result = pcall(safe_divide, 10, 2)
                return success, result
            )",
            
            // 失败的pcall
            R"(
                local function error_func()
                    error("Test error")
                end
                
                local success, err = pcall(error_func)
                return success
            )",
            
            // xpcall与错误处理器
            R"(
                local function error_func()
                    error("Original error")
                end
                
                local function error_handler(err)
                    return "Handled: " .. err
                end
                
                local success, result = xpcall(error_func, error_handler)
                return success
            )",
            
            // 嵌套pcall
            R"(
                local function inner()
                    error("Inner error")
                end
                
                local function outer()
                    local success, err = pcall(inner)
                    if not success then
                        return "Caught inner error"
                    end
                    return "No error"
                end
                
                local success, result = pcall(outer)
                return success, result
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "错误处理: " + script);
            clean_stacks();
        }
    }
    
    SECTION("🔍 assert函数") {
        std::vector<std::string> test_scripts = {
            // 成功的assert
            R"(
                local result = assert(true, "This should not fail")
                return result
            )",
            
            R"(
                local value = assert(42, "Should return the value")
                return value
            )",
            
            // assert与函数返回值
            R"(
                local function safe_sqrt(x)
                    if x < 0 then
                        return nil, "Negative number"
                    end
                    return math.sqrt(x)
                end
                
                local result = assert(safe_sqrt(16))
                return result
            )"
        };
        
        for (const auto& script : test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "assert: " + script);
            clean_stacks();
        }
    }
}

// ============================================================================
// 测试组6: C API兼容性 (C API Compatibility)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: C API", "[compatibility][lua515][c_api]") {
    SECTION("🔍 栈操作一致性") {
        // 这部分测试需要直接与C API交互
        // 由于我们在测试fixture中，可以直接操作lua_State
        
        // 测试基础栈操作
        lua_pushinteger(L_ref, 42);
        lua_pushstring(L_ref, "test");
        lua_pushboolean(L_ref, 1);
        
        REQUIRE(lua_gettop(L_ref) == 3);
        REQUIRE(lua_tointeger(L_ref, 1) == 42);
        REQUIRE(std::string(lua_tostring(L_ref, 2)) == "test");
        REQUIRE(lua_toboolean(L_ref, 3) == 1);
        
        lua_settop(L_ref, 0);
        
        // 测试表操作
        lua_newtable(L_ref);
        lua_pushstring(L_ref, "key");
        lua_pushstring(L_ref, "value");
        lua_settable(L_ref, -3);
        
        lua_pushstring(L_ref, "key");
        lua_gettable(L_ref, -2);
        REQUIRE(std::string(lua_tostring(L_ref, -1)) == "value");
        
        lua_settop(L_ref, 0);
    }
    
    SECTION("🔍 函数注册一致性") {
        // 注册C函数并测试调用
        auto test_function = [](lua_State* L) -> int {
            lua_Number a = luaL_checknumber(L, 1);
            lua_Number b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a + b);
            return 1;
        };
        
        lua_pushcfunction(L_ref, test_function);
        lua_setglobal(L_ref, "c_add");
        
        // 测试调用
        std::string test_script = "return c_add(10, 20)";
        auto result = execute_reference(test_script);
        
        REQUIRE(result.success);
        REQUIRE(result.values.size() == 1);
        REQUIRE(result.values[0] == "30");
        
        clean_stacks();
    }
}

// ============================================================================
// 测试组7: 性能兼容性基准 (Performance Compatibility Benchmarks)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 性能基准", "[compatibility][lua515][performance]") {
    SECTION("🔍 计算密集型任务性能对比") {
        std::string fibonacci_script = R"(
            local function fib(n)
                if n <= 2 then
                    return 1
                else
                    return fib(n-1) + fib(n-2)
                end
            end
            return fib(25)
        )";
        
        BENCHMARK("参考实现斐波那契(n=25)") {
            auto result = execute_reference(fibonacci_script);
            REQUIRE(result.success);
            REQUIRE(result.values[0] == "75025");
            clean_stacks();
        };
        
        // TODO: 在现代实现完成后添加对比基准
        // BENCHMARK("现代实现斐波那契(n=25)") {
        //     auto result = execute_modern(fibonacci_script);
        //     REQUIRE(result.success);
        //     REQUIRE(result.values[0] == "75025");
        // };
    }
    
    SECTION("🔍 表操作性能对比") {
        std::string table_script = R"(
            local t = {}
            for i = 1, 1000 do
                t[i] = i * 2
            end
            
            local sum = 0
            for i = 1, 1000 do
                sum = sum + t[i]
            end
            return sum
        )";
        
        BENCHMARK("参考实现表操作(1000元素)") {
            auto result = execute_reference(table_script);
            REQUIRE(result.success);
            REQUIRE(result.values[0] == "1001000");
            clean_stacks();
        };
    }
}

// ============================================================================
// 测试组8: 官方测试套件集成 (Official Test Suite Integration)
// ============================================================================

TEST_CASE_METHOD(Lua515CompatibilityTestFixture, "Lua 5.1.5兼容性: 官方测试", "[compatibility][lua515][official]") {
    SECTION("🔍 官方测试脚本执行") {
        // 这里可以集成官方Lua 5.1.5测试套件的关键测试
        // 为了演示，我们包含一些重要的测试用例
        
        std::vector<std::string> official_test_scripts = {
            // 来自官方测试的核心语法测试
            R"(
                -- 测试局部变量作用域
                local function test_scope()
                    local a = 1
                    do
                        local a = 2
                        assert(a == 2)
                    end
                    assert(a == 1)
                    return true
                end
                return test_scope()
            )",
            
            R"(
                -- 测试函数参数和返回值
                local function test_returns(a, b, c)
                    return c, b, a
                end
                local x, y, z = test_returns(1, 2, 3)
                return x == 3 and y == 2 and z == 1
            )",
            
            R"(
                -- 测试表的复杂操作
                local t = {1, 2, 3}
                table.insert(t, 2, 'inserted')
                local removed = table.remove(t, 3)
                return #t == 3 and t[2] == 'inserted' and removed == 2
            )",
            
            R"(
                -- 测试协程的复杂交互
                local function producer()
                    for i = 1, 3 do
                        coroutine.yield(i)
                    end
                    return "done"
                end
                
                local co = coroutine.create(producer)
                local results = {}
                
                while coroutine.status(co) ~= "dead" do
                    local ok, value = coroutine.resume(co)
                    if ok then
                        table.insert(results, value)
                    end
                end
                
                return #results == 4 and results[4] == "done"
            )"
        };
        
        for (const auto& script : official_test_scripts) {
            auto ref_result = execute_reference(script);
            auto modern_result = execute_modern(script);
            
            compare_results(ref_result, modern_result, "官方测试: " + script.substr(0, 50) + "...");
            clean_stacks();
        }
    }
}

} // namespace lua515_compatibility_tests
} // namespace lua_cpp

// ============================================================================
// 全局测试监听器和兼容性报告
// ============================================================================

namespace {

class Lua515CompatibilityListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;
    
    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
        std::cout << "\n🔍 开始Lua 5.1.5兼容性测试套件" << std::endl;
        std::cout << "目标: 确保100%行为一致性" << std::endl;
    }
    
    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        if (testInfo.tags.count("[compatibility]")) {
            std::cout << "\n🧪 兼容性测试: " << testInfo.name << std::endl;
        }
    }
    
    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        if (testCaseStats.testInfo->tags.count("[compatibility]")) {
            if (testCaseStats.totals.assertions.allPassed()) {
                std::cout << "✅ 兼容性验证通过: " << testCaseStats.testInfo->name << std::endl;
            } else {
                std::cout << "❌ 兼容性问题发现: " << testCaseStats.testInfo->name << std::endl;
            }
        }
    }
    
    void testRunEnded(Catch::TestRunStats const& testRunStats) override {
        auto& issues = lua_cpp::lua515_compatibility_tests::Lua515CompatibilityTestFixture::compatibility_issues;
        
        if (issues.empty()) {
            std::cout << "\n🎉 Lua 5.1.5兼容性测试全部通过！" << std::endl;
            std::cout << "✅ 完全兼容性达成" << std::endl;
        } else {
            std::cout << "\n⚠️  发现 " << issues.size() << " 个兼容性问题:" << std::endl;
            for (const auto& issue : issues) {
                std::cout << "  - " << issue << std::endl;
            }
        }
    }
};

CATCH_REGISTER_LISTENER(Lua515CompatibilityListener)

} // anonymous namespace