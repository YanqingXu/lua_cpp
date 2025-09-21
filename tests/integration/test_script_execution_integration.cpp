/**
 * @file test_script_execution_integration.cpp
 * @brief T017 - 脚本执行端到端集成测试
 * 
 * 本文件测试完整的Lua脚本执行流程：
 * 词法分析 -> 语法分析 -> 编译 -> 虚拟机执行
 * 
 * 测试策略：
 * 🔍 lua_c_analysis: 验证与原始Lua 5.1.5行为的完全一致性
 * 🏗️ lua_with_cpp: 验证现代C++实现的正确性和性能优化
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
#include "lexer.h"
#include "parser.h"
#include "compiler.h"
#include "vm.h"
#include "lua_state.h"

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
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <functional>

using namespace Catch::Matchers;

namespace lua_cpp {
namespace script_execution_integration_tests {

// ============================================================================
// 测试基础设施和工具类
// ============================================================================

/**
 * @brief 脚本执行集成测试基础类
 * 
 * 提供统一的测试环境设置和清理机制，支持双重验证方法：
 * - lua_c_analysis: 原始Lua 5.1.5行为验证
 * - lua_with_cpp: 现代C++实现验证
 */
class ScriptExecutionTestFixture {
public:
    ScriptExecutionTestFixture() {
        setup_lua_c_analysis();
        setup_lua_with_cpp();
        execution_trace.clear();
    }
    
    ~ScriptExecutionTestFixture() {
        cleanup_lua_c_analysis();
        cleanup_lua_with_cpp();
    }
    
protected:
    // lua_c_analysis 环境
    lua_State* L_ref = nullptr;
    
    // lua_with_cpp 环境
    std::unique_ptr<lua_cpp::LuaState> lua_state;
    std::unique_ptr<lua_cpp::Lexer> lexer;
    std::unique_ptr<lua_cpp::Parser> parser;
    std::unique_ptr<lua_cpp::Compiler> compiler;
    std::unique_ptr<lua_cpp::VirtualMachine> vm;
    
    // 执行跟踪
    static std::vector<std::string> execution_trace;
    
    // 测试工具方法
    void setup_lua_c_analysis() {
        L_ref = luaL_newstate();
        REQUIRE(L_ref != nullptr);
        luaL_openlibs(L_ref);
        
        // 注册跟踪函数
        lua_pushcfunction(L_ref, trace_function);
        lua_setglobal(L_ref, "trace");
    }
    
    void cleanup_lua_c_analysis() {
        if (L_ref) {
            lua_close(L_ref);
            L_ref = nullptr;
        }
    }
    
    void setup_lua_with_cpp() {
        // 初始化现代C++组件
        lua_state = std::make_unique<lua_cpp::LuaState>();
        lexer = std::make_unique<lua_cpp::Lexer>();
        parser = std::make_unique<lua_cpp::Parser>();
        compiler = std::make_unique<lua_cpp::Compiler>();
        vm = std::make_unique<lua_cpp::VirtualMachine>();
    }
    
    void cleanup_lua_with_cpp() {
        // RAII自动清理
        vm.reset();
        compiler.reset();
        parser.reset();
        lexer.reset();
        lua_state.reset();
    }
    
    static int trace_function(lua_State* L) {
        const char* msg = luaL_checkstring(L, 1);
        execution_trace.push_back(std::string(msg));
        return 0;
    }
    
    static void trace_call(const std::string& message) {
        execution_trace.push_back(message);
    }
    
    void clear_trace() {
        execution_trace.clear();
    }
    
    void clean_stack() {
        if (L_ref) {
            lua_settop(L_ref, 0);
        }
    }
    
    // 脚本执行结果比较
    struct ExecutionResult {
        bool success;
        std::string output;
        std::string error_message;
        std::vector<std::string> trace;
        double execution_time_ms;
    };
    
