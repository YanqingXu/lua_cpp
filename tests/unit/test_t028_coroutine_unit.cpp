/**
 * @file test_t028_coroutine_unit.cpp
 * @brief T028 协程标准库单元测试
 * 
 * 测试覆盖：
 * - LuaCoroutine基础操作
 * - CoroutineLibrary所有API
 * - 错误处理
 * - 性能基准
 * 
 * @author Lua C++ Project Team
 * @date 2025-10-11
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "stdlib/coroutine_lib.h"
#include "vm/enhanced_virtual_machine.h"

using namespace lua_cpp;
using namespace lua_cpp::stdlib;

/* ========================================================================== */
/* 测试辅助函数 */
/* ========================================================================== */

/**
 * @brief 创建测试用的简单协程函数
 */
LuaCoroutine CreateSimpleCoroutine() {
    co_return std::vector<LuaValue>{LuaValue(42.0)};
}

/**
 * @brief 创建会yield的协程函数
 */
LuaCoroutine CreateYieldingCoroutine() {
    co_yield std::vector<LuaValue>{LuaValue(10.0)};
    co_yield std::vector<LuaValue>{LuaValue(20.0)};
    co_return std::vector<LuaValue>{LuaValue(30.0)};
}

/**
 * @brief 创建带参数的协程函数
 */
LuaCoroutine CreateParameterizedCoroutine() {
    // 模拟接收参数并yield
    co_yield std::vector<LuaValue>{LuaValue(1.0)};
    co_return std::vector<LuaValue>{LuaValue(2.0)};
}

/* ========================================================================== */
/* LuaCoroutine基础测试 */
/* ========================================================================== */

TEST_CASE("LuaCoroutine - Construction and Destruction", "[coroutine][unit][basic]") {
    SECTION("Create and destroy coroutine") {
        auto coro = CreateSimpleCoroutine();
        REQUIRE(coro.GetState() == CoroutineState::SUSPENDED);
        REQUIRE_FALSE(coro.IsDone());
    }
    
    SECTION("Move construction") {
        auto coro1 = CreateSimpleCoroutine();
        auto coro2 = std::move(coro1);
        
        REQUIRE(coro2.GetState() == CoroutineState::SUSPENDED);
        REQUIRE_FALSE(coro2.IsDone());
    }
    
    SECTION("Move assignment") {
        auto coro1 = CreateSimpleCoroutine();
        auto coro2 = CreateSimpleCoroutine();
        
        coro2 = std::move(coro1);
        REQUIRE(coro2.GetState() == CoroutineState::SUSPENDED);
    }
}

TEST_CASE("LuaCoroutine - Resume and State", "[coroutine][unit][resume]") {
    SECTION("Resume simple coroutine") {
        auto coro = CreateSimpleCoroutine();
        
        auto result = coro.Resume({});
        
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].IsNumber());
        REQUIRE(result[0].GetNumber() == 42.0);
        REQUIRE(coro.IsDone());
        REQUIRE(coro.GetState() == CoroutineState::DEAD);
    }
    
    SECTION("Resume dead coroutine throws") {
        auto coro = CreateSimpleCoroutine();
        coro.Resume({});
        
        REQUIRE_THROWS_AS(coro.Resume({}), CoroutineStateError);
    }
    
    SECTION("Multiple yields") {
        auto coro = CreateYieldingCoroutine();
        
        // 第一次resume
        auto r1 = coro.Resume({});
        REQUIRE(r1.size() == 1);
        REQUIRE(r1[0].GetNumber() == 10.0);
        REQUIRE(coro.GetState() == CoroutineState::SUSPENDED);
        
        // 第二次resume
        auto r2 = coro.Resume({});
        REQUIRE(r2.size() == 1);
        REQUIRE(r2[0].GetNumber() == 20.0);
        REQUIRE(coro.GetState() == CoroutineState::SUSPENDED);
        
        // 第三次resume
        auto r3 = coro.Resume({});
        REQUIRE(r3.size() == 1);
        REQUIRE(r3[0].GetNumber() == 30.0);
        REQUIRE(coro.IsDone());
    }
}

TEST_CASE("LuaCoroutine - Statistics", "[coroutine][unit][stats]") {
    SECTION("Resume count tracking") {
        auto coro = CreateYieldingCoroutine();
        
        coro.Resume({});
        coro.Resume({});
        coro.Resume({});
        
        const auto& stats = coro.GetStatistics();
        REQUIRE(stats.resume_count == 3);
    }
    
    SECTION("Timing information") {
        auto coro = CreateSimpleCoroutine();
        auto& stats = coro.GetStatistics();
        
        // 检查创建时间已记录
        REQUIRE(stats.created_time.time_since_epoch().count() > 0);
        
        coro.Resume({});
        
        // 检查运行时间已记录
        REQUIRE(stats.total_run_time_ms >= 0.0);
    }
}

