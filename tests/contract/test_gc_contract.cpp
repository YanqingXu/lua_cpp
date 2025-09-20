/**
 * @file test_gc_contract.cpp
 * @brief GC（垃圾回收器）契约测试
 * @description 测试Lua垃圾回收器的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括内存分配、标记清除算法、增量回收、弱引用等GC核心机制
 * @date 2025-09-20
 * 
 * 测试覆盖范围：
 * 1. 基础GC架构和状态管理
 * 2. 内存分配和对象跟踪
 * 3. 三色标记清除算法
 * 4. 增量垃圾回收机制
 * 5. 弱引用和弱表处理
 * 6. 终结器执行和资源清理
 * 7. 写屏障和并发安全
 * 8. 性能基准和压力测试
 * 
 * 双重验证机制：
 * - 🔍 lua_c_analysis: 验证与lgc.c的三色标记算法行为一致性
 * - 🏗️ lua_with_cpp: 验证现代C++垃圾回收器设计正确性
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "gc/garbage_collector.h"
#include "gc/gc_object.h"
#include "gc/weak_table.h"
#include "types/tvalue.h"
#include "vm/virtual_machine.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* GC基础架构契约 */
/* ========================================================================== */

TEST_CASE("GC - 垃圾回收器初始化契约", "[gc][contract][basic]") {
    SECTION("GC应该正确初始化") {
        GarbageCollector gc;
        
        REQUIRE(gc.GetState() == GCState::Pause);
        REQUIRE(gc.GetAllocatedBytes() == 0);
        REQUIRE(gc.GetTotalObjects() == 0);
        REQUIRE(gc.GetThreshold() > 0);
        REQUIRE(gc.GetStepMultiplier() == 200); // Lua 5.1默认值
        REQUIRE(gc.GetPause() == 200);          // Lua 5.1默认值
    }

    SECTION("GC配置参数") {
        GCConfig config;
        config.initial_threshold = 1024;
        config.step_multiplier = 150;
        config.pause_multiplier = 180;
        config.enable_incremental = true;
        config.enable_generational = false;
        
        GarbageCollector gc(config);
        
        REQUIRE(gc.GetThreshold() == config.initial_threshold);
        REQUIRE(gc.GetStepMultiplier() == config.step_multiplier);
        REQUIRE(gc.GetPause() == config.pause_multiplier);
        REQUIRE(gc.IsIncrementalEnabled() == config.enable_incremental);
        REQUIRE(gc.IsGenerationalEnabled() == config.enable_generational);
    }

    SECTION("GC状态管理") {
        GarbageCollector gc;
        
        REQUIRE(gc.GetState() == GCState::Pause);
        
        // 模拟GC状态转换
        gc.SetState(GCState::Propagate);
        REQUIRE(gc.GetState() == GCState::Propagate);
        
        gc.SetState(GCState::AtomicMark);
        REQUIRE(gc.GetState() == GCState::AtomicMark);
        
        gc.SetState(GCState::Sweep);
        REQUIRE(gc.GetState() == GCState::Sweep);
        
        gc.SetState(GCState::Finalize);
        REQUIRE(gc.GetState() == GCState::Finalize);
    }
}

/* ========================================================================== */
/* 内存分配契约 */
/* ========================================================================== */

TEST_CASE("GC - 内存分配契约", "[gc][contract][allocation]") {
    SECTION("对象分配和跟踪") {
        GarbageCollector gc;
        
        // 分配不同类型的对象
        auto str_obj = gc.AllocateString("hello world");
        auto table_obj = gc.AllocateTable(4, 2); // array_size=4, hash_size=2
        auto func_obj = gc.AllocateFunction(nullptr);
        
        REQUIRE(str_obj != nullptr);
        REQUIRE(table_obj != nullptr);
        REQUIRE(func_obj != nullptr);
        
        // 验证对象计数
        REQUIRE(gc.GetTotalObjects() == 3);
        
        // 验证内存统计
        REQUIRE(gc.GetAllocatedBytes() > 0);
        
        // 验证对象类型
        REQUIRE(str_obj->GetType() == GCObjectType::String);
        REQUIRE(table_obj->GetType() == GCObjectType::Table);
        REQUIRE(func_obj->GetType() == GCObjectType::Function);
    }

    SECTION("内存分配触发GC") {
        GCConfig config;
        config.initial_threshold = 100; // 很小的阈值
        GarbageCollector gc(config);
        
        Size initial_collections = gc.GetCollectionCount();
        
        // 分配足够多的内存来触发GC
        std::vector<GCObject*> objects;
        for (int i = 0; i < 50; ++i) {
            objects.push_back(gc.AllocateString("test string " + std::to_string(i)));
        }
        
        // 应该触发至少一次GC
        REQUIRE(gc.GetCollectionCount() > initial_collections);
    }

    SECTION("内存分配失败处理") {
        GarbageCollector gc;
        
        // 设置内存限制
        gc.SetMemoryLimit(1024); // 1KB限制
        
        // 尝试分配大量内存应该失败
        REQUIRE_THROWS_AS([&]() {
            for (int i = 0; i < 1000; ++i) {
                gc.AllocateString(std::string(1024, 'x')); // 每个字符串1KB
            }
        }(), OutOfMemoryError);
    }

    SECTION("对象大小计算") {
        GarbageCollector gc;
        
        auto small_str = gc.AllocateString("hi");
        auto large_str = gc.AllocateString(std::string(1000, 'x'));
        auto table = gc.AllocateTable(100, 50);
        
        // 大对象应该占用更多内存
        REQUIRE(large_str->GetSize() > small_str->GetSize());
        REQUIRE(table->GetSize() > small_str->GetSize());
        
        // 内存统计应该反映实际分配
        Size expected_bytes = small_str->GetSize() + large_str->GetSize() + table->GetSize();
        REQUIRE(gc.GetAllocatedBytes() >= expected_bytes);
    }
}

