#include "vm/enhanced_virtual_machine.h"
#include "compiler/compiler.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <sstream>
#include <iomanip>

namespace lua_cpp {

/**
 * @brief T026性能分析器
 * 
 * 提供详细的T026功能性能分析和优化建议
 */
class T026PerformanceAnalyzer {
public:
    /**
     * @brief 性能测试结果
     */
    struct PerformanceResult {
        std::string test_name;
        double legacy_time_ms;
        double enhanced_time_ms;
        double improvement_percent;
        size_t iterations;
        std::string analysis;
    };
    
    /**
     * @brief 运行完整的性能分析
     * @return 分析报告
     */
    std::string RunCompleteAnalysis() {
        std::vector<PerformanceResult> results;
        
        // 基础函数调用测试
        results.push_back(TestFunctionCalls());
        
        // 尾调用优化测试
        results.push_back(TestTailCallOptimization());
        
        // 闭包性能测试
        results.push_back(TestClosurePerformance());
        
        // 深度递归测试
        results.push_back(TestDeepRecursion());
        
        // Upvalue管理测试
        results.push_back(TestUpvalueManagement());
        
        // 生成综合报告
        return GenerateReport(results);
    }
    
private:
    /**
     * @brief 编译Lua代码
     */
    std::unique_ptr<Proto> CompileCode(const std::string& code) {
        try {
            Lexer lexer(code);
            auto tokens = lexer.TokenizeAll();
            
            Parser parser(std::move(tokens));
            auto ast = parser.ParseProgram();
            
            Compiler compiler;
            return compiler.Compile(ast.get());
        } catch (...) {
            return nullptr;
        }
    }
    
    /**
     * @brief 运行性能测试
     */
    template<typename VMType>
    double RunPerformanceTest(VMType& vm, const Proto* proto, size_t iterations) {
        auto start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < iterations; ++i) {
            if constexpr (std::is_same_v<VMType, VirtualMachine>) {
                vm.ExecuteProgram(proto);
            } else {
                vm.ExecuteProgramEnhanced(proto);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        return duration.count() / 1000.0; // 转换为毫秒
    }
    
    /**
     * @brief 测试基础函数调用性能
     */
    PerformanceResult TestFunctionCalls() {
        const std::string code = R"(
            function fib(n)
                if n <= 1 then return n end
                return fib(n-1) + fib(n-2)
            end
            
            local result = 0
            for i = 1, 20 do
                result = result + fib(10)
            end
            return result
        )";
        
        auto proto = CompileCode(code);
        if (!proto) {
            return {"Function Calls", 0, 0, 0, 0, "Failed to compile test code"};
        }
        
        const size_t iterations = 100;
        
        // 传统VM测试
        VirtualMachine legacy_vm;
        double legacy_time = RunPerformanceTest(legacy_vm, proto.get(), iterations);
        
        // 增强VM测试
        auto enhanced_vm = CreateHighPerformanceEnhancedVM();
        double enhanced_time = RunPerformanceTest(*enhanced_vm, proto.get(), iterations);
        
        double improvement = ((legacy_time - enhanced_time) / legacy_time) * 100;
        
        std::string analysis = "Basic function call performance with optimized call stack";
        if (improvement > 5) {
            analysis += " - Significant improvement detected";
        } else if (improvement < -5) {
            analysis += " - Performance regression detected";
        } else {
            analysis += " - Similar performance";
        }
        
        return {"Function Calls", legacy_time, enhanced_time, improvement, iterations, analysis};
    }
    
    /**
     * @brief 测试尾调用优化性能
     */
    PerformanceResult TestTailCallOptimization() {
        const std::string code = R"(
            function factorial(n, acc)
                if n <= 1 then return acc end
                return factorial(n - 1, n * acc)
            end
            
            local result = 0
            for i = 1, 10 do
                result = result + factorial(100, 1)
            end
            return result
        )";
        
        auto proto = CompileCode(code);
        if (!proto) {
            return {"Tail Call Optimization", 0, 0, 0, 0, "Failed to compile test code"};
        }
        
        const size_t iterations = 50;
        
        // 禁用尾调用优化的VM
        auto vm_no_tco = CreateEnhancedVM();
        auto config = vm_no_tco->GetT026Config();
        config.enable_tail_call_optimization = false;
        vm_no_tco->SetT026Config(config);
        
        double no_tco_time = RunPerformanceTest(*vm_no_tco, proto.get(), iterations);
        
        // 启用尾调用优化的VM
        auto vm_with_tco = CreateHighPerformanceEnhancedVM();
        double with_tco_time = RunPerformanceTest(*vm_with_tco, proto.get(), iterations);
        
