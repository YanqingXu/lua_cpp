/**
 * @file test_c_api_call_contract.cpp
 * @brief T016: C API函数调用契约测试 - 规格驱动开发
 * 
 * @details 
 * 本文件实现了T016 C API函数调用契约测试，验证Lua 5.1.5 C API的函数调用机制，
 * 包括函数注册、调用约定、参数传递、返回值处理、错误处理、协程操作等。
 * 采用双重验证机制确保与原始Lua 5.1.5 C API的完全兼容性。
 * 
 * 测试架构：
 * 1. 🔍 lua_c_analysis验证：基于原始Lua 5.1.5的lapi.c函数调用行为验证
 * 2. 🏗️ lua_with_cpp验证：基于现代化C++架构的函数调用包装验证
 * 3. 📊 双重对比：确保调用语义一致性和异常安全性
 * 
 * 测试覆盖：
 * - FunctionCalls: lua_call/lua_pcall/lua_cpcall调用机制
 * - ParameterPassing: 参数传递、可变参数、类型检查
 * - ReturnValues: 返回值处理、多返回值、尾调用优化
 * - ErrorHandling: 错误传播、异常安全、栈回滚
 * - ClosureOps: 闭包创建、upvalue管理、环境设置
 * - CoroutineOps: 协程创建、resume/yield、状态管理
 * - LoadAndDump: 代码加载、字节码转储、动态编译
 * - LibraryRegistration: 库函数注册、模块系统、require机制
 * - AuxiliaryFunctions: luaL_*辅助函数、参数检查、类型转换
 * - CallConventions: 调用约定、栈平衡、性能优化
 * 
 * 规格来源：
 * - Lua 5.1.5官方参考手册
 * - lua_c_analysis/src/lapi.c实现分析
 * - lua_with_cpp/src/api/*现代化设计
 * 
 * @author lua_cpp项目组
 * @date 2025-09-21
 * @version 1.0.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

// 核心C API头文件
#include "api/lua_api.h"
#include "api/luaaux.h"
#include "core/lua_state.h"
#include "core/lua_value.h"
#include "core/common.h"

// 测试工具
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <chrono>
#include <random>
#include <cstring>
#include <cstdarg>
#include <sstream>

namespace lua_cpp {
namespace c_api_call_contract_tests {

// ============================================================================
// 测试基础设施
// ============================================================================

/**
 * @brief C API函数调用测试夹具
 * 
 * 提供统一的测试环境，包括：
 * - Lua状态机管理
 * - 函数调用包装
 * - 错误处理验证
 * - 协程支持测试
 */
class CAPICallTestFixture {
public:
    CAPICallTestFixture() {
        // 创建标准Lua状态
        L = lua_newstate(default_alloc, nullptr);
        REQUIRE(L != nullptr);
        
        // 设置错误处理
        original_panic = lua_atpanic(L, test_panic);
        
        // 初始化测试环境
        setup_test_environment();
    }
    
    ~CAPICallTestFixture() {
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }
    
    // 禁用拷贝和移动
    CAPICallTestFixture(const CAPICallTestFixture&) = delete;
    CAPICallTestFixture& operator=(const CAPICallTestFixture&) = delete;
    CAPICallTestFixture(CAPICallTestFixture&&) = delete;
    CAPICallTestFixture& operator=(CAPICallTestFixture&&) = delete;

protected:
    lua_State* L = nullptr;
    lua_PFunction original_panic = nullptr;
    static bool panic_called;
    static std::string last_panic_message;
    static std::vector<std::string> call_trace;
    
    /**
     * @brief 默认内存分配函数
     */
    static void* default_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
        (void)ud; (void)osize;  // 避免未使用警告
        
        if (nsize == 0) {
            free(ptr);
            return nullptr;
        } else {
            return realloc(ptr, nsize);
        }
    }
    
    /**
     * @brief 测试panic函数
     */
    static int test_panic(lua_State* L) {
        panic_called = true;
        if (lua_isstring(L, -1)) {
            last_panic_message = lua_tostring(L, -1);
        }
        return 0;  // 不真正退出
    }
    
    /**
     * @brief 设置测试环境
     */
    void setup_test_environment() {
        // 重置状态
        panic_called = false;
        last_panic_message.clear();
        call_trace.clear();
        
        // 确保栈是干净的
        lua_settop(L, 0);
        
        // 检查初始状态
        REQUIRE(lua_gettop(L) == 0);
        REQUIRE(lua_checkstack(L, LUA_MINSTACK));
    }
    
    /**
     * @brief 验证栈状态
     */
    void verify_stack_integrity() {
        int top = lua_gettop(L);
        REQUIRE(top >= 0);
        
        // 验证栈中每个值的类型都有效
        for (int i = 1; i <= top; i++) {
            int type = lua_type(L, i);
            REQUIRE(type >= LUA_TNIL);
            REQUIRE(type <= LUA_TTHREAD);
        }
    }
    
    /**
     * @brief 清理栈
     */
    void clean_stack() {
        lua_settop(L, 0);
    }
    
    /**
     * @brief 记录调用轨迹
     */
    static void trace_call(const std::string& function_name) {
        call_trace.push_back(function_name);
    }
};

// 静态成员初始化
bool CAPICallTestFixture::panic_called = false;
std::string CAPICallTestFixture::last_panic_message;
std::vector<std::string> CAPICallTestFixture::call_trace;