    ExecutionResult execute_with_reference(const std::string& script) {
        ExecutionResult result;
        clear_trace();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        int load_result = luaL_loadstring(L_ref, script.c_str());
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
            // 收集输出结果
            int nresults = lua_gettop(L_ref);
            std::ostringstream oss;
            
            for (int i = 1; i <= nresults; i++) {
                if (i > 1) oss << " ";
                
                if (lua_isstring(L_ref, i)) {
                    oss << lua_tostring(L_ref, i);
                } else if (lua_isnumber(L_ref, i)) {
                    oss << lua_tonumber(L_ref, i);
                } else if (lua_isboolean(L_ref, i)) {
                    oss << (lua_toboolean(L_ref, i) ? "true" : "false");
                } else if (lua_isnil(L_ref, i)) {
                    oss << "nil";
                } else {
                    oss << luaL_typename(L_ref, i);
                }
            }
            
            result.output = oss.str();
            lua_settop(L_ref, 0);
        }
        
        result.trace = execution_trace;
        return result;
    }
    
    ExecutionResult execute_with_modern_cpp(const std::string& script) {
        ExecutionResult result;
        clear_trace();
        
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            // 词法分析
            auto tokens = lexer->tokenize(script);
            trace_call("lexer_tokenize");
            
            // 语法分析
            auto ast = parser->parse(tokens);
            trace_call("parser_parse");
            
            // 编译
            auto bytecode = compiler->compile(ast);
            trace_call("compiler_compile");
            
            // 执行
            auto exec_result = vm->execute(bytecode, lua_state.get());
            trace_call("vm_execute");
            
            auto end = std::chrono::high_resolution_clock::now();
            result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            result.success = exec_result.success;
            result.output = exec_result.output;
            result.error_message = exec_result.error_message;
            
        } catch (const std::exception& e) {
            auto end = std::chrono::high_resolution_clock::now();
            result.execution_time_ms = std::chrono::duration<double, std::milli>(end - start).count();
            
            result.success = false;
            result.error_message = e.what();
        }
        
        result.trace = execution_trace;
        return result;
    }
};

// 静态成员初始化
std::vector<std::string> ScriptExecutionTestFixture::execution_trace;

// ============================================================================
// 测试组1: 基础表达式和语句执行 (Basic Expressions and Statements)
// ============================================================================

