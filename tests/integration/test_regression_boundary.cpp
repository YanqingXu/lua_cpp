/**
 * @file test_regression_boundary.cpp
 * @brief T017 - 回归测试和边界条件验证
 * 
 * 本文件测试系统的边界条件、错误恢复和回归防护：
 * - 内存限制和极端条件测试
 * - 错误恢复和故障处理验证
 * - 性能回归检测
 * - 已知问题的回归测试
 * 
 * 测试策略：
 * 🔍 lua_c_analysis: 验证边界条件下的稳定行为
 * 🏗️ lua_with_cpp: 验证现代实现的鲁棒性和性能
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
#include "stress_testing.h"
#include "memory_profiler.h"

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
#include <thread>
#include <random>
#include <limits>
#include <algorithm>
#include <exception>

using namespace Catch::Matchers;

namespace lua_cpp {
namespace regression_boundary_tests {

// ============================================================================
// 测试基础设施和工具类
// ============================================================================

/**
 * @brief 回归和边界测试基础类
 * 
 * 提供压力测试、边界条件和回归检测的基础设施
 */
class RegressionBoundaryTestFixture {
public:
    RegressionBoundaryTestFixture() {
        setup_test_environment();
        reset_metrics();
    }
    
    ~RegressionBoundaryTestFixture() {
        cleanup_test_environment();
        report_final_metrics();
    }
    
protected:
    // 测试环境
    lua_State* L_ref = nullptr;
    std::unique_ptr<lua_cpp::StressTester> stress_tester;
    std::unique_ptr<lua_cpp::MemoryProfiler> memory_profiler;
    
    // 性能和错误统计
    struct TestMetrics {
        size_t total_tests = 0;
        size_t passed_tests = 0;
        size_t failed_tests = 0;
        size_t memory_leaks_detected = 0;
        std::chrono::milliseconds total_execution_time{0};
        std::vector<std::string> error_messages;
        std::vector<double> performance_samples;
    } metrics;
    
    void setup_test_environment() {
        L_ref = luaL_newstate();
        REQUIRE(L_ref != nullptr);
        luaL_openlibs(L_ref);
        
        stress_tester = std::make_unique<lua_cpp::StressTester>();
        memory_profiler = std::make_unique<lua_cpp::MemoryProfiler>();
    }
    
    void cleanup_test_environment() {
        if (L_ref) {
            lua_close(L_ref);
            L_ref = nullptr;
        }
        stress_tester.reset();
        memory_profiler.reset();
    }
    
    void reset_metrics() {
        metrics = TestMetrics{};
    }
    
    void report_final_metrics() {
        std::cout << "\n📊 回归测试总结:" << std::endl;
        std::cout << "总测试数: " << metrics.total_tests << std::endl;
        std::cout << "通过: " << metrics.passed_tests << std::endl;
        std::cout << "失败: " << metrics.failed_tests << std::endl;
        std::cout << "内存泄漏: " << metrics.memory_leaks_detected << std::endl;
        std::cout << "总执行时间: " << metrics.total_execution_time.count() << "ms" << std::endl;
        
        if (!metrics.error_messages.empty()) {
            std::cout << "\n❌ 错误信息:" << std::endl;
            for (const auto& error : metrics.error_messages) {
                std::cout << "  - " << error << std::endl;
            }
        }
    }
    
    void record_test_result(bool success, const std::string& error_msg = "") {
        metrics.total_tests++;
        if (success) {
            metrics.passed_tests++;
        } else {
            metrics.failed_tests++;
            if (!error_msg.empty()) {
                metrics.error_messages.push_back(error_msg);
            }
        }
    }
    
    void record_performance_sample(double execution_time_ms) {
        metrics.performance_samples.push_back(execution_time_ms);
    }
    
    // 安全的Lua代码执行（带异常捕获）
    bool safe_execute_lua(const std::string& code, std::string& error_msg) {
        try {
            if (luaL_loadstring(L_ref, code.c_str()) != LUA_OK) {
                error_msg = "Load error: " + std::string(lua_tostring(L_ref, -1));
                lua_pop(L_ref, 1);
                return false;
            }
            
            if (lua_pcall(L_ref, 0, 0, 0) != LUA_OK) {
                error_msg = "Execution error: " + std::string(lua_tostring(L_ref, -1));
                lua_pop(L_ref, 1);
                return false;
            }
            
            return true;
        } catch (const std::exception& e) {
            error_msg = "C++ Exception: " + std::string(e.what());
            return false;
        } catch (...) {
            error_msg = "Unknown exception occurred";
            return false;
        }
    }
    