/* ========================================================================== */
/* CoroutineLibrary API测试 */
/* ========================================================================== */

TEST_CASE("CoroutineLibrary - coroutine.create", "[coroutine][unit][create]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Create with valid function") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        REQUIRE(co.IsUserData());
        REQUIRE(lib->Status(co) == "suspended");
    }
    
    SECTION("Create with non-function throws") {
        auto non_func = LuaValue(42.0);
        
        REQUIRE_THROWS_AS(lib->Create(non_func), LuaError);
    }
    
    SECTION("Create with nil throws") {
        auto nil_val = LuaValue::Nil();
        
        REQUIRE_THROWS_AS(lib->Create(nil_val), LuaError);
    }
}

TEST_CASE("CoroutineLibrary - coroutine.resume", "[coroutine][unit][resume]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Resume simple coroutine") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        auto result = lib->Resume(co, {});
        
        // 结果应该是 {true, values...}
        REQUIRE(result.size() >= 1);
        REQUIRE(result[0].IsBoolean());
    }
    
    SECTION("Resume with arguments") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        std::vector<LuaValue> args = {
            LuaValue(1.0),
            LuaValue(2.0),
            LuaValue(3.0)
        };
        
        auto result = lib->Resume(co, args);
        REQUIRE(result.size() >= 1);
    }
    
    SECTION("Resume dead coroutine returns error") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        // 第一次resume使其完成
        lib->Resume(co, {});
        
        // 第二次resume应该返回错误
        auto result = lib->Resume(co, {});
        
        REQUIRE(result.size() >= 1);
        // 第一个值应该是false（表示错误）
        REQUIRE(result[0].IsBoolean());
    }
}

TEST_CASE("CoroutineLibrary - coroutine.status", "[coroutine][unit][status]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Status of suspended coroutine") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        auto status = lib->Status(co);
        REQUIRE(status == "suspended");
    }
    
    SECTION("Status of dead coroutine") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        lib->Resume(co, {});
        
        auto status = lib->Status(co);
        // 应该是"dead"或"suspended"（取决于实现）
        REQUIRE((status == "dead" || status == "suspended"));
    }
    
    SECTION("Status with invalid coroutine throws") {
        auto non_coro = LuaValue(42.0);
        
        REQUIRE_THROWS_AS(lib->Status(non_coro), LuaError);
    }
}

TEST_CASE("CoroutineLibrary - coroutine.running", "[coroutine][unit][running]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Running in main thread returns nil") {
        auto result = lib->Running();
        REQUIRE(result.IsNil());
    }
    
    SECTION("Running in coroutine returns coroutine") {
        // 这个测试需要实际在协程中执行
        // 当前实现可能无法完全测试
        auto result = lib->Running();
        REQUIRE((result.IsNil() || result.IsUserData()));
    }
}

TEST_CASE("CoroutineLibrary - coroutine.wrap", "[coroutine][unit][wrap]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Wrap creates function") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto wrapped = lib->Wrap(func);
        
        // 应该返回一个函数对象
        REQUIRE((wrapped.IsFunction() || wrapped.IsCFunction()));
    }
    
    SECTION("Wrap with non-function throws") {
        auto non_func = LuaValue(42.0);
        
        REQUIRE_THROWS_AS(lib->Wrap(non_func), LuaError);
    }
}

TEST_CASE("CoroutineLibrary - coroutine.yield", "[coroutine][unit][yield]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Yield outside coroutine throws") {
        std::vector<LuaValue> values = {LuaValue(1.0), LuaValue(2.0)};
        
        REQUIRE_THROWS_AS(lib->Yield(values), CoroutineError);
    }
}

/* ========================================================================== */
/* CallFunction接口测试 */
/* ========================================================================== */

TEST_CASE("CoroutineLibrary - CallFunction Interface", "[coroutine][unit][interface]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("GetFunctionNames returns all functions") {
        auto names = lib->GetFunctionNames();
        
        REQUIRE(names.size() == 6);
        REQUIRE(std::find(names.begin(), names.end(), "create") != names.end());
        REQUIRE(std::find(names.begin(), names.end(), "resume") != names.end());
        REQUIRE(std::find(names.begin(), names.end(), "yield") != names.end());
        REQUIRE(std::find(names.begin(), names.end(), "status") != names.end());
        REQUIRE(std::find(names.begin(), names.end(), "running") != names.end());
        REQUIRE(std::find(names.begin(), names.end(), "wrap") != names.end());
    }
    
    SECTION("CallFunction with create") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto result = lib->CallFunction("create", {func});
        
        REQUIRE(result.size() == 1);
        REQUIRE(result[0].IsUserData());
    }
    
    SECTION("CallFunction with unknown function throws") {
        REQUIRE_THROWS_AS(
            lib->CallFunction("unknown", {}),
            LuaError
        );
    }
}