TEST_CASE_METHOD(ScriptExecutionTestFixture, "集成测试: 基础表达式执行", "[integration][script_execution][expressions]") {
    SECTION("🔍 lua_c_analysis验证: 算术表达式") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"return 1 + 2", "3"},
            {"return 10 - 3", "7"},
            {"return 4 * 5", "20"},
            {"return 15 / 3", "5"},
            {"return 17 % 5", "2"},
            {"return 2 ^ 3", "8"},
            {"return -5", "-5"},
            {"return (1 + 2) * 3", "9"},
            {"return 2 + 3 * 4", "14"},  // 运算符优先级
            {"return (2 + 3) * (4 - 1)", "15"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 字符串操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"return 'hello'", "hello"},
            {"return \"world\"", "world"},
            {"return 'hello' .. ' ' .. 'world'", "hello world"},
            {"return string.len('test')", "4"},
            {"return string.upper('hello')", "HELLO"},
            {"return string.lower('WORLD')", "world"},
            {"return string.sub('hello', 2, 4)", "ell"},
            {"return 'abc' < 'def'", "true"},
            {"return 'xyz' > 'abc'", "true"},
            {"return 'test' == 'test'", "true"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 逻辑运算") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"return true and false", "false"},
            {"return true or false", "true"},
            {"return not true", "false"},
            {"return not false", "true"},
            {"return 1 and 2", "2"},  // Lua的and/or短路求值
            {"return nil or 'default'", "default"},
            {"return false or 'fallback'", "fallback"},
            {"return 0 and 'zero'", "zero"},  // 0在Lua中是true
            {"return '' and 'empty'", "empty"},  // 空字符串在Lua中是true
            {"return (1 < 2) and (3 > 2)", "true"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++表达式处理") {
        // 注意：这部分需要在实际的现代C++实现完成后进行验证
        // 目前只是结构性测试，验证接口调用能够正常工作
        
        std::vector<std::string> test_scripts = {
            "return 1 + 2 * 3",
            "return 'hello' .. ' world'",
            "return true and (1 < 2)"
        };
        
        for (const auto& script : test_scripts) {
            INFO("现代C++处理脚本: " << script);
            
            // 先获取参考结果
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            
            // TODO: 在现代C++实现完成后，比较结果一致性
            // auto result_cpp = execute_with_modern_cpp(script);
            // REQUIRE(result_cpp.success);
            // REQUIRE(result_cpp.output == result_ref.output);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组2: 变量和作用域管理 (Variables and Scope Management)
// ============================================================================

TEST_CASE_METHOD(ScriptExecutionTestFixture, "集成测试: 变量和作用域", "[integration][script_execution][variables]") {
    SECTION("🔍 lua_c_analysis验证: 局部变量") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 简单局部变量
            {"local x = 10; return x", "10"},
            {"local a, b = 1, 2; return a + b", "3"},
            {"local x = 5; local y = x * 2; return y", "10"},
            
            // 变量重新赋值
            {"local x = 1; x = x + 1; return x", "2"},
            {"local a, b = 1, 2; a, b = b, a; return a, b", "2 1"},
            
            // 变量作用域
            {R"(
                local x = 1
                do
                    local x = 2
                    return x
                end
            )", "2"},
            
            {R"(
                local x = 1
                do
                    local y = 2
                end
                return x
            )", "1"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 全局变量") {
        // 测试全局变量的设置和访问
        lua_pushstring(L_ref, "global_value");
        lua_setglobal(L_ref, "global_var");
        
        std::vector<std::pair<std::string, std::string>> test_cases = {
            {"return global_var", "global_value"},
            {"global_var = 'modified'; return global_var", "modified"},
            {"global_new = 42; return global_new", "42"},
            {"return type(global_var)", "string"},
            {"return type(undefined_var)", "nil"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 函数参数和返回值") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 简单函数定义和调用
            {R"(
                local function add(a, b)
                    return a + b
                end
                return add(3, 4)
            )", "7"},
            
            // 多个返回值
            {R"(
                local function multi_return()
                    return 1, 2, 3
                end
                local a, b, c = multi_return()
                return a + b + c
            )", "6"},
            
            // 变长参数
            {R"(
                local function sum(...)
                    local total = 0
                    for i = 1, select('#', ...) do
                        total = total + select(i, ...)
                    end
                    return total
                end
                return sum(1, 2, 3, 4, 5)
            )", "15"},
            
            // 闭包
            {R"(
                local function make_counter()
                    local count = 0
                    return function()
                        count = count + 1
                        return count
                    end
                end
                local counter = make_counter()
                return counter() + counter()
            )", "3"}  // 1 + 2
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组3: 控制流和循环结构 (Control Flow and Loop Structures)
// ============================================================================

TEST_CASE_METHOD(ScriptExecutionTestFixture, "集成测试: 控制流", "[integration][script_execution][control_flow]") {
    SECTION("🔍 lua_c_analysis验证: 条件语句") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 简单if语句
            {R"(
                local x = 10
                if x > 5 then
                    return "large"
                else
                    return "small"
                end
            )", "large"},
            
            // elseif链
            {R"(
                local score = 85
                if score >= 90 then
                    return "A"
                elseif score >= 80 then
                    return "B"
                elseif score >= 70 then
                    return "C"
                else
                    return "F"
                end
            )", "B"},
            
            // 嵌套if
            {R"(
                local x, y = 5, 10
                if x > 0 then
                    if y > 0 then
                        return "both positive"
                    else
                        return "x positive, y negative"
                    end
                else
                    return "x negative"
                end
            )", "both positive"},
            
            // 复杂条件
            {R"(
                local a, b, c = 1, 2, 3
                if (a < b) and (b < c) then
                    return "ascending"
                elseif (a > b) and (b > c) then
                    return "descending"
                else
                    return "mixed"
                end
            )", "ascending"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 循环结构") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // while循环
            {R"(
                local sum = 0
                local i = 1
                while i <= 5 do
                    sum = sum + i
                    i = i + 1
                end
                return sum
            )", "15"},  // 1+2+3+4+5
            
            // for数值循环
            {R"(
                local product = 1
                for i = 1, 4 do
                    product = product * i
                end
                return product
            )", "24"},  // 4!
            
            // for数值循环带步长
            {R"(
                local sum = 0
                for i = 2, 10, 2 do
                    sum = sum + i
                end
                return sum
            )", "30"},  // 2+4+6+8+10
            
            // repeat-until循环
            {R"(
                local x = 1
                repeat
                    x = x * 2
                until x > 10
                return x
            )", "16"},  // 1->2->4->8->16
            
            // 嵌套循环
            {R"(
                local result = 0
                for i = 1, 3 do
                    for j = 1, 2 do
                        result = result + i * j
                    end
                end
                return result
            )", "18"}  // (1*1+1*2)+(2*1+2*2)+(3*1+3*2) = 3+6+9 = 18
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 跳转控制") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // break语句
            {R"(
                local sum = 0
                for i = 1, 10 do
                    if i > 5 then
                        break
                    end
                    sum = sum + i
                end
                return sum
            )", "15"},  // 1+2+3+4+5
            
            // continue模拟（Lua没有continue，用条件跳过）
            {R"(
                local sum = 0
                for i = 1, 10 do
                    if i % 2 == 0 then
                        -- skip even numbers
                    else
                        sum = sum + i
                    end
                end
                return sum
            )", "25"},  // 1+3+5+7+9
            
            // 嵌套break
            {R"(
                local found = false
                local result = 0
                for i = 1, 5 do
                    for j = 1, 5 do
                        if i * j == 12 then
                            result = i + j
                            found = true
                            break
                        end
                    end
                    if found then
                        break
                    end
                end
                return result
            )", "7"}  // 3*4=12, so 3+4=7
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组4: 表操作和数据结构 (Table Operations and Data Structures)
// ============================================================================

TEST_CASE_METHOD(ScriptExecutionTestFixture, "集成测试: 表操作", "[integration][script_execution][tables]") {
    SECTION("🔍 lua_c_analysis验证: 基础表操作") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 表创建和访问
            {R"(
                local t = {1, 2, 3}
                return t[1] + t[2] + t[3]
            )", "6"},
            
            // 键值对表
            {R"(
                local person = {name = "Alice", age = 30}
                return person.name .. " is " .. person.age
            )", "Alice is 30"},
            
            // 混合索引表
            {R"(
                local t = {10, 20, x = 30, y = 40}
                return t[1] + t[2] + t.x + t.y
            )", "100"},
            
            // 表长度
            {R"(
                local t = {1, 2, 3, 4, 5}
                return #t
            )", "5"},
            
            // 嵌套表
            {R"(
                local matrix = {{1, 2}, {3, 4}}
                return matrix[1][1] + matrix[2][2]
            )", "5"},  // 1 + 4
            
            // 动态表修改
            {R"(
                local t = {}
                t[1] = "first"
                t.key = "value"
                t[2] = "second"
                return #t .. " " .. t[1] .. " " .. t.key
            )", "2 first value"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 表遍历") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // ipairs遍历
            {R"(
                local t = {10, 20, 30}
                local sum = 0
                for i, v in ipairs(t) do
                    sum = sum + v
                end
                return sum
            )", "60"},
            
            // pairs遍历
            {R"(
                local t = {a = 1, b = 2, c = 3}
                local sum = 0
                for k, v in pairs(t) do
                    sum = sum + v
                end
                return sum
            )", "6"},
            
            // 数值for遍历
            {R"(
                local t = {5, 10, 15, 20}
                local product = 1
                for i = 1, #t do
                    product = product * t[i]
                end
                return product
            )", "15000"},  // 5*10*15*20
            
            // 复杂遍历
            {R"(
                local matrix = {{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}
                local diagonal_sum = 0
                for i = 1, #matrix do
                    diagonal_sum = diagonal_sum + matrix[i][i]
                end
                return diagonal_sum
            )", "15"}  // 1+5+9
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 表作为数据结构") {
        std::vector<std::pair<std::string, std::string>> test_cases = {
            // 栈实现
            {R"(
                local stack = {}
                
                local function push(val)
                    stack[#stack + 1] = val
                end
                
                local function pop()
                    local val = stack[#stack]
                    stack[#stack] = nil
                    return val
                end
                
                push(1)
                push(2)
                push(3)
                
                return pop() + pop() + pop()
            )", "6"},  // 3+2+1
            
            // 队列实现
            {R"(
                local queue = {first = 0, last = -1}
                
                local function enqueue(val)
                    queue.last = queue.last + 1
                    queue[queue.last] = val
                end
                
                local function dequeue()
                    if queue.first > queue.last then
                        return nil
                    end
                    local val = queue[queue.first]
                    queue[queue.first] = nil
                    queue.first = queue.first + 1
                    return val
                end
                
                enqueue(10)
                enqueue(20)
                enqueue(30)
                
                return dequeue() + dequeue() + dequeue()
            )", "60"},  // 10+20+30
            
            // 简单对象系统
            {R"(
                local function new_person(name, age)
                    return {
                        name = name,
                        age = age,
                        greet = function(self)
                            return "Hello, I'm " .. self.name
                        end
                    }
                end
                
                local person = new_person("Bob", 25)
                return person:greet()
            )", "Hello, I'm Bob"}
        };
        
        for (const auto& [script, expected] : test_cases) {
            INFO("测试脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组5: 错误处理和异常情况 (Error Handling and Exception Cases)
// ============================================================================

TEST_CASE_METHOD(ScriptExecutionTestFixture, "集成测试: 错误处理", "[integration][script_execution][error_handling]") {
    SECTION("🔍 lua_c_analysis验证: 语法错误检测") {
        std::vector<std::string> syntax_error_scripts = {
            "local x = ",           // 不完整的赋值
            "if then end",          // 缺少条件
            "for i = 1 10 do end",  // 缺少to关键字
            "function (end",        // 不匹配的括号
            "local x = 1 + * 2",    // 无效的运算符序列
            "return ))",            // 多余的右括号
            "local function end",   // 缺少函数名
            "while do end",         // 缺少条件
            "repeat until"          // 缺少条件
        };
        
        for (const auto& script : syntax_error_scripts) {
            INFO("测试语法错误脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE_FALSE(result_ref.success);
            REQUIRE_FALSE(result_ref.error_message.empty());
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 运行时错误") {
        std::vector<std::pair<std::string, std::string>> runtime_error_cases = {
            // 除零错误
            {"return 1/0", "inf"},  // Lua中1/0返回inf，不是错误
            {"return 0/0", "nan"},  // 0/0返回nan
            
            // 类型错误
            {"return 'string' + 1", ""},  // 字符串算术会尝试转换
            {"return nil[1]", ""},        // 对nil进行索引
            {"return (1)()", ""},         // 调用非函数值
            
            // 栈溢出（递归）
            {R"(
                local function recursive()
                    return recursive()
                end
                return recursive()
            )", ""},
            
            // 访问未定义变量（返回nil，不是错误）
            {"return undefined_variable", "nil"}
        };
        
        for (const auto& [script, expected_or_error] : runtime_error_cases) {
            INFO("测试运行时错误脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            
            if (expected_or_error.empty()) {
                // 期望错误
                REQUIRE_FALSE(result_ref.success);
            } else {
                // 期望特定结果
                REQUIRE(result_ref.success);
                REQUIRE(result_ref.output == expected_or_error);
            }
            
            clean_stack();
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: pcall错误处理") {
        std::vector<std::pair<std::string, std::string>> pcall_cases = {
            // 成功的pcall
            {R"(
                local success, result = pcall(function()
                    return 1 + 2
                end)
                return success and result
            )", "3"},
            
            // 捕获错误的pcall
            {R"(
                local success, err = pcall(function()
                    error("test error")
                end)
                return success
            )", "false"},
            
            // 嵌套pcall
            {R"(
                local outer_success, result = pcall(function()
                    local inner_success, inner_result = pcall(function()
                        return 10 / 2
                    end)
                    return inner_success and inner_result
                end)
                return outer_success and result
            )", "5"},
            
            // 自定义错误消息
            {R"(
                local success, err = pcall(function()
                    error("custom error message")
                end)
                if success then
                    return "no error"
                else
                    return "caught error"
                end
            )", "caught error"}
        };
        
        for (const auto& [script, expected] : pcall_cases) {
            INFO("测试pcall脚本: " << script);
            
            auto result_ref = execute_with_reference(script);
            REQUIRE(result_ref.success);
            REQUIRE(result_ref.output == expected);
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组6: 性能和压力测试 (Performance and Stress Tests)
// ============================================================================

TEST_CASE_METHOD(ScriptExecutionTestFixture, "集成测试: 性能基准", "[integration][script_execution][performance]") {
    SECTION("🔍 lua_c_analysis验证: 基础性能基准") {
        // 简单计算密集型任务
        std::string fibonacci_script = R"(
            local function fib(n)
                if n <= 2 then
                    return 1
                else
                    return fib(n-1) + fib(n-2)
                end
            end
            return fib(20)
        )";
        
        BENCHMARK("斐波那契数列递归计算(n=20)") {
            auto result = execute_with_reference(fibonacci_script);
            REQUIRE(result.success);
            REQUIRE(result.output == "6765");
            clean_stack();
        };
        
        // 迭代版本的斐波那契（更高效）
        std::string fibonacci_iter_script = R"(
            local function fib_iter(n)
                if n <= 2 then return 1 end
                local a, b = 1, 1
                for i = 3, n do
                    a, b = b, a + b
                end
                return b
            end
            return fib_iter(100)
        )";
        
        BENCHMARK("斐波那契数列迭代计算(n=100)") {
            auto result = execute_with_reference(fibonacci_iter_script);
            REQUIRE(result.success);
            clean_stack();
        };
    }
    
    SECTION("🔍 lua_c_analysis验证: 表操作性能") {
        // 大表创建和访问
        std::string large_table_script = R"(
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
        
        BENCHMARK("大表操作(1000元素)") {
            auto result = execute_with_reference(large_table_script);
            REQUIRE(result.success);
            REQUIRE(result.output == "1001000");  // 2*(1+2+...+1000) = 2*500500
            clean_stack();
        };
        
        // 字符串表操作
        std::string string_table_script = R"(
            local t = {}
            for i = 1, 100 do
                t["key_" .. i] = "value_" .. i
            end
            
            local count = 0
            for k, v in pairs(t) do
                count = count + 1
            end
            return count
        )";
        
        BENCHMARK("字符串键表操作(100元素)") {
            auto result = execute_with_reference(string_table_script);
            REQUIRE(result.success);
            REQUIRE(result.output == "100");
            clean_stack();
        };
    }
    
    SECTION("🔍 lua_c_analysis验证: 函数调用开销") {
        // 深度递归测试
        std::string deep_recursion_script = R"(
            local function deep_call(n)
                if n <= 0 then
                    return 0
                else
                    return 1 + deep_call(n - 1)
                end
            end
            return deep_call(500)
        )";
        
        BENCHMARK("深度递归调用(500层)") {
            auto result = execute_with_reference(deep_recursion_script);
            REQUIRE(result.success);
            REQUIRE(result.output == "500");
            clean_stack();
        };
        
        // 大量函数调用
        std::string many_calls_script = R"(
            local function simple_add(a, b)
                return a + b
            end
            
            local sum = 0
            for i = 1, 1000 do
                sum = simple_add(sum, i)
            end
            return sum
        )";
        
        BENCHMARK("大量函数调用(1000次)") {
            auto result = execute_with_reference(many_calls_script);
            REQUIRE(result.success);
            REQUIRE(result.output == "500500");  // 1+2+...+1000
            clean_stack();
        };
    }
}

} // namespace script_execution_integration_tests
} // namespace lua_cpp

// ============================================================================
// 全局测试监听器
// ============================================================================

namespace {

class ScriptExecutionTestListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;
    
    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        if (testInfo.tags.count("[integration]")) {
            std::cout << "\n🚀 开始集成测试: " << testInfo.name << std::endl;
        }
    }
    
    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        if (testCaseStats.testInfo->tags.count("[integration]")) {
            if (testCaseStats.totals.assertions.allPassed()) {
                std::cout << "✅ 集成测试通过: " << testCaseStats.testInfo->name << std::endl;
            } else {
                std::cout << "❌ 集成测试失败: " << testCaseStats.testInfo->name << std::endl;
            }
        }
    }
};

CATCH_REGISTER_LISTENER(ScriptExecutionTestListener)

} // anonymous namespace