#include <benchmark/benchmark.h>
#include "vm/enhanced_virtual_machine.h"
#include "vm/virtual_machine.h"
#include "compiler/compiler.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include <vector>
#include <string>
#include <memory>

namespace lua_cpp {
namespace benchmarks {

/* ========================================================================== */
/* 测试数据生成 */
/* ========================================================================== */

/**
 * @brief 生成简单函数调用的Lua代码
 */
std::string GenerateFunctionCallCode(size_t depth) {
    std::string code = "function f1() return 1 end\n";
    
    for (size_t i = 2; i <= depth; ++i) {
        code += "function f" + std::to_string(i) + "()\n";
        code += "    return f" + std::to_string(i-1) + "()\n";
        code += "end\n";
    }
    
    code += "return f" + std::to_string(depth) + "()\n";
    return code;
}

/**
 * @brief 生成尾调用优化的Lua代码
 */
std::string GenerateTailCallCode(size_t depth) {
    std::string code = "function factorial(n, acc)\n";
    code += "    if n <= 1 then return acc end\n";
    code += "    return factorial(n - 1, n * acc)\n";
    code += "end\n";
    code += "return factorial(" + std::to_string(depth) + ", 1)\n";
    return code;
}

/**
 * @brief 生成闭包测试的Lua代码
 */
std::string GenerateClosureCode(size_t count) {
    std::string code = "local closures = {}\n";
    
    for (size_t i = 1; i <= count; ++i) {
        code += "local x" + std::to_string(i) + " = " + std::to_string(i) + "\n";
        code += "closures[" + std::to_string(i) + "] = function() return x" + std::to_string(i) + " end\n";
    }
    
    code += "local sum = 0\n";
    code += "for i = 1, " + std::to_string(count) + " do\n";
    code += "    sum = sum + closures[i]()\n";
    code += "end\n";
    code += "return sum\n";
    
    return code;
}

/**
 * @brief 编译Lua代码到字节码
 */
std::unique_ptr<Proto> CompileCode(const std::string& code) {
    Lexer lexer(code);
    auto tokens = lexer.TokenizeAll();
    
    Parser parser(std::move(tokens));
    auto ast = parser.ParseProgram();
    
    Compiler compiler;
    return compiler.Compile(ast.get());
}

/* ========================================================================== */
/* VM性能基准测试 */
/* ========================================================================== */

/**
 * @brief 传统VM函数调用基准测试
 */
static void BM_LegacyVM_FunctionCalls(benchmark::State& state) {
    size_t call_depth = state.range(0);
    auto code = GenerateFunctionCallCode(call_depth);
    auto proto = CompileCode(code);
    
    for (auto _ : state) {
        VirtualMachine vm;
        auto results = vm.ExecuteProgram(proto.get());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * call_depth);
}

/**
 * @brief 增强VM函数调用基准测试
 */
static void BM_EnhancedVM_FunctionCalls(benchmark::State& state) {
    size_t call_depth = state.range(0);
    auto code = GenerateFunctionCallCode(call_depth);
    auto proto = CompileCode(code);
    
    auto vm = CreateEnhancedVM();
    
    for (auto _ : state) {
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * call_depth);
}

/**
 * @brief 尾调用优化基准测试
 */
static void BM_EnhancedVM_TailCallOptimization(benchmark::State& state) {
    size_t recursion_depth = state.range(0);
    auto code = GenerateTailCallCode(recursion_depth);
    auto proto = CompileCode(code);
    
    auto vm = CreateHighPerformanceEnhancedVM();
    
    for (auto _ : state) {
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * recursion_depth);
}

/**
 * @brief 闭包性能基准测试
 */
static void BM_EnhancedVM_Closures(benchmark::State& state) {
    size_t closure_count = state.range(0);
    auto code = GenerateClosureCode(closure_count);
    auto proto = CompileCode(code);
    
    auto vm = CreateEnhancedVM();
    
    for (auto _ : state) {
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * closure_count);
}

/**
 * @brief 传统VM闭包性能基准测试
 */
static void BM_LegacyVM_Closures(benchmark::State& state) {
    size_t closure_count = state.range(0);
    auto code = GenerateClosureCode(closure_count);
    auto proto = CompileCode(code);
    
    for (auto _ : state) {
        VirtualMachine vm;
        auto results = vm.ExecuteProgram(proto.get());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * closure_count);
}

/* ========================================================================== */
/* T026特性基准测试 */
/* ========================================================================== */

/**
 * @brief 调用栈深度性能测试
 */
static void BM_CallStackDepth(benchmark::State& state) {
    size_t depth = state.range(0);
    auto code = GenerateFunctionCallCode(depth);
    auto proto = CompileCode(code);
    
    auto vm = CreateEnhancedVM();
    
    for (auto _ : state) {
        auto start = std::chrono::high_resolution_clock::now();
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        auto end = std::chrono::high_resolution_clock::now();
        
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations());
    state.SetComplexityN(depth);
}

/**
 * @brief Upvalue管理性能测试
 */
static void BM_UpvalueManagement(benchmark::State& state) {
    size_t upvalue_count = state.range(0);
    
    // 生成有大量Upvalue的代码
    std::string code = "local function create_closures()\n";
    code += "    local upvalues = {}\n";
    
    for (size_t i = 1; i <= upvalue_count; ++i) {
        code += "    local x" + std::to_string(i) + " = " + std::to_string(i) + "\n";
        code += "    upvalues[" + std::to_string(i) + "] = function() return x" + std::to_string(i) + " end\n";
    }
    
    code += "    return upvalues\n";
    code += "end\n";
    code += "local closures = create_closures()\n";
    code += "return #closures\n";
    
    auto proto = CompileCode(code);
    auto vm = CreateEnhancedVM();
    
    for (auto _ : state) {
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        benchmark::DoNotOptimize(results);
    }
    
    state.SetItemsProcessed(state.iterations() * upvalue_count);
}

/**
 * @brief 性能监控开销测试
 */
static void BM_PerformanceMonitoringOverhead(benchmark::State& state) {
    auto code = GenerateFunctionCallCode(100);
    auto proto = CompileCode(code);
    
    // 启用性能监控的VM
    auto vm_with_monitoring = CreateEnhancedVM();
    
    // 禁用性能监控的VM
    auto vm_without_monitoring = CreateHighPerformanceEnhancedVM();
    auto config = vm_without_monitoring->GetT026Config();
    config.enable_performance_monitoring = false;
    vm_without_monitoring->SetT026Config(config);
    
    bool with_monitoring = state.range(0);
    
    for (auto _ : state) {
        if (with_monitoring) {
            auto results = vm_with_monitoring->ExecuteProgramEnhanced(proto.get());
            benchmark::DoNotOptimize(results);
        } else {
            auto results = vm_without_monitoring->ExecuteProgramEnhanced(proto.get());
            benchmark::DoNotOptimize(results);
        }
    }
    
    state.SetLabel(with_monitoring ? "WithMonitoring" : "WithoutMonitoring");
}

/* ========================================================================== */
/* 内存使用基准测试 */
/* ========================================================================== */

/**
 * @brief 内存分配性能测试
 */
static void BM_MemoryAllocation(benchmark::State& state) {
    size_t allocation_count = state.range(0);
    
    for (auto _ : state) {
        auto vm = CreateEnhancedVM();
        
        // 分配多个对象
        std::vector<LuaValue> values;
        values.reserve(allocation_count);
        
        for (size_t i = 0; i < allocation_count; ++i) {
            values.emplace_back(static_cast<double>(i));
        }
        
        benchmark::DoNotOptimize(values);
    }
    
    state.SetItemsProcessed(state.iterations() * allocation_count);
}

/* ========================================================================== */
/* 基准测试注册 */
/* ========================================================================== */

// 函数调用性能对比
BENCHMARK(BM_LegacyVM_FunctionCalls)
    ->Range(8, 512)
    ->Complexity(benchmark::oN);

BENCHMARK(BM_EnhancedVM_FunctionCalls)
    ->Range(8, 512)
    ->Complexity(benchmark::oN);

// 尾调用优化
BENCHMARK(BM_EnhancedVM_TailCallOptimization)
    ->Range(100, 10000)
    ->Complexity(benchmark::oN);

// 闭包性能对比
BENCHMARK(BM_LegacyVM_Closures)
    ->Range(8, 128)
    ->Complexity(benchmark::oN);

BENCHMARK(BM_EnhancedVM_Closures)
    ->Range(8, 128)
    ->Complexity(benchmark::oN);

// T026特性测试
BENCHMARK(BM_CallStackDepth)
    ->Range(10, 1000)
    ->Complexity(benchmark::oN);

BENCHMARK(BM_UpvalueManagement)
    ->Range(10, 500)
    ->Complexity(benchmark::oN);

BENCHMARK(BM_PerformanceMonitoringOverhead)
    ->Arg(0)  // 不启用监控
    ->Arg(1); // 启用监控

BENCHMARK(BM_MemoryAllocation)
    ->Range(64, 4096)
    ->Complexity(benchmark::oN);

} // namespace benchmarks
} // namespace lua_cpp

// 主函数
BENCHMARK_MAIN();