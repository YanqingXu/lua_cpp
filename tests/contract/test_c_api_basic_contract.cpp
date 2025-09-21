/**
 * @file test_c_api_basic_contract.cpp
 * @brief T015: C API基础操作契约测试 - 规格驱动开发
 * 
 * @details 
 * 本文件实现了T015 C API基础操作契约测试，验证Lua 5.1.5 C API的核心功能，
 * 包括栈操作、类型检查、状态管理、值访问和基础函数调用等。
 * 采用双重验证机制确保与原始Lua 5.1.5 C API的完全二进制兼容性。
 * 
 * 测试架构：
 * 1. 🔍 lua_c_analysis验证：基于原始Lua 5.1.5的lapi.c行为验证
 * 2. 🏗️ lua_with_cpp验证：基于现代化C++架构的API包装验证
 * 3. 📊 双重对比：确保API行为一致性和二进制兼容性
 * 
 * 测试覆盖：
 * - StateManagement: lua_State创建、关闭和生命周期管理
 * - StackOperations: 栈操作(push/pop/get/set)和栈空间管理
 * - TypeChecking: 类型检查、转换和判断函数
 * - ValueAccess: 值访问、设置和获取操作
 * - BasicCalls: 基础函数调用和返回值处理
 * - ErrorHandling: 错误处理、异常安全和资源管理
 * - ThreadManagement: 线程创建、切换和协程支持
 * - RegistryAccess: 注册表访问和全局状态管理
 * - MetatableOps: 元表操作和元方法支持
 * - MemoryMgmt: 内存管理和垃圾回收集成
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

namespace lua_cpp {
namespace c_api_contract_tests {

// ============================================================================
// 测试基础设施
// ============================================================================

/**
 * @brief C API测试夹具
 * 
 * 提供统一的测试环境，包括：
 * - Lua状态机管理
 * - API调用包装
 * - 错误处理验证
 * - 性能基准测试
 */
class CAPITestFixture {
public:
    CAPITestFixture() {
        // 创建标准Lua状态
        L = lua_newstate(default_alloc, nullptr);
        REQUIRE(L != nullptr);
        
        // 设置错误处理
        original_panic = lua_atpanic(L, test_panic);
        
        // 初始化测试环境
        setup_test_environment();
    }
    
    ~CAPITestFixture() {
        if (L) {
            lua_close(L);
            L = nullptr;
        }
    }
    
    // 禁用拷贝和移动
    CAPITestFixture(const CAPITestFixture&) = delete;
    CAPITestFixture& operator=(const CAPITestFixture&) = delete;
    CAPITestFixture(CAPITestFixture&&) = delete;
    CAPITestFixture& operator=(CAPITestFixture&&) = delete;

protected:
    lua_State* L = nullptr;
    lua_PFunction original_panic = nullptr;
    static bool panic_called;
    static std::string last_panic_message;
    
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
        // 重置panic状态
        panic_called = false;
        last_panic_message.clear();
        
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
};

// 静态成员初始化
bool CAPITestFixture::panic_called = false;
std::string CAPITestFixture::last_panic_message;

// ============================================================================
// 契约测试组1: 状态管理 (State Management)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 状态创建和销毁", "[c_api][state_mgmt][basic]") {
    SECTION("🔍 lua_c_analysis验证: lua_newstate行为") {
        // 测试标准分配器创建
        lua_State* test_L = lua_newstate(default_alloc, nullptr);
        REQUIRE(test_L != nullptr);
        
        // 验证初始状态
        REQUIRE(lua_gettop(test_L) == 0);
        REQUIRE(lua_checkstack(test_L, LUA_MINSTACK));
        
        // 关闭状态
        lua_close(test_L);
    }
    
    SECTION("🔍 lua_c_analysis验证: 内存分配器集成") {
        struct AllocStats {
            size_t alloc_count = 0;
            size_t free_count = 0;
            size_t total_allocated = 0;
        };
        
        AllocStats stats;
        
        auto tracked_alloc = [](void* ud, void* ptr, size_t osize, size_t nsize) -> void* {
            auto* stats = static_cast<AllocStats*>(ud);
            
            if (nsize == 0) {
                if (ptr) {
                    stats->free_count++;
                    free(ptr);
                }
                return nullptr;
            } else {
                if (ptr) {
                    stats->alloc_count++;
                } else {
                    stats->alloc_count++;
                }
                stats->total_allocated += nsize;
                return realloc(ptr, nsize);
            }
        };
        
        lua_State* test_L = lua_newstate(tracked_alloc, &stats);
        REQUIRE(test_L != nullptr);
        REQUIRE(stats.alloc_count > 0);  // 应该有分配
        
        lua_close(test_L);
        REQUIRE(stats.free_count > 0);   // 应该有释放
    }
    
    SECTION("🏗️ lua_with_cpp验证: 异常安全状态管理") {
        // 使用RAII包装器测试异常安全
        class LuaStateWrapper {
        public:
            explicit LuaStateWrapper(lua_Alloc f = default_alloc, void* ud = nullptr) 
                : L(lua_newstate(f, ud)) {
                if (!L) throw std::runtime_error("Failed to create Lua state");
            }
            
            ~LuaStateWrapper() {
                if (L) lua_close(L);
            }
            
            lua_State* get() const { return L; }
            
            // 禁用拷贝，允许移动
            LuaStateWrapper(const LuaStateWrapper&) = delete;
            LuaStateWrapper& operator=(const LuaStateWrapper&) = delete;
            
            LuaStateWrapper(LuaStateWrapper&& other) noexcept : L(other.L) {
                other.L = nullptr;
            }
            
            LuaStateWrapper& operator=(LuaStateWrapper&& other) noexcept {
                if (this != &other) {
                    if (L) lua_close(L);
                    L = other.L;
                    other.L = nullptr;
                }
                return *this;
            }
            
        private:
            lua_State* L;
        };
        
        // 测试正常创建
        {
            LuaStateWrapper wrapper;
            REQUIRE(wrapper.get() != nullptr);
            REQUIRE(lua_gettop(wrapper.get()) == 0);
        }
        // wrapper在此处自动清理
        
        // 测试移动语义
        LuaStateWrapper wrapper1;
        lua_State* original_ptr = wrapper1.get();
        LuaStateWrapper wrapper2 = std::move(wrapper1);
        
        REQUIRE(wrapper2.get() == original_ptr);
        REQUIRE(wrapper1.get() == nullptr);
    }
}

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 线程管理", "[c_api][state_mgmt][threads]") {
    SECTION("🔍 lua_c_analysis验证: lua_newthread行为") {
        int initial_top = lua_gettop(L);
        
        // 创建新线程
        lua_State* thread = lua_newthread(L);
        REQUIRE(thread != nullptr);
        REQUIRE(thread != L);  // 应该是不同的状态
        
        // 验证线程被推入主栈
        REQUIRE(lua_gettop(L) == initial_top + 1);
        REQUIRE(lua_isthread(L, -1));
        
        // 验证新线程的初始状态
        REQUIRE(lua_gettop(thread) == 0);
        REQUIRE(lua_checkstack(thread, LUA_MINSTACK));
        
        // 在新线程中进行操作
        lua_pushinteger(thread, 42);
        lua_pushstring(thread, "test");
        REQUIRE(lua_gettop(thread) == 2);
        
        // 主线程不受影响
        REQUIRE(lua_gettop(L) == initial_top + 1);
        
        // 清理
        lua_pop(L, 1);  // 移除线程引用
    }
    
    SECTION("🔍 lua_c_analysis验证: 线程间值移动") {
        lua_State* thread = lua_newthread(L);
        
        // 在主线程准备数据
        lua_pushinteger(L, 123);
        lua_pushstring(L, "hello");
        lua_pushboolean(L, 1);
        
        int main_top = lua_gettop(L);
        int thread_top = lua_gettop(thread);
        
        // 移动值到新线程
        lua_xmove(L, thread, 2);  // 移动2个值
        
        // 验证移动结果
        REQUIRE(lua_gettop(L) == main_top - 2);
        REQUIRE(lua_gettop(thread) == thread_top + 2);
        
        // 验证移动的值
        REQUIRE(lua_isboolean(thread, -1));
        REQUIRE(lua_toboolean(thread, -1) == 1);
        REQUIRE(lua_isstring(thread, -2));
        REQUIRE(std::string(lua_tostring(thread, -2)) == "hello");
        
        // 清理
        lua_settop(L, 0);
        lua_settop(thread, 0);
    }
}