/* ========================================================================== */
/* 标记清除算法契约 */
/* ========================================================================== */

TEST_CASE("GC - 标记清除算法契约", "[gc][contract][mark_sweep]") {
    SECTION("基础标记清除循环") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建可达对象（在栈上）
        auto reachable_str = gc.AllocateString("reachable");
        vm.Push(TValue::CreateString(reachable_str));
        
        // 创建不可达对象
        auto unreachable_str = gc.AllocateString("unreachable");
        
        Size initial_objects = gc.GetTotalObjects();
        REQUIRE(initial_objects == 2);
        
        // 执行完整的GC循环
        gc.CollectGarbage(&vm);
        
        // 不可达对象应该被回收
        REQUIRE(gc.GetTotalObjects() == 1);
        
        // 可达对象应该仍然存在
        REQUIRE(vm.Top().IsString());
        REQUIRE(vm.Top().GetString() == reachable_str);
    }

    SECTION("复杂对象图标记") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建复杂的对象图：表包含字符串和其他表
        auto root_table = gc.AllocateTable(2, 2);
        auto child_table = gc.AllocateTable(1, 1);
        auto str1 = gc.AllocateString("key1");
        auto str2 = gc.AllocateString("value1");
        auto str3 = gc.AllocateString("orphan"); // 孤立字符串
        
        // 建立引用关系
        root_table->Set(TValue::CreateString(str1), TValue::CreateString(str2));
        root_table->Set(TValue::CreateNumber(1), TValue::CreateTable(child_table));
        child_table->Set(TValue::CreateNumber(1), TValue::CreateString(str1)); // 循环引用
        
        // 只有root_table在栈上，形成可达性链
        vm.Push(TValue::CreateTable(root_table));
        
        Size initial_objects = gc.GetTotalObjects();
        REQUIRE(initial_objects == 5);
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        // 只有孤立的str3应该被回收
        REQUIRE(gc.GetTotalObjects() == 4);
    }

    SECTION("循环引用处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建循环引用：table1 -> table2 -> table1
        auto table1 = gc.AllocateTable(1, 1);
        auto table2 = gc.AllocateTable(1, 1);
        
        table1->Set(TValue::CreateString("next"), TValue::CreateTable(table2));
        table2->Set(TValue::CreateString("prev"), TValue::CreateTable(table1));
        
        // 没有外部引用到这个循环
        
        Size initial_objects = gc.GetTotalObjects();
        REQUIRE(initial_objects == 2);
        
        // 执行GC应该回收整个循环
        gc.CollectGarbage(&vm);
        
        REQUIRE(gc.GetTotalObjects() == 0);
    }

    SECTION("标记颜色管理") {
        GarbageCollector gc;
        
        auto obj = gc.AllocateString("test");
        
        // 新对象应该是白色
        REQUIRE(obj->GetColor() == GCColor::White);
        
        // 手动标记为灰色
        obj->SetColor(GCColor::Gray);
        REQUIRE(obj->GetColor() == GCColor::Gray);
        
        // 标记为黑色
        obj->SetColor(GCColor::Black);
        REQUIRE(obj->GetColor() == GCColor::Black);
        
        // 重置为白色
        obj->SetColor(GCColor::White);
        REQUIRE(obj->GetColor() == GCColor::White);
    }
}

/* ========================================================================== */
/* 增量垃圾回收契约 */
/* ========================================================================== */