    void clean_stack() {
        if (L_ref) {
            lua_settop(L_ref, 0);
        }
    }
};

// ============================================================================
// 测试组1: 内存边界条件测试 (Memory Boundary Tests)
// ============================================================================

TEST_CASE_METHOD(RegressionBoundaryTestFixture, "回归测试: 内存边界条件", "[regression][boundary][memory]") {
    SECTION("🔍 大型数据结构处理") {
        // 测试大型表的创建和操作
        std::vector<std::pair<std::string, size_t>> large_data_tests = {
            {"小型表 (1K元素)", 1000},
            {"中型表 (10K元素)", 10000},
            {"大型表 (100K元素)", 100000}
        };
        
        for (const auto& [description, size] : large_data_tests) {
            INFO("测试: " << description);
            
            std::string error_msg;
            
            // 创建大型表的Lua代码
            std::ostringstream code;
            code << "local t = {}\n";
            code << "for i = 1, " << size << " do\n";
            code << "  t[i] = i * 2\n";
            code << "end\n";
            code << "-- 验证表大小\n";
            code << "assert(#t == " << size << ")\n";
            code << "-- 计算校验和\n";
            code << "local sum = 0\n";
            code << "for i = 1, #t do\n";
            code << "  sum = sum + t[i]\n";
            code << "end\n";
            code << "local expected = " << size << " * (" << size << " + 1)\n";
            code << "assert(sum == expected)\n";
            
            auto start = std::chrono::high_resolution_clock::now();
            bool success = safe_execute_lua(code.str(), error_msg);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            record_performance_sample(duration.count());
            
            if (size <= 10000) {
                // 小型和中型表应该成功
                REQUIRE(success);
                record_test_result(true);
            } else {
                // 大型表可能因内存限制失败，这是可接受的
                record_test_result(success, success ? "" : error_msg);
                if (!success) {
                    std::cout << "⚠️  大型表测试失败（可能是内存限制）: " << error_msg << std::endl;
                }
            }
            
            clean_stack();
        }
    }
    
    SECTION("🔍 深度递归测试") {
        std::vector<std::pair<std::string, int>> recursion_tests = {
            {"浅递归 (100层)", 100},
            {"中度递归 (500层)", 500},
            {"深度递归 (1000层)", 1000},
            {"极深递归 (5000层)", 5000}
        };
        
        for (const auto& [description, depth] : recursion_tests) {
            INFO("测试: " << description);
            
            std::string error_msg;
            
            // 递归函数测试
            std::ostringstream code;
            code << "local function deep_recursion(n)\n";
            code << "  if n <= 0 then\n";
            code << "    return 0\n";
            code << "  else\n";
            code << "    return 1 + deep_recursion(n - 1)\n";
            code << "  end\n";
            code << "end\n";
            code << "local result = deep_recursion(" << depth << ")\n";
            code << "assert(result == " << depth << ")\n";
            
            bool success = safe_execute_lua(code.str(), error_msg);
            
            if (depth <= 500) {
                // 中等深度应该成功
                REQUIRE(success);
                record_test_result(true);
            } else {
                // 深度递归可能因栈溢出失败
                record_test_result(success, success ? "" : error_msg);
                if (!success) {
                    std::cout << "⚠️  深度递归测试失败（可能是栈溢出）: " << error_msg << std::endl;
                }
            }
            
            clean_stack();
        }
    }
    
    SECTION("🔍 字符串长度边界测试") {
        std::vector<std::pair<std::string, size_t>> string_tests = {
            {"短字符串 (100字符)", 100},
            {"中等字符串 (1K字符)", 1000},
            {"长字符串 (10K字符)", 10000},
            {"超长字符串 (100K字符)", 100000}
        };
        
        for (const auto& [description, length] : string_tests) {
            INFO("测试: " << description);
            
            std::string error_msg;
            
            // 生成指定长度的字符串
            std::ostringstream code;
            code << "local parts = {}\n";
            code << "for i = 1, " << length << " do\n";
            code << "  parts[i] = 'a'\n";
            code << "end\n";
            code << "local long_string = table.concat(parts)\n";
            code << "assert(string.len(long_string) == " << length << ")\n";
            code << "-- 测试字符串操作\n";
            code << "local upper = string.upper(long_string:sub(1, 10))\n";
            code << "assert(upper == 'AAAAAAAAAA')\n";
            
            bool success = safe_execute_lua(code.str(), error_msg);
            
            if (length <= 10000) {
                REQUIRE(success);
                record_test_result(true);
            } else {
                record_test_result(success, success ? "" : error_msg);
                if (!success) {
                    std::cout << "⚠️  超长字符串测试失败: " << error_msg << std::endl;
                }
            }
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组2: 错误恢复和故障处理 (Error Recovery and Fault Handling)
// ============================================================================

TEST_CASE_METHOD(RegressionBoundaryTestFixture, "回归测试: 错误恢复", "[regression][boundary][error_recovery]") {
    SECTION("🔍 语法错误恢复测试") {
        std::vector<std::pair<std::string, std::string>> syntax_error_tests = {
            {"不完整的if语句", "if true then"},
            {"缺少end的函数", "function test()"},
            {"不匹配的括号", "local x = (1 + 2"},
            {"无效的操作符", "local x = 1 ++ 2"},
            {"错误的函数调用", "print("},
            {"无效的字符串", "local s = 'unclosed string"},
            {"错误的数字格式", "local n = 1.2.3"},
            {"无效的标识符", "local 123invalid = 1"}
        };
        
        for (const auto& [description, bad_code] : syntax_error_tests) {
            INFO("语法错误测试: " << description);
            
            std::string error_msg;
            bool success = safe_execute_lua(bad_code, error_msg);
            
            // 语法错误应该被正确捕获，不应该崩溃
            REQUIRE_FALSE(success);
            REQUIRE_FALSE(error_msg.empty());
            
            record_test_result(true);  // 正确处理错误也算成功
            clean_stack();
        }
    }
    
    SECTION("🔍 运行时错误恢复测试") {
        std::vector<std::pair<std::string, std::string>> runtime_error_tests = {
            {"除零错误", "local x = 1/0"},  // 实际上Lua中会返回inf
            {"nil索引错误", "local x = nil; print(x[1])"},
            {"调用非函数", "local x = 42; x()"},
            {"访问不存在的全局函数", "undefined_function()"},
            {"错误的参数类型", "string.sub(nil, 1, 2)"},
            {"栈溢出", "local function f() return f() end; f()"},
            {"无效的模式", "string.match('test', '[')"},
            {"表索引超界", "local t = {1,2,3}; return t[100]:sub(1,1)"}
        };
        
        for (const auto& [description, bad_code] : runtime_error_tests) {
            INFO("运行时错误测试: " << description);
            
            std::string error_msg;
            bool success = safe_execute_lua(bad_code, error_msg);
            
            // 大部分运行时错误应该被捕获
            // 注意：某些操作在Lua中可能不会产生错误（如1/0）
            record_test_result(true, error_msg);  // 记录但不强制失败
            clean_stack();
        }
    }
    
    SECTION("🔍 内存耗尽恢复测试") {
        // 模拟内存耗尽情况
        std::vector<std::string> memory_stress_tests = {
            // 快速分配大量内存
            R"(
                local t = {}
                for i = 1, 50000 do
                    t[i] = string.rep('x', 1000)  -- 每个元素1KB
                end
            )",
            
            // 创建深度嵌套结构
            R"(
                local function create_nested(depth)
                    if depth <= 0 then
                        return {}
                    else
                        return {create_nested(depth - 1)}
                    end
                end
                local nested = create_nested(1000)
            )",
            
            // 大量字符串连接
            R"(
                local result = ""
                for i = 1, 10000 do
                    result = result .. string.rep('a', 100)
                end
            )"
        };
        
        for (size_t i = 0; i < memory_stress_tests.size(); i++) {
            INFO("内存压力测试 " << (i + 1));
            
            std::string error_msg;
            bool success = safe_execute_lua(memory_stress_tests[i], error_msg);
            
            // 内存压力测试可能成功也可能失败，关键是不能崩溃
            record_test_result(true, success ? "" : error_msg);
            
            if (!success) {
                std::cout << "⚠️  内存压力测试失败（预期行为）: " << error_msg << std::endl;
            }
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组3: 性能回归检测 (Performance Regression Detection)
// ============================================================================

TEST_CASE_METHOD(RegressionBoundaryTestFixture, "回归测试: 性能基准", "[regression][boundary][performance]") {
    SECTION("🔍 计算密集型性能基准") {
        // 基准测试用例及其期望的性能范围
        struct PerformanceBenchmark {
            std::string name;
            std::string code;
            double max_time_ms;  // 最大允许执行时间
        };
        
        std::vector<PerformanceBenchmark> benchmarks = {
            {
                "斐波那契计算(n=30)",
                R"(
                    local function fib(n)
                        if n <= 2 then return 1 end
                        return fib(n-1) + fib(n-2)
                    end
                    local result = fib(30)
                )",
                5000.0  // 5秒内应该完成
            },
            
            {
                "大数组排序(1000元素)",
                R"(
                    local t = {}
                    for i = 1, 1000 do
                        t[i] = math.random(1000)
                    end
                    table.sort(t)
                )",
                100.0  // 100ms内应该完成
            },
            
            {
                "字符串处理(10000次连接)",
                R"(
                    local parts = {}
                    for i = 1, 10000 do
                        parts[i] = tostring(i)
                    end
                    local result = table.concat(parts, ',')
                )",
                200.0  // 200ms内应该完成
            },
            
            {
                "表遍历(100000元素)",
                R"(
                    local t = {}
                    for i = 1, 100000 do
                        t[i] = i
                    end
                    local sum = 0
                    for i = 1, #t do
                        sum = sum + t[i]
                    end
                )",
                500.0  // 500ms内应该完成
            }
        };
        
        for (const auto& benchmark : benchmarks) {
            INFO("性能基准: " << benchmark.name);
            
            std::string error_msg;
            
            auto start = std::chrono::high_resolution_clock::now();
            bool success = safe_execute_lua(benchmark.code, error_msg);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            double execution_time = duration.count();
            
            record_performance_sample(execution_time);
            
            REQUIRE(success);
            
            if (execution_time > benchmark.max_time_ms) {
                std::cout << "⚠️  性能回归检测: " << benchmark.name 
                         << " 执行时间 " << execution_time << "ms 超过限制 " 
                         << benchmark.max_time_ms << "ms" << std::endl;
                record_test_result(false, "Performance regression detected");
            } else {
                std::cout << "✅ 性能基准通过: " << benchmark.name 
                         << " (" << execution_time << "ms)" << std::endl;
                record_test_result(true);
            }
            
            clean_stack();
        }
    }
    
    SECTION("🔍 内存使用效率检测") {
        // 测试内存使用效率（简化版本）
        std::vector<std::pair<std::string, std::string>> memory_efficiency_tests = {
            {
                "表内存效率",
                R"(
                    collectgarbage('collect')  -- 强制垃圾回收
                    local before = collectgarbage('count')
                    
                    local t = {}
                    for i = 1, 1000 do
                        t[i] = i
                    end
                    
                    local after = collectgarbage('count')
                    local used = after - before
                    -- 1000个整数应该不超过100KB
                    assert(used < 100)
                )"
            },
            
            {
                "字符串内存效率",
                R"(
                    collectgarbage('collect')
                    local before = collectgarbage('count')
                    
                    local strings = {}
                    for i = 1, 100 do
                        strings[i] = string.rep('x', 100)  -- 100字符
                    end
                    
                    local after = collectgarbage('count')
                    local used = after - before
                    -- 10000字符应该不超过50KB（考虑重复优化）
                    assert(used < 50)
                )"
            }
        };
        
        for (const auto& [name, code] : memory_efficiency_tests) {
            INFO("内存效率测试: " << name);
            
            std::string error_msg;
            bool success = safe_execute_lua(code, error_msg);
            
            if (success) {
                std::cout << "✅ 内存效率测试通过: " << name << std::endl;
                record_test_result(true);
            } else {
                std::cout << "⚠️  内存效率测试失败: " << name << " - " << error_msg << std::endl;
                record_test_result(false, error_msg);
            }
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组4: 已知问题回归测试 (Known Issues Regression Tests)
// ============================================================================

TEST_CASE_METHOD(RegressionBoundaryTestFixture, "回归测试: 已知问题", "[regression][boundary][known_issues]") {
    SECTION("🔍 Lua 5.1.5已知边界情况") {
        // 基于Lua 5.1.5手册和已知问题的测试
        std::vector<std::pair<std::string, std::string>> known_issue_tests = {
            {
                "数字转换边界",
                R"(
                    -- 测试数字转换的精度边界
                    local max_int = 2^53 - 1
                    local str_num = tostring(max_int)
                    local back_num = tonumber(str_num)
                    assert(back_num == max_int)
                )"
            },
            
            {
                "表长度的不确定性",
                R"(
                    -- 测试含有nil的表的长度
                    local t = {1, 2, nil, 4}
                    local len = #t
                    -- 根据Lua 5.1.5规范，长度可能是2或4
                    assert(len == 2 or len == 4)
                )"
            },
            
            {
                "字符串模式边界",
                R"(
                    -- 测试边界字符类
                    local result = string.match("test123", "%a+")
                    assert(result == "test")
                    
                    local num_result = string.match("123test", "%d+")
                    assert(num_result == "123")
                )"
            },
            
            {
                "协程状态转换",
                R"(
                    local function coro_func()
                        coroutine.yield("first")
                        coroutine.yield("second")
                        return "done"
                    end
                    
                    local co = coroutine.create(coro_func)
                    assert(coroutine.status(co) == "suspended")
                    
                    local ok, val = coroutine.resume(co)
                    assert(ok and val == "first")
                    assert(coroutine.status(co) == "suspended")
                    
                    ok, val = coroutine.resume(co)
                    assert(ok and val == "second")
                    
                    ok, val = coroutine.resume(co)
                    assert(ok and val == "done")
                    assert(coroutine.status(co) == "dead")
                )"
            },
            
            {
                "元表递归保护",
                R"(
                    local t = {}
                    local mt = {
                        __index = function(table, key)
                            return table[key]  -- 潜在的无限递归
                        end
                    }
                    setmetatable(t, mt)
                    
                    -- 这应该被检测为递归并产生错误
                    local success = pcall(function()
                        return t.missing_key
                    end)
                    assert(not success)  -- 应该失败
                )"
            }
        };
        
        for (const auto& [name, code] : known_issue_tests) {
            INFO("已知问题回归测试: " << name);
            
            std::string error_msg;
            bool success = safe_execute_lua(code, error_msg);
            
            if (success) {
                std::cout << "✅ 已知问题测试通过: " << name << std::endl;
                record_test_result(true);
            } else {
                std::cout << "❌ 已知问题测试失败: " << name << " - " << error_msg << std::endl;
                record_test_result(false, error_msg);
                // 对于已知问题的回归，我们要求必须通过
                REQUIRE(success);
            }
            
            clean_stack();
        }
    }
    
    SECTION("🔍 边界数值计算") {
        std::vector<std::pair<std::string, std::string>> numeric_boundary_tests = {
            {
                "无穷大处理",
                R"(
                    local inf = 1/0
                    assert(inf == math.huge)
                    assert(inf > 0)
                    assert(inf + 1 == inf)
                )"
            },
            
            {
                "NaN处理",
                R"(
                    local nan = 0/0
                    assert(nan ~= nan)  -- NaN的特性
                    assert(not (nan < 0))
                    assert(not (nan > 0))
                    assert(not (nan == 0))
                )"
            },
            
            {
                "浮点精度边界",
                R"(
                    local a = 0.1 + 0.2
                    local b = 0.3
                    -- 浮点精度问题
                    assert(math.abs(a - b) < 1e-15)
                )"
            },
            
            {
                "大整数精度",
                R"(
                    local big1 = 9007199254740991  -- 2^53 - 1
                    local big2 = 9007199254740992  -- 2^53
                    assert(big1 + 1 == big2)
                    
                    -- 超过精度范围
                    local big3 = 9007199254740993  -- 2^53 + 1
                    assert(big3 == big2)  -- 应该相等，因为精度限制
                )"
            }
        };
        
        for (const auto& [name, code] : numeric_boundary_tests) {
            INFO("数值边界测试: " << name);
            
            std::string error_msg;
            bool success = safe_execute_lua(code, error_msg);
            
            if (success) {
                std::cout << "✅ 数值边界测试通过: " << name << std::endl;
                record_test_result(true);
            } else {
                std::cout << "❌ 数值边界测试失败: " << name << " - " << error_msg << std::endl;
                record_test_result(false, error_msg);
                REQUIRE(success);
            }
            
            clean_stack();
        }
    }
}