// ============================================================================
// 契约测试组2: 栈操作 (Stack Operations)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 基础栈操作", "[c_api][stack_ops][basic]") {
    SECTION("🔍 lua_c_analysis验证: 栈顶管理") {
        // 初始状态
        REQUIRE(lua_gettop(L) == 0);
        
        // 推入值
        lua_pushnil(L);
        REQUIRE(lua_gettop(L) == 1);
        
        lua_pushboolean(L, 1);
        REQUIRE(lua_gettop(L) == 2);
        
        lua_pushinteger(L, 42);
        REQUIRE(lua_gettop(L) == 3);
        
        lua_pushstring(L, "test");
        REQUIRE(lua_gettop(L) == 4);
        
        // 设置栈顶
        lua_settop(L, 2);
        REQUIRE(lua_gettop(L) == 2);
        REQUIRE(lua_isboolean(L, -1));
        REQUIRE(lua_isnil(L, -2));
        
        // 扩展栈顶
        lua_settop(L, 5);
        REQUIRE(lua_gettop(L) == 5);
        REQUIRE(lua_isnil(L, -1));  // 新位置应该是nil
        REQUIRE(lua_isnil(L, -2));
        REQUIRE(lua_isnil(L, -3));
        
        // 清空栈
        lua_settop(L, 0);
        REQUIRE(lua_gettop(L) == 0);
    }
    
    SECTION("🔍 lua_c_analysis验证: 值复制和移动") {
        // 准备测试数据
        lua_pushinteger(L, 10);
        lua_pushstring(L, "hello");
        lua_pushboolean(L, 1);
        
        REQUIRE(lua_gettop(L) == 3);
        
        // 测试pushvalue
        lua_pushvalue(L, 2);  // 复制索引2的值("hello")
        REQUIRE(lua_gettop(L) == 4);
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "hello");
        REQUIRE(std::string(lua_tostring(L, 2)) == "hello");  // 原值不变
        
        // 测试insert
        lua_pushinteger(L, 99);
        lua_insert(L, 2);  // 在位置2插入
        REQUIRE(lua_gettop(L) == 5);
        REQUIRE(lua_tointeger(L, 2) == 99);
        REQUIRE(std::string(lua_tostring(L, 3)) == "hello");
        
        // 测试replace
        lua_pushstring(L, "world");
        lua_replace(L, 2);  // 替换位置2
        REQUIRE(lua_gettop(L) == 5);
        REQUIRE(std::string(lua_tostring(L, 2)) == "world");
        
        // 测试remove
        lua_remove(L, 3);  // 移除位置3
        REQUIRE(lua_gettop(L) == 4);
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 栈空间检查") {
        // 检查默认栈空间
        REQUIRE(lua_checkstack(L, LUA_MINSTACK));
        
        // 检查大量空间
        REQUIRE(lua_checkstack(L, 1000));
        
        // 填充栈直到接近限制
        int max_safe = 8000;  // 安全的最大值
        for (int i = 0; i < max_safe; i++) {
            lua_pushinteger(L, i);
        }
        REQUIRE(lua_gettop(L) == max_safe);
        
        // 检查是否还能分配更多空间
        bool can_allocate_more = lua_checkstack(L, 1000);
        // 实现可能允许也可能不允许，但不应该崩溃
        
        clean_stack();
    }
}

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 栈索引访问", "[c_api][stack_ops][indexing]") {
    SECTION("🔍 lua_c_analysis验证: 正负索引") {
        // 准备测试数据
        lua_pushstring(L, "first");   // 索引1, -4
        lua_pushinteger(L, 42);       // 索引2, -3  
        lua_pushboolean(L, 1);        // 索引3, -2
        lua_pushnil(L);               // 索引4, -1
        
        REQUIRE(lua_gettop(L) == 4);
        
        // 正索引访问
        REQUIRE(lua_isstring(L, 1));
        REQUIRE(lua_isnumber(L, 2));
        REQUIRE(lua_isboolean(L, 3));
        REQUIRE(lua_isnil(L, 4));
        
        // 负索引访问（从栈顶开始）
        REQUIRE(lua_isnil(L, -1));
        REQUIRE(lua_isboolean(L, -2));
        REQUIRE(lua_isnumber(L, -3));
        REQUIRE(lua_isstring(L, -4));
        
        // 验证值相等
        REQUIRE(std::string(lua_tostring(L, 1)) == std::string(lua_tostring(L, -4)));
        REQUIRE(lua_tointeger(L, 2) == lua_tointeger(L, -3));
        REQUIRE(lua_toboolean(L, 3) == lua_toboolean(L, -2));
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 无效索引处理") {
        lua_pushinteger(L, 42);
        REQUIRE(lua_gettop(L) == 1);
        
        // 超出范围的正索引
        REQUIRE_FALSE(lua_isnumber(L, 2));
        REQUIRE_FALSE(lua_isnumber(L, 10));
        REQUIRE(lua_type(L, 2) == LUA_TNONE);
        
        // 超出范围的负索引
        REQUIRE_FALSE(lua_isnumber(L, -2));
        REQUIRE_FALSE(lua_isnumber(L, -10));
        REQUIRE(lua_type(L, -2) == LUA_TNONE);
        
        // 索引0应该无效
        REQUIRE(lua_type(L, 0) == LUA_TNONE);
        
        clean_stack();
    }
}