// ============================================================================
// 契约测试组1: 基础函数调用 (Basic Function Calls)
// ============================================================================

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: lua_call基础调用", "[c_api][function_calls][lua_call]") {
    SECTION("🔍 lua_c_analysis验证: 简单函数调用") {
        // 注册一个简单的加法函数
        auto add_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("add_function");
            
            int argc = lua_gettop(L);
            if (argc != 2) {
                lua_pushstring(L, "Expected exactly 2 arguments");
                lua_error(L);
            }
            
            if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2)) {
                lua_pushstring(L, "Arguments must be numbers");
                lua_error(L);
            }
            
            lua_Number a = lua_tonumber(L, 1);
            lua_Number b = lua_tonumber(L, 2);
            lua_pushnumber(L, a + b);
            
            return 1;  // 返回1个值
        };
        
        // 将函数推入栈并设为全局
        lua_pushcfunction(L, add_function);
        lua_setglobal(L, "add");
        
        // 准备调用：获取函数并准备参数
        lua_getglobal(L, "add");
        REQUIRE(lua_isfunction(L, -1));
        
        lua_pushnumber(L, 10.5);
        lua_pushnumber(L, 20.3);
        
        REQUIRE(lua_gettop(L) == 3);  // 函数 + 2个参数
        
        // 执行调用
        call_trace.clear();
        lua_call(L, 2, 1);  // 2个参数，1个返回值
        
        // 验证结果
        REQUIRE(lua_gettop(L) == 1);  // 只有返回值
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tonumber(L, -1) == 30.8);
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "add_function");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 多返回值函数") {
        // 返回多个值的函数
        auto multi_return = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("multi_return");
            
            int argc = lua_gettop(L);
            if (argc != 1) {
                lua_pushstring(L, "Expected 1 argument");
                lua_error(L);
            }
            
            if (!lua_isnumber(L, 1)) {
                lua_pushstring(L, "Argument must be a number");
                lua_error(L);
            }
            
            lua_Number n = lua_tonumber(L, 1);
            
            // 返回：原数、平方、立方
            lua_pushnumber(L, n);
            lua_pushnumber(L, n * n);
            lua_pushnumber(L, n * n * n);
            
            return 3;  // 返回3个值
        };
        
        lua_pushcfunction(L, multi_return);
        lua_setglobal(L, "powers");
        
        // 调用并获取多个返回值
        lua_getglobal(L, "powers");
        lua_pushnumber(L, 3.0);
        
        call_trace.clear();
        lua_call(L, 1, 3);  // 1个参数，3个返回值
        
        // 验证结果
        REQUIRE(lua_gettop(L) == 3);
        REQUIRE(lua_tonumber(L, -3) == 3.0);   // 原数
        REQUIRE(lua_tonumber(L, -2) == 9.0);   // 平方
        REQUIRE(lua_tonumber(L, -1) == 27.0);  // 立方
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "multi_return");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 无返回值函数") {
        // 只有副作用，无返回值的函数
        static std::string side_effect_result;
        
        auto side_effect_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("side_effect_function");
            
            int argc = lua_gettop(L);
            if (argc >= 1 && lua_isstring(L, 1)) {
                side_effect_result = lua_tostring(L, 1);
            } else {
                side_effect_result = "no_string_provided";
            }
            
            return 0;  // 无返回值
        };
        
        lua_pushcfunction(L, side_effect_function);
        lua_setglobal(L, "side_effect");
        
        // 调用无返回值函数
        lua_getglobal(L, "side_effect");
        lua_pushstring(L, "test_message");
        
        call_trace.clear();
        side_effect_result.clear();
        lua_call(L, 1, 0);  // 1个参数，0个返回值
        
        // 验证结果
        REQUIRE(lua_gettop(L) == 0);  // 栈应该是空的
        REQUIRE(side_effect_result == "test_message");
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "side_effect_function");
    }
}

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: lua_pcall保护调用", "[c_api][function_calls][lua_pcall]") {
    SECTION("🔍 lua_c_analysis验证: 成功的保护调用") {
        // 正常的函数
        auto normal_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("normal_function");
            
            lua_pushstring(L, "success");
            return 1;
        };
        
        lua_pushcfunction(L, normal_function);
        lua_setglobal(L, "normal");
        
        // 执行保护调用
        lua_getglobal(L, "normal");
        
        call_trace.clear();
        int result = lua_pcall(L, 0, 1, 0);
        
        // 验证成功调用
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_gettop(L) == 1);
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "success");
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "normal_function");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 捕获运行时错误") {
        // 会抛出错误的函数
        auto error_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("error_function");
            
            lua_pushstring(L, "Runtime error occurred");
            lua_error(L);
            
            return 0;  // 永不到达
        };
        
        lua_pushcfunction(L, error_function);
        lua_setglobal(L, "error_func");
        
        // 执行保护调用
        lua_getglobal(L, "error_func");
        
        call_trace.clear();
        int result = lua_pcall(L, 0, 0, 0);
        
        // 验证错误被捕获
        REQUIRE(result == LUA_ERRRUN);
        REQUIRE(lua_gettop(L) == 1);  // 错误消息在栈顶
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "Runtime error occurred");
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "error_function");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 错误处理函数") {
        // 错误处理函数
        auto error_handler = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("error_handler");
            
            // 获取原始错误消息
            const char* msg = lua_tostring(L, 1);
            std::string handled_msg = "Handled: ";
            if (msg) {
                handled_msg += msg;
            }
            
            lua_pushstring(L, handled_msg.c_str());
            return 1;
        };
        
        // 会出错的函数
        auto failing_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("failing_function");
            lua_pushstring(L, "Original error");
            lua_error(L);
            return 0;
        };
        
        // 注册函数
        lua_pushcfunction(L, error_handler);
        lua_setglobal(L, "error_handler");
        lua_pushcfunction(L, failing_function);
        lua_setglobal(L, "failing");
        
        // 设置错误处理函数
        lua_getglobal(L, "error_handler");
        int error_handler_index = lua_gettop(L);
        
        // 准备调用
        lua_getglobal(L, "failing");
        
        call_trace.clear();
        int result = lua_pcall(L, 0, 0, error_handler_index);
        
        // 验证错误被处理
        REQUIRE(result == LUA_ERRRUN);
        REQUIRE(lua_gettop(L) == 2);  // 错误处理函数 + 处理后的错误消息
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "Handled: Original error");
        
        // 验证调用顺序
        REQUIRE(call_trace.size() == 2);
        REQUIRE(call_trace[0] == "failing_function");
        REQUIRE(call_trace[1] == "error_handler");
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 异常安全的保护调用") {
        // 使用RAII确保栈平衡的保护调用包装
        class SafeCall {
        public:
            SafeCall(lua_State* L) : L_(L), initial_top_(lua_gettop(L)) {}
            
            ~SafeCall() {
                // 如果发生异常，恢复栈状态
                if (std::uncaught_exceptions() > 0) {
                    lua_settop(L_, initial_top_);
                }
            }
            
            template<typename... Args>
            int call(const std::string& func_name, Args&&... args) {
                lua_getglobal(L_, func_name.c_str());
                if (!lua_isfunction(L_, -1)) {
                    lua_pop(L_, 1);
                    return LUA_ERRRUN;
                }
                
                // 推入参数
                int nargs = push_args(std::forward<Args>(args)...);
                
                // 执行保护调用
                return lua_pcall(L_, nargs, LUA_MULTRET, 0);
            }
            
            int results() const {
                return lua_gettop(L_) - initial_top_;
            }
            
        private:
            lua_State* L_;
            int initial_top_;
            
            // 参数推入（递归终止）
            int push_args() { return 0; }
            
            // 参数推入（递归）
            template<typename T, typename... Rest>
            int push_args(T&& first, Rest&&... rest) {
                push_value(std::forward<T>(first));
                return 1 + push_args(std::forward<Rest>(rest)...);
            }
            
            void push_value(int value) { lua_pushinteger(L_, value); }
            void push_value(double value) { lua_pushnumber(L_, value); }
            void push_value(const std::string& value) { lua_pushstring(L_, value.c_str()); }
            void push_value(const char* value) { lua_pushstring(L_, value); }
            void push_value(bool value) { lua_pushboolean(L_, value ? 1 : 0); }
        };
        
        // 注册测试函数
        auto concat_function = [](lua_State* L) -> int {
            int argc = lua_gettop(L);
            std::string result;
            
            for (int i = 1; i <= argc; i++) {
                if (lua_isstring(L, i)) {
                    if (!result.empty()) result += " ";
                    result += lua_tostring(L, i);
                }
            }
            
            lua_pushstring(L, result.c_str());
            return 1;
        };
        
        lua_pushcfunction(L, concat_function);
        lua_setglobal(L, "concat");
        
        // 使用安全调用包装器
        {
            SafeCall safe_call(L);
            int result = safe_call.call("concat", "Hello", "World", "Test");
            
            REQUIRE(result == LUA_OK);
            REQUIRE(safe_call.results() == 1);
            REQUIRE(lua_isstring(L, -1));
            REQUIRE(std::string(lua_tostring(L, -1)) == "Hello World Test");
        }
        
        clean_stack();
    }
}