TEST_CASE("GC - 增量垃圾回收契约", "[gc][contract][incremental]") {
    SECTION("增量GC步骤执行") {
        GCConfig config;
        config.enable_incremental = true;
        config.step_multiplier = 200;
        GarbageCollector gc(config);
        VirtualMachine vm;
        
        // 创建一些对象
        for (int i = 0; i < 10; ++i) {
            gc.AllocateString("test " + std::to_string(i));
        }
        
        // 开始增量GC
        gc.StartIncrementalCollection(&vm);
        REQUIRE(gc.GetState() != GCState::Pause);
        
        // 执行一些增量步骤
        bool completed = false;
        int max_steps = 100;
        for (int i = 0; i < max_steps && !completed; ++i) {
            completed = gc.IncrementalStep(&vm, 100); // 每步100字节工作量
        }
        
        // 应该最终完成
        REQUIRE(completed);
        REQUIRE(gc.GetState() == GCState::Pause);
    }

    SECTION("增量GC中断和恢复") {
        GCConfig config;
        config.enable_incremental = true;
        GarbageCollector gc(config);
        VirtualMachine vm;
        
        // 创建对象并开始GC
        auto obj = gc.AllocateString("persistent");
        vm.Push(TValue::CreateString(obj));
        
        gc.StartIncrementalCollection(&vm);
        GCState initial_state = gc.GetState();
        
        // 执行部分工作
        gc.IncrementalStep(&vm, 50);
        
        // 在GC过程中分配新对象
        auto new_obj = gc.AllocateString("new during gc");
        
        // 新对象应该被正确处理（标记为黑色或加入灰色队列）
        REQUIRE((new_obj->GetColor() == GCColor::Black) || 
                (new_obj->GetColor() == GCColor::Gray));
        
        // 完成GC
        while (!gc.IncrementalStep(&vm, 100)) {
            // 继续执行直到完成
        }
        
        REQUIRE(gc.GetState() == GCState::Pause);
    }

    SECTION("写屏障处理") {
        GCConfig config;
        config.enable_incremental = true;
        GarbageCollector gc(config);
        VirtualMachine vm;
        
        auto table = gc.AllocateTable(2, 2);
        auto str1 = gc.AllocateString("initial");
        auto str2 = gc.AllocateString("new");
        
        // 设置初始引用
        table->Set(TValue::CreateString("key"), TValue::CreateString(str1));
        vm.Push(TValue::CreateTable(table));
        
        // 开始增量GC并执行到传播阶段
        gc.StartIncrementalCollection(&vm);
        while (gc.GetState() != GCState::Propagate && 
               !gc.IncrementalStep(&vm, 100)) {
            // 等待到达传播阶段
        }
        
        // 模拟写屏障：在GC过程中修改引用
        table->Set(TValue::CreateString("key"), TValue::CreateString(str2));
        
        // 触发写屏障
        gc.WriteBarrier(table, str2);
        
        // 新引用的对象应该被正确标记
        REQUIRE(str2->GetColor() != GCColor::White);
    }

    SECTION("增量GC性能监控") {
        GCConfig config;
        config.enable_incremental = true;
        GarbageCollector gc(config);
        VirtualMachine vm;
        
        // 执行一次完整的增量GC
        gc.StartIncrementalCollection(&vm);
        
        auto start_time = std::chrono::high_resolution_clock::now();
        while (!gc.IncrementalStep(&vm, 100)) {
            // 继续执行
        }
        auto end_time = std::chrono::high_resolution_clock::now();
        
        // 获取性能统计
        auto stats = gc.GetStatistics();
        REQUIRE(stats.total_collections > 0);
        REQUIRE(stats.incremental_steps > 0);
        REQUIRE(stats.total_gc_time > 0);
        
        // 增量GC应该比全量GC有更好的响应时间
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time).count();
        REQUIRE(duration < 10000); // 应该在10ms内完成（取决于硬件）
    }
}

/* ========================================================================== */
/* 弱引用处理契约 */
/* ========================================================================== */

TEST_CASE("GC - 弱引用处理契约", "[gc][contract][weak_references]") {
    SECTION("弱键表处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建弱键表
        auto weak_table = gc.AllocateWeakTable(WeakMode::Keys);
        auto key_obj = gc.AllocateString("weak_key");
        auto value_obj = gc.AllocateString("strong_value");
        
        // 设置弱引用
        weak_table->Set(TValue::CreateString(key_obj), TValue::CreateString(value_obj));
        vm.Push(TValue::CreateTable(weak_table)); // 表本身是强引用
        
        // key_obj没有其他强引用
        
        Size initial_objects = gc.GetTotalObjects();
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        // 弱键应该被回收，对应的键值对也应该被移除
        REQUIRE(gc.GetTotalObjects() < initial_objects);
        REQUIRE(weak_table->Get(TValue::CreateString(key_obj)).IsNil());
    }

    SECTION("弱值表处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建弱值表
        auto weak_table = gc.AllocateWeakTable(WeakMode::Values);
        auto key_obj = gc.AllocateString("strong_key");
        auto value_obj = gc.AllocateString("weak_value");
        
        weak_table->Set(TValue::CreateString(key_obj), TValue::CreateString(value_obj));
        vm.Push(TValue::CreateTable(weak_table));
        vm.Push(TValue::CreateString(key_obj)); // 键是强引用
        
        // value_obj没有其他强引用
        
        Size initial_objects = gc.GetTotalObjects();
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        // 弱值应该被回收，对应的键值对也应该被移除
        REQUIRE(gc.GetTotalObjects() < initial_objects);
        REQUIRE(weak_table->Get(TValue::CreateString(key_obj)).IsNil());
    }

    SECTION("双向弱引用表处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建双向弱引用表
        auto weak_table = gc.AllocateWeakTable(WeakMode::KeysAndValues);
        auto key_obj = gc.AllocateString("weak_key");
        auto value_obj = gc.AllocateString("weak_value");
        
        weak_table->Set(TValue::CreateString(key_obj), TValue::CreateString(value_obj));
        vm.Push(TValue::CreateTable(weak_table));
        
        // 键和值都没有其他强引用
        
        Size initial_objects = gc.GetTotalObjects();
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        // 键值对都应该被回收
        REQUIRE(gc.GetTotalObjects() < initial_objects);
        REQUIRE(weak_table->Size() == 0);
    }

    SECTION("弱引用复活处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        auto weak_table = gc.AllocateWeakTable(WeakMode::Values);
        auto key = gc.AllocateString("key");
        auto value = gc.AllocateString("value");
        
        // 设置弱引用
        weak_table->Set(TValue::CreateString(key), TValue::CreateString(value));
        vm.Push(TValue::CreateTable(weak_table));
        vm.Push(TValue::CreateString(key));
        
        // 在GC过程中，如果值被复活（通过某种方式获得强引用）
        // 它应该被保留
        
        // 设置复活回调来模拟终结器复活对象
        gc.SetResurrectionCallback([&](GCObject* obj) {
            if (obj == value) {
                vm.Push(TValue::CreateString(static_cast<StringObject*>(obj)));
                return true; // 对象被复活
            }
            return false;
        });
        
        Size initial_objects = gc.GetTotalObjects();
        gc.CollectGarbage(&vm);
        
        // 值应该被复活并保留在弱表中
        REQUIRE(!weak_table->Get(TValue::CreateString(key)).IsNil());
    }
}

