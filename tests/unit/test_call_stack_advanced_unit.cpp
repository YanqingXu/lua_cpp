#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "vm/call_stack_advanced.h"
#include "core/proto.h"
#include "core/lua_value.h"
#include <chrono>
#include <thread>

namespace lua_cpp {

/**
 * @brief AdvancedCallStack 单元测试
 * 
 * 详细测试高级调用栈的各个功能模块
 */

TEST_CASE("AdvancedCallStack - Construction and Initialization", "[unit][call_stack_advanced]") {
    SECTION("Default construction") {
        AdvancedCallStack stack;
        
        REQUIRE(stack.GetDepth() == 0);
        REQUIRE(stack.IsEmpty());
        REQUIRE(stack.GetMaxDepth() > 0);  // 应该有默认最大深度
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("Construction with custom max depth") {
        const Size custom_depth = 500;
        AdvancedCallStack stack(custom_depth);
        
        REQUIRE(stack.GetMaxDepth() == custom_depth);
        REQUIRE(stack.GetDepth() == 0);
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("Construction with zero depth should throw") {
        REQUIRE_THROWS_AS(AdvancedCallStack(0), std::invalid_argument);
    }
}

TEST_CASE("AdvancedCallStack - Basic Frame Operations", "[unit][call_stack_advanced]") {
    AdvancedCallStack stack(100);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Push single frame") {
        std::vector<LuaValue> args = {LuaValue::Number(1), LuaValue::String("test")};
        
        REQUIRE_NOTHROW(stack.PushFrame(func, args, 0));
        REQUIRE(stack.GetDepth() == 1);
        REQUIRE_FALSE(stack.IsEmpty());
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_calls == 1);
        REQUIRE(stats.max_depth == 1);
    }
    
    SECTION("Push multiple frames") {
        const int num_frames = 10;
        
        for (int i = 0; i < num_frames; ++i) {
            std::vector<LuaValue> args = {LuaValue::Number(i)};
            stack.PushFrame(func, args, 0);
        }
        
        REQUIRE(stack.GetDepth() == num_frames);
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_calls == num_frames);
        REQUIRE(stats.max_depth == num_frames);
    }
    
    SECTION("Pop frame") {
        std::vector<LuaValue> args = {LuaValue::Boolean(true)};
        stack.PushFrame(func, args, 0);
        
        std::vector<LuaValue> result = {LuaValue::Number(42)};
        REQUIRE_NOTHROW(stack.PopFrame(result));
        
        REQUIRE(stack.GetDepth() == 0);
        REQUIRE(stack.IsEmpty());
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_returns == 1);
    }
    
    SECTION("Pop empty stack should throw") {
        REQUIRE_THROWS_AS(stack.PopFrame(std::vector<LuaValue>{}), CallStackError);
    }
}

TEST_CASE("AdvancedCallStack - Tail Call Optimization", "[unit][call_stack_advanced][tail_call]") {
    AdvancedCallStack stack(100);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Basic tail call") {
        std::vector<LuaValue> args = {LuaValue::Number(1)};
        
        // 建立基础调用
        stack.PushFrame(func, args, 0);
        Size base_depth = stack.GetDepth();
        
        // 尾调用不应该增加栈深度
        REQUIRE_NOTHROW(stack.PushTailCall(func, args, 0));
        REQUIRE(stack.GetDepth() == base_depth);
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_tail_calls == 1);
    }
    
    SECTION("Chain of tail calls") {
        std::vector<LuaValue> args = {LuaValue::String("tail")};
        
        stack.PushFrame(func, args, 0);
        Size base_depth = stack.GetDepth();
        
        // 连续尾调用
        const int tail_calls = 100;
        for (int i = 0; i < tail_calls; ++i) {
            stack.PushTailCall(func, args, 0);
            REQUIRE(stack.GetDepth() == base_depth);
        }
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_tail_calls == tail_calls);
        
        // 验证尾调用优化统计
        REQUIRE(stats.tail_call_elimination_count > 0);
    }
    
    SECTION("Tail call on empty stack should throw") {
        std::vector<LuaValue> args;
        REQUIRE_THROWS_AS(stack.PushTailCall(func, args, 0), CallStackError);
    }
    
    SECTION("Mixed regular and tail calls") {
        std::vector<LuaValue> args;
        
        // 建立调用链
        stack.PushFrame(func, args, 0);  // 深度: 1
        stack.PushFrame(func, args, 0);  // 深度: 2
        stack.PushTailCall(func, args, 0); // 深度: 2 (尾调用优化)
        stack.PushFrame(func, args, 0);  // 深度: 3
        
        REQUIRE(stack.GetDepth() == 3);
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_calls == 3);
        REQUIRE(stats.total_tail_calls == 1);
    }
}