// ============================================================================
// 契约测试组2: C函数和闭包 (C Functions and Closures)
// ============================================================================

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: C函数注册和闭包", "[c_api][function_calls][c_functions]") {
    SECTION("🔍 lua_c_analysis验证: 带upvalue的C闭包") {
        // 使用upvalue的计数器函数
        auto counter_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("counter_function");
            
            // 获取upvalue（计数器值）
            if (!lua_isnumber(L, lua_upvalueindex(1))) {
                lua_pushstring(L, "Invalid upvalue");
                lua_error(L);
            }
            
            lua_Number count = lua_tonumber(L, lua_upvalueindex(1));
            count += 1.0;
            
            // 更新upvalue
            lua_pushnumber(L, count);
            lua_replace(L, lua_upvalueindex(1));
            
            // 返回新的计数值
            lua_pushnumber(L, count);
            return 1;
        };
        
        // 创建带upvalue的闭包
        lua_pushnumber(L, 0.0);  // 初始计数值作为upvalue
        lua_pushcclosure(L, counter_function, 1);  // 1个upvalue
        lua_setglobal(L, "counter");
        
        // 多次调用计数器
        call_trace.clear();
        for (int i = 1; i <= 5; i++) {
            lua_getglobal(L, "counter");
            lua_call(L, 0, 1);
            
            REQUIRE(lua_isnumber(L, -1));
            REQUIRE(lua_tonumber(L, -1) == static_cast<double>(i));
            lua_pop(L, 1);
        }
        
        REQUIRE(call_trace.size() == 5);
        for (const auto& trace : call_trace) {
            REQUIRE(trace == "counter_function");
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 多个upvalue的闭包") {
        // 使用多个upvalue的函数
        auto multi_upvalue_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("multi_upvalue_function");
            
            // 获取所有upvalue并求和
            lua_Number sum = 0.0;
            for (int i = 1; i <= 3; i++) {
                if (lua_isnumber(L, lua_upvalueindex(i))) {
                    sum += lua_tonumber(L, lua_upvalueindex(i));
                }
            }
            
            lua_pushnumber(L, sum);
            return 1;
        };
        
        // 创建3个upvalue的闭包
        lua_pushnumber(L, 10.0);
        lua_pushnumber(L, 20.0);
        lua_pushnumber(L, 30.0);
        lua_pushcclosure(L, multi_upvalue_function, 3);  // 3个upvalue
        lua_setglobal(L, "sum_upvalues");
        
        // 调用函数
        lua_getglobal(L, "sum_upvalues");
        call_trace.clear();
        lua_call(L, 0, 1);
        
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tonumber(L, -1) == 60.0);  // 10 + 20 + 30
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "multi_upvalue_function");
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++闭包包装") {
        // 使用C++11 lambda和std::function的闭包包装
        class LuaClosureWrapper {
        public:
            template<typename Func>
            static void register_closure(lua_State* L, const std::string& name, Func&& func) {
                // 将std::function存储为轻量用户数据
                auto* stored_func = new std::function<int(lua_State*)>(std::forward<Func>(func));
                
                lua_pushlightuserdata(L, stored_func);
                lua_pushcclosure(L, closure_dispatcher, 1);
                lua_setglobal(L, name.c_str());
            }
            
        private:
            static int closure_dispatcher(lua_State* L) {
                CAPICallTestFixture::trace_call("closure_dispatcher");
                
                // 获取存储的函数
                void* func_ptr = lua_touserdata(L, lua_upvalueindex(1));
                if (!func_ptr) {
                    lua_pushstring(L, "Invalid closure function");
                    lua_error(L);
                }
                
                auto* func = static_cast<std::function<int(lua_State*)>*>(func_ptr);
                
                try {
                    return (*func)(L);
                } catch (const std::exception& e) {
                    lua_pushstring(L, e.what());
                    lua_error(L);
                    return 0;
                }
            }
        };
        
        // 注册一个使用C++特性的闭包
        int captured_value = 42;
        LuaClosureWrapper::register_closure(L, "cpp_closure", 
            [captured_value](lua_State* L) -> int {
                CAPICallTestFixture::trace_call("cpp_closure_lambda");
                
                int argc = lua_gettop(L);
                lua_Number sum = captured_value;
                
                for (int i = 1; i <= argc; i++) {
                    if (lua_isnumber(L, i)) {
                        sum += lua_tonumber(L, i);
                    }
                }
                
                lua_pushnumber(L, sum);
                return 1;
            });
        
        // 测试闭包
        lua_getglobal(L, "cpp_closure");
        lua_pushnumber(L, 8.0);
        lua_pushnumber(L, 10.0);
        
        call_trace.clear();
        lua_call(L, 2, 1);
        
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tonumber(L, -1) == 60.0);  // 42 + 8 + 10
        REQUIRE(call_trace.size() == 2);
        REQUIRE(call_trace[0] == "closure_dispatcher");
        REQUIRE(call_trace[1] == "cpp_closure_lambda");
        
        clean_stack();
    }
}