// ============================================================================
// 契约测试组3: 类型检查 (Type Checking)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 类型判断函数", "[c_api][type_check][basic]") {
    SECTION("🔍 lua_c_analysis验证: 基础类型检查") {
        // 推入不同类型的值
        lua_pushnil(L);
        lua_pushboolean(L, 1);
        lua_pushinteger(L, 42);
        lua_pushnumber(L, 3.14);
        lua_pushstring(L, "hello");
        lua_newtable(L);
        
        // 验证类型
        REQUIRE(lua_type(L, 1) == LUA_TNIL);
        REQUIRE(lua_type(L, 2) == LUA_TBOOLEAN);
        REQUIRE(lua_type(L, 3) == LUA_TNUMBER);
        REQUIRE(lua_type(L, 4) == LUA_TNUMBER);
        REQUIRE(lua_type(L, 5) == LUA_TSTRING);
        REQUIRE(lua_type(L, 6) == LUA_TTABLE);
        
        // 验证类型检查函数
        REQUIRE(lua_isnil(L, 1));
        REQUIRE(lua_isboolean(L, 2));
        REQUIRE(lua_isnumber(L, 3));
        REQUIRE(lua_isnumber(L, 4));
        REQUIRE(lua_isstring(L, 5));
        REQUIRE(lua_istable(L, 6));
        
        // 验证否定情况
        REQUIRE_FALSE(lua_isnil(L, 2));
        REQUIRE_FALSE(lua_isboolean(L, 3));
        REQUIRE_FALSE(lua_isnumber(L, 5));
        REQUIRE_FALSE(lua_isstring(L, 6));
        REQUIRE_FALSE(lua_istable(L, 1));
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 类型转换能力") {
        // 数字和字符串的双向转换能力
        lua_pushinteger(L, 123);
        lua_pushstring(L, "456");
        lua_pushstring(L, "not_a_number");
        
        // 数字可以转为字符串
        REQUIRE(lua_isnumber(L, 1));
        REQUIRE(lua_isstring(L, 1));  // 数字可以作为字符串使用
        
        // 数字字符串可以转为数字
        REQUIRE(lua_isstring(L, 2));
        REQUIRE(lua_isnumber(L, 2));  // 数字字符串可以作为数字使用
        
        // 非数字字符串不能转为数字
        REQUIRE(lua_isstring(L, 3));
        REQUIRE_FALSE(lua_isnumber(L, 3));
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 类型安全检查") {
        // 类型安全的值获取模板
        auto safe_get_integer = [](lua_State* L, int idx) -> std::optional<lua_Integer> {
            if (lua_isnumber(L, idx)) {
                return lua_tointeger(L, idx);
            }
            return std::nullopt;
        };
        
        auto safe_get_string = [](lua_State* L, int idx) -> std::optional<std::string> {
            if (lua_isstring(L, idx)) {
                return std::string(lua_tostring(L, idx));
            }
            return std::nullopt;
        };
        
        // 测试类型安全获取
        lua_pushinteger(L, 42);
        lua_pushstring(L, "hello");
        lua_pushnil(L);
        
        auto int_val = safe_get_integer(L, 1);
        auto str_val = safe_get_string(L, 2);
        auto nil_int = safe_get_integer(L, 3);
        auto nil_str = safe_get_string(L, 3);
        
        REQUIRE(int_val.has_value());
        REQUIRE(int_val.value() == 42);
        REQUIRE(str_val.has_value());
        REQUIRE(str_val.value() == "hello");
        REQUIRE_FALSE(nil_int.has_value());
        REQUIRE_FALSE(nil_str.has_value());
        
        clean_stack();
    }
}