        double improvement = ((no_tco_time - with_tco_time) / no_tco_time) * 100;
        
        std::string analysis = "Tail call optimization for recursive functions";
        if (improvement > 10) {
            analysis += " - Excellent TCO performance";
        } else if (improvement > 5) {
            analysis += " - Good TCO performance";
        } else {
            analysis += " - Limited TCO benefit for this workload";
        }
        
        return {"Tail Call Optimization", no_tco_time, with_tco_time, improvement, iterations, analysis};
    }
    
    /**
     * @brief 测试闭包性能
     */
    PerformanceResult TestClosurePerformance() {
        const std::string code = R"(
            local function create_counter()
                local count = 0
                return function()
                    count = count + 1
                    return count
                end
            end
            
            local counters = {}
            for i = 1, 100 do
                counters[i] = create_counter()
            end
            
            local total = 0
            for i = 1, 100 do
                for j = 1, 10 do
                    total = total + counters[i]()
                end
            end
            
            return total
        )";
        
        auto proto = CompileCode(code);
        if (!proto) {
            return {"Closure Performance", 0, 0, 0, 0, "Failed to compile test code"};
        }
        
        const size_t iterations = 20;
        
        // 传统VM测试
        VirtualMachine legacy_vm;
        double legacy_time = RunPerformanceTest(legacy_vm, proto.get(), iterations);
        
        // 增强VM测试
        auto enhanced_vm = CreateEnhancedVM();
        double enhanced_time = RunPerformanceTest(*enhanced_vm, proto.get(), iterations);
        
        double improvement = ((legacy_time - enhanced_time) / legacy_time) * 100;
        
        std::string analysis = "Closure creation and upvalue management performance";
        if (improvement > 15) {
            analysis += " - Excellent upvalue optimization";
        } else if (improvement > 5) {
            analysis += " - Good upvalue management";
        } else {
            analysis += " - Standard closure performance";
        }
        