/* ========================================================================== */
/* 终结器处理契约 */
/* ========================================================================== */

TEST_CASE("GC - 终结器处理契约", "[gc][contract][finalizers]") {
    SECTION("基础终结器执行") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        bool finalizer_called = false;
        auto obj = gc.AllocateUserData(100);
        
        // 设置终结器
        obj->SetFinalizer([&finalizer_called](GCObject* obj) {
            finalizer_called = true;
        });
        
        // 对象没有强引用，应该被回收
        gc.CollectGarbage(&vm);
        
        // 终结器应该被调用
        REQUIRE(finalizer_called);
    }

    SECTION("终结器执行顺序") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        std::vector<int> execution_order;
        
        // 创建多个有终结器的对象
        for (int i = 0; i < 5; ++i) {
            auto obj = gc.AllocateUserData(10);
            obj->SetFinalizer([&execution_order, i](GCObject* obj) {
                execution_order.push_back(i);
            });
        }
        
        gc.CollectGarbage(&vm);
        
        // 所有终结器都应该被执行
        REQUIRE(execution_order.size() == 5);
        
        // 执行顺序应该是确定的（通常是创建的逆序）
        bool is_reverse_order = true;
        for (size_t i = 0; i < execution_order.size(); ++i) {
            if (execution_order[i] != static_cast<int>(4 - i)) {
                is_reverse_order = false;
                break;
            }
        }
        REQUIRE(is_reverse_order);
    }

    SECTION("终结器异常处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        bool normal_finalizer_called = false;
        auto obj1 = gc.AllocateUserData(10);
        auto obj2 = gc.AllocateUserData(10);
        
        // 设置会抛出异常的终结器
        obj1->SetFinalizer([](GCObject* obj) {
            throw std::runtime_error("Finalizer error");
        });
        
        // 设置正常的终结器
        obj2->SetFinalizer([&normal_finalizer_called](GCObject* obj) {
            normal_finalizer_called = true;
        });
        
        // GC应该处理终结器异常，不影响其他终结器
        REQUIRE_NOTHROW(gc.CollectGarbage(&vm));
        
        // 正常的终结器应该仍然被执行
        REQUIRE(normal_finalizer_called);
    }

    SECTION("终结器对象复活") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        auto obj = gc.AllocateUserData(100);
        bool finalizer_called = false;
        
        // 设置会复活对象的终结器
        obj->SetFinalizer([&](GCObject* obj) {
            finalizer_called = true;
            // 将对象重新添加到全局表中，复活它
            vm.Push(TValue::CreateUserData(static_cast<UserDataObject*>(obj)));
        });
        
        Size initial_objects = gc.GetTotalObjects();
        gc.CollectGarbage(&vm);
        
        // 终结器应该被调用
        REQUIRE(finalizer_called);
        
        // 但对象应该被复活，仍然存在
        REQUIRE(gc.GetTotalObjects() == initial_objects);
        REQUIRE(!vm.Top().IsNil());
    }
}

/* ========================================================================== */
/* GC API契约 */
/* ========================================================================== */