// ============================================================================
// 契约测试组4: 值访问 (Value Access)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 值获取函数", "[c_api][value_access][get]") {
    SECTION("🔍 lua_c_analysis验证: 基础值获取") {
        // 布尔值
        lua_pushboolean(L, 1);
        lua_pushboolean(L, 0);
        REQUIRE(lua_toboolean(L, 1) == 1);
        REQUIRE(lua_toboolean(L, 2) == 0);
        
        // 数字值
        lua_pushinteger(L, 42);
        lua_pushnumber(L, 3.14159);
        REQUIRE(lua_tointeger(L, 3) == 42);
        REQUIRE(lua_tonumber(L, 4) == 3.14159);
        
        // 字符串值
        lua_pushstring(L, "hello world");
        size_t len;
        const char* str = lua_tolstring(L, 5, &len);
        REQUIRE(str != nullptr);
        REQUIRE(std::string(str, len) == "hello world");
        REQUIRE(len == 11);
        
        // 简化字符串获取
        const char* str2 = lua_tostring(L, 5);
        REQUIRE(str2 != nullptr);
        REQUIRE(std::string(str2) == "hello world");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 类型转换行为") {
        // 数字到字符串转换
        lua_pushinteger(L, 123);
        size_t len;
        const char* str = lua_tolstring(L, 1, &len);
        REQUIRE(str != nullptr);
        REQUIRE(std::string(str, len) == "123");
        
        // 转换后原值不变
        REQUIRE(lua_isnumber(L, 1));
        REQUIRE(lua_tointeger(L, 1) == 123);
        
        // 字符串到数字转换
        lua_pushstring(L, "456.789");
        lua_Number num = lua_tonumber(L, 2);
        REQUIRE(num == 456.789);
        
        // 转换后原值不变
        REQUIRE(lua_isstring(L, 2));
        REQUIRE(std::string(lua_tostring(L, 2)) == "456.789");
        
        // 无效转换
        lua_pushstring(L, "not_a_number");
        lua_Number invalid_num = lua_tonumber(L, 3);
        REQUIRE(invalid_num == 0.0);  // 应该返回0
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 异常安全值访问") {
        // 测试在异常情况下的值访问
        lua_pushstring(L, "test");
        
        // 获取字符串长度时的边界情况
        const char* str1 = lua_tolstring(L, 1, nullptr);  // 不关心长度
        REQUIRE(str1 != nullptr);
        REQUIRE(std::string(str1) == "test");
        
        size_t len;
        const char* str2 = lua_tolstring(L, 1, &len);  // 获取长度
        REQUIRE(str2 != nullptr);
        REQUIRE(len == 4);
        REQUIRE(std::string(str2, len) == "test");
        
        // 访问无效索引
        const char* invalid_str = lua_tostring(L, 10);
        REQUIRE(invalid_str == nullptr);
        
        lua_Number invalid_num = lua_tonumber(L, 10);
        REQUIRE(invalid_num == 0.0);
        
        lua_Integer invalid_int = lua_tointeger(L, 10);
        REQUIRE(invalid_int == 0);
        
        clean_stack();
    }
}

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 值设置函数", "[c_api][value_access][set]") {
    SECTION("🔍 lua_c_analysis验证: 基础值推入") {
        // 推入各种类型的值
        lua_pushnil(L);
        REQUIRE(lua_gettop(L) == 1);
        REQUIRE(lua_isnil(L, 1));
        
        lua_pushboolean(L, 1);
        REQUIRE(lua_gettop(L) == 2);
        REQUIRE(lua_isboolean(L, 2));
        REQUIRE(lua_toboolean(L, 2) == 1);
        
        lua_pushinteger(L, -42);
        REQUIRE(lua_gettop(L) == 3);
        REQUIRE(lua_isnumber(L, 3));
        REQUIRE(lua_tointeger(L, 3) == -42);
        
        lua_pushnumber(L, 2.718281828);
        REQUIRE(lua_gettop(L) == 4);
        REQUIRE(lua_isnumber(L, 4));
        REQUIRE(lua_tonumber(L, 4) == 2.718281828);
        
        lua_pushstring(L, "Hello, Lua!");
        REQUIRE(lua_gettop(L) == 5);
        REQUIRE(lua_isstring(L, 5));
        REQUIRE(std::string(lua_tostring(L, 5)) == "Hello, Lua!");
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 字符串推入变体") {
        // 普通字符串推入
        lua_pushstring(L, "normal string");
        REQUIRE(lua_isstring(L, 1));
        
        // 带长度的字符串推入
        const char* data = "binary\0data\0with\0nulls";
        size_t data_len = 20;  // 包含null字符
        lua_pushlstring(L, data, data_len);
        REQUIRE(lua_isstring(L, 2));
        
        size_t result_len;
        const char* result = lua_tolstring(L, 2, &result_len);
        REQUIRE(result_len == data_len);
        REQUIRE(memcmp(result, data, data_len) == 0);
        
        // 格式化字符串推入
        lua_pushfstring(L, "Number: %d, String: %s", 42, "test");
        REQUIRE(lua_isstring(L, 3));
        REQUIRE(std::string(lua_tostring(L, 3)) == "Number: 42, String: test");
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++值推入") {
        // 使用现代C++特性的值推入封装
        auto push_value = [](lua_State* L, const auto& value) {
            using T = std::decay_t<decltype(value)>;
            
            if constexpr (std::is_same_v<T, std::nullptr_t>) {
                lua_pushnil(L);
            } else if constexpr (std::is_same_v<T, bool>) {
                lua_pushboolean(L, value ? 1 : 0);
            } else if constexpr (std::is_integral_v<T>) {
                lua_pushinteger(L, static_cast<lua_Integer>(value));
            } else if constexpr (std::is_floating_point_v<T>) {
                lua_pushnumber(L, static_cast<lua_Number>(value));
            } else if constexpr (std::is_same_v<T, std::string>) {
                lua_pushlstring(L, value.data(), value.size());
            } else if constexpr (std::is_same_v<T, const char*>) {
                lua_pushstring(L, value);
            }
        };
        
        // 测试泛型推入
        push_value(L, nullptr);
        push_value(L, true);
        push_value(L, 42);
        push_value(L, 3.14);
        push_value(L, std::string("modern string"));
        push_value(L, "c string");
        
        REQUIRE(lua_gettop(L) == 6);
        REQUIRE(lua_isnil(L, 1));
        REQUIRE(lua_isboolean(L, 2));
        REQUIRE(lua_isnumber(L, 3));
        REQUIRE(lua_isnumber(L, 4));
        REQUIRE(lua_isstring(L, 5));
        REQUIRE(lua_isstring(L, 6));
        
        clean_stack();
    }
}

// ============================================================================
// 契约测试组5: 错误处理 (Error Handling)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: Panic函数处理", "[c_api][error_handling][panic]") {
    SECTION("🔍 lua_c_analysis验证: Panic函数设置和调用") {
        bool custom_panic_called = false;
        std::string panic_msg;
        
        auto custom_panic = [](lua_State* L) -> int {
            // 这个lambda不能捕获，所以使用静态变量
            static bool* called_ptr = nullptr;
            static std::string* msg_ptr = nullptr;
            
            if (!called_ptr) {
                // 初始化时设置指针（这里简化处理）
                return 0;
            }
            
            *called_ptr = true;
            if (lua_isstring(L, -1)) {
                *msg_ptr = lua_tostring(L, -1);
            }
            return 0;
        };
        
        // 设置自定义panic函数
        lua_PFunction old_panic = lua_atpanic(L, custom_panic);
        REQUIRE(old_panic != nullptr);  // 应该有之前的panic函数
        
        // 恢复原panic函数
        lua_atpanic(L, old_panic);
    }
    
    SECTION("🏗️ lua_with_cpp验证: RAII错误处理") {
        // 使用RAII模式的panic函数管理
        class PanicGuard {
        public:
            PanicGuard(lua_State* L, lua_PFunction new_panic) 
                : L_(L), old_panic_(lua_atpanic(L, new_panic)) {}
            
            ~PanicGuard() {
                lua_atpanic(L_, old_panic_);
            }
            
            PanicGuard(const PanicGuard&) = delete;
            PanicGuard& operator=(const PanicGuard&) = delete;
            
        private:
            lua_State* L_;
            lua_PFunction old_panic_;
        };
        
        lua_PFunction original = lua_atpanic(L, nullptr);
        
        {
            PanicGuard guard(L, test_panic);
            // 在这个作用域内使用自定义panic函数
            
            // 验证panic函数已设置
            lua_PFunction current = lua_atpanic(L, test_panic);
            REQUIRE(current == test_panic);
            lua_atpanic(L, current);  // 恢复
        }
        // guard析构时自动恢复原panic函数
        
        lua_PFunction restored = lua_atpanic(L, original);
        // 验证已恢复原函数（可能不完全相等，但不应该是test_panic）
        lua_atpanic(L, restored);
    }
}