        return {"Closure Performance", legacy_time, enhanced_time, improvement, iterations, analysis};
    }
    
    /**
     * @brief 测试深度递归性能
     */
    PerformanceResult TestDeepRecursion() {
        const std::string code = R"(
            function deep_recursion(n, acc)
                if n <= 0 then return acc end
                return deep_recursion(n - 1, acc + n)
            end
            
            local result = 0
            for i = 1, 5 do
                result = result + deep_recursion(1000, 0)
            end
            return result
        )";
        
        auto proto = CompileCode(code);
        if (!proto) {
            return {"Deep Recursion", 0, 0, 0, 0, "Failed to compile test code"};
        }
        
        const size_t iterations = 10;
        
        // 传统VM测试
        VirtualMachine legacy_vm;
        double legacy_time = RunPerformanceTest(legacy_vm, proto.get(), iterations);
        
        // 增强VM测试
        auto enhanced_vm = CreateHighPerformanceEnhancedVM();
        double enhanced_time = RunPerformanceTest(*enhanced_vm, proto.get(), iterations);
        
        double improvement = ((legacy_time - enhanced_time) / legacy_time) * 100;
        
        std::string analysis = "Deep recursion with advanced call stack management";
        if (improvement > 20) {
            analysis += " - Excellent call stack optimization";
        } else if (improvement > 10) {
            analysis += " - Good call stack performance";
        } else {
            analysis += " - Standard recursion handling";
        }
        
        return {"Deep Recursion", legacy_time, enhanced_time, improvement, iterations, analysis};
    }
    
    /**
     * @brief 测试Upvalue管理性能
     */
    PerformanceResult TestUpvalueManagement() {
        const std::string code = R"(
            local function create_nested_closures(depth)
                if depth <= 0 then
                    return function() return depth end
                else
                    local inner = create_nested_closures(depth - 1)
                    return function()
                        return depth + inner()
                    end
                end
            end
            
            local closures = {}
            for i = 1, 50 do
                closures[i] = create_nested_closures(10)
            end
            
            local total = 0
            for i = 1, 50 do
                total = total + closures[i]()
            end
            
            return total
        )";
        
        auto proto = CompileCode(code);
        if (!proto) {
            return {"Upvalue Management", 0, 0, 0, 0, "Failed to compile test code"};
        }
        
        const size_t iterations = 10;
        
        // 禁用Upvalue优化的VM
        auto vm_no_upvalue_opt = CreateEnhancedVM();
        auto config = vm_no_upvalue_opt->GetT026Config();
        config.enable_upvalue_caching = false;
        config.enable_upvalue_sharing = false;
        vm_no_upvalue_opt->SetT026Config(config);
        
        double no_opt_time = RunPerformanceTest(*vm_no_upvalue_opt, proto.get(), iterations);
        
        // 启用Upvalue优化的VM
        auto vm_with_upvalue_opt = CreateEnhancedVM();
        double with_opt_time = RunPerformanceTest(*vm_with_upvalue_opt, proto.get(), iterations);
        
        double improvement = ((no_opt_time - with_opt_time) / no_opt_time) * 100;
        
        std::string analysis = "Nested closures with upvalue optimization";
        if (improvement > 25) {
            analysis += " - Excellent upvalue caching";
        } else if (improvement > 10) {
            analysis += " - Good upvalue management";
        } else {
            analysis += " - Limited upvalue optimization benefit";
        }
        
        return {"Upvalue Management", no_opt_time, with_opt_time, improvement, iterations, analysis};
    }
    
    /**
     * @brief 生成性能分析报告
     */
    std::string GenerateReport(const std::vector<PerformanceResult>& results) {
        std::ostringstream oss;
        
        oss << "========================================\n";
        oss << "       T026 Performance Analysis       \n";
        oss << "========================================\n\n";
        
        // 总体统计
        double total_improvement = 0;
        size_t valid_tests = 0;
        
        for (const auto& result : results) {
            if (result.legacy_time_ms > 0 && result.enhanced_time_ms > 0) {
                total_improvement += result.improvement_percent;
                valid_tests++;
            }
        }
        
        if (valid_tests > 0) {
            double avg_improvement = total_improvement / valid_tests;
            oss << "Average Performance Improvement: " 
                << std::fixed << std::setprecision(1) << avg_improvement << "%\n\n";
        }
        
        // 详细结果
        oss << "Detailed Results:\n";
        oss << std::string(80, '-') << "\n";
        oss << std::left << std::setw(25) << "Test Name"
            << std::setw(12) << "Legacy (ms)"
            << std::setw(12) << "Enhanced (ms)"
            << std::setw(12) << "Improvement"
            << "Iterations\n";
        oss << std::string(80, '-') << "\n";
        
        for (const auto& result : results) {
            oss << std::left << std::setw(25) << result.test_name
                << std::setw(12) << std::fixed << std::setprecision(2) << result.legacy_time_ms
                << std::setw(12) << std::fixed << std::setprecision(2) << result.enhanced_time_ms
                << std::setw(12) << std::fixed << std::setprecision(1) << result.improvement_percent << "%"
                << result.iterations << "\n";
        }
        
        oss << std::string(80, '-') << "\n\n";
        
        // 分析和建议
        oss << "Analysis & Recommendations:\n";
        oss << std::string(40, '=') << "\n";
        
        for (const auto& result : results) {
            oss << "• " << result.test_name << ": " << result.analysis << "\n";
        }
        
        oss << "\nOptimization Recommendations:\n";
        oss << std::string(40, '-') << "\n";
        
        double best_improvement = 0;
        std::string best_feature;
        
        for (const auto& result : results) {
            if (result.improvement_percent > best_improvement) {
                best_improvement = result.improvement_percent;
                best_feature = result.test_name;
            }
        }
        
        if (best_improvement > 10) {
            oss << "✓ Best performing feature: " << best_feature 
                << " (" << std::fixed << std::setprecision(1) << best_improvement << "% improvement)\n";
        }
        
        oss << "✓ Enable tail call optimization for recursive functions\n";
        oss << "✓ Use upvalue caching for closure-heavy applications\n";
        oss << "✓ Enable performance monitoring for production debugging\n";
        oss << "✓ Consider coroutine support for cooperative multitasking\n";
        
        oss << "\nT026 Status: ";
        if (total_improvement > 0) {
            oss << "PERFORMANCE ENHANCED - Ready for production\n";
        } else {
            oss << "BASELINE PERFORMANCE - Consider workload-specific tuning\n";
        }
        
        return oss.str();
    }
};

} // namespace lua_cpp

/**
 * @brief 主函数 - 运行T026性能分析
 */
int main() {
    try {
        lua_cpp::T026PerformanceAnalyzer analyzer;
        std::string report = analyzer.RunCompleteAnalysis();
        
        std::cout << report << std::endl;
        
        // 保存报告到文件
        std::ofstream file("T026_performance_report.txt");
        if (file.is_open()) {
            file << report;
            file.close();
            std::cout << "\nReport saved to: T026_performance_report.txt\n";
        }
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}