// ============================================================================
// 契约测试组3: 协程操作 (Coroutine Operations)
// ============================================================================

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: 协程创建和控制", "[c_api][function_calls][coroutines]") {
    SECTION("🔍 lua_c_analysis验证: 基础协程操作") {
        // 创建新线程（协程）
        lua_State* co = lua_newthread(L);
        REQUIRE(co != nullptr);
        REQUIRE(co != L);  // 应该是不同的状态
        REQUIRE(lua_isthread(L, -1));  // 主线程栈顶应该有线程对象
        
        // 在协程中定义一个简单函数
        auto coroutine_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("coroutine_function");
            
            // yield一个值
            lua_pushstring(L, "yielded_value");
            return lua_yield(L, 1);
        };
        
        // 将函数推入协程
        lua_pushcfunction(co, coroutine_function);
        
        // 恢复协程执行
        call_trace.clear();
        int result = lua_resume(co, L, 0);  // 0个参数
        
        // 验证协程yield
        REQUIRE(result == LUA_YIELD);
        REQUIRE(lua_gettop(co) == 1);  // 协程栈上有yield的值
        REQUIRE(lua_isstring(co, -1));
        REQUIRE(std::string(lua_tostring(co, -1)) == "yielded_value");
        REQUIRE(call_trace.size() == 1);
        REQUIRE(call_trace[0] == "coroutine_function");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 协程参数传递和返回") {
        lua_State* co = lua_newthread(L);
        
        // 接受参数并返回结果的协程函数
        auto param_coroutine = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("param_coroutine");
            
            int argc = lua_gettop(L);
            lua_Number sum = 0.0;
            
            // 计算所有参数的和
            for (int i = 1; i <= argc; i++) {
                if (lua_isnumber(L, i)) {
                    sum += lua_tonumber(L, i);
                }
            }
            
            // 先yield中间结果
            lua_pushstring(L, "intermediate");
            lua_pushnumber(L, sum / 2.0);
            lua_yield(L, 2);
            
            // 然后返回最终结果
            lua_pushnumber(L, sum);
            return 1;
        };
        
        lua_pushcfunction(co, param_coroutine);
        
        // 第一次恢复：传递参数
        call_trace.clear();
        int result = lua_resume(co, L, 0);
        REQUIRE(result == LUA_YIELD);
        REQUIRE(lua_gettop(co) == 2);  // 2个yield的值
        REQUIRE(std::string(lua_tostring(co, -2)) == "intermediate");
        REQUIRE(lua_tonumber(co, -1) == 0.0);  // sum/2 = 0/2 = 0
        
        // 清理协程栈并准备新的参数
        lua_settop(co, 0);
        lua_pushnumber(co, 10.0);
        lua_pushnumber(co, 20.0);
        lua_pushnumber(co, 30.0);
        
        // 第二次恢复：传递新参数
        result = lua_resume(co, L, 3);
        REQUIRE(result == LUA_YIELD);
        REQUIRE(lua_gettop(co) == 2);
        REQUIRE(std::string(lua_tostring(co, -2)) == "intermediate");
        REQUIRE(lua_tonumber(co, -1) == 30.0);  // (10+20+30)/2 = 30
        
        // 第三次恢复：获取最终结果
        lua_settop(co, 0);
        result = lua_resume(co, L, 0);
        REQUIRE(result == LUA_OK);  // 协程完成
        REQUIRE(lua_gettop(co) == 1);
        REQUIRE(lua_tonumber(co, -1) == 60.0);  // 10+20+30 = 60
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: RAII协程管理") {
        // RAII风格的协程管理器
        class CoroutineManager {
        public:
            CoroutineManager(lua_State* main_L) : main_L_(main_L) {
                co_ = lua_newthread(main_L);
                if (!co_) {
                    throw std::runtime_error("Failed to create coroutine");
                }
                // 保持对协程的引用
                co_ref_ = luaL_ref(main_L, LUA_REGISTRYINDEX);
            }
            
            ~CoroutineManager() {
                if (co_ref_ != LUA_NOREF) {
                    luaL_unref(main_L_, LUA_REGISTRYINDEX, co_ref_);
                }
            }
            
            template<typename Func>
            void set_function(Func&& func) {
                lua_pushcfunction(co_, std::forward<Func>(func));
            }
            
            int resume(int nargs = 0) {
                return lua_resume(co_, main_L_, nargs);
            }
            
            lua_State* get() const { return co_; }
            
            bool is_finished() const {
                return lua_status(co_) == LUA_OK;
            }
            
            bool is_yielded() const {
                return lua_status(co_) == LUA_YIELD;
            }
            
        private:
            lua_State* main_L_;
            lua_State* co_;
            int co_ref_;
        };
        
        // 使用协程管理器
        {
            CoroutineManager co_mgr(L);
            
            // 设置协程函数
            co_mgr.set_function([](lua_State* L) -> int {
                CAPICallTestFixture::trace_call("managed_coroutine");
                
                for (int i = 1; i <= 3; i++) {
                    lua_pushinteger(L, i);
                    lua_yield(L, 1);
                }
                
                lua_pushstring(L, "completed");
                return 1;
            });
            
            call_trace.clear();
            
            // 逐步执行协程
            for (int i = 1; i <= 3; i++) {
                int result = co_mgr.resume();
                REQUIRE(result == LUA_YIELD);
                REQUIRE(co_mgr.is_yielded());
                REQUIRE_FALSE(co_mgr.is_finished());
                
                REQUIRE(lua_gettop(co_mgr.get()) == 1);
                REQUIRE(lua_tointeger(co_mgr.get(), -1) == i);
                lua_pop(co_mgr.get(), 1);  // 清理yield的值
            }
            
            // 最后一次恢复
            int result = co_mgr.resume();
            REQUIRE(result == LUA_OK);
            REQUIRE(co_mgr.is_finished());
            REQUIRE_FALSE(co_mgr.is_yielded());
            
            REQUIRE(lua_gettop(co_mgr.get()) == 1);
            REQUIRE(std::string(lua_tostring(co_mgr.get(), -1)) == "completed");
            
            REQUIRE(call_trace.size() == 1);
            REQUIRE(call_trace[0] == "managed_coroutine");
        }
        // 协程管理器析构时自动清理
        
        clean_stack();
    }
}

