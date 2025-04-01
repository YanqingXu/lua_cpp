#include "lua/lua.hpp"
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

void printHelp() {
    std::cout << "lua interpreter\n"
              << "Usage:\n"
              << "  lua [options] [script [args]]\n\n"
              << "Available options are:\n"
              << "  -e stat  execute string 'stat'\n"
              << "  -i       enter interactive mode after executing 'script'\n"
              << "  -v       show version information\n"
              << "  --       stop handling options\n"
              << "  -        stop handling options and execute stdin\n";
}

void printVersion() {
    std::cout << "lua 0.1.0\n";
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return "";
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void executeScript(const std::string& script, lua::State* L) {
    if (L->doString(script) != 0) {
        std::cerr << "Error: " << L->toString(-1) << std::endl;
        L->pop(1);  // 清除错误消息
    }
}

void runInteractive(lua::State* L) {
    std::string line;
    std::cout << "> ";
    
    while (std::getline(std::cin, line)) {
        if (line == "exit" || line == "quit") {
            break;
        }
        
        if (L->doString(line) != 0) {
            std::cerr << "Error: " << L->toString(-1) << std::endl;
            L->pop(1);  // 清除错误消息
        }
        
        std::cout << "> ";
    }
}

int main(int argc, char* argv[]) {
    // 创建Lua状态
    lua::State L;
    
    // 初始化标准库
    L.openLibs();
    
    // 解析命令行参数
    bool interactive = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg[0] == '-') {
            if (arg == "-e" && i + 1 < argc) {
                // 执行命令行中的代码
                executeScript(argv[i+1], &L);
                ++i;  // 跳过下一个参数
            } else if (arg == "-i") {
                // 设置交互模式
                interactive = true;
            } else if (arg == "-v") {
                // 显示版本信息
                printVersion();
                return 0;
            } else if (arg == "--") {
                // 停止处理选项
                ++i;
                break;
            } else if (arg == "-") {
                // 从标准输入读取
                std::string script;
                std::string line;
                while (std::getline(std::cin, line)) {
                    script += line + "\n";
                }
                executeScript(script, &L);
                return 0;
            } else {
                std::cerr << "Error: Unknown option " << arg << std::endl;
                printHelp();
                return 1;
            }
        } else {
            // 执行文件
            std::string script = readFile(arg);
            if (script.empty()) {
                return 1;
            }
            
            executeScript(script, &L);
            
            // 设置脚本参数
            L.createTable(0, 0);
            for (int j = i + 1; j < argc; ++j) {
                L.pushString(argv[j]);
                L.rawSetI(-2, j - i);
            }
            L.setGlobal("arg");
            
            break;
        }
    }
    
    // 交互模式
    if (interactive) {
        printVersion();
        runInteractive(&L);
    }
    
    return 0;
}
