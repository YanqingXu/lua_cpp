#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "vm/call_stack_advanced.h"
#include "vm/upvalue_manager.h"
#include "vm/coroutine_support.h"
#include "vm/virtual_machine.h"
#include "core/proto.h"
#include "core/lua_value.h"
#include <memory>
#include <vector>
#include <chrono>

namespace lua_cpp {

/**
 * @brief T026 高级调用栈管理集成测试
 * 
 * 测试各个T026组件之间的集成和协作
 */

// 简化的VM实现用于集成测试
class TestVirtualMachine : public VirtualMachine {
public:
    TestVirtualMachine() 
        : call_stack_(std::make_unique<AdvancedCallStack>(200))
        , upvalue_manager_(std::make_unique<UpvalueManager>())
        , coroutine_support_(std::make_unique<CoroutineSupport>(this)) {
    }
    
    AdvancedCallStack* GetCallStack() { return call_stack_.get(); }
    UpvalueManager* GetUpvalueManager() { return upvalue_manager_.get(); }
    CoroutineSupport* GetCoroutineSupport() { return coroutine_support_.get(); }
    
private:
    std::unique_ptr<AdvancedCallStack> call_stack_;
    std::unique_ptr<UpvalueManager> upvalue_manager_;
    std::unique_ptr<CoroutineSupport> coroutine_support_;
};

TEST_CASE("T026 Integration - CallStack and UpvalueManager", "[integration][t026]") {
    TestVirtualMachine vm;
    auto* call_stack = vm.GetCallStack();
    auto* upvalue_manager = vm.GetUpvalueManager();
    
    SECTION("Function call with upvalues") {
        LuaStack stack(256);
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        // 模拟闭包环境：创建外部变量
        std::vector<LuaValue> outer_vars = {
            LuaValue::Number(100),
            LuaValue::String("closure_var"),
            LuaValue::Boolean(true)
        };
        
        for (const auto& var : outer_vars) {
            stack.Push(var);
        }
        
        // 为外部变量创建Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (Size i = 0; i < outer_vars.size(); ++i) {
            upvalues.push_back(upvalue_manager->CreateUpvalue(&stack, i));
        }
        
        // 调用函数
        std::vector<LuaValue> args = {LuaValue::Number(42)};
        call_stack->PushFrame(func, args, 0);
        
        // 验证状态
        REQUIRE(call_stack->GetDepth() == 1);
        REQUIRE(upvalue_manager->GetStatistics().total_upvalues == 3);
        REQUIRE(upvalue_manager->GetStatistics().open_upvalues == 3);
        
        // 模拟函数返回，关闭Upvalue
        upvalue_manager->CloseUpvalues(&stack, 0);
        
        std::vector<LuaValue> result = {LuaValue::Number(84)};
        call_stack->PopFrame(result);
        
        // 验证清理
        REQUIRE(call_stack->GetDepth() == 0);
        REQUIRE(upvalue_manager->GetStatistics().open_upvalues == 0);
        REQUIRE(upvalue_manager->GetStatistics().closed_upvalues == 3);
        
        // 验证Upvalue值保持
        for (Size i = 0; i < upvalues.size(); ++i) {
            REQUIRE(upvalues[i]->IsClosed());
            REQUIRE(upvalues[i]->GetValue() == outer_vars[i]);
        }
    }
    
    SECTION("Tail call with upvalue optimization") {
        LuaStack stack(256);
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        // 建立初始调用
        std::vector<LuaValue> args;
        call_stack->PushFrame(func, args, 0);
        
        // 创建一些Upvalue
        stack.Push(LuaValue::String("tail_call_test"));
        auto upvalue = upvalue_manager->CreateUpvalue(&stack, 0);
        
        auto initial_depth = call_stack->GetDepth();
        
        // 尾调用不应该影响Upvalue管理
        call_stack->PushTailCall(func, args, 0);
        
        REQUIRE(call_stack->GetDepth() == initial_depth);  // 尾调用优化
        REQUIRE(upvalue->IsOpen());  // Upvalue应该保持开放
        
        // 清理
        upvalue_manager->CloseUpvalues(&stack, 0);
        std::vector<LuaValue> result;
        call_stack->PopFrame(result);
        
        REQUIRE(upvalue->IsClosed());
    }
    