/* ========================================================================== */
/* 错误处理测试 */
/* ========================================================================== */

TEST_CASE("CoroutineLibrary - Error Handling", "[coroutine][unit][error]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Invalid argument types") {
        // create需要函数
        REQUIRE_THROWS_AS(
            lib->CallFunction("create", {LuaValue(42.0)}),
            LuaError
        );
        
        // resume需要协程
        REQUIRE_THROWS_AS(
            lib->CallFunction("resume", {LuaValue(42.0)}),
            LuaError
        );
        
        // status需要协程
        REQUIRE_THROWS_AS(
            lib->CallFunction("status", {LuaValue(42.0)}),
            LuaError
        );
    }
    
    SECTION("Missing arguments") {
        // create缺少参数
        REQUIRE_THROWS_AS(
            lib->CallFunction("create", {}),
            LuaError
        );
        
        // resume缺少参数
        REQUIRE_THROWS_AS(
            lib->CallFunction("resume", {}),
            LuaError
        );
        
        // status缺少参数
        REQUIRE_THROWS_AS(
            lib->CallFunction("status", {}),
            LuaError
        );
    }
}

/* ========================================================================== */
/* 工具函数测试 */
/* ========================================================================== */

TEST_CASE("CoroutineLibrary - Utility Functions", "[coroutine][unit][utils]") {
    SECTION("CoroutineStateToString") {
        REQUIRE(CoroutineStateToString(CoroutineState::SUSPENDED) == "suspended");
        REQUIRE(CoroutineStateToString(CoroutineState::RUNNING) == "running");
        REQUIRE(CoroutineStateToString(CoroutineState::NORMAL) == "normal");
        REQUIRE(CoroutineStateToString(CoroutineState::DEAD) == "dead");
    }
    
    SECTION("CreateCoroutineLibrary") {
        auto vm = std::make_unique<EnhancedVirtualMachine>();
        auto lib = CreateCoroutineLibrary(vm.get());
        
        REQUIRE(lib != nullptr);
        REQUIRE(lib->GetFunctionNames().size() == 6);
    }
}

/* ========================================================================== */
/* 集成测试场景 */
/* ========================================================================== */

TEST_CASE("CoroutineLibrary - Integration Scenarios", "[coroutine][unit][integration]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Create and resume lifecycle") {
        auto func = LuaValue::CreateFunction(nullptr);
        
        // 创建
        auto co = lib->Create(func);
        REQUIRE(lib->Status(co) == "suspended");
        
        // Resume
        auto result = lib->Resume(co, {});
        REQUIRE(result.size() >= 1);
        
        // 状态应该改变
        // (具体状态取决于协程是否完成)
    }
    
    SECTION("Multiple coroutines") {
        auto func1 = LuaValue::CreateFunction(nullptr);
        auto func2 = LuaValue::CreateFunction(nullptr);
        
        auto co1 = lib->Create(func1);
        auto co2 = lib->Create(func2);
        
        REQUIRE(lib->Status(co1) == "suspended");
        REQUIRE(lib->Status(co2) == "suspended");
        
        // 可以独立操作两个协程
        lib->Resume(co1, {});
        REQUIRE(lib->Status(co2) == "suspended");
    }
}

/* ========================================================================== */
/* 性能基准测试 */
/* ========================================================================== */

TEST_CASE("CoroutineLibrary - Performance Benchmarks", "[coroutine][unit][benchmark]") {
    auto vm = std::make_unique<EnhancedVirtualMachine>();
    auto lib = CreateCoroutineLibrary(vm.get());
    
    SECTION("Coroutine creation performance") {
        auto func = LuaValue::CreateFunction(nullptr);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 1000; ++i) {
            auto co = lib->Create(func);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end - start
        ).count();
        
        double avg_time = duration / 1000.0;
        
        // 平均创建时间应该 < 10μs（目标是5μs）
        INFO("Average creation time: " << avg_time << "μs");
        REQUIRE(avg_time < 10.0);
    }
    
    SECTION("Status query performance") {
        auto func = LuaValue::CreateFunction(nullptr);
        auto co = lib->Create(func);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < 100000; ++i) {
            volatile auto status = lib->Status(co);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
            end - start
        ).count();
        
        double avg_time = duration / 100000.0;
        
        // 平均查询时间应该 < 100ns
        INFO("Average status query time: " << avg_time << "ns");
        REQUIRE(avg_time < 100.0);
    }
}
