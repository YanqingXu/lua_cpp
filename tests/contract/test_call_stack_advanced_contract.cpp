#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "vm/call_stack_advanced.h"
#include "core/proto.h"
#include "core/lua_value.h"

namespace lua_cpp {

/**
 * @brief AdvancedCallStack 契约测试
 * 
 * 验证高级调用栈的基本契约和不变式
 */
class AdvancedCallStackContractTest {
public:
    static void TestBasicContracts();
    static void TestTailCallContracts();
    static void TestPerformanceContracts();
    static void TestStateInvariants();
};

TEST_CASE("AdvancedCallStack - Basic Contracts", "[contract][call_stack_advanced]") {
    AdvancedCallStackContractTest::TestBasicContracts();
}

TEST_CASE("AdvancedCallStack - Tail Call Contracts", "[contract][call_stack_advanced][tail_call]") {
    AdvancedCallStackContractTest::TestTailCallContracts();
}

TEST_CASE("AdvancedCallStack - Performance Contracts", "[contract][call_stack_advanced][performance]") {
    AdvancedCallStackContractTest::TestPerformanceContracts();
}

TEST_CASE("AdvancedCallStack - State Invariants", "[contract][call_stack_advanced][invariants]") {
    AdvancedCallStackContractTest::TestStateInvariants();
}

/* ========================================================================== */
/* 基本契约测试实现 */
/* ========================================================================== */

void AdvancedCallStackContractTest::TestBasicContracts() {
    AdvancedCallStack stack(100);
    
    SECTION("构造后的初始状态契约") {
        // 栈应该为空
        REQUIRE(stack.GetDepth() == 0);
        REQUIRE(stack.IsEmpty());
        
        // 统计信息应该初始化
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_calls == 0);
        REQUIRE(stats.total_returns == 0);
        REQUIRE(stats.total_tail_calls == 0);
        REQUIRE(stats.max_depth == 0);
        
        // 完整性应该验证通过
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("Push操作契约") {
        // 模拟创建一个Proto对象
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args = {LuaValue::Number(1), LuaValue::Number(2)};
        
        auto initial_depth = stack.GetDepth();
        
        // Push应该增加栈深度
        stack.PushFrame(func, args, 0);
        REQUIRE(stack.GetDepth() == initial_depth + 1);
        REQUIRE(!stack.IsEmpty());
        
        // 统计信息应该更新
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_calls == 1);
        REQUIRE(stats.max_depth == 1);
        
        // 完整性应该保持
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("Pop操作契约") {
        // 先Push一个帧
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        stack.PushFrame(func, args, 0);
        auto depth_after_push = stack.GetDepth();
        
        // Pop应该减少栈深度
        std::vector<LuaValue> result = {LuaValue::Number(42)};
        stack.PopFrame(result);
        
        REQUIRE(stack.GetDepth() == depth_after_push - 1);
        
        // 统计信息应该更新
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_returns == 1);
        
        // 完整性应该保持
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("栈满时的契约") {
        AdvancedCallStack small_stack(2);  // 只允许2层调用
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 前两次Push应该成功
        REQUIRE_NOTHROW(small_stack.PushFrame(func, args, 0));
        REQUIRE_NOTHROW(small_stack.PushFrame(func, args, 0));
        
        // 第三次Push应该抛出异常
        REQUIRE_THROWS_AS(small_stack.PushFrame(func, args, 0), StackOverflowError);
        
        // 完整性应该保持
        REQUIRE(small_stack.ValidateIntegrity());
    }
}

void AdvancedCallStackContractTest::TestTailCallContracts() {
    AdvancedCallStack stack(100);
    
    SECTION("尾调用优化契约") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 先建立一个普通调用
        stack.PushFrame(func, args, 0);
        auto initial_depth = stack.GetDepth();
        
        // 尾调用不应该增加栈深度
        stack.PushTailCall(func, args, 0);
        REQUIRE(stack.GetDepth() == initial_depth);
        
        // 但应该增加尾调用统计
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_tail_calls == 1);
        
        // 完整性应该保持
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("尾调用链契约") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        stack.PushFrame(func, args, 0);
        auto base_depth = stack.GetDepth();
        
        // 连续多个尾调用不应该增加栈深度
        for (int i = 0; i < 10; ++i) {
            stack.PushTailCall(func, args, 0);
            REQUIRE(stack.GetDepth() == base_depth);
        }
        
        auto stats = stack.GetStatistics();
        REQUIRE(stats.total_tail_calls == 10);
        
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("空栈尾调用契约") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 空栈时尾调用应该抛出异常
        REQUIRE_THROWS_AS(stack.PushTailCall(func, args, 0), CallStackError);
        
        REQUIRE(stack.ValidateIntegrity());
    }
}