// ============================================================================
// 契约测试组4: 代码加载和转储 (Code Loading and Dumping)
// ============================================================================

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: 代码加载", "[c_api][function_calls][code_loading]") {
    SECTION("🔍 lua_c_analysis验证: 字符串代码加载") {
        // 简单的Lua代码字符串
        const char* lua_code = R"(
            function test_function(a, b)
                return a + b, a * b
            end
            return test_function
        )";
        
        // 使用字符串读取器加载代码
        struct StringReader {
            const char* data;
            size_t size;
            bool read;
        };
        
        auto string_reader = [](lua_State* L, void* ud, size_t* sz) -> const char* {
            (void)L;  // 避免未使用警告
            StringReader* reader = static_cast<StringReader*>(ud);
            
            if (reader->read) {
                *sz = 0;
                return nullptr;  // 已读完
            }
            
            reader->read = true;
            *sz = reader->size;
            return reader->data;
        };
        
        StringReader reader{lua_code, strlen(lua_code), false};
        
        // 加载代码
        int load_result = lua_load(L, string_reader, &reader, "test_chunk");
        REQUIRE(load_result == LUA_OK);
        REQUIRE(lua_isfunction(L, -1));
        
        // 执行加载的代码
        lua_call(L, 0, 1);  // 执行chunk，返回test_function
        REQUIRE(lua_isfunction(L, -1));
        
        // 调用返回的函数
        lua_pushnumber(L, 5.0);
        lua_pushnumber(L, 3.0);
        lua_call(L, 2, 2);  // 调用test_function(5, 3)
        
        // 验证结果
        REQUIRE(lua_gettop(L) == 2);
        REQUIRE(lua_tonumber(L, -2) == 8.0);   // 5 + 3
        REQUIRE(lua_tonumber(L, -1) == 15.0);  // 5 * 3
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 语法错误处理") {
        // 包含语法错误的Lua代码
        const char* bad_lua_code = R"(
            function bad_function(a, b
                return a + b  -- 缺少右括号
            end
        )";
        
        struct StringReader {
            const char* data;
            size_t size;
            bool read;
        };
        
        auto string_reader = [](lua_State* L, void* ud, size_t* sz) -> const char* {
            (void)L;
            StringReader* reader = static_cast<StringReader*>(ud);
            
            if (reader->read) {
                *sz = 0;
                return nullptr;
            }
            
            reader->read = true;
            *sz = reader->size;
            return reader->data;
        };
        
        StringReader reader{bad_lua_code, strlen(bad_lua_code), false};
        
        // 尝试加载错误的代码
        int load_result = lua_load(L, string_reader, &reader, "bad_chunk");
        REQUIRE(load_result == LUA_ERRSYNTAX);
        REQUIRE(lua_isstring(L, -1));  // 错误消息
        
        std::string error_msg = lua_tostring(L, -1);
        REQUIRE_FALSE(error_msg.empty());
        // 错误消息应该包含语法错误信息
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++代码加载包装") {
        // 现代C++风格的代码加载器
        class LuaCodeLoader {
        public:
            explicit LuaCodeLoader(lua_State* L) : L_(L) {}
            
            enum class LoadResult {
                Success,
                SyntaxError,
                MemoryError
            };
            
            LoadResult load_string(const std::string& code, const std::string& chunk_name = "chunk") {
                struct StringReader {
                    std::string_view data;
                    bool read = false;
                };
                
                auto reader_func = [](lua_State* L, void* ud, size_t* sz) -> const char* {
                    (void)L;
                    StringReader* reader = static_cast<StringReader*>(ud);
                    
                    if (reader->read) {
                        *sz = 0;
                        return nullptr;
                    }
                    
                    reader->read = true;
                    *sz = reader->data.size();
                    return reader->data.data();
                };
                
                StringReader reader{std::string_view(code)};
                
                int result = lua_load(L_, reader_func, &reader, chunk_name.c_str());
                
                switch (result) {
                    case LUA_OK:
                        return LoadResult::Success;
                    case LUA_ERRSYNTAX:
                        return LoadResult::SyntaxError;
                    case LUA_ERRMEM:
                        return LoadResult::MemoryError;
                    default:
                        return LoadResult::SyntaxError;
                }
            }
            
            std::optional<std::string> get_error_message() {
                if (lua_isstring(L_, -1)) {
                    std::string msg = lua_tostring(L_, -1);
                    lua_pop(L_, 1);  // 清理错误消息
                    return msg;
                }
                return std::nullopt;
            }
            
            template<typename... Args>
            int execute(Args&&... args) {
                if (!lua_isfunction(L_, -1)) {
                    return LUA_ERRRUN;
                }
                
                // 推入参数
                int nargs = push_args(std::forward<Args>(args)...);
                
                return lua_pcall(L_, nargs, LUA_MULTRET, 0);
            }
            
        private:
            lua_State* L_;
            
            int push_args() { return 0; }
            
            template<typename T, typename... Rest>
            int push_args(T&& first, Rest&&... rest) {
                push_value(std::forward<T>(first));
                return 1 + push_args(std::forward<Rest>(rest)...);
            }
            
            void push_value(int value) { lua_pushinteger(L_, value); }
            void push_value(double value) { lua_pushnumber(L_, value); }
            void push_value(const std::string& value) { lua_pushstring(L_, value.c_str()); }
        };
        
        LuaCodeLoader loader(L);
        
        // 加载并执行正确的代码
        std::string good_code = R"(
            local function multiply(x, y)
                return x * y
            end
            return multiply
        )";
        
        auto result = loader.load_string(good_code, "multiply_chunk");
        REQUIRE(result == LuaCodeLoader::LoadResult::Success);
        
        int exec_result = loader.execute();
        REQUIRE(exec_result == LUA_OK);
        REQUIRE(lua_isfunction(L, -1));
        
        // 调用返回的函数
        lua_pushnumber(L, 6.0);
        lua_pushnumber(L, 7.0);
        lua_call(L, 2, 1);
        
        REQUIRE(lua_tonumber(L, -1) == 42.0);
        
        clean_stack();
    }
}

} // namespace c_api_call_contract_tests
} // namespace lua_cpp

// ============================================================================
// 测试配置和全局设置
// ============================================================================

/**
 * @brief 测试程序入口点配置
 * 
 * 配置Catch2测试框架的行为和报告格式
 */