    SECTION("Nested calls with multiple upvalues") {
        LuaStack stack(256);
        Proto proto1, proto2, proto3;
        LuaValue func1 = LuaValue::Function(&proto1);
        LuaValue func2 = LuaValue::Function(&proto2);
        LuaValue func3 = LuaValue::Function(&proto3);
        
        // 创建嵌套调用环境
        std::vector<LuaValue> level0_vars = {LuaValue::Number(0), LuaValue::String("level0")};
        std::vector<LuaValue> level1_vars = {LuaValue::Number(1), LuaValue::Boolean(false)};
        std::vector<LuaValue> level2_vars = {LuaValue::Number(2)};
        
        // Level 0
        for (const auto& var : level0_vars) {
            stack.Push(var);
        }
        
        std::vector<std::shared_ptr<Upvalue>> level0_upvalues;
        for (Size i = 0; i < level0_vars.size(); ++i) {
            level0_upvalues.push_back(upvalue_manager->CreateUpvalue(&stack, i));
        }
        
        call_stack->PushFrame(func1, std::vector<LuaValue>{}, 0);
        
        // Level 1
        for (const auto& var : level1_vars) {
            stack.Push(var);
        }
        
        std::vector<std::shared_ptr<Upvalue>> level1_upvalues;
        Size base_index = level0_vars.size();
        for (Size i = 0; i < level1_vars.size(); ++i) {
            level1_upvalues.push_back(upvalue_manager->CreateUpvalue(&stack, base_index + i));
        }
        
        call_stack->PushFrame(func2, std::vector<LuaValue>{}, 0);
        
        // Level 2
        for (const auto& var : level2_vars) {
            stack.Push(var);
        }
        
        std::vector<std::shared_ptr<Upvalue>> level2_upvalues;
        base_index += level1_vars.size();
        for (Size i = 0; i < level2_vars.size(); ++i) {
            level2_upvalues.push_back(upvalue_manager->CreateUpvalue(&stack, base_index + i));
        }
        
        call_stack->PushFrame(func3, std::vector<LuaValue>{}, 0);
        
        // 验证嵌套状态
        REQUIRE(call_stack->GetDepth() == 3);
        auto stats = upvalue_manager->GetStatistics();
        REQUIRE(stats.total_upvalues == 5);  // 2 + 2 + 1
        REQUIRE(stats.open_upvalues == 5);
        
        // 逐级返回
        std::vector<LuaValue> result;
        
        // Level 2 return
        call_stack->PopFrame(result);
        upvalue_manager->CloseUpvalues(&stack, base_index);
        
        // Level 1 return  
        call_stack->PopFrame(result);
        upvalue_manager->CloseUpvalues(&stack, level0_vars.size());
        
        // Level 0 return
        call_stack->PopFrame(result);
        upvalue_manager->CloseUpvalues(&stack, 0);
        
        // 验证最终状态
        REQUIRE(call_stack->GetDepth() == 0);
        auto final_stats = upvalue_manager->GetStatistics();
        REQUIRE(final_stats.open_upvalues == 0);
        REQUIRE(final_stats.closed_upvalues == 5);
    }
}