TEST_CASE("AdvancedCallStack - Performance Monitoring", "[unit][call_stack_advanced][performance]") {
    AdvancedCallStack stack(100);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Execution time tracking") {
        std::vector<LuaValue> args;
        
        stack.PushFrame(func, args, 0);
        
        // 模拟一些执行时间
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_execution_time > 0.0);
        REQUIRE(stats.avg_call_time > 0.0);
    }
    
    SECTION("Call depth statistics") {
        std::vector<LuaValue> args;
        
        // 创建不同深度的调用
        for (int depth = 1; depth <= 5; ++depth) {
            stack.PushFrame(func, args, 0);
        }
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.max_depth == 5);
        REQUIRE(stats.current_depth == 5);
        
        // 清理栈
        for (int i = 0; i < 5; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
        
        auto final_stats = stack.GetStatistics();
        REQUIRE(final_stats.current_depth == 0);
        REQUIRE(final_stats.max_depth == 5);  // 历史最大深度保持
    }
    
    SECTION("Memory usage tracking") {
        auto initial_memory = stack.GetMemoryUsage();
        REQUIRE(initial_memory > 0);
        
        std::vector<LuaValue> args;
        
        // 添加多个帧
        for (int i = 0; i < 10; ++i) {
            stack.PushFrame(func, args, 0);
        }
        
        auto peak_memory = stack.GetMemoryUsage();
        REQUIRE(peak_memory > initial_memory);
        
        // 清理帧
        for (int i = 0; i < 10; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
        
        auto final_memory = stack.GetMemoryUsage();
        REQUIRE(final_memory <= peak_memory);
    }
}