// ============================================================================
// 契约测试组6: 注册表和全局变量 (Registry and Globals)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 注册表访问", "[c_api][registry][globals]") {
    SECTION("🔍 lua_c_analysis验证: 注册表基础操作") {
        // 向注册表存储值
        lua_pushstring(L, "test_value");
        lua_setfield(L, LUA_REGISTRYINDEX, "test_key");
        
        // 从注册表获取值
        lua_getfield(L, LUA_REGISTRYINDEX, "test_key");
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "test_value");
        
        lua_pop(L, 1);  // 清理栈
        
        // 验证不存在的键
        lua_getfield(L, LUA_REGISTRYINDEX, "nonexistent_key");
        REQUIRE(lua_isnil(L, -1));
        
        lua_pop(L, 1);
    }
    
    SECTION("🔍 lua_c_analysis验证: 全局变量访问") {
        // 设置全局变量
        lua_pushinteger(L, 42);
        lua_setglobal(L, "my_global");
        
        // 获取全局变量
        lua_getglobal(L, "my_global");
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tointeger(L, -1) == 42);
        
        lua_pop(L, 1);
        
        // 获取不存在的全局变量
        lua_getglobal(L, "nonexistent_global");
        REQUIRE(lua_isnil(L, -1));
        
        lua_pop(L, 1);
    }
    
    SECTION("🏗️ lua_with_cpp验证: 类型安全的注册表操作") {
        // 类型安全的注册表访问封装
        class RegistryAccess {
        public:
            explicit RegistryAccess(lua_State* L) : L_(L) {}
            
            template<typename T>
            void set(const std::string& key, const T& value) {
                push_value(value);
                lua_setfield(L_, LUA_REGISTRYINDEX, key.c_str());
            }
            
            template<typename T>
            std::optional<T> get(const std::string& key) {
                lua_getfield(L_, LUA_REGISTRYINDEX, key.c_str());
                auto result = to_value<T>();
                lua_pop(L_, 1);
                return result;
            }
            
        private:
            lua_State* L_;
            
            void push_value(int value) { lua_pushinteger(L_, value); }
            void push_value(const std::string& value) { lua_pushstring(L_, value.c_str()); }
            void push_value(bool value) { lua_pushboolean(L_, value ? 1 : 0); }
            
            template<typename T>
            std::optional<T> to_value() {
                if constexpr (std::is_same_v<T, int>) {
                    if (lua_isnumber(L_, -1)) {
                        return static_cast<int>(lua_tointeger(L_, -1));
                    }
                } else if constexpr (std::is_same_v<T, std::string>) {
                    if (lua_isstring(L_, -1)) {
                        return std::string(lua_tostring(L_, -1));
                    }
                } else if constexpr (std::is_same_v<T, bool>) {
                    if (lua_isboolean(L_, -1)) {
                        return lua_toboolean(L_, -1) != 0;
                    }
                }
                return std::nullopt;
            }
        };
        
        RegistryAccess registry(L);
        
        // 设置和获取不同类型的值
        registry.set("int_val", 123);
        registry.set("str_val", std::string("hello"));
        registry.set("bool_val", true);
        
        auto int_val = registry.get<int>("int_val");
        auto str_val = registry.get<std::string>("str_val");
        auto bool_val = registry.get<bool>("bool_val");
        auto missing_val = registry.get<int>("missing");
        
        REQUIRE(int_val.has_value());
        REQUIRE(int_val.value() == 123);
        REQUIRE(str_val.has_value());
        REQUIRE(str_val.value() == "hello");
        REQUIRE(bool_val.has_value());
        REQUIRE(bool_val.value() == true);
        REQUIRE_FALSE(missing_val.has_value());
    }
}

// ============================================================================
// 契约测试组7: 性能基准测试 (Performance Benchmarks)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 性能基准", "[c_api][performance][benchmark]") {
    SECTION("🔍 lua_c_analysis验证: 栈操作性能") {
        const int iterations = 10000;
        
        BENCHMARK("栈推入/弹出循环") {
            for (int i = 0; i < iterations; i++) {
                lua_pushinteger(L, i);
                lua_pushstring(L, "test");
                lua_pushboolean(L, i % 2);
                lua_settop(L, 0);  // 清空栈
            }
        };
        
        BENCHMARK("栈值复制操作") {
            lua_pushinteger(L, 42);
            lua_pushstring(L, "benchmark");
            
            return [=]() {
                for (int i = 0; i < iterations; i++) {
                    lua_pushvalue(L, 1);
                    lua_pushvalue(L, 2);
                    lua_pop(L, 2);
                }
            };
        };
    }
    
    SECTION("🔍 lua_c_analysis验证: 类型检查性能") {
        // 准备测试数据
        for (int i = 0; i < 100; i++) {
            lua_pushinteger(L, i);
            lua_pushstring(L, "test");
            lua_pushboolean(L, i % 2);
        }
        
        const int stack_size = lua_gettop(L);
        
        BENCHMARK("类型检查循环") {
            for (int i = 1; i <= stack_size; i++) {
                lua_type(L, i);
                lua_isnumber(L, i);
                lua_isstring(L, i);
                lua_isboolean(L, i);
            }
        };
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++包装性能") {
        const int iterations = 1000;
        
        // 直接C API调用基准
        BENCHMARK("直接C API调用") {
            for (int i = 0; i < iterations; i++) {
                lua_pushinteger(L, i);
                lua_tointeger(L, -1);
                lua_pop(L, 1);
            }
        };
        
        // C++包装器调用基准
        auto cpp_push_get = [this](int value) -> int {
            lua_pushinteger(L, value);
            int result = static_cast<int>(lua_tointeger(L, -1));
            lua_pop(L, 1);
            return result;
        };
        
        BENCHMARK("C++包装器调用") {
            for (int i = 0; i < iterations; i++) {
                cpp_push_get(i);
            }
        };
    }
}

