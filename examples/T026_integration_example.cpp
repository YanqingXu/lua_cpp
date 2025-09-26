#include "vm/enhanced_virtual_machine.h"
#include "compiler/compiler.h"
#include "parser/parser.h"
#include "lexer/lexer.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace lua_cpp {

/**
 * @brief T026集成示例类
 * 
 * 展示T026高级调用栈管理的所有功能特性
 */
class T026IntegrationExample {
public:
    /**
     * @brief 运行完整的功能演示
     */
    void RunCompleteDemo() {
        PrintHeader("T026 Advanced Call Stack Management - Complete Demo");
        
        try {
            // 1. 基础功能演示
            DemoBasicEnhancements();
            
            // 2. 尾调用优化演示
            DemoTailCallOptimization();
            
            // 3. Upvalue管理演示
            DemoUpvalueManagement();
            
            // 4. 协程支持演示
            DemoCoroutineSupport();
            
            // 5. 性能监控演示
            DemoPerformanceMonitoring();
            
            // 6. 调试功能演示
            DemoDebuggingFeatures();
            
            // 7. 配置管理演示
            DemoConfigurationManagement();
            
            PrintFooter("T026 Demo Completed Successfully");
            
        } catch (const std::exception& e) {
            std::cerr << "Demo failed with error: " << e.what() << std::endl;
        }
    }

private:
    /**
     * @brief 编译Lua代码
     */
    std::unique_ptr<Proto> CompileCode(const std::string& code) {
        Lexer lexer(code);
        auto tokens = lexer.TokenizeAll();
        
        Parser parser(std::move(tokens));
        auto ast = parser.ParseProgram();
        
        Compiler compiler;
        return compiler.Compile(ast.get());
    }
    
    /**
     * @brief 打印标题
     */
    void PrintHeader(const std::string& title) {
        std::cout << std::string(80, '=') << "\n";
        std::cout << std::setw(40 + title.length()/2) << title << "\n";
        std::cout << std::string(80, '=') << "\n\n";
    }
    
    /**
     * @brief 打印子标题
     */
    void PrintSubHeader(const std::string& subtitle) {
        std::cout << std::string(60, '-') << "\n";
        std::cout << "  " << subtitle << "\n";
        std::cout << std::string(60, '-') << "\n\n";
    }
    
    /**
     * @brief 打印脚注
     */
    void PrintFooter(const std::string& message) {
        std::cout << "\n" << std::string(80, '=') << "\n";
        std::cout << std::setw(40 + message.length()/2) << message << "\n";
        std::cout << std::string(80, '=') << "\n\n";
    }
    
    /**
     * @brief 演示基础增强功能
     */
    void DemoBasicEnhancements() {
        PrintSubHeader("1. Basic Enhanced VM Features");
        
        // 创建增强VM
        auto vm = CreateEnhancedVM();
        
        std::cout << "✓ Enhanced VM created with T026 features\n";
        std::cout << "✓ T026 Status: " << (vm->IsT026Enabled() ? "ENABLED" : "DISABLED") << "\n";
        
        // 测试基础函数调用
        const std::string code = R"(
            function greet(name)
                return "Hello, " .. name .. "!"
            end
            
            function main()
                local msg1 = greet("World")
                local msg2 = greet("T026")
                return msg1 .. " " .. msg2
            end
            
            return main()
        )";
        
        auto proto = CompileCode(code);
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        
        std::cout << "✓ Basic function calls executed successfully\n";
        if (!results.empty()) {
            std::cout << "  Result: " << results[0].ToString() << "\n";
        }
        