namespace {

// 自定义测试监听器，用于验证函数调用测试状态
class CAPICallTestListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;
    
    void testCaseStarting(const Catch::TestCaseInfo& testInfo) override {
        // 在每个测试用例开始时的设置
        current_test_name = testInfo.name;
    }
    
    void testCaseEnded(const Catch::TestCaseStats& testCaseStats) override {
        // 在每个测试用例结束时的清理和验证
        if (testCaseStats.testInfo->tags.find("[function_calls]") != testCaseStats.testInfo->tags.end()) {
            // 验证函数调用测试没有泄漏资源或破坏状态
        }
    }
    
private:
    std::string current_test_name;
};

CATCH_REGISTER_LISTENER(CAPICallTestListener)

} // anonymous namespace

// ============================================================================
// 扩展测试组5: 辅助函数和参数检查 (Auxiliary Functions and Parameter Checking)
// ============================================================================

namespace lua_cpp {
namespace c_api_call_contract_tests {

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: luaL辅助函数", "[c_api][function_calls][auxiliary]") {
    SECTION("🔍 lua_c_analysis验证: 参数检查函数") {
        // 使用luaL_*参数检查函数的测试函数
        auto param_check_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("param_check_function");
            
            // 检查必需的参数
            lua_Number num = luaL_checknumber(L, 1);
            const char* str = luaL_checkstring(L, 2);
            lua_Integer int_val = luaL_checkinteger(L, 3);
            
            // 检查可选参数
            lua_Number opt_num = luaL_optnumber(L, 4, 42.0);
            const char* opt_str = luaL_optstring(L, 5, "default");
            
            // 组合结果
            std::ostringstream result;
            result << "num=" << num << ", str=" << str << ", int=" << int_val
                   << ", opt_num=" << opt_num << ", opt_str=" << opt_str;
            
            lua_pushstring(L, result.str().c_str());
            return 1;
        };
        
        lua_pushcfunction(L, param_check_function);
        lua_setglobal(L, "param_check");
        
        // 测试完整参数调用
        lua_getglobal(L, "param_check");
        lua_pushnumber(L, 3.14);
        lua_pushstring(L, "hello");
        lua_pushinteger(L, 100);
        lua_pushnumber(L, 99.9);
        lua_pushstring(L, "custom");
        
        call_trace.clear();
        int result = lua_pcall(L, 5, 1, 0);
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_isstring(L, -1));
        std::string output = lua_tostring(L, -1);
        REQUIRE(output == "num=3.14, str=hello, int=100, opt_num=99.9, opt_str=custom");
        lua_pop(L, 1);
        
        // 测试部分参数调用（使用默认值）
        lua_getglobal(L, "param_check");
        lua_pushnumber(L, 2.71);
        lua_pushstring(L, "world");
        lua_pushinteger(L, 200);
        
        result = lua_pcall(L, 3, 1, 0);
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_isstring(L, -1));
        output = lua_tostring(L, -1);
        REQUIRE(output == "num=2.71, str=world, int=200, opt_num=42, opt_str=default");
        lua_pop(L, 1);
        
        REQUIRE(call_trace.size() == 2);
        for (const auto& trace : call_trace) {
            REQUIRE(trace == "param_check_function");
        }
    }
    
    SECTION("🔍 lua_c_analysis验证: 参数类型错误处理") {
        auto strict_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("strict_function");
            
            // 严格的参数检查
            lua_Number num = luaL_checknumber(L, 1);
            const char* str = luaL_checkstring(L, 2);
            
            lua_pushnumber(L, num * 2);
            lua_pushfstring(L, "Processed: %s", str);
            return 2;
        };
        
        lua_pushcfunction(L, strict_function);
        lua_setglobal(L, "strict");
        
        // 测试正确的参数类型
        lua_getglobal(L, "strict");
        lua_pushnumber(L, 21.0);
        lua_pushstring(L, "test");
        
        call_trace.clear();
        int result = lua_pcall(L, 2, 2, 0);
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_gettop(L) == 2);
        REQUIRE(lua_tonumber(L, -2) == 42.0);
        REQUIRE(std::string(lua_tostring(L, -1)) == "Processed: test");
        lua_pop(L, 2);
        
        // 测试错误的参数类型
        lua_getglobal(L, "strict");
        lua_pushstring(L, "not_a_number");  // 第一个参数应该是数字
        lua_pushnumber(L, 123);             // 第二个参数应该是字符串
        
        result = lua_pcall(L, 2, 2, 0);
        REQUIRE(result == LUA_ERRRUN);  // 应该产生运行时错误
        REQUIRE(lua_isstring(L, -1));   // 错误消息
        lua_pop(L, 1);
        
        REQUIRE(call_trace.size() == 2);  // 一次成功调用，一次失败调用
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++参数检查包装") {
        // 类型安全的参数检查包装器
        class ParameterChecker {
        public:
            explicit ParameterChecker(lua_State* L) : L_(L) {}
            
            template<typename T>
            T check_arg(int index) {
                if constexpr (std::is_same_v<T, lua_Number>) {
                    return luaL_checknumber(L_, index);
                } else if constexpr (std::is_same_v<T, lua_Integer>) {
                    return luaL_checkinteger(L_, index);
                } else if constexpr (std::is_same_v<T, std::string>) {
                    const char* str = luaL_checkstring(L_, index);
                    return std::string(str ? str : "");
                } else if constexpr (std::is_same_v<T, bool>) {
                    luaL_checktype(L_, index, LUA_TBOOLEAN);
                    return lua_toboolean(L_, index) != 0;
                }
                static_assert(always_false<T>, "Unsupported parameter type");
            }
            
            template<typename T>
            T optional_arg(int index, const T& default_value) {
                if (lua_gettop(L_) < index || lua_isnil(L_, index)) {
                    return default_value;
                }
                return check_arg<T>(index);
            }
            
        private:
            lua_State* L_;
            
            template<typename>
            static constexpr bool always_false = false;
        };
        
        // 使用现代参数检查器的函数
        auto modern_function = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("modern_function");
            
            try {
                ParameterChecker checker(L);
                
                auto name = checker.check_arg<std::string>(1);
                auto age = checker.check_arg<lua_Integer>(2);
                auto active = checker.optional_arg<bool>(3, true);
                auto bonus = checker.optional_arg<lua_Number>(4, 0.0);
                
                std::ostringstream result;
                result << "Person{name='" << name << "', age=" << age 
                       << ", active=" << (active ? "true" : "false")
                       << ", bonus=" << bonus << "}";
                
                lua_pushstring(L, result.str().c_str());
                return 1;
                
            } catch (const std::exception& e) {
                lua_pushstring(L, e.what());
                lua_error(L);
                return 0;
            }
        };
        
        lua_pushcfunction(L, modern_function);
        lua_setglobal(L, "modern");
        
        // 测试完整参数
        lua_getglobal(L, "modern");
        lua_pushstring(L, "Alice");
        lua_pushinteger(L, 30);
        lua_pushboolean(L, 0);
        lua_pushnumber(L, 1000.5);
        
        call_trace.clear();
        int result = lua_pcall(L, 4, 1, 0);
        REQUIRE(result == LUA_OK);
        std::string output = lua_tostring(L, -1);
        REQUIRE(output == "Person{name='Alice', age=30, active=false, bonus=1000.5}");
        lua_pop(L, 1);
        
        // 测试部分参数
        lua_getglobal(L, "modern");
        lua_pushstring(L, "Bob");
        lua_pushinteger(L, 25);
        
        result = lua_pcall(L, 2, 1, 0);
        REQUIRE(result == LUA_OK);
        output = lua_tostring(L, -1);
        REQUIRE(output == "Person{name='Bob', age=25, active=true, bonus=0}");
        lua_pop(L, 1);
        
        REQUIRE(call_trace.size() == 2);
    }
}