TEST_CASE("GC - API契约", "[gc][contract][api]") {
    SECTION("手动GC控制") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 停止自动GC
        gc.SetAutomaticGC(false);
        REQUIRE(!gc.IsAutomaticGCEnabled());
        
        // 分配大量对象
        for (int i = 0; i < 100; ++i) {
            gc.AllocateString("test " + std::to_string(i));
        }
        
        Size objects_before = gc.GetTotalObjects();
        
        // 手动触发GC
        gc.CollectGarbage(&vm);
        
        // 对象应该被回收
        REQUIRE(gc.GetTotalObjects() < objects_before);
        
        // 重新启用自动GC
        gc.SetAutomaticGC(true);
        REQUIRE(gc.IsAutomaticGCEnabled());
    }

    SECTION("GC参数调整") {
        GarbageCollector gc;
        
        // 调整GC参数
        gc.SetPause(150);
        gc.SetStepMultiplier(250);
        gc.SetThreshold(2048);
        
        REQUIRE(gc.GetPause() == 150);
        REQUIRE(gc.GetStepMultiplier() == 250);
        REQUIRE(gc.GetThreshold() == 2048);
        
        // 参数应该影响GC行为
        Size old_threshold = gc.GetThreshold();
        VirtualMachine vm;
        
        // 分配内存直到触发GC
        while (gc.GetAllocatedBytes() < old_threshold) {
            gc.AllocateString("trigger gc");
        }
        
        // 新的阈值应该基于pause参数计算
        Size new_threshold = gc.GetThreshold();
        REQUIRE(new_threshold > old_threshold);
    }

    SECTION("GC统计信息") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        auto initial_stats = gc.GetStatistics();
        
        // 执行一些分配和回收
        for (int i = 0; i < 50; ++i) {
            gc.AllocateString("test");
        }
        gc.CollectGarbage(&vm);
        
        auto final_stats = gc.GetStatistics();
        
        // 统计信息应该被更新
        REQUIRE(final_stats.total_collections > initial_stats.total_collections);
        REQUIRE(final_stats.total_allocated > initial_stats.total_allocated);
        REQUIRE(final_stats.total_freed >= 0);
        REQUIRE(final_stats.total_gc_time > initial_stats.total_gc_time);
    }

    SECTION("内存使用限制") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 设置内存限制
        Size memory_limit = 4096; // 4KB
        gc.SetMemoryLimit(memory_limit);
        REQUIRE(gc.GetMemoryLimit() == memory_limit);
        
        // 尝试分配超过限制的内存
        REQUIRE_THROWS_AS([&]() {
            for (int i = 0; i < 1000; ++i) {
                gc.AllocateString(std::string(100, 'x')); // 每个约100字节
            }
        }(), OutOfMemoryError);
        
        // 移除限制
        gc.SetMemoryLimit(0); // 0表示无限制
        REQUIRE(gc.GetMemoryLimit() == 0);
        
        // 现在应该可以分配更多内存
        REQUIRE_NOTHROW([&]() {
            for (int i = 0; i < 10; ++i) {
                gc.AllocateString(std::string(100, 'x'));
            }
        }());
    }
}

/* ========================================================================== */
/* 多线程安全契约 */
/* ========================================================================== */

TEST_CASE("GC - 多线程安全契约", "[gc][contract][threading]") {
    SECTION("并发分配安全") {
        GarbageCollector gc;
        const int num_threads = 4;
        const int objects_per_thread = 100;
        
        std::vector<std::thread> threads;
        std::atomic<int> total_allocated{0};
        
        // 多线程并发分配
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&gc, &total_allocated, objects_per_thread, t]() {
                for (int i = 0; i < objects_per_thread; ++i) {
                    auto obj = gc.AllocateString("thread_" + std::to_string(t) + "_obj_" + std::to_string(i));
                    if (obj) {
                        total_allocated++;
                    }
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 验证所有对象都被正确分配
        REQUIRE(total_allocated == num_threads * objects_per_thread);
        REQUIRE(gc.GetTotalObjects() == static_cast<Size>(num_threads * objects_per_thread));
    }

    SECTION("GC执行期间的分配") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        std::atomic<bool> gc_running{false};
        std::atomic<bool> allocation_succeeded{true};
        
        // 在一个线程中执行GC
        std::thread gc_thread([&]() {
            gc_running = true;
            gc.CollectGarbage(&vm);
            gc_running = false;
        });
        
        // 在另一个线程中尝试分配
        std::thread alloc_thread([&]() {
            while (!gc_running) {
                std::this_thread::yield();
            }
            
            try {
                for (int i = 0; i < 10; ++i) {
                    gc.AllocateString("concurrent_alloc_" + std::to_string(i));
                }
            } catch (...) {
                allocation_succeeded = false;
            }
        });
        
        gc_thread.join();
        alloc_thread.join();
        
        // 并发分配应该成功或安全失败
        REQUIRE(allocation_succeeded);
    }

    SECTION("多VM共享GC") {
        GarbageCollector shared_gc;
        VirtualMachine vm1, vm2, vm3;
        
        // 多个VM使用同一个GC
        vm1.SetGarbageCollector(&shared_gc);
        vm2.SetGarbageCollector(&shared_gc);
        vm3.SetGarbageCollector(&shared_gc);
        
        // 在不同VM中分配对象
        auto obj1 = shared_gc.AllocateString("vm1_object");
        auto obj2 = shared_gc.AllocateString("vm2_object");
        auto obj3 = shared_gc.AllocateString("vm3_object");
        
        vm1.Push(TValue::CreateString(obj1));
        vm2.Push(TValue::CreateString(obj2));
        vm3.Push(TValue::CreateString(obj3));
        
        Size initial_objects = shared_gc.GetTotalObjects();
        
        // 在一个VM中触发GC，应该扫描所有VM的根对象
        shared_gc.CollectGarbage(&vm1, {&vm2, &vm3});
        
        // 所有VM中的对象都应该被保留
        REQUIRE(shared_gc.GetTotalObjects() == initial_objects);
        REQUIRE(!vm1.Top().IsNil());
        REQUIRE(!vm2.Top().IsNil());
        REQUIRE(!vm3.Top().IsNil());
    }
}