TEST_CASE("T026 Integration - CallStack and CoroutineSupport", "[integration][t026]") {
    TestVirtualMachine vm;
    auto* call_stack = vm.GetCallStack();
    auto* coroutine_support = vm.GetCoroutineSupport();
    
    SECTION("Coroutine creation with call stack") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args = {LuaValue::Number(123)};
        
        // 主线程调用栈状态
        auto initial_depth = call_stack->GetDepth();
        
        // 创建协程
        auto coroutine = coroutine_support->CreateCoroutine(func, args);
        REQUIRE(coroutine.GetType() != LuaValueType::Nil);
        
        // 主线程调用栈不应该受影响
        REQUIRE(call_stack->GetDepth() == initial_depth);
        
        // 协程状态应该正确
        auto status = coroutine_support->GetCoroutineStatus(coroutine);
        REQUIRE(status == "suspended");
        
        auto& scheduler = coroutine_support->GetScheduler();
        REQUIRE(scheduler.GetActiveCoroutineCount() == 2);  // 主线程 + 1个协程
    }
    
    SECTION("Call stack isolation between coroutines") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        // 在主线程建立调用栈
        std::vector<LuaValue> main_args = {LuaValue::String("main")};
        call_stack->PushFrame(func, main_args, 0);
        
        auto main_depth = call_stack->GetDepth();
        REQUIRE(main_depth == 1);
        
        // 创建协程
        std::vector<LuaValue> coro_args = {LuaValue::String("coroutine")};
        auto coroutine = coroutine_support->CreateCoroutine(func, coro_args);
        
        // 主线程调用栈应该不受影响
        REQUIRE(call_stack->GetDepth() == main_depth);
        
        // 验证协程有独立的调用栈
        auto& scheduler = coroutine_support->GetScheduler();
        auto coro_id = scheduler.GetAllCoroutineIds().back();  // 最新创建的协程
        auto coro_context = scheduler.GetCoroutine(coro_id);
        
        if (coro_context) {
            // 协程应该有自己的调用栈
            REQUIRE(&coro_context->GetCallStack() != call_stack);
            REQUIRE(coro_context->GetCallStack().GetDepth() == 0);  // 协程栈为空
        }
        
        // 清理主线程栈
        std::vector<LuaValue> result;
        call_stack->PopFrame(result);
    }
    
    SECTION("Tail call optimization in coroutines") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        // 创建协程
        auto coroutine = coroutine_support->CreateCoroutine(func);
        
        auto& scheduler = coroutine_support->GetScheduler();
        auto coro_id = scheduler.GetAllCoroutineIds().back();
        auto coro_context = scheduler.GetCoroutine(coro_id);
        
        if (coro_context) {
            auto& coro_stack = coro_context->GetCallStack();
            
            // 在协程中建立调用
            std::vector<LuaValue> args;
            coro_stack.PushFrame(func, args, 0);
            
            auto base_depth = coro_stack.GetDepth();
            
            // 协程中的尾调用也应该被优化
            coro_stack.PushTailCall(func, args, 0);
            REQUIRE(coro_stack.GetDepth() == base_depth);
            
            auto stats = coro_stack.GetStatistics();
            REQUIRE(stats.total_tail_calls == 1);
            
            // 清理
            std::vector<LuaValue> result;
            coro_stack.PopFrame(result);
        }
    }
}

TEST_CASE("T026 Integration - UpvalueManager and CoroutineSupport", "[integration][t026]") {
    TestVirtualMachine vm;
    auto* upvalue_manager = vm.GetUpvalueManager();
    auto* coroutine_support = vm.GetCoroutineSupport();
    
    SECTION("Upvalue isolation between coroutines") {
        LuaStack main_stack(256);
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        // 主线程创建Upvalue
        main_stack.Push(LuaValue::String("main_upvalue"));
        auto main_upvalue = upvalue_manager->CreateUpvalue(&main_stack, 0);
        
        // 创建协程
        auto coroutine = coroutine_support->CreateCoroutine(func);
        
        auto& scheduler = coroutine_support->GetScheduler();
        auto coro_id = scheduler.GetAllCoroutineIds().back();
        auto coro_context = scheduler.GetCoroutine(coro_id);
        
        if (coro_context) {
            auto& coro_stack = coro_context->GetLuaStack();
            auto& coro_upvalue_mgr = coro_context->GetUpvalueManager();
            
            // 协程应该有独立的Upvalue管理器
            REQUIRE(&coro_upvalue_mgr != upvalue_manager);
            
            // 协程创建自己的Upvalue
            coro_stack.Push(LuaValue::String("coro_upvalue"));
            auto coro_upvalue = coro_upvalue_mgr.CreateUpvalue(&coro_stack, 0);
            
            // 两个Upvalue应该独立
            REQUIRE(main_upvalue.get() != coro_upvalue.get());
            REQUIRE(main_upvalue->GetValue().GetString() == "main_upvalue");
            REQUIRE(coro_upvalue->GetValue().GetString() == "coro_upvalue");
            
            // 统计应该独立
            auto main_stats = upvalue_manager->GetStatistics();
            auto coro_stats = coro_upvalue_mgr.GetStatistics();
            
            REQUIRE(main_stats.total_upvalues == 1);
            REQUIRE(coro_stats.total_upvalues == 1);
        }
    }
    
    SECTION("Upvalue lifecycle with coroutine suspension") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        // 创建协程
        auto coroutine = coroutine_support->CreateCoroutine(func);
        
        auto& scheduler = coroutine_support->GetScheduler();
        auto coro_id = scheduler.GetAllCoroutineIds().back();
        auto coro_context = scheduler.GetCoroutine(coro_id);
        
        if (coro_context) {
            auto& coro_stack = coro_context->GetLuaStack();
            auto& coro_upvalue_mgr = coro_context->GetUpvalueManager();
            
            // 在协程中创建Upvalue
            coro_stack.Push(LuaValue::Number(456));
            auto upvalue = coro_upvalue_mgr.CreateUpvalue(&coro_stack, 0);
            
            REQUIRE(upvalue->IsOpen());
            
            // 模拟协程挂起（Upvalue应该保持）
            coro_context->SetState(CoroutineState::SUSPENDED);
            
            REQUIRE(upvalue->IsOpen());
            REQUIRE(upvalue->GetValue().GetNumber() == 456);
            
            // 模拟协程恢复后关闭Upvalue
            coro_context->SetState(CoroutineState::RUNNING);
            coro_upvalue_mgr.CloseUpvalues(&coro_stack, 0);
            
            REQUIRE(upvalue->IsClosed());
            REQUIRE(upvalue->GetValue().GetNumber() == 456);  // 值保持
        }
    }
}

