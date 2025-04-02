#include "vm/state.hpp"
#include "vm/vm.hpp"
#include "lib/lib.hpp"
#include <iostream>
#include <string>

using namespace Lua;

// 简单测试函数，测试Lua状态和标准库
void runTest() {
    try {
        // 创建Lua状态
        auto state = VM::State::create();
        
        // 加载标准库
        state->openLibs();
        
        // 输出一条测试消息
        std::cout << "Lua interpreter initialized successfully!" << std::endl;
        
        // 测试执行简单的Lua代码
        const std::string testCode = R"(
            -- 测试基本库
            print("Hello from Lua!")
            print("Type of 5 is: " .. type(5))
            print("Type of 'text' is: " .. type('text'))
            
            -- 测试数学库
            print("Math.abs(-10) = " .. math.abs(-10))
            print("Math.sin(1) = " .. math.sin(1))
            print("Random number: " .. math.random())
            
            -- 测试字符串库
            local str = "Lua String Test"
            print("String length: " .. string.len(str))
            print("Uppercase: " .. string.upper(str))
            print("Find 'Test': " .. tostring(string.find(str, "Test")))
            
            -- 测试表操作
            local t = {1, 2, 3, name = "table"}
            print("Table output:")
            for k, v in pairs(t) do
                print("  " .. tostring(k) .. ": " .. tostring(v))
            end
        )";
        
        // 执行测试代码
        int result = state->doString(testCode);
        
        // 检查执行结果
        if (result == 0) {
            std::cout << "Test code executed successfully!" << std::endl;
        } else {
            std::cout << "Error executing test code." << std::endl;
        }
        
    } catch (const VM::LuaException& e) {
        std::cerr << "Lua error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Lua Interpreter Test ===" << std::endl;
    runTest();
    std::cout << "=== Test Complete ===" << std::endl;
    return 0;
}