TEST_CASE("AdvancedCallStack - Call Pattern Analysis", "[unit][call_stack_advanced][patterns]") {
    AdvancedCallStack stack(100);
    Proto proto1, proto2;
    LuaValue func1 = LuaValue::Function(&proto1);
    LuaValue func2 = LuaValue::Function(&proto2);
    
    SECTION("Function call histogram") {
        std::vector<LuaValue> args;
        
        // 调用函数1多次
        for (int i = 0; i < 3; ++i) {
            stack.PushFrame(func1, args, 0);
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
        
        // 调用函数2一次
        stack.PushFrame(func2, args, 0);
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        
        auto patterns = stack.GetCallPatterns();
        REQUIRE(patterns.function_call_histogram.size() >= 1);
        
        // 验证统计正确性
        Size total_calls = 0;
        for (const auto& entry : patterns.function_call_histogram) {
            total_calls += entry.second;
        }
        REQUIRE(total_calls == 4);
    }
    
    SECTION("Recursion detection") {
        std::vector<LuaValue> args;
        
        // 模拟递归调用（同一函数多次压栈）
        for (int i = 0; i < 5; ++i) {
            stack.PushFrame(func1, args, 0);
        }
        
        auto patterns = stack.GetCallPatterns();
        REQUIRE(patterns.recursive_call_count > 0);
        REQUIRE(patterns.max_recursion_depth >= 5);
        
        // 清理栈
        for (int i = 0; i < 5; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
    }
    
    SECTION("Call frequency analysis") {
        std::vector<LuaValue> args;
        
        // 创建调用模式
        stack.PushFrame(func1, args, 0);
        stack.PushFrame(func2, args, 0);
        stack.PushFrame(func1, args, 0);  // func1被调用两次
        
        auto patterns = stack.GetCallPatterns();
        REQUIRE(patterns.hot_functions.size() >= 1);
        
        // 清理
        for (int i = 0; i < 3; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
    }
}

TEST_CASE("AdvancedCallStack - Stack Overflow Protection", "[unit][call_stack_advanced][overflow]") {
    AdvancedCallStack stack(3);  // 很小的栈
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Stack overflow detection") {
        std::vector<LuaValue> args;
        
        // 填满栈
        stack.PushFrame(func, args, 0);
        stack.PushFrame(func, args, 0);
        stack.PushFrame(func, args, 0);
        
        // 下一次push应该抛出异常
        REQUIRE_THROWS_AS(stack.PushFrame(func, args, 0), StackOverflowError);
        
        // 栈状态应该保持完整
        REQUIRE(stack.ValidateIntegrity());
        REQUIRE(stack.GetDepth() == 3);
    }
    
    SECTION("Stack overflow with tail calls") {
        std::vector<LuaValue> args;
        
        // 填满栈
        stack.PushFrame(func, args, 0);
        stack.PushFrame(func, args, 0);
        stack.PushFrame(func, args, 0);
        
        // 尾调用不应该导致栈溢出
        REQUIRE_NOTHROW(stack.PushTailCall(func, args, 0));
        REQUIRE(stack.GetDepth() == 3);
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_tail_calls == 1);
    }
}

TEST_CASE("AdvancedCallStack - Debug and Diagnostics", "[unit][call_stack_advanced][debug]") {
    AdvancedCallStack stack(100);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Debug information") {
        std::vector<LuaValue> args = {LuaValue::Number(123)};
        stack.PushFrame(func, args, 0);
        
        auto debug_info = stack.GetDebugInfo();
        REQUIRE_FALSE(debug_info.empty());
        REQUIRE(debug_info.find("Depth") != std::string::npos);
    }
    
    SECTION("Trace information") {
        std::vector<LuaValue> args;
        
        // 建立调用链
        for (int i = 0; i < 3; ++i) {
            stack.PushFrame(func, args, 0);
        }
        
        auto trace = stack.GetCallTrace();
        REQUIRE(trace.size() == 3);
        
        for (const auto& entry : trace) {
            REQUIRE(entry.function != nullptr);
            REQUIRE(entry.call_time > 0);
        }
        
        // 清理
        for (int i = 0; i < 3; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
    }
    
    SECTION("Integrity validation") {
        std::vector<LuaValue> args;
        
        // 各种操作后完整性都应该保持
        REQUIRE(stack.ValidateIntegrity());
        
        stack.PushFrame(func, args, 0);
        REQUIRE(stack.ValidateIntegrity());
        
        stack.PushTailCall(func, args, 0);
        REQUIRE(stack.ValidateIntegrity());
        
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        REQUIRE(stack.ValidateIntegrity());
    }
}

TEST_CASE("AdvancedCallStack - Statistics Reset", "[unit][call_stack_advanced][stats]") {
    AdvancedCallStack stack(100);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Statistics accumulation and reset") {
        std::vector<LuaValue> args;
        
        // 执行一些操作
        for (int i = 0; i < 5; ++i) {
            stack.PushFrame(func, args, 0);
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
        
        auto stats_before = stack.GetStatistics();
        REQUIRE(stats_before.total_calls == 5);
        REQUIRE(stats_before.total_returns == 5);
        
        // 重置统计
        stack.ResetStatistics();
        
        auto stats_after = stack.GetStatistics();
        REQUIRE(stats_after.total_calls == 0);
        REQUIRE(stats_after.total_returns == 0);
        REQUIRE(stats_after.total_tail_calls == 0);
        REQUIRE(stats_after.total_execution_time == 0.0);
        
        // 但当前状态应该保持
        REQUIRE(stack.GetDepth() == 0);
        REQUIRE(stack.ValidateIntegrity());
    }
}

TEST_CASE("AdvancedCallStack - Concurrent Access Safety", "[unit][call_stack_advanced][thread_safety]") {
    AdvancedCallStack stack(100);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    SECTION("Basic thread safety validation") {
        // 注意：这里只是基本的单线程测试
        // 真正的多线程测试需要更复杂的设置
        
        std::vector<LuaValue> args;
        
        // 快速连续操作
        for (int i = 0; i < 100; ++i) {
            stack.PushFrame(func, args, 0);
            
            if (i % 2 == 0) {
                std::vector<LuaValue> result;
                stack.PopFrame(result);
            }
        }
        
        // 系统应该保持稳定
        REQUIRE(stack.ValidateIntegrity());
        
        // 清理剩余的帧
        while (!stack.IsEmpty()) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
    }
}

} // namespace lua_cpp