/**
 * @file vm_test.cpp
 * @brief 虚拟机集成测试
 * @description 测试完整的编译和执行流水线
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include <iostream>
#include <string>
#include <memory>
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/virtual_machine.h"

using namespace lua_cpp;

void TestSimpleArithmetic() {
    std::cout << "\n=== Testing Simple Arithmetic ===" << std::endl;
    
    std::string source = R"(
        local a = 10
        local b = 20
        local c = a + b
        return c
    )";
    
    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.TokenizeAll();
        std::cout << "Lexical analysis: " << tokens.size() << " tokens generated" << std::endl;
        
        // 语法分析
        Parser parser(tokens);
        auto ast = parser.Parse();
        std::cout << "Syntax analysis: AST generated successfully" << std::endl;
        
        // 编译
        Compiler compiler;
        auto chunk = compiler.Compile(ast.get());
        std::cout << "Compilation: " << chunk->instructions.size() << " instructions generated" << std::endl;
        
        // 执行
        VirtualMachine vm;
        auto result = vm.ExecuteProgram(chunk.get());
        std::cout << "Execution result: " << result.ToString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void TestFunctionCall() {
    std::cout << "\n=== Testing Function Call ===" << std::endl;
    
    std::string source = R"(
        function add(x, y)
            return x + y
        end
        
        local result = add(15, 25)
        return result
    )";
    
    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.TokenizeAll();
        std::cout << "Lexical analysis: " << tokens.size() << " tokens generated" << std::endl;
        
        // 语法分析
        Parser parser(tokens);
        auto ast = parser.Parse();
        std::cout << "Syntax analysis: AST generated successfully" << std::endl;
        
        // 编译
        Compiler compiler;
        auto chunk = compiler.Compile(ast.get());
        std::cout << "Compilation: " << chunk->instructions.size() << " instructions generated" << std::endl;
        
        // 执行
        VirtualMachine vm;
        auto result = vm.ExecuteProgram(chunk.get());
        std::cout << "Execution result: " << result.ToString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void TestControlFlow() {
    std::cout << "\n=== Testing Control Flow ===" << std::endl;
    
    std::string source = R"(
        local x = 10
        if x > 5 then
            x = x * 2
        else
            x = x + 1
        end
        return x
    )";
    
    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.TokenizeAll();
        std::cout << "Lexical analysis: " << tokens.size() << " tokens generated" << std::endl;
        
        // 语法分析
        Parser parser(tokens);
        auto ast = parser.Parse();
        std::cout << "Syntax analysis: AST generated successfully" << std::endl;
        
        // 编译
        Compiler compiler;
        auto chunk = compiler.Compile(ast.get());
        std::cout << "Compilation: " << chunk->instructions.size() << " instructions generated" << std::endl;
        
        // 执行
        VirtualMachine vm;
        auto result = vm.ExecuteProgram(chunk.get());
        std::cout << "Execution result: " << result.ToString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void TestLoop() {
    std::cout << "\n=== Testing Loop ===" << std::endl;
    
    std::string source = R"(
        local sum = 0
        local i = 1
        while i <= 5 do
            sum = sum + i
            i = i + 1
        end
        return sum
    )";
    
    try {
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.TokenizeAll();
        std::cout << "Lexical analysis: " << tokens.size() << " tokens generated" << std::endl;
        
        // 语法分析
        Parser parser(tokens);
        auto ast = parser.Parse();
        std::cout << "Syntax analysis: AST generated successfully" << std::endl;
        
        // 编译
        Compiler compiler;
        auto chunk = compiler.Compile(ast.get());
        std::cout << "Compilation: " << chunk->instructions.size() << " instructions generated" << std::endl;
        
        // 执行
        VirtualMachine vm;
        auto result = vm.ExecuteProgram(chunk.get());
        std::cout << "Execution result: " << result.ToString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

void TestStackOperations() {
    std::cout << "\n=== Testing Stack Operations ===" << std::endl;
    
    try {
        LuaStack stack(10, 100);
        
        // 测试基本推入和弹出
        stack.Push(LuaValue(42));
        stack.Push(LuaValue("hello"));
        stack.Push(LuaValue(3.14));
        
        std::cout << "Stack after pushes:" << std::endl;
        stack.Dump();
        
        auto val1 = stack.Pop();
        auto val2 = stack.Pop();
        auto val3 = stack.Pop();
        
        std::cout << "Popped values: " << val1.ToString() << ", " 
                  << val2.ToString() << ", " << val3.ToString() << std::endl;
        
        // 测试Lua式索引
        stack.Push(LuaValue(1));
        stack.Push(LuaValue(2));
        stack.Push(LuaValue(3));
        
        std::cout << "Lua index 1: " << stack.GetLuaIndex(1).ToString() << std::endl;
        std::cout << "Lua index -1: " << stack.GetLuaIndex(-1).ToString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Stack test error: " << e.what() << std::endl;
    }
}

void TestCallStack() {
    std::cout << "\n=== Testing Call Stack ===" << std::endl;
    
    try {
        CallStack call_stack(100);
        
        // 创建测试函数
        LuaFunction func1;
        func1.debug_name = "main";
        func1.source = "test.lua";
        func1.line_defined = 1;
        func1.last_line_defined = 10;
        func1.max_stack_size = 5;
        
        LuaFunction func2;
        func2.debug_name = "add";
        func2.source = "test.lua";
        func2.line_defined = 5;
        func2.last_line_defined = 8;
        func2.max_stack_size = 3;
        
        // 推入调用帧
        CallFrame frame1(&func1, 0, 1);
        CallFrame frame2(&func2, 5, 1);
        
        call_stack.PushFrame(std::move(frame1));
        call_stack.PushFrame(std::move(frame2));
        
        std::cout << "Call stack depth: " << call_stack.Depth() << std::endl;
        std::cout << "Current frame: " << call_stack.CurrentFrame().ToString() << std::endl;
        
        call_stack.DumpStackTrace();
        
        // 弹出调用帧
        auto popped = call_stack.PopFrame();
        std::cout << "Popped frame: " << popped.ToString() << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Call stack test error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "Lua C++ Virtual Machine Integration Test" << std::endl;
    std::cout << "=========================================" << std::endl;
    
    // 测试VM组件
    TestStackOperations();
    TestCallStack();
    
    // 测试完整流水线
    TestSimpleArithmetic();
    TestFunctionCall();
    TestControlFlow();
    TestLoop();
    
    std::cout << "\n=== All Tests Completed ===" << std::endl;
    
    return 0;
}