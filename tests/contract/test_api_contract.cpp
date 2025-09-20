/**
 * @file test_api_contract.cpp
 * @brief API（C接口）契约测试
 * @description 测试Lua C API的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括Lua-C互操作、堆栈操作、类型转换、函数调用、用户数据管理
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "api/lua_api.h"
#include "api/lua_state.h"
#include "api/userdata.h"
#include "types/tvalue.h"
#include "vm/virtual_machine.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* Lua状态管理契约 */
/* ========================================================================== */

TEST_CASE("API - Lua状态创建和销毁契约", "[api][contract][state]") {
    SECTION("基础状态管理") {
        // 创建新的Lua状态
        lua_State* L = luaL_newstate();
        REQUIRE(L != nullptr);
        
        // 验证初始状态
        REQUIRE(lua_gettop(L) == 0);
        REQUIRE(lua_type(L, 1) == LUA_TNONE);
        REQUIRE(lua_status(L) == LUA_OK);
        
        // 销毁状态
        lua_close(L);
        // 注意：销毁后不能再使用L指针
    }

    SECTION("标准库加载") {
        lua_State* L = luaL_newstate();
        
        // 加载所有标准库
        luaL_openlibs(L);
        
        // 验证全局环境存在
        lua_getglobal(L, "_G");
        REQUIRE(lua_type(L, -1) == LUA_TTABLE);
        
        // 验证基础函数存在
        lua_getglobal(L, "print");
        REQUIRE(lua_type(L, -1) == LUA_TFUNCTION);
        
        lua_getglobal(L, "type");
        REQUIRE(lua_type(L, -1) == LUA_TFUNCTION);
        
        lua_close(L);
    }

    SECTION("自定义分配器") {
        size_t total_allocated = 0;
        
        // 自定义分配器函数
        auto custom_alloc = [](void* ud, void* ptr, size_t osize, size_t nsize) -> void* {
            size_t* total = static_cast<size_t*>(ud);
            *total = *total - osize + nsize;
            
            if (nsize == 0) {
                free(ptr);
                return nullptr;
            } else {
                return realloc(ptr, nsize);
            }
        };
        
        lua_State* L = lua_newstate(custom_alloc, &total_allocated);
        REQUIRE(L != nullptr);
        
        // 分配一些内存
        lua_pushstring(L, "test string");
        lua_newtable(L);
        
        REQUIRE(total_allocated > 0);
        
        lua_close(L);
        REQUIRE(total_allocated == 0); // 所有内存应该被释放
    }

    SECTION("错误状态处理") {
        lua_State* L = luaL_newstate();
        
        // 正常状态
        REQUIRE(lua_status(L) == LUA_OK);
        
        // 模拟运行时错误
        luaL_loadstring(L, "error('test error')");
        int result = lua_pcall(L, 0, 0, 0);
        
        REQUIRE(result == LUA_ERRRUN);
        REQUIRE(lua_status(L) == LUA_OK); // pcall捕获了错误
        
        // 错误消息应该在栈上
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 堆栈操作契约 */
/* ========================================================================== */

TEST_CASE("API - 堆栈操作契约", "[api][contract][stack]") {
    SECTION("基础堆栈操作") {
        lua_State* L = luaL_newstate();
        
        // 初始栈应该为空
        REQUIRE(lua_gettop(L) == 0);
        
        // 推入不同类型的值
        lua_pushnil(L);
        lua_pushboolean(L, 1);
        lua_pushnumber(L, 42.0);
        lua_pushstring(L, "hello");
        
        // 验证栈顶
        REQUIRE(lua_gettop(L) == 4);
        
        // 验证类型
        REQUIRE(lua_type(L, 1) == LUA_TNIL);
        REQUIRE(lua_type(L, 2) == LUA_TBOOLEAN);
        REQUIRE(lua_type(L, 3) == LUA_TNUMBER);
        REQUIRE(lua_type(L, 4) == LUA_TSTRING);
        
        // 负索引访问
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        REQUIRE(lua_type(L, -2) == LUA_TNUMBER);
        REQUIRE(lua_type(L, -3) == LUA_TBOOLEAN);
        REQUIRE(lua_type(L, -4) == LUA_TNIL);
        
        lua_close(L);
    }

    SECTION("堆栈操作函数") {
        lua_State* L = luaL_newstate();
        
        // 推入测试值
        lua_pushstring(L, "a");
        lua_pushstring(L, "b");
        lua_pushstring(L, "c");
        REQUIRE(lua_gettop(L) == 3);
        
        // 复制值
        lua_pushvalue(L, 2); // 复制"b"到栈顶
        REQUIRE(lua_gettop(L) == 4);
        REQUIRE(strcmp(lua_tostring(L, -1), "b") == 0);
        
        // 移除值
        lua_remove(L, 2); // 移除原来的"b"
        REQUIRE(lua_gettop(L) == 3);
        REQUIRE(strcmp(lua_tostring(L, 2), "c") == 0);
        
        // 插入值
        lua_pushstring(L, "x");
        lua_insert(L, 2); // 在位置2插入"x"
        REQUIRE(strcmp(lua_tostring(L, 2), "x") == 0);
        REQUIRE(strcmp(lua_tostring(L, 3), "c") == 0);
        
        // 替换值
        lua_pushstring(L, "y");
        lua_replace(L, 2); // 用"y"替换位置2的值
        REQUIRE(strcmp(lua_tostring(L, 2), "y") == 0);
        
        // 设置栈顶
        lua_settop(L, 1);
        REQUIRE(lua_gettop(L) == 1);
        REQUIRE(strcmp(lua_tostring(L, 1), "a") == 0);
        
        lua_close(L);
    }

    SECTION("堆栈边界检查") {
        lua_State* L = luaL_newstate();
        
        // 无效索引访问
        REQUIRE(lua_type(L, 100) == LUA_TNONE);
        REQUIRE(lua_type(L, -100) == LUA_TNONE);
        
        // 检查栈空间
        REQUIRE(lua_checkstack(L, 100) == 1); // 应该成功
        
        // 尝试分配巨大的栈空间
        REQUIRE(lua_checkstack(L, 1000000) == 0); // 应该失败
        
        lua_close(L);
    }

    SECTION("堆栈迭代和遍历") {
        lua_State* L = luaL_newstate();
        
        // 推入一系列值
        for (int i = 1; i <= 5; ++i) {
            lua_pushnumber(L, i);
        }
        
        // 从底部遍历
        for (int i = 1; i <= 5; ++i) {
            double value = lua_tonumber(L, i);
            REQUIRE(value == Approx(i));
        }
        
        // 从顶部遍历
        for (int i = 1; i <= 5; ++i) {
            double value = lua_tonumber(L, -i);
            REQUIRE(value == Approx(6 - i));
        }
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 类型转换契约 */
/* ========================================================================== */

TEST_CASE("API - 类型转换契约", "[api][contract][conversion]") {
    SECTION("数字类型转换") {
        lua_State* L = luaL_newstate();
        
        // 推入数字
        lua_pushnumber(L, 42.75);
        
        // 各种数字转换
        REQUIRE(lua_tonumber(L, -1) == Approx(42.75));
        REQUIRE(lua_tointeger(L, -1) == 42);
        REQUIRE(lua_toboolean(L, -1) == 1); // 非零数字为真
        
        // 转换为字符串
        const char* str = lua_tostring(L, -1);
        REQUIRE(str != nullptr);
        REQUIRE(strstr(str, "42.75") != nullptr);
        
        // 推入零
        lua_pushnumber(L, 0.0);
        REQUIRE(lua_toboolean(L, -1) == 0); // 零为假
        
        lua_close(L);
    }

    SECTION("字符串类型转换") {
        lua_State* L = luaL_newstate();
        
        // 推入字符串
        lua_pushstring(L, "123.45");
        
        // 字符串到数字转换
        REQUIRE(lua_tonumber(L, -1) == Approx(123.45));
        REQUIRE(lua_tointeger(L, -1) == 123);
        REQUIRE(lua_toboolean(L, -1) == 1); // 非空字符串为真
        
        // 推入非数字字符串
        lua_pushstring(L, "hello");
        REQUIRE(lua_tonumber(L, -1) == 0.0); // 无法转换返回0
        REQUIRE(lua_toboolean(L, -1) == 1);  // 仍然为真
        
        // 推入空字符串
        lua_pushstring(L, "");
        REQUIRE(lua_toboolean(L, -1) == 1); // 空字符串也为真
        
        lua_close(L);
    }

    SECTION("布尔类型转换") {
        lua_State* L = luaL_newstate();
        
        // 推入true
        lua_pushboolean(L, 1);
        REQUIRE(lua_toboolean(L, -1) == 1);
        REQUIRE(lua_tonumber(L, -1) == 1.0);
        
        const char* true_str = lua_tostring(L, -1);
        REQUIRE(strcmp(true_str, "true") == 0);
        
        // 推入false
        lua_pushboolean(L, 0);
        REQUIRE(lua_toboolean(L, -1) == 0);
        REQUIRE(lua_tonumber(L, -1) == 0.0);
        
        const char* false_str = lua_tostring(L, -1);
        REQUIRE(strcmp(false_str, "false") == 0);
        
        lua_close(L);
    }

    SECTION("nil和none类型转换") {
        lua_State* L = luaL_newstate();
        
        // 推入nil
        lua_pushnil(L);
        REQUIRE(lua_type(L, -1) == LUA_TNIL);
        REQUIRE(lua_toboolean(L, -1) == 0); // nil为假
        REQUIRE(lua_tonumber(L, -1) == 0.0);
        REQUIRE(lua_tostring(L, -1) == nullptr); // nil转字符串返回NULL
        
        // 访问不存在的位置
        REQUIRE(lua_type(L, 100) == LUA_TNONE);
        REQUIRE(lua_toboolean(L, 100) == 0);
        REQUIRE(lua_tonumber(L, 100) == 0.0);
        REQUIRE(lua_tostring(L, 100) == nullptr);
        
        lua_close(L);
    }

    SECTION("强制类型转换") {
        lua_State* L = luaL_newstate();
        
        // 推入数字字符串
        lua_pushstring(L, "456");
        
        // 使用luaL_checknumber进行强制转换
        double num = luaL_checknumber(L, -1);
        REQUIRE(num == Approx(456.0));
        
        // 推入非数字字符串
        lua_pushstring(L, "not a number");
        
        // 强制转换应该抛出错误
        REQUIRE_THROWS_AS([&]() {
            luaL_checknumber(L, -1);
        }(), LuaAPIError);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 表操作契约 */
/* ========================================================================== */

TEST_CASE("API - 表操作契约", "[api][contract][table]") {
    SECTION("基础表操作") {
        lua_State* L = luaL_newstate();
        
        // 创建新表
        lua_newtable(L);
        REQUIRE(lua_type(L, -1) == LUA_TTABLE);
        
        // 设置字符串键值对
        lua_pushstring(L, "key1");
        lua_pushstring(L, "value1");
        lua_settable(L, -3); // table[key1] = value1
        
        // 获取值
        lua_pushstring(L, "key1");
        lua_gettable(L, -2);
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        REQUIRE(strcmp(lua_tostring(L, -1), "value1") == 0);
        lua_pop(L, 1);
        
        // 设置数字键值对
        lua_pushnumber(L, 1);
        lua_pushstring(L, "first");
        lua_settable(L, -3); // table[1] = "first"
        
        // 使用rawget/rawset
        lua_pushnumber(L, 2);
        lua_pushstring(L, "second");
        lua_rawset(L, -3); // table[2] = "second" (绕过元方法)
        
        lua_pushnumber(L, 2);
        lua_rawget(L, -2);
        REQUIRE(strcmp(lua_tostring(L, -1), "second") == 0);
        
        lua_close(L);
    }

    SECTION("表的字段访问") {
        lua_State* L = luaL_newstate();
        
        lua_newtable(L);
        
        // 使用lua_setfield/lua_getfield
        lua_pushstring(L, "hello");
        lua_setfield(L, -2, "greeting");
        
        lua_getfield(L, -1, "greeting");
        REQUIRE(strcmp(lua_tostring(L, -1), "hello") == 0);
        lua_pop(L, 1);
        
        // 使用lua_seti/lua_geti (数组风格)
        lua_pushstring(L, "first element");
        lua_seti(L, -2, 1);
        
        lua_geti(L, -1, 1);
        REQUIRE(strcmp(lua_tostring(L, -1), "first element") == 0);
        lua_pop(L, 1);
        
        lua_close(L);
    }

    SECTION("表的迭代") {
        lua_State* L = luaL_newstate();
        
        // 创建包含多个元素的表
        lua_newtable(L);
        
        // 添加一些键值对
        for (int i = 1; i <= 3; ++i) {
            lua_pushnumber(L, i);
            lua_pushstring(L, ("value" + std::to_string(i)).c_str());
            lua_settable(L, -3);
        }
        
        lua_pushstring(L, "string_value");
        lua_setfield(L, -2, "string_key");
        
        // 迭代表
        int count = 0;
        lua_pushnil(L); // 首个键
        while (lua_next(L, -2) != 0) {
            // 现在栈上有: table, key, value
            count++;
            
            // 验证键值对存在
            REQUIRE(lua_type(L, -2) != LUA_TNIL); // 键不为nil
            REQUIRE(lua_type(L, -1) != LUA_TNIL); // 值不为nil
            
            lua_pop(L, 1); // 移除值，保留键用于下次迭代
        }
        
        REQUIRE(count == 4); // 3个数字键 + 1个字符串键
        
        lua_close(L);
    }

    SECTION("表的长度和元信息") {
        lua_State* L = luaL_newstate();
        
        lua_newtable(L);
        
        // 添加连续的数组元素
        for (int i = 1; i <= 5; ++i) {
            lua_pushnumber(L, i * 10);
            lua_seti(L, -2, i);
        }
        
        // 获取表长度
        lua_len(L, -1);
        int length = lua_tointeger(L, -1);
        REQUIRE(length == 5);
        lua_pop(L, 1);
        
        // 使用rawlen获取原始长度
        size_t rawlen = lua_rawlen(L, -1);
        REQUIRE(rawlen == 5);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 函数调用契约 */
/* ========================================================================== */

TEST_CASE("API - 函数调用契约", "[api][contract][function]") {
    SECTION("C函数注册和调用") {
        lua_State* L = luaL_newstate();
        
        // 定义C函数
        auto add_function = [](lua_State* L) -> int {
            double a = luaL_checknumber(L, 1);
            double b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a + b);
            return 1; // 返回1个值
        };
        
        // 注册函数
        lua_pushcfunction(L, add_function);
        lua_setglobal(L, "add");
        
        // 调用函数
        lua_getglobal(L, "add");
        lua_pushnumber(L, 10);
        lua_pushnumber(L, 20);
        
        int result = lua_pcall(L, 2, 1, 0); // 2个参数，1个返回值
        REQUIRE(result == LUA_OK);
        
        // 检查结果
        REQUIRE(lua_type(L, -1) == LUA_TNUMBER);
        REQUIRE(lua_tonumber(L, -1) == Approx(30.0));
        
        lua_close(L);
    }

    SECTION("Lua函数执行") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 加载Lua代码
        const char* code = R"(
            function multiply(x, y)
                return x * y
            end
            
            function factorial(n)
                if n <= 1 then
                    return 1
                else
                    return n * factorial(n - 1)
                end
            end
        )";
        
        int load_result = luaL_loadstring(L, code);
        REQUIRE(load_result == LUA_OK);
        
        int exec_result = lua_pcall(L, 0, 0, 0);
        REQUIRE(exec_result == LUA_OK);
        
        // 调用multiply函数
        lua_getglobal(L, "multiply");
        lua_pushnumber(L, 6);
        lua_pushnumber(L, 7);
        
        int call_result = lua_pcall(L, 2, 1, 0);
        REQUIRE(call_result == LUA_OK);
        REQUIRE(lua_tonumber(L, -1) == Approx(42.0));
        lua_pop(L, 1);
        
        // 调用factorial函数
        lua_getglobal(L, "factorial");
        lua_pushnumber(L, 5);
        
        call_result = lua_pcall(L, 1, 1, 0);
        REQUIRE(call_result == LUA_OK);
        REQUIRE(lua_tonumber(L, -1) == Approx(120.0));
        
        lua_close(L);
    }

    SECTION("错误处理和保护调用") {
        lua_State* L = luaL_newstate();
        
        // 定义会出错的C函数
        auto error_function = [](lua_State* L) -> int {
            return luaL_error(L, "This is a test error");
        };
        
        lua_pushcfunction(L, error_function);
        lua_setglobal(L, "error_func");
        
        // 使用保护调用
        lua_getglobal(L, "error_func");
        int result = lua_pcall(L, 0, 0, 0);
        
        REQUIRE(result == LUA_ERRRUN);
        REQUIRE(lua_type(L, -1) == LUA_TSTRING); // 错误消息
        
        const char* error_msg = lua_tostring(L, -1);
        REQUIRE(strstr(error_msg, "test error") != nullptr);
        
        lua_close(L);
    }

    SECTION("协程和yield") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 创建协程
        lua_State* co = lua_newthread(L);
        REQUIRE(co != nullptr);
        REQUIRE(lua_type(L, -1) == LUA_TTHREAD);
        
        // 在协程中加载函数
        const char* coroutine_code = R"(
            function coroutine_func()
                coroutine.yield(1)
                coroutine.yield(2)
                return 3
            end
            return coroutine_func
        )";
        
        int load_result = luaL_loadstring(co, coroutine_code);
        REQUIRE(load_result == LUA_OK);
        
        // 执行以获取函数
        int exec_result = lua_resume(co, nullptr, 0);
        REQUIRE(exec_result == LUA_OK);
        
        // 开始执行协程函数
        lua_pushvalue(co, -1); // 复制函数
        exec_result = lua_resume(co, nullptr, 0);
        REQUIRE(exec_result == LUA_YIELD);
        REQUIRE(lua_tonumber(co, -1) == Approx(1.0));
        
        // 继续执行
        exec_result = lua_resume(co, nullptr, 0);
        REQUIRE(exec_result == LUA_YIELD);
        REQUIRE(lua_tonumber(co, -1) == Approx(2.0));
        
        // 最后一次执行
        exec_result = lua_resume(co, nullptr, 0);
        REQUIRE(exec_result == LUA_OK);
        REQUIRE(lua_tonumber(co, -1) == Approx(3.0));
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 用户数据契约 */
/* ========================================================================== */

TEST_CASE("API - 用户数据契约", "[api][contract][userdata]") {
    SECTION("基础用户数据操作") {
        lua_State* L = luaL_newstate();
        
        // 创建用户数据
        void* userdata = lua_newuserdata(L, sizeof(int));
        REQUIRE(userdata != nullptr);
        REQUIRE(lua_type(L, -1) == LUA_TUSERDATA);
        
        // 设置用户数据内容
        *static_cast<int*>(userdata) = 42;
        
        // 获取用户数据
        void* retrieved = lua_touserdata(L, -1);
        REQUIRE(retrieved == userdata);
        REQUIRE(*static_cast<int*>(retrieved) == 42);
        
        // 检查用户数据大小
        size_t size = lua_rawlen(L, -1);
        REQUIRE(size == sizeof(int));
        
        lua_close(L);
    }

    SECTION("轻量用户数据") {
        lua_State* L = luaL_newstate();
        
        int value = 123;
        
        // 创建轻量用户数据
        lua_pushlightuserdata(L, &value);
        REQUIRE(lua_type(L, -1) == LUA_TLIGHTUSERDATA);
        
        // 获取轻量用户数据
        void* retrieved = lua_touserdata(L, -1);
        REQUIRE(retrieved == &value);
        REQUIRE(*static_cast<int*>(retrieved) == 123);
        
        lua_close(L);
    }

    SECTION("用户数据元表") {
        lua_State* L = luaL_newstate();
        
        // 创建用户数据
        void* userdata = lua_newuserdata(L, sizeof(double));
        *static_cast<double*>(userdata) = 3.14;
        
        // 创建元表
        lua_newtable(L);
        
        // 设置__tostring元方法
        auto tostring_meta = [](lua_State* L) -> int {
            double* value = static_cast<double*>(lua_touserdata(L, 1));
            lua_pushstring(L, ("UserData: " + std::to_string(*value)).c_str());
            return 1;
        };
        
        lua_pushcfunction(L, tostring_meta);
        lua_setfield(L, -2, "__tostring");
        
        // 设置元表
        lua_setmetatable(L, -2);
        
        // 测试元方法
        lua_getglobal(L, "tostring");
        lua_pushvalue(L, -2); // 推入用户数据
        lua_call(L, 1, 1);
        
        const char* result = lua_tostring(L, -1);
        REQUIRE(strstr(result, "UserData: 3.14") != nullptr);
        
        lua_close(L);
    }

    SECTION("用户数据类型检查") {
        lua_State* L = luaL_newstate();
        
        // 定义用户数据类型名
        const char* mt_name = "MyType";
        
        // 创建元表并注册
        luaL_newmetatable(L, mt_name);
        lua_pop(L, 1);
        
        // 创建用户数据并设置元表
        void* userdata = lua_newuserdata(L, sizeof(int));
        luaL_getmetatable(L, mt_name);
        lua_setmetatable(L, -2);
        
        // 类型检查
        void* checked = luaL_checkudata(L, -1, mt_name);
        REQUIRE(checked == userdata);
        
        // 尝试错误的类型检查
        lua_pushstring(L, "not userdata");
        REQUIRE_THROWS_AS([&]() {
            luaL_checkudata(L, -1, mt_name);
        }(), LuaAPIError);
        
        lua_close(L);
    }

    SECTION("用户数据终结器") {
        lua_State* L = luaL_newstate();
        
        bool finalizer_called = false;
        
        // 创建用户数据
        bool* flag = static_cast<bool*>(lua_newuserdata(L, sizeof(bool)));
        *flag = false;
        
        // 创建元表和终结器
        lua_newtable(L);
        
        // 捕获finalizer_called的引用
        lua_pushlightuserdata(L, &finalizer_called);
        lua_pushcclosure(L, [](lua_State* L) -> int {
            bool* flag_ptr = static_cast<bool*>(lua_touserdata(L, lua_upvalueindex(1)));
            *flag_ptr = true;
            return 0;
        }, 1);
        lua_setfield(L, -2, "__gc");
        
        lua_setmetatable(L, -2);
        
        // 移除对用户数据的引用
        lua_pop(L, 1);
        
        // 强制垃圾回收
        lua_gc(L, LUA_GCCOLLECT, 0);
        
        // 终结器应该被调用
        REQUIRE(finalizer_called);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 模块和库契约 */
/* ========================================================================== */

TEST_CASE("API - 模块和库契约", "[api][contract][module]") {
    SECTION("C模块注册") {
        lua_State* L = luaL_newstate();
        
        // 定义模块函数
        auto module_add = [](lua_State* L) -> int {
            double a = luaL_checknumber(L, 1);
            double b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a + b);
            return 1;
        };
        
        auto module_multiply = [](lua_State* L) -> int {
            double a = luaL_checknumber(L, 1);
            double b = luaL_checknumber(L, 2);
            lua_pushnumber(L, a * b);
            return 1;
        };
        
        // 模块函数表
        const luaL_Reg module_functions[] = {
            {"add", module_add},
            {"multiply", module_multiply},
            {nullptr, nullptr}
        };
        
        // 注册模块
        luaL_newlib(L, module_functions);
        lua_setglobal(L, "math_module");
        
        // 测试模块函数
        luaL_dostring(L, R"(
            result1 = math_module.add(10, 20)
            result2 = math_module.multiply(6, 7)
        )");
        
        lua_getglobal(L, "result1");
        REQUIRE(lua_tonumber(L, -1) == Approx(30.0));
        
        lua_getglobal(L, "result2");
        REQUIRE(lua_tonumber(L, -1) == Approx(42.0));
        
        lua_close(L);
    }

    SECTION("包路径和require") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 获取package.path
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "path");
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        
        const char* package_path = lua_tostring(L, -1);
        REQUIRE(strlen(package_path) > 0);
        
        lua_close(L);
    }

    SECTION("预加载模块") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 定义预加载函数
        auto preload_module = [](lua_State* L) -> int {
            lua_newtable(L);
            lua_pushstring(L, "Hello from preloaded module!");
            lua_setfield(L, -2, "message");
            return 1;
        };
        
        // 注册到package.preload
        lua_getglobal(L, "package");
        lua_getfield(L, -1, "preload");
        lua_pushcfunction(L, preload_module);
        lua_setfield(L, -2, "mymodule");
        lua_pop(L, 2); // 移除package和preload表
        
        // 使用require加载模块
        luaL_dostring(L, R"(
            local mymod = require('mymodule')
            test_message = mymod.message
        )");
        
        lua_getglobal(L, "test_message");
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        REQUIRE(strcmp(lua_tostring(L, -1), "Hello from preloaded module!") == 0);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 引用系统契约 */
/* ========================================================================== */

TEST_CASE("API - 引用系统契约", "[api][contract][reference]") {
    SECTION("基础引用操作") {
        lua_State* L = luaL_newstate();
        
        // 创建一个表
        lua_newtable(L);
        lua_pushstring(L, "test_value");
        lua_setfield(L, -2, "key");
        
        // 创建引用
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);
        REQUIRE(ref != LUA_REFNIL);
        REQUIRE(ref != LUA_NOREF);
        
        // 现在栈应该是空的
        REQUIRE(lua_gettop(L) == 0);
        
        // 通过引用获取对象
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
        REQUIRE(lua_type(L, -1) == LUA_TTABLE);
        
        lua_getfield(L, -1, "key");
        REQUIRE(strcmp(lua_tostring(L, -1), "test_value") == 0);
        lua_pop(L, 2);
        
        // 释放引用
        luaL_unref(L, LUA_REGISTRYINDEX, ref);
        
        lua_close(L);
    }

    SECTION("弱引用表") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 创建弱引用表
        luaL_dostring(L, R"(
            weak_table = {}
            setmetatable(weak_table, {__mode = "v"}) -- 弱值引用
            
            -- 添加一些对象
            weak_table[1] = {name = "object1"}
            weak_table[2] = {name = "object2"}
            
            -- 保持对第一个对象的强引用
            strong_ref = weak_table[1]
        )");
        
        // 触发垃圾回收
        lua_gc(L, LUA_GCCOLLECT, 0);
        
        // 检查弱引用表的内容
        luaL_dostring(L, R"(
            count = 0
            for k, v in pairs(weak_table) do
                count = count + 1
            end
        )");
        
        lua_getglobal(L, "count");
        int count = lua_tointeger(L, -1);
        
        // 只有有强引用的对象应该保留
        REQUIRE(count == 1);
        
        lua_close(L);
    }

    SECTION("注册表操作") {
        lua_State* L = luaL_newstate();
        
        // 在注册表中存储值
        lua_pushstring(L, "registry_value");
        lua_setfield(L, LUA_REGISTRYINDEX, "my_key");
        
        // 从注册表获取值
        lua_getfield(L, LUA_REGISTRYINDEX, "my_key");
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        REQUIRE(strcmp(lua_tostring(L, -1), "registry_value") == 0);
        lua_pop(L, 1);
        
        // 使用整数键
        lua_pushstring(L, "integer_key_value");
        lua_rawseti(L, LUA_REGISTRYINDEX, 12345);
        
        lua_rawgeti(L, LUA_REGISTRYINDEX, 12345);
        REQUIRE(strcmp(lua_tostring(L, -1), "integer_key_value") == 0);
        lua_pop(L, 1);
        
        // 清理注册表
        lua_pushnil(L);
        lua_setfield(L, LUA_REGISTRYINDEX, "my_key");
        
        lua_pushnil(L);
        lua_rawseti(L, LUA_REGISTRYINDEX, 12345);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 调试和诊断契约 */
/* ========================================================================== */

TEST_CASE("API - 调试和诊断契约", "[api][contract][debug]") {
    SECTION("调试信息获取") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 加载有调试信息的代码
        const char* code = R"(
            function test_function(a, b)
                local c = a + b
                return c * 2
            end
            
            test_function(10, 20)
        )";
        
        luaL_loadstring(L, code);
        lua_pcall(L, 0, 0, 0);
        
        // 获取调试信息
        lua_Debug debug_info;
        int level = 0;
        while (lua_getstack(L, level, &debug_info)) {
            lua_getinfo(L, "Sln", &debug_info);
            
            // 验证调试信息字段
            if (debug_info.name) {
                REQUIRE(strlen(debug_info.name) > 0);
            }
            if (debug_info.source) {
                REQUIRE(strlen(debug_info.source) > 0);
            }
            REQUIRE(debug_info.currentline >= 0);
            
            level++;
        }
        
        lua_close(L);
    }

    SECTION("错误回溯") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 设置错误处理函数
        lua_pushcfunction(L, [](lua_State* L) -> int {
            luaL_traceback(L, L, lua_tostring(L, 1), 1);
            return 1;
        });
        
        // 执行会出错的代码
        const char* error_code = R"(
            function level3()
                error("Test error at level 3")
            end
            
            function level2()
                level3()
            end
            
            function level1()
                level2()
            end
            
            level1()
        )";
        
        luaL_loadstring(L, error_code);
        int result = lua_pcall(L, 0, 0, 1); // 使用错误处理函数
        
        REQUIRE(result != LUA_OK);
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        
        const char* traceback = lua_tostring(L, -1);
        REQUIRE(strstr(traceback, "level1") != nullptr);
        REQUIRE(strstr(traceback, "level2") != nullptr);
        REQUIRE(strstr(traceback, "level3") != nullptr);
        
        lua_close(L);
    }

    SECTION("钩子函数") {
        lua_State* L = luaL_newstate();
        
        int hook_count = 0;
        
        // 设置调试钩子
        lua_sethook(L, [](lua_State* L, lua_Debug* ar) {
            int* count = static_cast<int*>(lua_touserdata(L, lua_upvalueindex(1)));
            (*count)++;
        }, LUA_MASKLINE, 0);
        
        // 将计数器存储为upvalue
        lua_pushlightuserdata(L, &hook_count);
        lua_setupvalue(L, -2, 1);
        
        // 执行一些代码
        luaL_dostring(L, R"(
            local x = 1
            local y = 2
            local z = x + y
        )");
        
        // 钩子应该被调用多次
        REQUIRE(hook_count > 0);
        
        // 移除钩子
        lua_sethook(L, nullptr, 0, 0);
        
        lua_close(L);
    }

    SECTION("内存和性能监控") {
        lua_State* L = luaL_newstate();
        
        // 获取初始内存使用
        int initial_memory = lua_gc(L, LUA_GCCOUNT, 0);
        
        // 分配一些内存
        for (int i = 0; i < 100; ++i) {
            lua_pushstring(L, ("string_" + std::to_string(i)).c_str());
        }
        
        // 检查内存增长
        int current_memory = lua_gc(L, LUA_GCCOUNT, 0);
        REQUIRE(current_memory > initial_memory);
        
        // 手动触发GC
        lua_gc(L, LUA_GCCOLLECT, 0);
        
        // 内存使用应该减少
        int after_gc_memory = lua_gc(L, LUA_GCCOUNT, 0);
        REQUIRE(after_gc_memory <= current_memory);
        
        lua_close(L);
    }
}

/* ========================================================================== */
/* 兼容性和边界情况契约 */
/* ========================================================================== */

TEST_CASE("API - 兼容性和边界情况契约", "[api][contract][compatibility]") {
    SECTION("Lua 5.1兼容性") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 测试Lua 5.1特有的特性
        luaL_dostring(L, R"(
            -- 测试setfenv/getfenv
            function test_func() return x end
            env = {x = 42}
            setfenv(test_func, env)
            result = test_func()
        )");
        
        lua_getglobal(L, "result");
        REQUIRE(lua_tonumber(L, -1) == Approx(42.0));
        
        // 测试module函数
        luaL_dostring(L, R"(
            module("testmod", package.seeall)
            function hello()
                return "Hello from module"
            end
        )");
        
        lua_getglobal(L, "testmod");
        REQUIRE(lua_type(L, -1) == LUA_TTABLE);
        
        lua_close(L);
    }

    SECTION("大数据处理") {
        lua_State* L = luaL_newstate();
        
        // 测试大字符串
        std::string large_string(1000000, 'x'); // 1MB字符串
        lua_pushstring(L, large_string.c_str());
        REQUIRE(lua_type(L, -1) == LUA_TSTRING);
        REQUIRE(lua_rawlen(L, -1) == 1000000);
        
        // 测试大表
        lua_newtable(L);
        for (int i = 0; i < 10000; ++i) {
            lua_pushnumber(L, i);
            lua_pushnumber(L, i * i);
            lua_settable(L, -3);
        }
        
        // 验证表内容
        lua_pushnumber(L, 9999);
        lua_gettable(L, -2);
        REQUIRE(lua_tonumber(L, -1) == Approx(9999 * 9999));
        
        lua_close(L);
    }

    SECTION("错误边界情况") {
        lua_State* L = luaL_newstate();
        
        // 栈溢出保护
        REQUIRE_NOTHROW([&]() {
            for (int i = 0; i < 1000000; ++i) {
                if (!lua_checkstack(L, 1)) {
                    break; // 栈空间不足，正常退出
                }
                lua_pushnil(L);
            }
        }());
        
        // 清理栈
        lua_settop(L, 0);
        
        // 无限递归保护
        luaL_dostring(L, R"(
            function recursive_func()
                return recursive_func()
            end
        )");
        
        lua_getglobal(L, "recursive_func");
        int result = lua_pcall(L, 0, 0, 0);
        REQUIRE(result != LUA_OK); // 应该因为栈溢出而失败
        
        lua_close(L);
    }

    SECTION("Unicode和多字节字符") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 测试UTF-8字符串
        const char* utf8_string = "Hello 世界 🌍";
        lua_pushstring(L, utf8_string);
        
        const char* retrieved = lua_tostring(L, -1);
        REQUIRE(strcmp(retrieved, utf8_string) == 0);
        
        // 测试字符串长度（字节长度，不是字符长度）
        size_t len = lua_rawlen(L, -1);
        REQUIRE(len > 10); // UTF-8编码的字节长度
        
        lua_close(L);
    }

    SECTION("平台特定特性") {
        lua_State* L = luaL_newstate();
        luaL_openlibs(L);
        
        // 测试数字精度
        lua_pushnumber(L, 1.7976931348623157e+308); // 接近double最大值
        REQUIRE(lua_type(L, -1) == LUA_TNUMBER);
        REQUIRE(lua_tonumber(L, -1) > 1e+300);
        
        // 测试整数边界
        lua_pushinteger(L, LLONG_MAX);
        REQUIRE(lua_type(L, -1) == LUA_TNUMBER);
        
        lua_pushinteger(L, LLONG_MIN);
        REQUIRE(lua_type(L, -1) == LUA_TNUMBER);
        
        lua_close(L);
    }
}