/**
 * @file lua_cli.cpp
 * @brief Lua C++ 命令行接口
 * @description 提供Lua解释器的命令行接口
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>

#include "core/common.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "compiler/compiler.h"
#include "vm/virtual_machine.h"

using namespace lua_cpp;

/**
 * @brief 显示版本信息
 */
void ShowVersion() {
    std::cout << "Lua C++ " << LUA_CPP_VERSION << std::endl;
    std::cout << "Compatible with " << LUA_VERSION_COMPAT << std::endl;
    std::cout << "Copyright (C) 2025 Lua C++ Project" << std::endl;
}

/**
 * @brief 显示帮助信息
 */
void ShowHelp() {
    std::cout << "Usage: lua_cpp [options] [script [args]]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help     Show this help message" << std::endl;
    std::cout << "  -v, --version  Show version information" << std::endl;
    std::cout << "  -i, --interactive  Enter interactive mode" << std::endl;
    std::cout << "  -c, --compile  Compile script to bytecode" << std::endl;
    std::cout << "  -d, --debug    Enable debug output" << std::endl;
}

/**
 * @brief 执行Lua文件
 */
bool ExecuteFile(const std::string& filename, bool debug_mode = false) {
    try {
        // 读取文件
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot open file '" << filename << "'" << std::endl;
            return false;
        }
        
        std::string source((std::istreambuf_iterator<char>(file)),
                          std::istreambuf_iterator<char>());
        file.close();
        
        if (debug_mode) {
            std::cout << "Read " << source.length() << " characters from '" << filename << "'" << std::endl;
        }
        
        // 词法分析
        Lexer lexer(source);
        auto tokens = lexer.TokenizeAll();
        
        if (debug_mode) {
            std::cout << "Lexical analysis: " << tokens.size() << " tokens" << std::endl;
        }
        
        // 语法分析
        Parser parser(tokens);
        auto ast = parser.Parse();
        
        if (debug_mode) {
            std::cout << "Syntax analysis: AST generated" << std::endl;
        }
        
        // 编译
        Compiler compiler;
        auto chunk = compiler.Compile(ast.get());
        
        if (debug_mode) {
            std::cout << "Compilation: " << chunk->instructions.size() << " instructions" << std::endl;
        }
        
        // 执行
        VirtualMachine vm;
        auto result = vm.ExecuteProgram(chunk.get());
        
        if (debug_mode) {
            std::cout << "Execution completed" << std::endl;
        }
        
        // 如果有返回值，显示它
        if (result.GetType() != LuaValueType::NIL) {
            std::cout << result.ToString() << std::endl;
        }
        
        return true;
        
    } catch (const LexerError& e) {
        std::cerr << "Lexer error: " << e.what() << std::endl;
        return false;
    } catch (const ParserError& e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
        return false;
    } catch (const CompilerError& e) {
        std::cerr << "Compiler error: " << e.what() << std::endl;
        return false;
    } catch (const VMError& e) {
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 执行单行代码
 */
bool ExecuteLine(const std::string& line, bool debug_mode = false) {
    try {
        // 词法分析
        Lexer lexer(line);
        auto tokens = lexer.TokenizeAll();
        
        // 语法分析
        Parser parser(tokens);
        auto ast = parser.Parse();
        
        // 编译
        Compiler compiler;
        auto chunk = compiler.Compile(ast.get());
        
        // 执行
        VirtualMachine vm;
        auto result = vm.ExecuteProgram(chunk.get());
        
        // 显示结果
        if (result.GetType() != LuaValueType::NIL) {
            std::cout << result.ToString() << std::endl;
        }
        
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
}

/**
 * @brief 交互模式
 */
void InteractiveMode(bool debug_mode = false) {
    std::cout << "Lua C++ " << LUA_CPP_VERSION << " Interactive Mode" << std::endl;
    std::cout << "Type 'exit' or press Ctrl+C to quit" << std::endl;
    
    std::string line;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) {
            break; // EOF
        }
        
        if (line == "exit" || line == "quit") {
            break;
        }
        
        if (line.empty()) {
            continue;
        }
        
        ExecuteLine(line, debug_mode);
    }
    
    std::cout << "Goodbye!" << std::endl;
}

/**
 * @brief 主函数
 */
int main(int argc, char* argv[]) {
    std::vector<std::string> args(argv, argv + argc);
    
    bool interactive_mode = false;
    bool debug_mode = false;
    bool compile_mode = false;
    std::string script_file;
    
    // 解析命令行参数
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& arg = args[i];
        
        if (arg == "-h" || arg == "--help") {
            ShowHelp();
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            ShowVersion();
            return 0;
        } else if (arg == "-i" || arg == "--interactive") {
            interactive_mode = true;
        } else if (arg == "-d" || arg == "--debug") {
            debug_mode = true;
        } else if (arg == "-c" || arg == "--compile") {
            compile_mode = true;
        } else if (arg[0] != '-') {
            script_file = arg;
            break; // 剩余参数作为脚本参数
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            ShowHelp();
            return 1;
        }
    }
    
    try {
        // 如果指定了脚本文件
        if (!script_file.empty()) {
            bool success = ExecuteFile(script_file, debug_mode);
            return success ? 0 : 1;
        }
        
        // 如果没有参数或指定了交互模式
        if (interactive_mode || argc == 1) {
            InteractiveMode(debug_mode);
            return 0;
        }
        
        // 其他情况显示帮助
        ShowHelp();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
}