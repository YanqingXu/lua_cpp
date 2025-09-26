#include <benchmark/benchmark.h>
#include "vm/call_stack_advanced.h"
#include "vm/upvalue_manager.h"
#include "vm/coroutine_support.h"
#include "vm/virtual_machine.h"
#include "core/proto.h"
#include "core/lua_value.h"
#include "vm/stack.h"
#include <memory>
#include <vector>
#include <random>

namespace lua_cpp {

/**
 * @brief T026 高级调用栈管理性能基准测试
 * 
 * 测试各个组件的性能特征和优化效果
 */

// 简化VM用于基准测试
class BenchmarkVM : public VirtualMachine {
public:
    BenchmarkVM() : call_stack_(200), upvalue_manager_(), coroutine_support_(this) {}
    
    AdvancedCallStack* GetCallStack() { return &call_stack_; }
    UpvalueManager* GetUpvalueManager() { return &upvalue_manager_; }
    CoroutineSupport* GetCoroutineSupport() { return &coroutine_support_; }
    
private:
    AdvancedCallStack call_stack_;
    UpvalueManager upvalue_manager_;
    CoroutineSupport coroutine_support_;
};

/* ========================================================================== */
/* AdvancedCallStack 性能基准 */
/* ========================================================================== */

static void BM_CallStack_PushPop(benchmark::State& state) {
    AdvancedCallStack stack(1000);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    std::vector<LuaValue> args = {LuaValue::Number(42)};
    std::vector<LuaValue> result = {LuaValue::Number(84)};
    
    for (auto _ : state) {
        stack.PushFrame(func, args, 0);
        stack.PopFrame(result);
    }
    
    state.SetItemsProcessed(state.iterations() * 2);  // Push + Pop
}
BENCHMARK(BM_CallStack_PushPop);

static void BM_CallStack_TailCallOptimization(benchmark::State& state) {
    AdvancedCallStack stack(1000);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    std::vector<LuaValue> args;
    
    // 建立基础调用
    stack.PushFrame(func, args, 0);
    
    for (auto _ : state) {
        stack.PushTailCall(func, args, 0);
    }
    
    // 清理
    std::vector<LuaValue> result;
    stack.PopFrame(result);
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_CallStack_TailCallOptimization);

static void BM_CallStack_DeepNesting(benchmark::State& state) {
    const Size depth = state.range(0);
    
    for (auto _ : state) {
        AdvancedCallStack stack(depth + 100);
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        std::vector<LuaValue> args;
        
        // 建立深度嵌套
        for (Size i = 0; i < depth; ++i) {
            stack.PushFrame(func, args, 0);
        }
        
        // 清理
        for (Size i = 0; i < depth; ++i) {
            std::vector<LuaValue> result;
            stack.PopFrame(result);
        }
    }
    
    state.SetComplexityN(depth);
}
BENCHMARK(BM_CallStack_DeepNesting)->Range(8, 512)->Complexity(benchmark::oN);

static void BM_CallStack_StatisticsCollection(benchmark::State& state) {
    AdvancedCallStack stack(1000);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    std::vector<LuaValue> args;
    std::vector<LuaValue> result;
    
    // 预热：建立一些调用历史
    for (int i = 0; i < 100; ++i) {
        stack.PushFrame(func, args, 0);
        stack.PopFrame(result);
    }
    
    for (auto _ : state) {
        auto stats = stack.GetStatistics();
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK(BM_CallStack_StatisticsCollection);

static void BM_CallStack_CallPatternAnalysis(benchmark::State& state) {
    AdvancedCallStack stack(1000);
    Proto proto1, proto2;
    LuaValue func1 = LuaValue::Function(&proto1);
    LuaValue func2 = LuaValue::Function(&proto2);
    std::vector<LuaValue> args;
    std::vector<LuaValue> result;
    
    // 建立复杂的调用模式
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 1);
    
    for (int i = 0; i < 200; ++i) {
        LuaValue func = dis(gen) ? func1 : func2;
        stack.PushFrame(func, args, 0);
        stack.PopFrame(result);
    }
    
    for (auto _ : state) {
        auto patterns = stack.GetCallPatterns();
        benchmark::DoNotOptimize(patterns);
    }
}
BENCHMARK(BM_CallStack_CallPatternAnalysis);

/* ========================================================================== */
/* UpvalueManager 性能基准 */
/* ========================================================================== */

static void BM_Upvalue_CreateAndAccess(benchmark::State& state) {
    UpvalueManager manager;
    LuaStack stack(1000);
    
    // 预填栈
    for (int i = 0; i < 100; ++i) {
        stack.Push(LuaValue::Number(i));
    }
    
    for (auto _ : state) {
        auto upvalue = manager.CreateUpvalue(&stack, 50);
        auto value = upvalue->GetValue();
        benchmark::DoNotOptimize(value);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Upvalue_CreateAndAccess);

static void BM_Upvalue_Sharing(benchmark::State& state) {
    UpvalueManager manager;
    LuaStack stack(1000);
    
    stack.Push(LuaValue::String("shared_value"));
    
    for (auto _ : state) {
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        benchmark::DoNotOptimize(upvalue);
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Upvalue_Sharing);

static void BM_Upvalue_MassCreation(benchmark::State& state) {
    const Size count = state.range(0);
    
    for (auto _ : state) {
        UpvalueManager manager;
        LuaStack stack(count + 100);
        
        // 填充栈
        for (Size i = 0; i < count; ++i) {
            stack.Push(LuaValue::Number(i));
        }
        
        // 创建大量Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        upvalues.reserve(count);
        
        for (Size i = 0; i < count; ++i) {
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        benchmark::DoNotOptimize(upvalues);
    }
    
    state.SetComplexityN(count);
}
BENCHMARK(BM_Upvalue_MassCreation)->Range(8, 1024)->Complexity(benchmark::oN);

static void BM_Upvalue_CloseOperations(benchmark::State& state) {
    const Size count = state.range(0);
    
    for (auto _ : state) {
        state.PauseTiming();
        
        UpvalueManager manager;
        LuaStack stack(count + 100);
        
        // 创建Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (Size i = 0; i < count; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        state.ResumeTiming();
        
        // 关闭所有Upvalue
        manager.CloseUpvalues(&stack, 0);
        
        benchmark::DoNotOptimize(upvalues);
    }
    
    state.SetComplexityN(count);
}
BENCHMARK(BM_Upvalue_CloseOperations)->Range(8, 512)->Complexity(benchmark::oN);

static void BM_Upvalue_GarbageCollection(benchmark::State& state) {
    UpvalueManager manager;
    LuaStack stack(1000);
    
    // 创建一些Upvalue
    std::vector<std::shared_ptr<Upvalue>> upvalues;
    for (int i = 0; i < 100; ++i) {
        stack.Push(LuaValue::Number(i));
        upvalues.push_back(manager.CreateUpvalue(&stack, i));
    }
    
    // 释放一半的引用
    for (Size i = 0; i < upvalues.size(); i += 2) {
        upvalues[i]->Unmark();
    }
    
    for (auto _ : state) {
        manager.MarkReachableUpvalues();
        auto swept = manager.SweepUnmarkedUpvalues();
        benchmark::DoNotOptimize(swept);
    }
}
BENCHMARK(BM_Upvalue_GarbageCollection);

/* ========================================================================== */
/* CoroutineSupport 性能基准 */
/* ========================================================================== */

static void BM_Coroutine_CreateAndDestroy(benchmark::State& state) {
    BenchmarkVM vm;
    auto* support = vm.GetCoroutineSupport();
    
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    for (auto _ : state) {
        auto coroutine = support->CreateCoroutine(func);
        benchmark::DoNotOptimize(coroutine);
        // 协程会在作用域结束时自动清理
    }
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coroutine_CreateAndDestroy);

static void BM_Coroutine_ContextSwitch(benchmark::State& state) {
    BenchmarkVM vm;
    auto* support = vm.GetCoroutineSupport();
    
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    // 创建多个协程
    std::vector<LuaValue> coroutines;
    for (int i = 0; i < 10; ++i) {
        coroutines.push_back(support->CreateCoroutine(func));
    }
    
    auto& scheduler = support->GetScheduler();
    auto coro_ids = scheduler.GetAllCoroutineIds();
    
    Size current_index = 1;  // 跳过主线程
    
    for (auto _ : state) {
        // 模拟上下文切换
        if (current_index < coro_ids.size()) {
            scheduler.SwitchToCoroutine(coro_ids[current_index]);
            current_index = (current_index + 1) % coro_ids.size();
            if (current_index == 0) current_index = 1;  // 跳过主线程
        }
    }
    
    // 切换回主线程
    scheduler.SwitchToMainThread();
    
    state.SetItemsProcessed(state.iterations());
}
BENCHMARK(BM_Coroutine_ContextSwitch);

static void BM_Coroutine_MassCreation(benchmark::State& state) {
    const Size count = state.range(0);
    
    for (auto _ : state) {
        BenchmarkVM vm;
        auto* support = vm.GetCoroutineSupport();
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        
        std::vector<LuaValue> coroutines;
        coroutines.reserve(count);
        
        for (Size i = 0; i < count; ++i) {
            coroutines.push_back(support->CreateCoroutine(func));
        }
        
        benchmark::DoNotOptimize(coroutines);
    }
    
    state.SetComplexityN(count);
}
BENCHMARK(BM_Coroutine_MassCreation)->Range(8, 256)->Complexity(benchmark::oN);

static void BM_Coroutine_SchedulerOperations(benchmark::State& state) {
    BenchmarkVM vm;
    auto* support = vm.GetCoroutineSupport();
    
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    
    // 创建协程
    std::vector<LuaValue> coroutines;
    for (int i = 0; i < 50; ++i) {
        coroutines.push_back(support->CreateCoroutine(func));
    }
    
    auto& scheduler = support->GetScheduler();
    
    for (auto _ : state) {
        // 执行调度器操作
        scheduler.GetActiveCoroutineCount();
        scheduler.ValidateIntegrity();
        auto stats = scheduler.GetStats();
        benchmark::DoNotOptimize(stats);
    }
}
BENCHMARK(BM_Coroutine_SchedulerOperations);

/* ========================================================================== */
/* 集成性能基准 */
/* ========================================================================== */

static void BM_Integration_ComplexScenario(benchmark::State& state) {
    const Size complexity = state.range(0);
    
    for (auto _ : state) {
        BenchmarkVM vm;
        auto* call_stack = vm.GetCallStack();
        auto* upvalue_manager = vm.GetUpvalueManager();
        auto* coroutine_support = vm.GetCoroutineSupport();
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        LuaStack stack(complexity * 10);
        
        // 复杂场景：混合使用所有组件
        
        // 1. 建立主线程调用栈
        std::vector<LuaValue> args;
        for (Size i = 0; i < complexity / 4; ++i) {
            call_stack->PushFrame(func, args, 0);
        }
        
        // 2. 创建Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (Size i = 0; i < complexity / 2; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(upvalue_manager->CreateUpvalue(&stack, i));
        }
        
        // 3. 创建协程
        std::vector<LuaValue> coroutines;
        for (Size i = 0; i < complexity / 8; ++i) {
            coroutines.push_back(coroutine_support->CreateCoroutine(func));
        }
        
        // 4. 执行一些尾调用优化
        for (Size i = 0; i < complexity / 4; ++i) {
            call_stack->PushTailCall(func, args, 0);
        }
        
        // 5. 清理
        upvalue_manager->CloseUpvalues(&stack, 0);
        
        for (Size i = 0; i < complexity / 4; ++i) {
            std::vector<LuaValue> result;
            call_stack->PopFrame(result);
        }
        
        coroutine_support->Cleanup();
        
        benchmark::DoNotOptimize(upvalues);
        benchmark::DoNotOptimize(coroutines);
    }
    
    state.SetComplexityN(complexity);
}
BENCHMARK(BM_Integration_ComplexScenario)->Range(8, 128)->Complexity(benchmark::oN);

static void BM_Integration_MemoryPressure(benchmark::State& state) {
    for (auto _ : state) {
        BenchmarkVM vm;
        auto* call_stack = vm.GetCallStack();
        auto* upvalue_manager = vm.GetUpvalueManager();
        auto* coroutine_support = vm.GetCoroutineSupport();
        
        Proto proto;
        LuaValue func = LuaValue::Function(&proto);
        LuaStack stack(10000);
        
        // 创建大量对象测试内存压力
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        std::vector<LuaValue> coroutines;
        
        for (int i = 0; i < 1000; ++i) {
            // 交替创建不同类型的对象
            if (i % 3 == 0) {
                stack.Push(LuaValue::Number(i));
                upvalues.push_back(upvalue_manager->CreateUpvalue(&stack, 
                    stack.GetSize() - 1));
            } else if (i % 3 == 1) {
                coroutines.push_back(coroutine_support->CreateCoroutine(func));
            } else {
                std::vector<LuaValue> args = {LuaValue::Number(i)};
                call_stack->PushFrame(func, args, 0);
                
                std::vector<LuaValue> result;
                call_stack->PopFrame(result);
            }
        }
        
        // 测量清理性能
        auto cleanup_start = std::chrono::high_resolution_clock::now();
        
        upvalue_manager->CloseUpvalues(&stack, 0);
        coroutine_support->Cleanup();
        
        auto cleanup_end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(upvalues);
        benchmark::DoNotOptimize(coroutines);
    }
}
BENCHMARK(BM_Integration_MemoryPressure);

/* ========================================================================== */
/* 基准测试配置 */
/* ========================================================================== */

// 自定义计数器
static void BM_ThroughputTest(benchmark::State& state) {
    AdvancedCallStack stack(1000);
    Proto proto;
    LuaValue func = LuaValue::Function(&proto);
    std::vector<LuaValue> args;
    std::vector<LuaValue> result;
    
    Size operations = 0;
    
    for (auto _ : state) {
        stack.PushFrame(func, args, 0);
        stack.PopFrame(result);
        operations++;
    }
    
    state.SetItemsProcessed(operations);
    state.counters["ops/sec"] = benchmark::Counter(operations, benchmark::Counter::kIsRate);
}
BENCHMARK(BM_ThroughputTest);

} // namespace lua_cpp

// 基准测试主函数
BENCHMARK_MAIN();