/* ========================================================================== */
/* 性能和压力测试契约 */
/* ========================================================================== */

TEST_CASE("GC - 性能测试契约", "[gc][contract][performance]") {
    SECTION("大规模对象分配回收") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        const int num_objects = 10000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // 分配大量对象
        std::vector<GCObject*> objects;
        for (int i = 0; i < num_objects; ++i) {
            objects.push_back(gc.AllocateString("object_" + std::to_string(i)));
        }
        
        auto alloc_time = std::chrono::high_resolution_clock::now();
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        auto gc_time = std::chrono::high_resolution_clock::now();
        
        // 计算性能指标
        auto alloc_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            alloc_time - start_time).count();
        auto gc_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            gc_time - alloc_time).count();
        
        // 性能应该在可接受范围内
        REQUIRE(alloc_duration < 1000); // 分配应该在1秒内完成
        REQUIRE(gc_duration < 500);     // GC应该在0.5秒内完成
        
        // 所有对象应该被回收（没有根引用）
        REQUIRE(gc.GetTotalObjects() == 0);
    }

    SECTION("内存碎片化处理") {
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 分配和释放不同大小的对象以产生碎片
        std::vector<GCObject*> kept_objects;
        
        for (int cycle = 0; cycle < 10; ++cycle) {
            // 分配各种大小的对象
            for (int size = 1; size <= 1000; size *= 2) {
                auto obj = gc.AllocateString(std::string(size, 'x'));
                if (cycle % 2 == 0) {
                    kept_objects.push_back(obj);
                    vm.Push(TValue::CreateString(static_cast<StringObject*>(obj)));
                }
            }
            
            // 定期GC
            if (cycle % 3 == 0) {
                gc.CollectGarbage(&vm);
            }
        }
        
        auto stats = gc.GetStatistics();
        
        // 内存效率应该保持合理
        REQUIRE(stats.fragmentation_ratio < 0.5); // 碎片率应该小于50%
        REQUIRE(stats.memory_efficiency > 0.7);   // 内存效率应该大于70%
    }

    SECTION("GC暂停时间测试") {
        GCConfig config;
        config.enable_incremental = true;
        config.step_multiplier = 100; // 更小的步骤
        GarbageCollector gc(config);
        VirtualMachine vm;
        
        // 分配足够多的对象
        for (int i = 0; i < 1000; ++i) {
            gc.AllocateString("pause_test_" + std::to_string(i));
        }
        
        // 测量增量GC的最大暂停时间
        std::vector<long> pause_times;
        
        gc.StartIncrementalCollection(&vm);
        while (gc.GetState() != GCState::Pause) {
            auto start = std::chrono::high_resolution_clock::now();
            gc.IncrementalStep(&vm, 100);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end - start).count();
            pause_times.push_back(duration);
        }
        
        // 计算最大暂停时间
        auto max_pause = *std::max_element(pause_times.begin(), pause_times.end());
        
        // 增量GC的暂停时间应该很短
        REQUIRE(max_pause < 1000); // 应该小于1ms
        
        // 大部分暂停应该更短
        int short_pauses = std::count_if(pause_times.begin(), pause_times.end(),
                                       [](long pause) { return pause < 100; });
        REQUIRE(short_pauses > static_cast<int>(pause_times.size() * 0.8)); // 80%的暂停应该小于100微秒
    }
}

/* ========================================================================== */
/* Lua 5.1.5兼容性验证契约 */
/* ========================================================================== */