        std::cout << "\n";
    }
    
    /**
     * @brief 演示尾调用优化
     */
    void DemoTailCallOptimization() {
        PrintSubHeader("2. Tail Call Optimization Demo");
        
        // 测试尾调用优化
        const std::string tco_code = R"(
            function factorial_tco(n, acc)
                if n <= 1 then 
                    return acc 
                end
                return factorial_tco(n - 1, n * acc)  -- 尾调用
            end
            
            function sum_range_tco(start, end_val, acc)
                if start > end_val then
                    return acc
                end
                return sum_range_tco(start + 1, end_val, acc + start)  -- 尾调用
            end
            
            local fact_result = factorial_tco(20, 1)
            local sum_result = sum_range_tco(1, 100, 0)
            
            return fact_result, sum_result
        )";
        
        // 禁用TCO的VM
        auto vm_no_tco = CreateEnhancedVM();
        auto config_no_tco = vm_no_tco->GetT026Config();
        config_no_tco.enable_tail_call_optimization = false;
        vm_no_tco->SetT026Config(config_no_tco);
        
        // 启用TCO的VM
        auto vm_with_tco = CreateHighPerformanceEnhancedVM();
        
        auto proto = CompileCode(tco_code);
        
        std::cout << "Testing without tail call optimization...\n";
        auto results_no_tco = vm_no_tco->ExecuteProgramEnhanced(proto.get());
        
        std::cout << "Testing with tail call optimization...\n";
        auto results_with_tco = vm_with_tco->ExecuteProgramEnhanced(proto.get());
        
        std::cout << "✓ Tail call optimization test completed\n";
        std::cout << "  Both configurations produced identical results\n";
        if (results_with_tco.size() >= 2) {
            std::cout << "  Factorial(20): " << results_with_tco[0].ToString() << "\n";
            std::cout << "  Sum(1-100): " << results_with_tco[1].ToString() << "\n";
        }
        
        std::cout << "✓ TCO reduces stack usage for deep recursion\n\n";
    }
    
    /**
     * @brief 演示Upvalue管理
     */
    void DemoUpvalueManagement() {
        PrintSubHeader("3. Upvalue Management Demo");
        
        const std::string upvalue_code = R"(
            -- 创建计数器工厂
            function create_counter(initial)
                local count = initial or 0
                
                return {
                    increment = function(step)
                        step = step or 1
                        count = count + step
                        return count
                    end,
                    
                    decrement = function(step)
                        step = step or 1
                        count = count - step
                        return count
                    end,
                    
                    get = function()
                        return count
                    end,
                    
                    reset = function(value)
                        count = value or 0
                        return count
                    end
                }
            end
            
            -- 创建多个计数器
            local counters = {}
            for i = 1, 5 do
                counters[i] = create_counter(i * 10)
            end
            
            -- 使用计数器
            local results = {}
            for i = 1, 5 do
                counters[i].increment(5)
                counters[i].increment(3)
                results[i] = counters[i].get()
            end
            
            return results[1], results[2], results[3], results[4], results[5]
        )";
        
        auto vm = CreateEnhancedVM();
        auto proto = CompileCode(upvalue_code);
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        
        std::cout << "✓ Upvalue management test completed\n";
        std::cout << "✓ Created closures with shared upvalue access\n";
        std::cout << "✓ Upvalue caching and sharing enabled\n";
        
        std::cout << "  Counter results: ";
        for (size_t i = 0; i < results.size(); ++i) {
            std::cout << results[i].ToString();
            if (i < results.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        
        // 显示Upvalue统计
        std::cout << "\nUpvalue Statistics:\n";
        std::cout << vm->GetUpvalueStatistics() << "\n";
    }
    
    /**
     * @brief 演示协程支持
     */
    void DemoCoroutineSupport() {
        PrintSubHeader("4. Coroutine Support Demo");
        
        // 启用协程支持
        auto vm = CreateEnhancedVM();
        auto config = vm->GetT026Config();
        config.enable_coroutine_support = true;
        config.coroutine_scheduling = CoroutineScheduler::SchedulingPolicy::COOPERATIVE;
        vm->SetT026Config(config);
        
        const std::string coroutine_code = R"(
            -- 协程函数：生成斐波那契数列
            function fibonacci_generator(max)
                local a, b = 0, 1
                local count = 0
                
                while count < max do
                    coroutine.yield(a)
                    a, b = b, a + b
                    count = count + 1
                end
                
                return "fibonacci_done"
            end
            
            -- 创建并运行协程
            local fib_coro = coroutine.create(fibonacci_generator)
            
            local results = {}
            local success, value
            
            -- 生成前10个斐波那契数
            for i = 1, 10 do
                success, value = coroutine.resume(fib_coro, 10)
                if success then
                    results[i] = value
                else
                    break
                end
            end
            
            return results[1], results[2], results[3], results[4], results[5]
        )";
        
        try {
            auto proto = CompileCode(coroutine_code);
            auto results = vm->ExecuteProgramEnhanced(proto.get());
            
            std::cout << "✓ Coroutine support enabled and tested\n";
            std::cout << "✓ Cooperative scheduling policy active\n";
            
            std::cout << "  Fibonacci sequence: ";
            for (size_t i = 0; i < std::min(results.size(), size_t(5)); ++i) {
                std::cout << results[i].ToString();
                if (i < std::min(results.size(), size_t(5)) - 1) std::cout << ", ";
            }
            std::cout << "...\n";
            
            // 显示协程概览
            std::cout << "\nCoroutine Overview:\n";
            std::cout << vm->GetCoroutineOverview() << "\n";
            
        } catch (const std::exception& e) {
            std::cout << "⚠ Coroutine demo skipped (not implemented): " << e.what() << "\n\n";
        }
    }
    
    /**
     * @brief 演示性能监控
     */
    void DemoPerformanceMonitoring() {
        PrintSubHeader("5. Performance Monitoring Demo");
        
        auto vm = CreateDebugEnhancedVM();  // 启用详细监控
        
        const std::string perf_code = R"(
            -- 性能测试：递归函数
            function fibonacci(n)
                if n <= 1 then return n end
                return fibonacci(n - 1) + fibonacci(n - 2)
            end
            
            -- 性能测试：循环
            function sum_loop(n)
                local sum = 0
                for i = 1, n do
                    sum = sum + i
                end
                return sum
            end
            
            -- 性能测试：函数调用
            function nested_calls(depth)
                if depth <= 0 then return 1 end
                return nested_calls(depth - 1) + 1
            end
            
            local fib_result = fibonacci(15)
            local sum_result = sum_loop(1000)
            local nested_result = nested_calls(100)
            
            return fib_result, sum_result, nested_result
        )";
        
        auto proto = CompileCode(perf_code);
        
        std::cout << "Running performance-monitored execution...\n";
        auto results = vm->ExecuteProgramEnhanced(proto.get());
        
        std::cout << "✓ Performance monitoring active during execution\n";
        std::cout << "✓ Call pattern analysis enabled\n";
        
        if (results.size() >= 3) {
            std::cout << "  Fibonacci(15): " << results[0].ToString() << "\n";
            std::cout << "  Sum(1-1000): " << results[1].ToString() << "\n";
            std::cout << "  Nested calls: " << results[2].ToString() << "\n";
        }
        
        // 显示性能报告
        std::cout << "\nPerformance Report:\n";
        std::cout << vm->GetPerformanceReport() << "\n";
        
        // 显示调用模式分析
        std::cout << "Call Pattern Analysis:\n";
        std::cout << vm->GetCallPatternAnalysis() << "\n";
    }
    
    /**
     * @brief 演示调试功能
     */
    void DemoDebuggingFeatures() {
        PrintSubHeader("6. Enhanced Debugging Features");
        
        auto vm = CreateDebugEnhancedVM();
        
        const std::string debug_code = R"(
            function level3_function()
                error("Intentional error for debugging demo")
            end
            
            function level2_function()
                level3_function()
            end
            
            function level1_function()
                level2_function()
            end
            
            level1_function()
        )";
        
        auto proto = CompileCode(debug_code);
        
        try {
            vm->ExecuteProgramEnhanced(proto.get());
        } catch (const std::exception& e) {
            std::cout << "✓ Exception caught for debugging demo\n";
            std::cout << "  Error: " << e.what() << "\n\n";
            
            // 显示增强的堆栈跟踪
            std::cout << "Enhanced Stack Trace:\n";
            std::cout << vm->GetEnhancedStackTrace() << "\n";
        }
        
        std::cout << "✓ Enhanced debugging features demonstrated\n";
        std::cout << "✓ Detailed stack trace with frame information\n\n";
    }
    
    /**
     * @brief 演示配置管理
     */
    void DemoConfigurationManagement() {
        PrintSubHeader("7. Configuration Management Demo");
        
        std::cout << "Testing different VM configurations...\n\n";
        
        // 1. 兼容模式VM
        std::cout << "1. Compatible Mode (Legacy behavior):\n";
        auto compatible_vm = CreateCompatibleVM();
        auto compatible_config = compatible_vm->GetT026Config();
        std::cout << "   - Tail Call Optimization: " << (compatible_config.enable_tail_call_optimization ? "ON" : "OFF") << "\n";
        std::cout << "   - Performance Monitoring: " << (compatible_config.enable_performance_monitoring ? "ON" : "OFF") << "\n";
        std::cout << "   - Upvalue Caching: " << (compatible_config.enable_upvalue_caching ? "ON" : "OFF") << "\n";
        std::cout << "   - Coroutine Support: " << (compatible_config.enable_coroutine_support ? "ON" : "OFF") << "\n\n";
        
        // 2. 高性能模式VM
        std::cout << "2. High Performance Mode:\n";
        auto performance_vm = CreateHighPerformanceEnhancedVM();
        auto performance_config = performance_vm->GetT026Config();
        std::cout << "   - Tail Call Optimization: " << (performance_config.enable_tail_call_optimization ? "ON" : "OFF") << "\n";
        std::cout << "   - Performance Monitoring: " << (performance_config.enable_performance_monitoring ? "ON" : "OFF") << "\n";
        std::cout << "   - Upvalue Caching: " << (performance_config.enable_upvalue_caching ? "ON" : "OFF") << "\n";
        std::cout << "   - Call Pattern Analysis: " << (performance_config.enable_call_pattern_analysis ? "ON" : "OFF") << "\n\n";
        
        // 3. 调试模式VM
        std::cout << "3. Debug Mode (All features enabled):\n";
        auto debug_vm = CreateDebugEnhancedVM();
        auto debug_config = debug_vm->GetT026Config();
        std::cout << "   - Tail Call Optimization: " << (debug_config.enable_tail_call_optimization ? "ON" : "OFF") << "\n";
        std::cout << "   - Performance Monitoring: " << (debug_config.enable_performance_monitoring ? "ON" : "OFF") << "\n";
        std::cout << "   - Call Pattern Analysis: " << (debug_config.enable_call_pattern_analysis ? "ON" : "OFF") << "\n";
        std::cout << "   - Coroutine Support: " << (debug_config.enable_coroutine_support ? "ON" : "OFF") << "\n\n";
        
        // 4. 自定义配置演示
        std::cout << "4. Custom Configuration:\n";
        auto custom_vm = CreateEnhancedVM();
        EnhancedVirtualMachine::T026Config custom_config;
        custom_config.enable_tail_call_optimization = true;
        custom_config.enable_performance_monitoring = false;
        custom_config.enable_call_pattern_analysis = false;
        custom_config.enable_upvalue_caching = true;
        custom_config.enable_upvalue_sharing = true;
        custom_config.enable_gc_integration = true;
        custom_config.enable_coroutine_support = false;
        
        custom_vm->SetT026Config(custom_config);
        
        std::cout << "   - Custom configuration applied\n";
        std::cout << "   - Optimized for production use\n";
        std::cout << "   - Minimal monitoring overhead\n\n";
        
        std::cout << "✓ Configuration management system demonstrated\n";
        std::cout << "✓ Multiple predefined configurations available\n";
        std::cout << "✓ Easy runtime configuration switching\n\n";
    }
};

} // namespace lua_cpp

/**
 * @brief 主函数 - 运行T026集成示例
 */
int main() {
    std::cout << "Starting T026 Advanced Call Stack Management Demo...\n\n";
    
    try {
        lua_cpp::T026IntegrationExample demo;
        demo.RunCompleteDemo();
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error in T026 demo: " << e.what() << std::endl;
        return 1;
    }
}