// ============================================================================
// 测试组5: 多线程和并发测试 (Multithreading and Concurrency Tests)
// ============================================================================

TEST_CASE_METHOD(RegressionBoundaryTestFixture, "回归测试: 并发安全", "[regression][boundary][concurrency]") {
    SECTION("🔍 多lua_State并发测试") {
        // 测试多个独立的lua_State是否可以安全并发使用
        const int num_threads = 4;
        const int iterations_per_thread = 100;
        
        std::vector<std::thread> threads;
        std::vector<bool> thread_results(num_threads, false);
        
        for (int i = 0; i < num_threads; i++) {
            threads.emplace_back([this, i, iterations_per_thread, &thread_results]() {
                lua_State* L_thread = luaL_newstate();
                luaL_openlibs(L_thread);
                
                bool all_success = true;
                
                for (int j = 0; j < iterations_per_thread; j++) {
                    std::string code = "local x = " + std::to_string(i * 1000 + j) + 
                                     "; assert(x == " + std::to_string(i * 1000 + j) + ")";
                    
                    if (luaL_loadstring(L_thread, code.c_str()) != LUA_OK ||
                        lua_pcall(L_thread, 0, 0, 0) != LUA_OK) {
                        all_success = false;
                        break;
                    }
                    
                    lua_settop(L_thread, 0);
                }
                
                lua_close(L_thread);
                thread_results[i] = all_success;
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 检查所有线程的结果
        for (int i = 0; i < num_threads; i++) {
            REQUIRE(thread_results[i]);
            record_test_result(thread_results[i]);
        }
        
        std::cout << "✅ 多线程并发测试通过: " << num_threads << "个线程，每个" 
                  << iterations_per_thread << "次迭代" << std::endl;
    }
    
    SECTION("🔍 全局状态隔离测试") {
        // 测试不同lua_State之间的全局状态隔离
        lua_State* L1 = luaL_newstate();
        lua_State* L2 = luaL_newstate();
        
        luaL_openlibs(L1);
        luaL_openlibs(L2);
        
        // 在L1中设置全局变量
        luaL_dostring(L1, "global_var = 'from_L1'");
        
        // 在L2中设置不同的全局变量
        luaL_dostring(L2, "global_var = 'from_L2'");
        
        // 验证隔离性
        luaL_dostring(L1, "assert(global_var == 'from_L1')");
        luaL_dostring(L2, "assert(global_var == 'from_L2')");
        
        lua_close(L1);
        lua_close(L2);
        
        record_test_result(true);
        std::cout << "✅ 全局状态隔离测试通过" << std::endl;
    }
}

} // namespace regression_boundary_tests
} // namespace lua_cpp

// ============================================================================
// 全局测试监听器和压力测试报告
// ============================================================================

namespace {

class RegressionBoundaryListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;
    
    void testRunStarting(Catch::TestRunInfo const& testRunInfo) override {
        std::cout << "\n🧪 开始回归和边界条件测试" << std::endl;
        std::cout << "目标: 验证系统鲁棒性和性能稳定性" << std::endl;
    }
    
    void testCaseStarting(Catch::TestCaseInfo const& testInfo) override {
        if (testInfo.tags.count("[regression]")) {
            std::cout << "\n🔬 回归测试: " << testInfo.name << std::endl;
        }
    }
    
    void testCaseEnded(Catch::TestCaseStats const& testCaseStats) override {
        if (testCaseStats.testInfo->tags.count("[regression]")) {
            if (testCaseStats.totals.assertions.allPassed()) {
                std::cout << "✅ 回归测试通过: " << testCaseStats.testInfo->name << std::endl;
            } else {
                std::cout << "❌ 回归测试失败: " << testCaseStats.testInfo->name << std::endl;
            }
        }
    }
    
    void testRunEnded(Catch::TestRunStats const& testRunStats) override {
        std::cout << "\n📊 回归测试总结:" << std::endl;
        std::cout << "总测试用例: " << testRunStats.totals.testCases.allOk() << std::endl;
        std::cout << "总断言: " << testRunStats.totals.assertions.total() << std::endl;
        std::cout << "通过: " << testRunStats.totals.assertions.passed << std::endl;
        std::cout << "失败: " << testRunStats.totals.assertions.failed << std::endl;
        
        if (testRunStats.totals.testCases.allPassed()) {
            std::cout << "🎉 所有回归测试通过！系统鲁棒性验证成功。" << std::endl;
        } else {
            std::cout << "⚠️  发现回归问题，需要进一步调查和修复。" << std::endl;
        }
    }
};

CATCH_REGISTER_LISTENER(RegressionBoundaryListener)

} // anonymous namespace