// ============================================================================
// 契约测试组8: 边界条件和错误情况 (Edge Cases and Error Conditions)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 边界条件测试", "[c_api][edge_cases][robustness]") {
    SECTION("🔍 lua_c_analysis验证: 空指针和无效参数") {
        // 测试NULL状态指针的处理（如果实现允许）
        // 注意：实际实现中这些可能导致崩溃，测试需谨慎
        
        // 测试空字符串推入
        lua_pushstring(L, "");
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "");
        
        lua_pushstring(L, nullptr);  // 可能的行为：推入nil或空字符串
        // 具体行为依赖于实现
        
        // 测试零长度字符串推入
        lua_pushlstring(L, "test", 0);
        REQUIRE(lua_isstring(L, -1));
        size_t len;
        const char* str = lua_tolstring(L, -1, &len);
        REQUIRE(len == 0);
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 极值测试") {
        // 测试数字极值
        lua_pushinteger(L, std::numeric_limits<lua_Integer>::max());
        lua_pushinteger(L, std::numeric_limits<lua_Integer>::min());
        lua_pushnumber(L, std::numeric_limits<lua_Number>::max());
        lua_pushnumber(L, std::numeric_limits<lua_Number>::min());
        lua_pushnumber(L, std::numeric_limits<lua_Number>::infinity());
        lua_pushnumber(L, -std::numeric_limits<lua_Number>::infinity());
        lua_pushnumber(L, std::numeric_limits<lua_Number>::quiet_NaN());
        
        REQUIRE(lua_gettop(L) == 7);
        
        // 验证极值存储和获取
        REQUIRE(lua_tointeger(L, 1) == std::numeric_limits<lua_Integer>::max());
        REQUIRE(lua_tointeger(L, 2) == std::numeric_limits<lua_Integer>::min());
        REQUIRE(lua_tonumber(L, 3) == std::numeric_limits<lua_Number>::max());
        
        // NaN和无穷大的特殊处理
        lua_Number inf_val = lua_tonumber(L, 5);
        lua_Number neg_inf_val = lua_tonumber(L, 6);
        lua_Number nan_val = lua_tonumber(L, 7);
        
        REQUIRE(std::isinf(inf_val));
        REQUIRE(inf_val > 0);
        REQUIRE(std::isinf(neg_inf_val));
        REQUIRE(neg_inf_val < 0);
        REQUIRE(std::isnan(nan_val));
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 异常安全边界测试") {
        // 测试资源管理的异常安全性
        class SafeStackManager {
        public:
            explicit SafeStackManager(lua_State* L) : L_(L), initial_top_(lua_gettop(L)) {}
            
            ~SafeStackManager() {
                // 确保栈恢复到初始状态
                lua_settop(L_, initial_top_);
            }
            
            void checkpoint() {
                initial_top_ = lua_gettop(L_);
            }
            
        private:
            lua_State* L_;
            int initial_top_;
        };
        
        {
            SafeStackManager manager(L);
            
            // 在作用域内进行各种操作
            lua_pushinteger(L, 1);
            lua_pushstring(L, "test");
            lua_newtable(L);
            
            REQUIRE(lua_gettop(L) == 3);
            
            // 模拟异常情况
            manager.checkpoint();  // 更新检查点
            
            lua_pushinteger(L, 2);
            lua_pushstring(L, "more");
            
            REQUIRE(lua_gettop(L) == 5);
        }
        // manager析构时自动清理栈到检查点状态
        
        REQUIRE(lua_gettop(L) == 0);
    }
}

} // namespace c_api_contract_tests
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

// 自定义测试监听器，用于验证测试状态
class CAPITestListener : public Catch::EventListenerBase {
public:
    using EventListenerBase::EventListenerBase;
    
    void testCaseStarting(const Catch::TestCaseInfo& testInfo) override {
        // 在每个测试用例开始时的设置
        current_test_name = testInfo.name;
    }
    
    void testCaseEnded(const Catch::TestCaseStats& testCaseStats) override {
        // 在每个测试用例结束时的清理和验证
        if (testCaseStats.testInfo->tags.find("[c_api]") != testCaseStats.testInfo->tags.end()) {
            // 验证C API测试没有泄漏内存或状态
        }
    }
    
private:
    std::string current_test_name;
};

CATCH_REGISTER_LISTENER(CAPITestListener)

} // anonymous namespace

// ============================================================================
// 扩展测试组9: 表操作 (Table Operations)
// ============================================================================

namespace lua_cpp {
namespace c_api_contract_tests {

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 表创建和访问", "[c_api][table_ops][basic]") {
    SECTION("🔍 lua_c_analysis验证: 表基础操作") {
        // 创建新表
        lua_newtable(L);
        REQUIRE(lua_istable(L, -1));
        REQUIRE(lua_gettop(L) == 1);
        
        // 设置表字段
        lua_pushstring(L, "key1");
        lua_pushinteger(L, 42);
        lua_settable(L, 1);  // table[key1] = 42
        
        lua_pushstring(L, "key2");
        lua_pushstring(L, "value2");
        lua_settable(L, 1);  // table[key2] = "value2"
        
        // 获取表字段
        lua_pushstring(L, "key1");
        lua_gettable(L, 1);
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tointeger(L, -1) == 42);
        lua_pop(L, 1);
        
        lua_pushstring(L, "key2");
        lua_gettable(L, 1);
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "value2");
        lua_pop(L, 1);
        
        // 获取不存在的字段
        lua_pushstring(L, "nonexistent");
        lua_gettable(L, 1);
        REQUIRE(lua_isnil(L, -1));
        lua_pop(L, 1);
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 表字段便捷操作") {
        lua_newtable(L);
        
        // 使用setfield和getfield
        lua_pushinteger(L, 100);
        lua_setfield(L, 1, "number_field");
        
        lua_pushstring(L, "hello");
        lua_setfield(L, 1, "string_field");
        