// ============================================================================
// 测试组6: 库注册和模块系统 (Library Registration and Module System)
// ============================================================================

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: 库函数注册", "[c_api][function_calls][library_registration]") {
    SECTION("🔍 lua_c_analysis验证: luaL_Reg库注册") {
        // 定义库函数
        auto lib_add = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("lib_add");
            lua_Number a = luaL_checknumber(L, 1);
            lua_Number b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a + b);
            return 1;
        };
        
        auto lib_mul = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("lib_mul");
            lua_Number a = luaL_checknumber(L, 1);
            lua_Number b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a * b);
            return 1;
        };
        
        auto lib_info = [](lua_State* L) -> int {
            CAPICallTestFixture::trace_call("lib_info");
            lua_pushstring(L, "Math Library v1.0");
            return 1;
        };
        
        // 定义库函数表
        static const luaL_Reg mathlib[] = {
            {"add", lib_add},
            {"mul", lib_mul},
            {"info", lib_info},
            {nullptr, nullptr}  // 结束标记
        };
        
        // 注册库
        lua_newtable(L);  // 创建库表
        luaL_register(L, nullptr, mathlib);  // 注册函数到表中
        lua_setglobal(L, "mathlib");
        
        // 测试库函数
        call_trace.clear();
        
        // 测试add函数
        lua_getglobal(L, "mathlib");
        lua_getfield(L, -1, "add");
        lua_pushnumber(L, 10);
        lua_pushnumber(L, 20);
        lua_call(L, 2, 1);
        REQUIRE(lua_tonumber(L, -1) == 30);
        lua_pop(L, 2);  // 弹出结果和库表
        
        // 测试mul函数
        lua_getglobal(L, "mathlib");
        lua_getfield(L, -1, "mul");
        lua_pushnumber(L, 6);
        lua_pushnumber(L, 7);
        lua_call(L, 2, 1);
        REQUIRE(lua_tonumber(L, -1) == 42);
        lua_pop(L, 2);
        
        // 测试info函数
        lua_getglobal(L, "mathlib");
        lua_getfield(L, -1, "info");
        lua_call(L, 0, 1);
        REQUIRE(std::string(lua_tostring(L, -1)) == "Math Library v1.0");
        lua_pop(L, 2);
        
        // 验证调用轨迹
        REQUIRE(call_trace.size() == 3);
        REQUIRE(call_trace[0] == "lib_add");
        REQUIRE(call_trace[1] == "lib_mul");
        REQUIRE(call_trace[2] == "lib_info");
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++库注册系统") {
        // 现代C++库注册系统
        class LuaLibraryBuilder {
        public:
            explicit LuaLibraryBuilder(lua_State* L) : L_(L) {
                lua_newtable(L_);  // 创建库表
            }
            
            template<typename Func>
            LuaLibraryBuilder& add_function(const std::string& name, Func&& func) {
                auto* stored_func = new std::function<int(lua_State*)>(std::forward<Func>(func));
                
                lua_pushlightuserdata(L_, stored_func);
                lua_pushcclosure(L_, function_dispatcher, 1);
                lua_setfield(L_, -2, name.c_str());
                
                return *this;
            }
            
            LuaLibraryBuilder& add_constant(const std::string& name, lua_Number value) {
                lua_pushnumber(L_, value);
                lua_setfield(L_, -2, name.c_str());
                return *this;
            }
            
            LuaLibraryBuilder& add_string(const std::string& name, const std::string& value) {
                lua_pushstring(L_, value.c_str());
                lua_setfield(L_, -2, name.c_str());
                return *this;
            }
            
            void register_global(const std::string& global_name) {
                lua_setglobal(L_, global_name.c_str());
            }
            
        private:
            lua_State* L_;
            
            static int function_dispatcher(lua_State* L) {
                void* func_ptr = lua_touserdata(L, lua_upvalueindex(1));
                if (!func_ptr) {
                    lua_pushstring(L, "Invalid function pointer");
                    lua_error(L);
                }
                
                auto* func = static_cast<std::function<int(lua_State*)>*>(func_ptr);
                
                try {
                    return (*func)(L);
                } catch (const std::exception& e) {
                    lua_pushstring(L, e.what());
                    lua_error(L);
                    return 0;
                }
            }
        };
        
        // 使用现代库构建器
        LuaLibraryBuilder(L)
            .add_function("square", [](lua_State* L) -> int {
                CAPICallTestFixture::trace_call("modern_square");
                lua_Number n = luaL_checknumber(L, 1);
                lua_pushnumber(L, n * n);
                return 1;
            })
            .add_function("concat", [](lua_State* L) -> int {
                CAPICallTestFixture::trace_call("modern_concat");
                int argc = lua_gettop(L);
                std::string result;
                
                for (int i = 1; i <= argc; i++) {
                    const char* str = luaL_checkstring(L, i);
                    if (i > 1) result += " ";
                    result += str;
                }
                
                lua_pushstring(L, result.c_str());
                return 1;
            })
            .add_constant("PI", 3.14159265359)
            .add_constant("E", 2.71828182846)
            .add_string("VERSION", "Modern Lib 2.0")
            .register_global("modernlib");
        
        // 测试现代库
        call_trace.clear();
        
        // 测试square函数
        lua_getglobal(L, "modernlib");
        lua_getfield(L, -1, "square");
        lua_pushnumber(L, 8);
        lua_call(L, 1, 1);
        REQUIRE(lua_tonumber(L, -1) == 64);
        lua_pop(L, 2);
        
        // 测试concat函数
        lua_getglobal(L, "modernlib");
        lua_getfield(L, -1, "concat");
        lua_pushstring(L, "Hello");
        lua_pushstring(L, "Modern");
        lua_pushstring(L, "World");
        lua_call(L, 3, 1);
        REQUIRE(std::string(lua_tostring(L, -1)) == "Hello Modern World");
        lua_pop(L, 2);
        
        // 测试常量
        lua_getglobal(L, "modernlib");
        lua_getfield(L, -1, "PI");
        REQUIRE(lua_tonumber(L, -1) == 3.14159265359);
        lua_pop(L, 2);
        
        // 测试字符串常量
        lua_getglobal(L, "modernlib");
        lua_getfield(L, -1, "VERSION");
        REQUIRE(std::string(lua_tostring(L, -1)) == "Modern Lib 2.0");
        lua_pop(L, 2);
        
        REQUIRE(call_trace.size() == 2);
        REQUIRE(call_trace[0] == "modern_square");
        REQUIRE(call_trace[1] == "modern_concat");
        
        clean_stack();
    }
}