void AdvancedCallStackContractTest::TestPerformanceContracts() {
    AdvancedCallStack stack(1000);
    
    SECTION("性能统计契约") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        auto initial_stats = stack.GetStatistics();
        
        // 执行一些操作
        stack.PushFrame(func, args, 0);
        stack.PushTailCall(func, args, 0);
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        
        auto final_stats = stack.GetStatistics();
        
        // 统计应该递增
        REQUIRE(final_stats.total_calls > initial_stats.total_calls);
        REQUIRE(final_stats.total_tail_calls > initial_stats.total_tail_calls);
        REQUIRE(final_stats.total_returns > initial_stats.total_returns);
        
        // 时间统计应该有意义
        REQUIRE(final_stats.total_execution_time >= 0.0);
        REQUIRE(final_stats.avg_call_time >= 0.0);
    }
    
    SECTION("内存使用契约") {
        auto initial_memory = stack.GetMemoryUsage();
        REQUIRE(initial_memory > 0);
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 添加帧应该增加内存使用
        stack.PushFrame(func, args, 0);
        auto memory_after_push = stack.GetMemoryUsage();
        REQUIRE(memory_after_push > initial_memory);
        
        // 移除帧应该减少内存使用
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        auto memory_after_pop = stack.GetMemoryUsage();
        REQUIRE(memory_after_pop <= memory_after_push);
    }
    
    SECTION("调用模式分析契约") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 建立一些调用模式
        for (int i = 0; i < 5; ++i) {
            stack.PushFrame(func, args, 0);
        }
        
        auto patterns = stack.GetCallPatterns();
        REQUIRE(patterns.recursive_call_count >= 0);
        REQUIRE(patterns.max_recursion_depth >= 0);
        REQUIRE(patterns.function_call_histogram.size() >= 0);
        
        // 清理
        for (int i = 0; i < 5; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
    }
}

void AdvancedCallStackContractTest::TestStateInvariants() {
    AdvancedCallStack stack(100);
    
    SECTION("栈深度不变式") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        auto initial_depth = stack.GetDepth();
        
        // Push N次后，栈深度应该增加N
        const int N = 10;
        for (int i = 0; i < N; ++i) {
            stack.PushFrame(func, args, 0);
        }
        REQUIRE(stack.GetDepth() == initial_depth + N);
        
        // Pop N次后，栈深度应该回到初始值
        for (int i = 0; i < N; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
        REQUIRE(stack.GetDepth() == initial_depth);
        
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("统计信息单调性不变式") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        auto prev_stats = stack.GetStatistics();
        
        // 执行操作后，相关统计应该单调递增
        stack.PushFrame(func, args, 0);
        auto curr_stats = stack.GetStatistics();
        
        REQUIRE(curr_stats.total_calls >= prev_stats.total_calls);
        REQUIRE(curr_stats.max_depth >= prev_stats.max_depth);
        REQUIRE(curr_stats.total_execution_time >= prev_stats.total_execution_time);
        
        prev_stats = curr_stats;
        
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        curr_stats = stack.GetStatistics();
        
        REQUIRE(curr_stats.total_returns >= prev_stats.total_returns);
        REQUIRE(curr_stats.total_execution_time >= prev_stats.total_execution_time);
    }
    
    SECTION("完整性不变式") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 在任何操作序列后，完整性检查都应该通过
        REQUIRE(stack.ValidateIntegrity());
        
        stack.PushFrame(func, args, 0);
        REQUIRE(stack.ValidateIntegrity());
        
        stack.PushFrame(func, args, 0);
        REQUIRE(stack.ValidateIntegrity());
        
        stack.PushTailCall(func, args, 0);
        REQUIRE(stack.ValidateIntegrity());
        
        std::vector<LuaValue> result;
        stack.PopFrame(result);
        REQUIRE(stack.ValidateIntegrity());
        
        stack.PopFrame(result);
        REQUIRE(stack.ValidateIntegrity());
    }
    
    SECTION("异常安全性不变式") {
        AdvancedCallStack small_stack(2);
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 填满栈
        small_stack.PushFrame(func, args, 0);
        small_stack.PushFrame(func, args, 0);
        
        // 尝试溢出应该保持栈的完整性
        REQUIRE_THROWS(small_stack.PushFrame(func, args, 0));
        REQUIRE(small_stack.ValidateIntegrity());
        REQUIRE(small_stack.GetDepth() == 2);  // 栈状态不应该改变
        
        // 空栈Pop应该保持完整性
        AdvancedCallStack empty_stack(10);
        REQUIRE_THROWS(empty_stack.PopFrame(std::vector<LuaValue>{}));
        REQUIRE(empty_stack.ValidateIntegrity());
        REQUIRE(empty_stack.IsEmpty());
    }
}

} // namespace lua_cpp