TEST_CASE("GC - Lua 5.1.5兼容性验证契约", "[gc][contract][compatibility]") {
    SECTION("GC状态机兼容性") {
        // 🔍 lua_c_analysis验证: 确保与lgc.c中的GC状态机一致
        GarbageCollector gc;
        
        // Lua 5.1.5的5状态GC机器：Pause -> Propagate -> Sweep -> Finalize -> Pause
        REQUIRE(gc.GetState() == GCState::Pause);
        
        // 验证状态转换序列与Lua 5.1.5一致
        gc.SetState(GCState::Propagate);
        REQUIRE(gc.GetState() == GCState::Propagate);
        
        gc.SetState(GCState::AtomicMark);
        REQUIRE(gc.GetState() == GCState::AtomicMark);
        
        gc.SetState(GCState::Sweep);
        REQUIRE(gc.GetState() == GCState::Sweep);
        
        gc.SetState(GCState::Finalize);
        REQUIRE(gc.GetState() == GCState::Finalize);
        
        gc.SetState(GCState::Pause);
        REQUIRE(gc.GetState() == GCState::Pause);
    }

    SECTION("内存阈值计算兼容性") {
        // 🔍 lua_c_analysis验证: 阈值计算与Lua 5.1.5公式一致
        GCConfig config;
        config.pause_multiplier = 200; // Lua 5.1.5默认值
        GarbageCollector gc(config);
        
        Size initial_memory = 1024;
        gc.SetAllocatedMemory(initial_memory);
        
        // Lua 5.1.5公式: threshold = totalbytes * pause / 100
        Size expected_threshold = initial_memory * config.pause_multiplier / 100;
        gc.UpdateThreshold();
        
        REQUIRE(gc.GetThreshold() == expected_threshold);
    }

    SECTION("三色标记算法兼容性") {
        // 🔍 lua_c_analysis验证: 三色标记与lgc.c算法一致
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 创建对象图模拟Lua 5.1.5的典型场景
        auto table = gc.AllocateTable(4, 4);
        auto str1 = gc.AllocateString("test1");
        auto str2 = gc.AllocateString("test2");
        auto func = gc.AllocateFunction(nullptr);
        
        // 建立复杂引用关系
        table->Set(TValue::CreateString("key1"), TValue::CreateString(str1));
        table->Set(TValue::CreateString("key2"), TValue::CreateFunction(func));
        table->Set(TValue::CreateNumber(1), TValue::CreateString(str2));
        
        vm.Push(TValue::CreateTable(table));
        
        // 手动控制标记过程，验证颜色转换
        REQUIRE(table->GetColor() == GCColor::White);
        REQUIRE(str1->GetColor() == GCColor::White);
        REQUIRE(str2->GetColor() == GCColor::White);
        
        // 开始标记过程
        gc.StartMarking(&vm);
        
        // 根对象应该变为灰色
        REQUIRE(table->GetColor() == GCColor::Gray);
        
        // 传播标记
        gc.PropagateMarks();
        
        // 所有可达对象应该变为黑色
        REQUIRE(table->GetColor() == GCColor::Black);
        REQUIRE(str1->GetColor() == GCColor::Black);
        REQUIRE(str2->GetColor() == GCColor::Black);
        REQUIRE(func->GetColor() == GCColor::Black);
    }

    SECTION("增量回收步长兼容性") {
        // 🔍 lua_c_analysis验证: 步长计算与Lua 5.1.5一致
        GCConfig config;
        config.step_multiplier = 200;
        GarbageCollector gc(config);
        VirtualMachine vm;
        
        // 分配足够的对象触发增量GC
        Size allocated_before = gc.GetAllocatedBytes();
        for (int i = 0; i < 100; ++i) {
            gc.AllocateString("step_test_" + std::to_string(i));
        }
        Size allocated_after = gc.GetAllocatedBytes();
        Size allocated_delta = allocated_after - allocated_before;
        
        // Lua 5.1.5步长计算公式
        Size expected_step_size = allocated_delta * config.step_multiplier / 100;
        
        gc.StartIncrementalCollection(&vm);
        Size actual_step_size = gc.GetStepSize();
        
        REQUIRE(actual_step_size >= expected_step_size * 0.8); // 允许20%的误差
        REQUIRE(actual_step_size <= expected_step_size * 1.2);
    }

    SECTION("弱表处理兼容性") {
        // 🔍 lua_c_analysis验证: 弱表处理与Lua 5.1.5一致
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 测试Lua 5.1.5的弱表语义
        auto weak_k_table = gc.AllocateWeakTable(WeakMode::Keys);
        auto weak_v_table = gc.AllocateWeakTable(WeakMode::Values);
        auto weak_kv_table = gc.AllocateWeakTable(WeakMode::KeysAndValues);
        
        auto key = gc.AllocateString("weak_key");
        auto value = gc.AllocateString("weak_value");
        
        // 设置弱引用
        weak_k_table->Set(TValue::CreateString(key), TValue::CreateString(value));
        weak_v_table->Set(TValue::CreateString(key), TValue::CreateString(value));
        weak_kv_table->Set(TValue::CreateString(key), TValue::CreateString(value));
        
        // 保持表的强引用，但不保持key/value的强引用
        vm.Push(TValue::CreateTable(weak_k_table));
        vm.Push(TValue::CreateTable(weak_v_table));
        vm.Push(TValue::CreateTable(weak_kv_table));
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        // 验证弱引用清理符合Lua 5.1.5语义
        REQUIRE(weak_k_table->Get(TValue::CreateString(key)).IsNil());
        REQUIRE(weak_v_table->Get(TValue::CreateString(key)).IsNil());
        REQUIRE(weak_kv_table->Size() == 0);
    }
}