TEST_CASE("T026 Integration - Complete System Integration", "[integration][t026]") {
    TestVirtualMachine vm;
    auto* call_stack = vm.GetCallStack();
    auto* upvalue_manager = vm.GetUpvalueManager();
    auto* coroutine_support = vm.GetCoroutineSupport();
    
    SECTION("Complex scenario: Nested coroutines with closures") {
        LuaStack main_stack(256);
        Proto outer_proto, inner_proto;
        LuaValue outer_func = LuaValue::Function(&outer_proto);
        LuaValue inner_func = LuaValue::Function(&inner_proto);
        
        // 主线程环境
        main_stack.Push(LuaValue::String("global_var"));
        auto global_upvalue = upvalue_manager->CreateUpvalue(&main_stack, 0);
        
        call_stack->PushFrame(outer_func, std::vector<LuaValue>{}, 0);
        
        // 创建第一个协程
        auto coro1 = coroutine_support->CreateCoroutine(inner_func);
        
        // 创建第二个协程  
        auto coro2 = coroutine_support->CreateCoroutine(inner_func);
        
        auto& scheduler = coroutine_support->GetScheduler();
        REQUIRE(scheduler.GetActiveCoroutineCount() == 3);  // 主线程 + 2个协程
        
        // 为每个协程设置独立的环境
        auto coro_ids = scheduler.GetAllCoroutineIds();
        
        for (Size i = 1; i < coro_ids.size(); ++i) {  // 跳过主线程(ID=0)
            auto coro_context = scheduler.GetCoroutine(coro_ids[i]);
            if (coro_context) {
                auto& coro_stack = coro_context->GetLuaStack();
                auto& coro_call_stack = coro_context->GetCallStack();
                auto& coro_upvalue_mgr = coro_context->GetUpvalueManager();
                
                // 协程中创建局部变量和Upvalue
                coro_stack.Push(LuaValue::Number(i * 100));
                auto local_upvalue = coro_upvalue_mgr.CreateUpvalue(&coro_stack, 0);
                
                // 协程中建立调用栈
                coro_call_stack.PushFrame(inner_func, std::vector<LuaValue>{}, 0);
                
                // 使用尾调用优化
                coro_call_stack.PushTailCall(inner_func, std::vector<LuaValue>{}, 0);
                
                // 验证状态
                REQUIRE(coro_call_stack.GetDepth() == 1);  // 尾调用优化
                REQUIRE(local_upvalue->IsOpen());
                
                auto coro_stats = coro_call_stack.GetStatistics();
                REQUIRE(coro_stats.total_tail_calls == 1);
            }
        }
        
        // 验证系统整体状态
        REQUIRE(call_stack->GetDepth() == 1);  // 主线程栈
        REQUIRE(global_upvalue->IsOpen());
        
        auto main_stats = upvalue_manager->GetStatistics();
        REQUIRE(main_stats.total_upvalues == 1);  // 只有主线程的Upvalue
        
        // 清理：协程先结束
        coroutine_support->Cleanup();
        REQUIRE(scheduler.GetActiveCoroutineCount() == 1);  // 只剩主线程
        
        // 主线程清理
        upvalue_manager->CloseUpvalues(&main_stack, 0);
        std::vector<LuaValue> result;
        call_stack->PopFrame(result);
        
        REQUIRE(call_stack->GetDepth() == 0);
        REQUIRE(global_upvalue->IsClosed());
    }
    
    SECTION("Performance integration under load") {
        const int NUM_COROUTINES = 10;
        const int OPERATIONS_PER_CORO = 50;
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        auto start_time = std::chrono::steady_clock::now();
        
        // 创建多个协程
        std::vector<LuaValue> coroutines;
        for (int i = 0; i < NUM_COROUTINES; ++i) {
            coroutines.push_back(coroutine_support->CreateCoroutine(func));
        }
        
        auto& scheduler = coroutine_support->GetScheduler();
        auto coro_ids = scheduler.GetAllCoroutineIds();
        
        // 每个协程执行复杂操作
        for (Size i = 1; i < coro_ids.size(); ++i) {
            auto coro_context = scheduler.GetCoroutine(coro_ids[i]);
            if (coro_context) {
                auto& coro_stack = coro_context->GetLuaStack();
                auto& coro_call_stack = coro_context->GetCallStack();
                auto& coro_upvalue_mgr = coro_context->GetUpvalueManager();
                
                for (int op = 0; op < OPERATIONS_PER_CORO; ++op) {
                    // 创建变量和Upvalue
                    coro_stack.Push(LuaValue::Number(op));
                    auto upvalue = coro_upvalue_mgr.CreateUpvalue(&coro_stack, 
                        coro_stack.GetSize() - 1);
                    
                    // 调用栈操作
                    coro_call_stack.PushFrame(func, std::vector<LuaValue>{}, 0);
                    
                    if (op % 3 == 0) {
                        // 使用尾调用
                        coro_call_stack.PushTailCall(func, std::vector<LuaValue>{}, 0);
                    }
                    
                    // 清理
                    std::vector<LuaValue> result;
                    coro_call_stack.PopFrame(result);
                    
                    if (op % 10 == 0) {
                        // 定期关闭一些Upvalue
                        Size close_level = coro_stack.GetSize() > 5 ? 
                            coro_stack.GetSize() - 5 : 0;
                        coro_upvalue_mgr.CloseUpvalues(&coro_stack, close_level);
                    }
                }
            }
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time).count();
        
        // 验证性能合理（总操作时间不应该过长）
        REQUIRE(duration < 1.0);  // 应该在1秒内完成
        
        // 验证系统状态
        REQUIRE(scheduler.ValidateIntegrity());
        
        // 收集统计信息
        auto scheduler_stats = scheduler.GetStats();
        auto main_upvalue_stats = upvalue_manager->GetStatistics();
        auto call_stack_stats = call_stack->GetStatistics();
        
        // 验证统计合理性
        REQUIRE(scheduler_stats.total_coroutines_created == NUM_COROUTINES);
        REQUIRE(scheduler_stats.current_coroutine_count >= 1);  // 至少主线程
        
        // 清理
        coroutine_support->Cleanup();
        
        // 最终验证
        REQUIRE(scheduler.GetActiveCoroutineCount() == 1);
        REQUIRE(scheduler.ValidateIntegrity());
    }
    
    SECTION("Error recovery integration") {
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        LuaStack stack(256);
        
        // 建立初始状态
        stack.Push(LuaValue::String("test"));
        auto upvalue = upvalue_manager->CreateUpvalue(&stack, 0);
        call_stack->PushFrame(func, std::vector<LuaValue>{}, 0);
        
        auto initial_call_depth = call_stack->GetDepth();
        auto initial_upvalue_count = upvalue_manager->GetStatistics().total_upvalues;
        
        // 模拟各种错误情况
        try {
            // 栈溢出错误
            AdvancedCallStack small_stack(1);
            small_stack.PushFrame(func, std::vector<LuaValue>{}, 0);
            small_stack.PushFrame(func, std::vector<LuaValue>{}, 0);  // 应该失败
        } catch (...) {
            // 主系统应该不受影响
            REQUIRE(call_stack->GetDepth() == initial_call_depth);
        }
        
        try {
            // Upvalue错误
            upvalue_manager->CreateUpvalue(&stack, 100);  // 无效索引
        } catch (...) {
            // 系统状态应该保持
            auto stats = upvalue_manager->GetStatistics();
            REQUIRE(stats.total_upvalues == initial_upvalue_count);
        }
        
        try {
            // 协程错误
            LuaValue invalid_func = LuaValue::Number(123);
            coroutine_support->CreateCoroutine(invalid_func);
        } catch (...) {
            // 协程系统应该不受影响
            auto& scheduler = coroutine_support->GetScheduler();
            REQUIRE(scheduler.ValidateIntegrity());
        }
        
        // 验证所有组件完整性
        REQUIRE(call_stack->ValidateIntegrity());
        REQUIRE(upvalue_manager->ValidateIntegrity());
        REQUIRE(coroutine_support->GetScheduler().ValidateIntegrity());
        
        // 正常清理
        upvalue_manager->CloseUpvalues(&stack, 0);
        std::vector<LuaValue> result;
        call_stack->PopFrame(result);
        
        REQUIRE(upvalue->IsClosed());
        REQUIRE(call_stack->GetDepth() == 0);
    }
}

} // namespace lua_cpp