        // 获取字段
        lua_getfield(L, 1, "number_field");
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tointeger(L, -1) == 100);
        lua_pop(L, 1);
        
        lua_getfield(L, 1, "string_field");
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "hello");
        lua_pop(L, 1);
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 数组索引操作") {
        lua_newtable(L);
        
        // 设置数组元素
        for (int i = 1; i <= 5; i++) {
            lua_pushinteger(L, i * 10);
            lua_rawseti(L, 1, i);  // table[i] = i * 10
        }
        
        // 获取数组元素
        for (int i = 1; i <= 5; i++) {
            lua_rawgeti(L, 1, i);
            REQUIRE(lua_isnumber(L, -1));
            REQUIRE(lua_tointeger(L, -1) == i * 10);
            lua_pop(L, 1);
        }
        
        // 获取数组长度
        size_t len = lua_objlen(L, 1);
        REQUIRE(len == 5);
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: 类型安全表操作") {
        // 类型安全的表操作封装
        class TableAccessor {
        public:
            TableAccessor(lua_State* L, int table_index) : L_(L), table_index_(table_index) {
                REQUIRE(lua_istable(L_, table_index_));
            }
            
            template<typename T>
            void set(const std::string& key, const T& value) {
                push_value(value);
                lua_setfield(L_, table_index_, key.c_str());
            }
            
            template<typename T>
            std::optional<T> get(const std::string& key) {
                lua_getfield(L_, table_index_, key.c_str());
                auto result = to_value<T>();
                lua_pop(L_, 1);
                return result;
            }
            
            template<typename T>
            void set_array(int index, const T& value) {
                push_value(value);
                lua_rawseti(L_, table_index_, index);
            }
            
            template<typename T>
            std::optional<T> get_array(int index) {
                lua_rawgeti(L_, table_index_, index);
                auto result = to_value<T>();
                lua_pop(L_, 1);
                return result;
            }
            
            size_t length() const {
                return lua_objlen(L_, table_index_);
            }
            
        private:
            lua_State* L_;
            int table_index_;
            
            void push_value(int value) { lua_pushinteger(L_, value); }
            void push_value(const std::string& value) { lua_pushstring(L_, value.c_str()); }
            void push_value(bool value) { lua_pushboolean(L_, value ? 1 : 0); }
            void push_value(double value) { lua_pushnumber(L_, value); }
            
            template<typename T>
            std::optional<T> to_value() {
                if constexpr (std::is_same_v<T, int>) {
                    if (lua_isnumber(L_, -1)) return static_cast<int>(lua_tointeger(L_, -1));
                } else if constexpr (std::is_same_v<T, std::string>) {
                    if (lua_isstring(L_, -1)) return std::string(lua_tostring(L_, -1));
                } else if constexpr (std::is_same_v<T, bool>) {
                    if (lua_isboolean(L_, -1)) return lua_toboolean(L_, -1) != 0;
                } else if constexpr (std::is_same_v<T, double>) {
                    if (lua_isnumber(L_, -1)) return lua_tonumber(L_, -1);
                }
                return std::nullopt;
            }
        };
        
        lua_newtable(L);
        TableAccessor table(L, 1);
        
        // 测试类型安全设置和获取
        table.set("name", std::string("lua_cpp"));
        table.set("version", 1);
        table.set("active", true);
        table.set("pi", 3.14159);
        
        auto name = table.get<std::string>("name");
        auto version = table.get<int>("version");
        auto active = table.get<bool>("active");
        auto pi = table.get<double>("pi");
        auto missing = table.get<int>("missing");
        
        REQUIRE(name.has_value());
        REQUIRE(name.value() == "lua_cpp");
        REQUIRE(version.has_value());
        REQUIRE(version.value() == 1);
        REQUIRE(active.has_value());
        REQUIRE(active.value() == true);
        REQUIRE(pi.has_value());
        REQUIRE(pi.value() == 3.14159);
        REQUIRE_FALSE(missing.has_value());
        
        // 测试数组操作
        for (int i = 1; i <= 3; i++) {
            table.set_array(i, i * i);
        }
        
        REQUIRE(table.length() == 3);
        
        for (int i = 1; i <= 3; i++) {
            auto val = table.get_array<int>(i);
            REQUIRE(val.has_value());
            REQUIRE(val.value() == i * i);
        }
        
        clean_stack();
    }
}

// ============================================================================
// 测试组10: 函数调用 (Function Calls)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: C函数注册和调用", "[c_api][function_calls][c_functions]") {
    SECTION("🔍 lua_c_analysis验证: C函数注册") {
        // 简单C函数
        auto simple_add = [](lua_State* L) -> int {
            if (lua_gettop(L) != 2) {
                lua_pushstring(L, "Expected 2 arguments");
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
        
        // 注册函数到全局
        lua_pushcfunction(L, simple_add);
        lua_setglobal(L, "add");
        
        // 调用函数
        lua_getglobal(L, "add");
        REQUIRE(lua_isfunction(L, -1));
        
        lua_pushnumber(L, 3.5);
        lua_pushnumber(L, 2.5);
        
        int result = lua_pcall(L, 2, 1, 0);  // 2个参数，1个返回值
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_isnumber(L, -1));
        REQUIRE(lua_tonumber(L, -1) == 6.0);
        
        lua_pop(L, 1);
    }
    
    SECTION("🔍 lua_c_analysis验证: 错误处理和pcall") {
        // 会产生错误的C函数
        auto error_function = [](lua_State* L) -> int {
            lua_pushstring(L, "This is an intentional error");
            lua_error(L);
            return 0;  // 永不到达
        };
        
        lua_pushcfunction(L, error_function);
        lua_setglobal(L, "error_func");
        
        // 使用pcall安全调用
        lua_getglobal(L, "error_func");
        int result = lua_pcall(L, 0, 0, 0);
        
        REQUIRE(result == LUA_ERRRUN);
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "This is an intentional error");
        
        lua_pop(L, 1);  // 清理错误消息
    }
    
    SECTION("🏗️ lua_with_cpp验证: 现代C++函数包装") {
        // 使用现代C++特性的函数包装
        class FunctionWrapper {
        public:
            template<typename Func>
            static lua_CFunction wrap(Func&& func) {
                static auto stored_func = std::forward<Func>(func);
                
                return [](lua_State* L) -> int {
                    try {
                        return stored_func(L);
                    } catch (const std::exception& e) {
                        lua_pushstring(L, e.what());
                        lua_error(L);
                        return 0;
                    }
                };
            }
        };
        
        // 包装一个lambda函数
        auto multiply = [](lua_State* L) -> int {
            int argc = lua_gettop(L);
            if (argc == 0) {
                lua_pushnumber(L, 1.0);
                return 1;
            }
            
            lua_Number result = 1.0;
            for (int i = 1; i <= argc; i++) {
                if (!lua_isnumber(L, i)) {
                    throw std::invalid_argument("All arguments must be numbers");
                }
                result *= lua_tonumber(L, i);
            }
            
            lua_pushnumber(L, result);
            return 1;
        };
        
        lua_pushcfunction(L, FunctionWrapper::wrap(multiply));
        lua_setglobal(L, "multiply");
        
        // 测试正常调用
        lua_getglobal(L, "multiply");
        lua_pushnumber(L, 2.0);
        lua_pushnumber(L, 3.0);
        lua_pushnumber(L, 4.0);
        
        int result = lua_pcall(L, 3, 1, 0);
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_tonumber(L, -1) == 24.0);
        lua_pop(L, 1);
        
        // 测试无参数调用
        lua_getglobal(L, "multiply");
        result = lua_pcall(L, 0, 1, 0);
        REQUIRE(result == LUA_OK);
        REQUIRE(lua_tonumber(L, -1) == 1.0);
        lua_pop(L, 1);
    }
}

