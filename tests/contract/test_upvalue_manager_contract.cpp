#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "vm/upvalue_manager.h"
#include "core/lua_value.h"
#include "vm/stack.h"

namespace lua_cpp {

/**
 * @brief UpvalueManager 契约测试
 * 
 * 验证Upvalue管理系统的基本契约和不变式
 */
class UpvalueManagerContractTest {
public:
    static void TestBasicContracts();
    static void TestLifecycleContracts();
    static void TestSharingContracts();
    static void TestGCContracts();
    static void TestPerformanceContracts();
};

TEST_CASE("UpvalueManager - Basic Contracts", "[contract][upvalue_manager]") {
    UpvalueManagerContractTest::TestBasicContracts();
}

TEST_CASE("UpvalueManager - Lifecycle Contracts", "[contract][upvalue_manager][lifecycle]") {
    UpvalueManagerContractTest::TestLifecycleContracts();
}

TEST_CASE("UpvalueManager - Sharing Contracts", "[contract][upvalue_manager][sharing]") {
    UpvalueManagerContractTest::TestSharingContracts();
}

TEST_CASE("UpvalueManager - GC Contracts", "[contract][upvalue_manager][gc]") {
    UpvalueManagerContractTest::TestGCContracts();
}

TEST_CASE("UpvalueManager - Performance Contracts", "[contract][upvalue_manager][performance]") {
    UpvalueManagerContractTest::TestPerformanceContracts();
}

/* ========================================================================== */
/* 基本契约测试实现 */
/* ========================================================================== */

void UpvalueManagerContractTest::TestBasicContracts() {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("初始状态契约") {
        // 管理器应该为空
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 0);
        REQUIRE(stats.open_upvalues == 0);
        REQUIRE(stats.closed_upvalues == 0);
        
        // 完整性应该验证通过
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("创建Upvalue契约") {
        // 在栈上放置一个值
        LuaValue value = LuaValue::Number(42.0);
        stack.Push(value);
        
        // 创建Upvalue应该成功
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        REQUIRE(upvalue != nullptr);
        REQUIRE(upvalue->IsOpen());
        REQUIRE(upvalue->GetValue() == value);
        
        // 统计信息应该更新
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 1);
        REQUIRE(stats.open_upvalues == 1);
        REQUIRE(stats.closed_upvalues == 0);
        
        // 完整性应该保持
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("关闭Upvalue契约") {
        LuaValue value = LuaValue::String("test");
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        REQUIRE(upvalue->IsOpen());
        
        // 关闭Upvalue
        manager.CloseUpvalues(&stack, 0);
        
        // Upvalue应该变为关闭状态
        REQUIRE(upvalue->IsClosed());
        REQUIRE(upvalue->GetValue() == value);  // 值应该保持
        
        // 统计信息应该更新
        auto stats = manager.GetStatistics();
        REQUIRE(stats.open_upvalues == 0);
        REQUIRE(stats.closed_upvalues == 1);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("无效索引契约") {
        // 无效的栈索引应该抛出异常
        REQUIRE_THROWS_AS(manager.CreateUpvalue(&stack, 100), UpvalueError);
        REQUIRE_THROWS_AS(manager.CreateUpvalue(&stack, -1), UpvalueError);
        
        REQUIRE(manager.ValidateIntegrity());
    }
}

void UpvalueManagerContractTest::TestLifecycleContracts() {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Upvalue生命周期契约") {
        LuaValue value = LuaValue::Number(123.0);
        stack.Push(value);
        
        // 创建阶段
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        REQUIRE(upvalue->IsOpen());
        REQUIRE(upvalue->GetReferenceCount() == 1);
        
        // 增加引用
        upvalue->AddReference();
        REQUIRE(upvalue->GetReferenceCount() == 2);
        
        // 减少引用
        upvalue->RemoveReference();
        REQUIRE(upvalue->GetReferenceCount() == 1);
        
        // 关闭阶段
        manager.CloseUpvalues(&stack, 0);
        REQUIRE(upvalue->IsClosed());
        
        // 关闭后的Upvalue仍然有效
        REQUIRE(upvalue->GetValue() == value);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("栈收缩时的契约") {
        // 创建多个Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        
        for (int i = 0; i < 5; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        // 模拟栈收缩（关闭部分Upvalue）
        manager.CloseUpvalues(&stack, 2);
        
        // 索引2及以上的Upvalue应该关闭
        REQUIRE(upvalues[0]->IsOpen());
        REQUIRE(upvalues[1]->IsOpen());
        REQUIRE(upvalues[2]->IsClosed());
        REQUIRE(upvalues[3]->IsClosed());
        REQUIRE(upvalues[4]->IsClosed());
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("引用计数零时的契约") {
        LuaValue value = LuaValue::Boolean(true);
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        auto initial_count = manager.GetStatistics().total_upvalues;
        
        // 模拟引用计数归零
        upvalue->RemoveReference();  // 应该触发自动销毁
        
        // 注意：实际实现中可能需要显式的垃圾回收
        // 这里假设有自动清理机制
        
        REQUIRE(manager.ValidateIntegrity());
    }
}

void UpvalueManagerContractTest::TestSharingContracts() {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("相同位置共享契约") {
        LuaValue value = LuaValue::Number(456.0);
        stack.Push(value);
        
        // 对相同栈位置创建多个Upvalue应该返回同一个对象
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        auto upvalue2 = manager.CreateUpvalue(&stack, 0);
        
        REQUIRE(upvalue1.get() == upvalue2.get());
        REQUIRE(upvalue1->GetReferenceCount() == 2);
        
        // 统计应该正确
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 1);  // 只有一个物理Upvalue
        REQUIRE(stats.shared_upvalues == 1);  // 但被共享
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("不同位置独立契约") {
        stack.Push(LuaValue::Number(1));
        stack.Push(LuaValue::Number(2));
        
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        auto upvalue2 = manager.CreateUpvalue(&stack, 1);
        
        // 不同位置应该创建不同的Upvalue
        REQUIRE(upvalue1.get() != upvalue2.get());
        REQUIRE(upvalue1->GetReferenceCount() == 1);
        REQUIRE(upvalue2->GetReferenceCount() == 1);
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 2);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("关闭后不共享契约") {
        LuaValue value = LuaValue::String("shared");
        stack.Push(value);
        
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        manager.CloseUpvalues(&stack, 0);
        
        // 关闭后再创建应该得到新的Upvalue
        stack.Push(value);  // 重新压入相同值
        auto upvalue2 = manager.CreateUpvalue(&stack, 0);
        
        REQUIRE(upvalue1.get() != upvalue2.get());
        REQUIRE(upvalue1->IsClosed());
        REQUIRE(upvalue2->IsOpen());
        
        REQUIRE(manager.ValidateIntegrity());
    }
}

void UpvalueManagerContractTest::TestGCContracts() {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("GC标记契约") {
        LuaValue value = LuaValue::Number(789.0);
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        
        // 模拟GC标记阶段
        manager.MarkReachableUpvalues();
        
        // Upvalue应该被标记为可达
        REQUIRE(upvalue->IsMarked());
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("GC清理契约") {
        LuaValue value = LuaValue::String("gc_test");
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        auto initial_count = manager.GetStatistics().total_upvalues;
        
        // 模拟GC清理未标记的对象
        upvalue->Unmark();  // 取消标记
        auto cleaned = manager.SweepUnmarkedUpvalues();
        
        // 应该清理掉未标记的Upvalue
        REQUIRE(cleaned > 0);
        
        auto final_stats = manager.GetStatistics();
        REQUIRE(final_stats.total_upvalues < initial_count);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("GC统计契约") {
        // 创建一些Upvalue
        for (int i = 0; i < 10; ++i) {
            stack.Push(LuaValue::Number(i));
            manager.CreateUpvalue(&stack, i);
        }
        
        auto pre_gc_stats = manager.GetStatistics();
        
        // 执行完整的GC周期
        manager.MarkReachableUpvalues();
        auto swept = manager.SweepUnmarkedUpvalues();
        
        auto post_gc_stats = manager.GetStatistics();
        
        // GC统计应该更新
        REQUIRE(post_gc_stats.gc_cycles > pre_gc_stats.gc_cycles);
        REQUIRE(post_gc_stats.total_swept >= swept);
        
        REQUIRE(manager.ValidateIntegrity());
    }
}

void UpvalueManagerContractTest::TestPerformanceContracts() {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("缓存效率契约") {
        LuaValue value = LuaValue::Number(999.0);
        stack.Push(value);
        
        // 第一次创建
        auto start_stats = manager.GetStatistics();
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        
        // 第二次创建（应该从缓存获取）
        auto upvalue2 = manager.CreateUpvalue(&stack, 0);
        auto end_stats = manager.GetStatistics();
        
        // 缓存命中应该增加
        REQUIRE(end_stats.cache_hits > start_stats.cache_hits);
        REQUIRE(upvalue1.get() == upvalue2.get());
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("内存使用契约") {
        auto initial_memory = manager.GetMemoryUsage();
        
        // 创建多个Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (int i = 0; i < 100; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        auto peak_memory = manager.GetMemoryUsage();
        REQUIRE(peak_memory > initial_memory);
        
        // 清理Upvalue
        upvalues.clear();
        manager.SweepUnmarkedUpvalues();
        
        auto final_memory = manager.GetMemoryUsage();
        REQUIRE(final_memory <= peak_memory);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("操作时间复杂度契约") {
        auto stats = manager.GetStatistics();
        
        // 创建操作的时间应该合理
        REQUIRE(stats.avg_create_time >= 0.0);
        REQUIRE(stats.avg_close_time >= 0.0);
        
        // 执行一些操作并测量
        stack.Push(LuaValue::String("timing_test"));
        
        auto start_time = std::chrono::steady_clock::now();
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        auto end_time = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration<double, std::micro>(end_time - start_time).count();
        
        // 创建操作应该在合理时间内完成（比如1毫秒）
        REQUIRE(duration < 1000.0);  // 1000微秒 = 1毫秒
        
        REQUIRE(manager.ValidateIntegrity());
    }
}

} // namespace lua_cpp