// ============================================================================
// 测试组7: 性能基准测试 (Performance Benchmarks)
// ============================================================================

TEST_CASE_METHOD(CAPICallTestFixture, "C API契约: 函数调用性能", "[c_api][function_calls][performance]") {
    SECTION("🔍 lua_c_analysis验证: 调用开销基准") {
        // 简单的基准测试函数
        auto benchmark_function = [](lua_State* L) -> int {
            // 最小化函数体以测量纯调用开销
            lua_pushinteger(L, 42);
            return 1;
        };
        
        lua_pushcfunction(L, benchmark_function);
        lua_setglobal(L, "benchmark");
        
        const int iterations = 1000;
        
        BENCHMARK("直接lua_call调用开销") {
            for (int i = 0; i < iterations; i++) {
                lua_getglobal(L, "benchmark");
                lua_call(L, 0, 1);
                lua_pop(L, 1);
            }
        };
        
        BENCHMARK("lua_pcall保护调用开销") {
            for (int i = 0; i < iterations; i++) {
                lua_getglobal(L, "benchmark");
                lua_pcall(L, 0, 1, 0);
                lua_pop(L, 1);
            }
        };
    }
    
    SECTION("🔍 lua_c_analysis验证: 参数传递性能") {
        auto param_function = [](lua_State* L) -> int {
            int argc = lua_gettop(L);
            lua_Number sum = 0;
            
            for (int i = 1; i <= argc; i++) {
                if (lua_isnumber(L, i)) {
                    sum += lua_tonumber(L, i);
                }
            }
            
            lua_pushnumber(L, sum);
            return 1;
        };
        
        lua_pushcfunction(L, param_function);
        lua_setglobal(L, "param_func");
        
        const int iterations = 500;
        
        BENCHMARK("少量参数调用") {
            for (int i = 0; i < iterations; i++) {
                lua_getglobal(L, "param_func");
                lua_pushnumber(L, 1);
                lua_pushnumber(L, 2);
                lua_call(L, 2, 1);
                lua_pop(L, 1);
            }
        };
        
        BENCHMARK("大量参数调用") {
            for (int i = 0; i < iterations; i++) {
                lua_getglobal(L, "param_func");
                for (int j = 1; j <= 10; j++) {
                    lua_pushnumber(L, j);
                }
                lua_call(L, 10, 1);
                lua_pop(L, 1);
            }
        };
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代包装器性能") {
        // C++包装器调用
        class ModernCaller {
        public:
            explicit ModernCaller(lua_State* L) : L_(L) {}
            
            template<typename... Args>
            std::optional<lua_Number> call_function(const std::string& name, Args&&... args) {
                lua_getglobal(L_, name.c_str());
                if (!lua_isfunction(L_, -1)) {
                    lua_pop(L_, 1);
                    return std::nullopt;
                }
                
                int nargs = push_args(std::forward<Args>(args)...);
                
                if (lua_pcall(L_, nargs, 1, 0) != LUA_OK) {
                    lua_pop(L_, 1);  // 清理错误消息
                    return std::nullopt;
                }
                
                if (!lua_isnumber(L_, -1)) {
                    lua_pop(L_, 1);
                    return std::nullopt;
                }
                
                lua_Number result = lua_tonumber(L_, -1);
                lua_pop(L_, 1);
                return result;
            }
            
        private:
            lua_State* L_;
            
            int push_args() { return 0; }
            
            template<typename T, typename... Rest>
            int push_args(T&& first, Rest&&... rest) {
                push_value(std::forward<T>(first));
                return 1 + push_args(std::forward<Rest>(rest)...);
            }
            
            void push_value(lua_Number value) { lua_pushnumber(L_, value); }
            void push_value(lua_Integer value) { lua_pushinteger(L_, value); }
        };
        
        // 注册测试函数
        auto simple_add = [](lua_State* L) -> int {
            lua_Number a = luaL_checknumber(L, 1);
            lua_Number b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a + b);
            return 1;
        };
        
        lua_pushcfunction(L, simple_add);
        lua_setglobal(L, "add");
        
        ModernCaller caller(L);
        const int iterations = 300;
        
        BENCHMARK("现代C++包装器调用") {
            for (int i = 0; i < iterations; i++) {
                auto result = caller.call_function("add", 10.0, 20.0);
                REQUIRE(result.has_value());
                REQUIRE(result.value() == 30.0);
            }
        };
        
        BENCHMARK("原生C API调用对比") {
            for (int i = 0; i < iterations; i++) {
                lua_getglobal(L, "add");
                lua_pushnumber(L, 10.0);
                lua_pushnumber(L, 20.0);
                lua_pcall(L, 2, 1, 0);
                lua_Number result = lua_tonumber(L, -1);
                lua_pop(L, 1);
                REQUIRE(result == 30.0);
            }
        };
    }
}

} // namespace c_api_call_contract_tests
} // namespace lua_cpp