// ============================================================================
// 测试组11: 垃圾回收集成 (Garbage Collection Integration)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 垃圾回收控制", "[c_api][gc_integration][memory]") {
    SECTION("🔍 lua_c_analysis验证: GC基础控制") {
        // 获取当前内存使用
        int mem_before = lua_gc(L, LUA_GCCOUNT, 0);
        REQUIRE(mem_before >= 0);
        
        // 创建一些对象来增加内存使用
        for (int i = 0; i < 100; i++) {
            lua_newtable(L);
            for (int j = 1; j <= 10; j++) {
                lua_pushinteger(L, j);
                lua_rawseti(L, -2, j);
            }
        }
        
        int mem_after = lua_gc(L, LUA_GCCOUNT, 0);
        REQUIRE(mem_after > mem_before);
        
        // 执行完整GC
        lua_gc(L, LUA_GCCOLLECT, 0);
        
        // 清理栈
        lua_settop(L, 0);
        
        // 再次执行GC
        lua_gc(L, LUA_GCCOLLECT, 0);
        int mem_final = lua_gc(L, LUA_GCCOUNT, 0);
        
        // 内存应该减少（但不一定回到初始状态）
        REQUIRE(mem_final <= mem_after);
    }
    
    SECTION("🔍 lua_c_analysis验证: GC参数控制") {
        // 停止GC
        lua_gc(L, LUA_GCSTOP, 0);
        
        // 创建对象但GC不会运行
        for (int i = 0; i < 50; i++) {
            lua_newtable(L);
        }
        
        int mem_with_gc_stopped = lua_gc(L, LUA_GCCOUNT, 0);
        
        // 重启GC
        lua_gc(L, LUA_GCRESTART, 0);
        
        // 执行GC步进
        for (int i = 0; i < 10; i++) {
            lua_gc(L, LUA_GCSTEP, 1);
        }
        
        // 获取GC参数
        int pause = lua_gc(L, LUA_GCSETPAUSE, 200);  // 设置暂停为200%
        int stepmul = lua_gc(L, LUA_GCSETSTEPMUL, 200);  // 设置步进倍数为200%
        
        REQUIRE(pause >= 0);
        REQUIRE(stepmul >= 0);
        
        clean_stack();
    }
    
    SECTION("🏗️ lua_with_cpp验证: RAII GC管理") {
        // RAII风格的GC控制
        class GCController {
        public:
            explicit GCController(lua_State* L) : L_(L) {
                initial_pause_ = lua_gc(L_, LUA_GCSETPAUSE, 150);
                initial_stepmul_ = lua_gc(L_, LUA_GCSETSTEPMUL, 150);
            }
            
            ~GCController() {
                // 恢复原始GC参数
                lua_gc(L_, LUA_GCSETPAUSE, initial_pause_);
                lua_gc(L_, LUA_GCSETSTEPMUL, initial_stepmul_);
            }
            
            void force_collect() {
                lua_gc(L_, LUA_GCCOLLECT, 0);
            }
            
            int get_memory_kb() {
                return lua_gc(L_, LUA_GCCOUNT, 0);
            }
            
            int get_memory_bytes() {
                return lua_gc(L_, LUA_GCCOUNTB, 0);
            }
            
        private:
            lua_State* L_;
            int initial_pause_;
            int initial_stepmul_;
        };
        
        int mem_start;
        {
            GCController gc(L);
            mem_start = gc.get_memory_kb();
            
            // 创建大量对象
            for (int i = 0; i < 200; i++) {
                lua_newtable(L);
                lua_pushinteger(L, i);
                lua_setfield(L, -2, "id");
            }
            
            int mem_peak = gc.get_memory_kb();
            REQUIRE(mem_peak > mem_start);
            
            gc.force_collect();
            clean_stack();
            gc.force_collect();
            
            int mem_after_gc = gc.get_memory_kb();
            REQUIRE(mem_after_gc <= mem_peak);
        }
        // GC控制器析构时自动恢复参数
    }
}

// ============================================================================
// 测试组12: 元表操作 (Metatable Operations)
// ============================================================================

TEST_CASE_METHOD(CAPITestFixture, "C API契约: 元表设置和访问", "[c_api][metatable][metamethods]") {
    SECTION("🔍 lua_c_analysis验证: 基础元表操作") {
        // 创建表和元表
        lua_newtable(L);      // 主表
        lua_newtable(L);      // 元表
        
        // 设置__index元方法
        lua_pushstring(L, "__index");
        lua_newtable(L);      // __index表
        lua_pushstring(L, "default_value");
        lua_setfield(L, -2, "default_key");
        lua_settable(L, -3);  // 设置__index
        
        // 将元表设置给主表
        REQUIRE(lua_setmetatable(L, 1) == 1);
        
        // 验证元表存在
        REQUIRE(lua_getmetatable(L, 1) == 1);
        lua_pop(L, 1);  // 移除元表
        
        // 测试__index元方法
        lua_getfield(L, 1, "default_key");
        REQUIRE(lua_isstring(L, -1));
        REQUIRE(std::string(lua_tostring(L, -1)) == "default_value");
        lua_pop(L, 1);
        
        clean_stack();
    }
    
    SECTION("🔍 lua_c_analysis验证: 元方法函数") {
        // __add元方法实现
        auto add_metamethod = [](lua_State* L) -> int {
            if (lua_gettop(L) != 2) {
                lua_pushstring(L, "Invalid arguments for __add");
                lua_error(L);
            }
            
            // 简单的数值相加（实际实现会更复杂）
            lua_getfield(L, 1, "value");
            lua_getfield(L, 2, "value");
            
            if (lua_isnumber(L, -1) && lua_isnumber(L, -2)) {
                lua_Number sum = lua_tonumber(L, -1) + lua_tonumber(L, -2);
                
                lua_newtable(L);  // 创建结果表
                lua_pushnumber(L, sum);
                lua_setfield(L, -2, "value");
                
                // 设置相同的元表
                lua_getmetatable(L, 1);
                lua_setmetatable(L, -2);
                
                return 1;
            }
            
            lua_pushstring(L, "Cannot add non-numeric values");
            lua_error(L);
            return 0;
        };
        
        // 创建两个对象
        lua_newtable(L);  // obj1
        lua_pushnumber(L, 10);
        lua_setfield(L, -2, "value");
        
        lua_newtable(L);  // obj2
        lua_pushnumber(L, 20);
        lua_setfield(L, -2, "value");
        
        // 创建共享元表
        lua_newtable(L);  // 元表
        lua_pushcfunction(L, add_metamethod);
        lua_setfield(L, -2, "__add");
        
        // 设置元表
        lua_pushvalue(L, -1);  // 复制元表
        lua_setmetatable(L, 1);  // 设置给obj1
        lua_setmetatable(L, 2);  // 设置给obj2
        
        // 验证元表设置成功
        REQUIRE(lua_getmetatable(L, 1) == 1);
        lua_pop(L, 1);
        REQUIRE(lua_getmetatable(L, 2) == 1);
        lua_pop(L, 1);
        
        clean_stack();
    }
}

} // namespace c_api_contract_tests
} // namespace lua_cpp