/* ========================================================================== */
/* 双重验证机制集成测试 */
/* ========================================================================== */

TEST_CASE("GC - 双重验证机制集成测试", "[gc][contract][verification]") {
    SECTION("lua_c_analysis行为验证") {
        // 🔍 验证与Lua 5.1.5原版lgc.c的行为一致性
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 模拟典型的Lua程序内存分配模式
        std::vector<GCObject*> objects;
        
        // 分配模式1: 大量短生命周期字符串
        for (int i = 0; i < 1000; ++i) {
            objects.push_back(gc.AllocateString("temp_" + std::to_string(i)));
        }
        
        // 分配模式2: 少量长生命周期表结构
        for (int i = 0; i < 10; ++i) {
            auto table = gc.AllocateTable(16, 8);
            vm.Push(TValue::CreateTable(table)); // 保持强引用
            objects.push_back(table);
        }
        
        Size before_gc = gc.GetAllocatedBytes();
        Size objects_before = gc.GetTotalObjects();
        
        // 执行完整GC周期
        gc.CollectGarbage(&vm);
        
        Size after_gc = gc.GetAllocatedBytes();
        Size objects_after = gc.GetTotalObjects();
        
        // 验证回收效果符合预期
        REQUIRE(after_gc < before_gc); // 应该回收了内存
        REQUIRE(objects_after < objects_before); // 应该回收了对象
        REQUIRE(objects_after >= 10); // 至少保留了栈上的表对象
        
        // 验证GC状态正确重置
        REQUIRE(gc.GetState() == GCState::Pause);
    }

    SECTION("lua_with_cpp设计验证") {
        // 🏗️ 验证现代C++垃圾回收器设计的正确性
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 测试现代C++特性集成
        
        // 1. RAII和异常安全
        try {
            auto obj = gc.AllocateString("exception_test");
            vm.Push(TValue::CreateString(obj));
            
            // 模拟异常情况
            if (obj != nullptr) {
                // 正常情况，无异常
                SUCCEED();
            }
        } catch (...) {
            FAIL("GC allocation should not throw exceptions");
        }
        
        // 2. 类型安全和强类型检查
        auto str_obj = gc.AllocateString("type_test");
        auto table_obj = gc.AllocateTable(2, 2);
        
        REQUIRE(str_obj->GetType() == GCObjectType::String);
        REQUIRE(table_obj->GetType() == GCObjectType::Table);
        
        // 3. 现代内存管理
        Size initial_memory = gc.GetAllocatedBytes();
        {
            // 作用域内分配
            auto scoped_obj = gc.AllocateString("scoped");
            REQUIRE(gc.GetAllocatedBytes() > initial_memory);
        }
        // 作用域外应该能够被GC回收
        gc.CollectGarbage(&vm);
        
        // 4. 线程安全检查（如果支持）
        if (gc.IsThreadSafe()) {
            std::atomic<bool> allocation_successful{true};
            
            // 简单的并发分配测试
            std::thread t1([&gc, &allocation_successful]() {
                try {
                    for (int i = 0; i < 100; ++i) {
                        gc.AllocateString("thread1_" + std::to_string(i));
                    }
                } catch (...) {
                    allocation_successful = false;
                }
            });
            
            std::thread t2([&gc, &allocation_successful]() {
                try {
                    for (int i = 0; i < 100; ++i) {
                        gc.AllocateString("thread2_" + std::to_string(i));
                    }
                } catch (...) {
                    allocation_successful = false;
                }
            });
            
            t1.join();
            t2.join();
            
            REQUIRE(allocation_successful);
        }
    }

    SECTION("性能基准验证") {
        // 🔍🏗️ 双重验证: 性能应该接近lua_c_analysis，但具备lua_with_cpp的现代特性
        GarbageCollector gc;
        VirtualMachine vm;
        
        // 大规模分配性能测试
        const int test_objects = 10000;
        auto start_time = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < test_objects; ++i) {
            auto obj = gc.AllocateString("perf_test_" + std::to_string(i));
            if (i % 10 == 0) {
                vm.Push(TValue::CreateString(obj)); // 保持部分对象存活
            }
        }
        
        auto alloc_time = std::chrono::high_resolution_clock::now();
        
        // 执行GC
        gc.CollectGarbage(&vm);
        
        auto gc_time = std::chrono::high_resolution_clock::now();
        
        // 计算性能指标
        auto alloc_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            alloc_time - start_time).count();
        auto gc_duration = std::chrono::duration_cast<std::chrono::microseconds>(
            gc_time - alloc_time).count();
        
        // 性能要求（基于经验值）
        REQUIRE(alloc_duration < test_objects * 10); // 平均每个对象分配<10微秒
        REQUIRE(gc_duration < test_objects * 5);     // 平均每个对象GC<5微秒
        
        // 内存效率验证
        auto stats = gc.GetStatistics();
        REQUIRE(stats.memory_efficiency > 0.8);     // 内存效率>80%
        REQUIRE(stats.fragmentation_ratio < 0.3);   // 碎片率<30%
    }
}