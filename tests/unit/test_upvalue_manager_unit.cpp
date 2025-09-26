#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "vm/upvalue_manager.h"
#include "vm/stack.h"
#include "core/lua_value.h"
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

namespace lua_cpp {

/**
 * @brief UpvalueManager 单元测试
 * 
 * 详细测试Upvalue管理系统的各个功能模块
 */

TEST_CASE("UpvalueManager - Construction and Initialization", "[unit][upvalue_manager]") {
    SECTION("Default construction") {
        UpvalueManager manager;
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 0);
        REQUIRE(stats.open_upvalues == 0);
        REQUIRE(stats.closed_upvalues == 0);
        REQUIRE(stats.cache_hits == 0);
        REQUIRE(stats.cache_misses == 0);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("Memory usage initialization") {
        UpvalueManager manager;
        
        auto initial_memory = manager.GetMemoryUsage();
        REQUIRE(initial_memory >= sizeof(UpvalueManager));
    }
}

TEST_CASE("UpvalueManager - Upvalue Creation", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Create upvalue for valid stack position") {
        LuaValue value = LuaValue::Number(42.5);
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        
        REQUIRE(upvalue != nullptr);
        REQUIRE(upvalue->IsOpen());
        REQUIRE(upvalue->GetValue() == value);
        REQUIRE(upvalue->GetReferenceCount() == 1);
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 1);
        REQUIRE(stats.open_upvalues == 1);
        REQUIRE(stats.closed_upvalues == 0);
    }
    
    SECTION("Create multiple upvalues") {
        std::vector<LuaValue> values = {
            LuaValue::Number(1.0),
            LuaValue::String("test"),
            LuaValue::Boolean(true),
            LuaValue::Nil()
        };
        
        for (const auto& val : values) {
            stack.Push(val);
        }
        
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (Size i = 0; i < values.size(); ++i) {
            auto upvalue = manager.CreateUpvalue(&stack, i);
            upvalues.push_back(upvalue);
            
            REQUIRE(upvalue->GetValue() == values[i]);
        }
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == values.size());
        REQUIRE(stats.open_upvalues == values.size());
    }
    
    SECTION("Invalid stack position should throw") {
        // 空栈
        REQUIRE_THROWS_AS(manager.CreateUpvalue(&stack, 0), UpvalueError);
        
        // 超出范围的索引
        stack.Push(LuaValue::Number(1));
        REQUIRE_THROWS_AS(manager.CreateUpvalue(&stack, 5), UpvalueError);
        
        // 负索引
        REQUIRE_THROWS_AS(manager.CreateUpvalue(&stack, -1), UpvalueError);
    }
    
    SECTION("Null stack should throw") {
        REQUIRE_THROWS_AS(manager.CreateUpvalue(nullptr, 0), UpvalueError);
    }
}

TEST_CASE("UpvalueManager - Upvalue Sharing", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Same position sharing") {
        LuaValue value = LuaValue::String("shared_value");
        stack.Push(value);
        
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        auto upvalue2 = manager.CreateUpvalue(&stack, 0);
        
        // 应该返回同一个Upvalue对象
        REQUIRE(upvalue1.get() == upvalue2.get());
        REQUIRE(upvalue1->GetReferenceCount() == 2);
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 1);  // 物理上只有一个
        REQUIRE(stats.shared_upvalues == 1);
        REQUIRE(stats.cache_hits == 1);  // 第二次应该命中缓存
    }
    
    SECTION("Different positions no sharing") {
        stack.Push(LuaValue::Number(1));
        stack.Push(LuaValue::Number(2));
        
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        auto upvalue2 = manager.CreateUpvalue(&stack, 1);
        
        // 应该是不同的Upvalue对象
        REQUIRE(upvalue1.get() != upvalue2.get());
        REQUIRE(upvalue1->GetReferenceCount() == 1);
        REQUIRE(upvalue2->GetReferenceCount() == 1);
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 2);
    }
    
    SECTION("Sharing cache efficiency") {
        LuaValue value = LuaValue::Boolean(false);
        stack.Push(value);
        
        auto initial_stats = manager.GetStatistics();
        
        // 第一次创建
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        auto after_first = manager.GetStatistics();
        REQUIRE(after_first.cache_misses == initial_stats.cache_misses + 1);
        
        // 第二次创建（应该命中缓存）
        auto upvalue2 = manager.CreateUpvalue(&stack, 0);
        auto after_second = manager.GetStatistics();
        REQUIRE(after_second.cache_hits == initial_stats.cache_hits + 1);
    }
}

TEST_CASE("UpvalueManager - Upvalue Closure", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Close single upvalue") {
        LuaValue value = LuaValue::Number(123.0);
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        REQUIRE(upvalue->IsOpen());
        
        manager.CloseUpvalues(&stack, 0);
        
        REQUIRE(upvalue->IsClosed());
        REQUIRE(upvalue->GetValue() == value);  // 值应该保持
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.open_upvalues == 0);
        REQUIRE(stats.closed_upvalues == 1);
    }
    
    SECTION("Close multiple upvalues") {
        std::vector<LuaValue> values = {
            LuaValue::Number(1),
            LuaValue::Number(2),
            LuaValue::Number(3),
            LuaValue::Number(4),
            LuaValue::Number(5)
        };
        
        for (const auto& val : values) {
            stack.Push(val);
        }
        
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (Size i = 0; i < values.size(); ++i) {
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        // 关闭索引2及以上的Upvalue
        manager.CloseUpvalues(&stack, 2);
        
        // 验证关闭状态
        REQUIRE(upvalues[0]->IsOpen());
        REQUIRE(upvalues[1]->IsOpen());
        REQUIRE(upvalues[2]->IsClosed());
        REQUIRE(upvalues[3]->IsClosed());
        REQUIRE(upvalues[4]->IsClosed());
        
        // 验证值保持正确
        for (Size i = 0; i < values.size(); ++i) {
            REQUIRE(upvalues[i]->GetValue() == values[i]);
        }
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.open_upvalues == 2);
        REQUIRE(stats.closed_upvalues == 3);
    }
    
    SECTION("Close all upvalues") {
        for (int i = 0; i < 3; ++i) {
            stack.Push(LuaValue::Number(i));
            manager.CreateUpvalue(&stack, i);
        }
        
        manager.CloseUpvalues(&stack, 0);  // 关闭所有
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.open_upvalues == 0);
        REQUIRE(stats.closed_upvalues == 3);
    }
    
    SECTION("Close with no open upvalues") {
        // 没有开放的Upvalue时关闭不应该出错
        REQUIRE_NOTHROW(manager.CloseUpvalues(&stack, 0));
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.open_upvalues == 0);
        REQUIRE(stats.closed_upvalues == 0);
    }
}

TEST_CASE("UpvalueManager - Reference Counting", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Reference count management") {
        LuaValue value = LuaValue::String("ref_test");
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        REQUIRE(upvalue->GetReferenceCount() == 1);
        
        // 增加引用
        upvalue->AddReference();
        REQUIRE(upvalue->GetReferenceCount() == 2);
        
        upvalue->AddReference();
        REQUIRE(upvalue->GetReferenceCount() == 3);
        
        // 减少引用
        upvalue->RemoveReference();
        REQUIRE(upvalue->GetReferenceCount() == 2);
        
        upvalue->RemoveReference();
        REQUIRE(upvalue->GetReferenceCount() == 1);
    }
    
    SECTION("Shared upvalue reference counting") {
        LuaValue value = LuaValue::Number(456);
        stack.Push(value);
        
        auto upvalue1 = manager.CreateUpvalue(&stack, 0);
        auto upvalue2 = manager.CreateUpvalue(&stack, 0);  // 共享
        
        REQUIRE(upvalue1.get() == upvalue2.get());
        REQUIRE(upvalue1->GetReferenceCount() == 2);
        
        // 释放一个引用
        upvalue2.reset();
        REQUIRE(upvalue1->GetReferenceCount() == 1);
    }
    
    SECTION("Reference count after closure") {
        LuaValue value = LuaValue::Boolean(true);
        stack.Push(value);
        
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        upvalue->AddReference();  // 增加额外引用
        
        REQUIRE(upvalue->GetReferenceCount() == 2);
        
        // 关闭Upvalue不应该影响引用计数
        manager.CloseUpvalues(&stack, 0);
        
        REQUIRE(upvalue->IsClosed());
        REQUIRE(upvalue->GetReferenceCount() == 2);
    }
}

TEST_CASE("UpvalueManager - Garbage Collection Integration", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Mark reachable upvalues") {
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        
        for (int i = 0; i < 5; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        // 执行标记
        manager.MarkReachableUpvalues();
        
        // 所有Upvalue应该被标记
        for (const auto& upvalue : upvalues) {
            REQUIRE(upvalue->IsMarked());
        }
    }
    
    SECTION("Sweep unmarked upvalues") {
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        
        for (int i = 0; i < 10; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        auto initial_count = manager.GetStatistics().total_upvalues;
        
        // 取消部分Upvalue的标记
        for (Size i = 0; i < upvalues.size(); i += 2) {
            upvalues[i]->Unmark();
        }
        
        // 执行清理
        Size swept = manager.SweepUnmarkedUpvalues();
        
        REQUIRE(swept > 0);
        
        auto final_stats = manager.GetStatistics();
        REQUIRE(final_stats.total_upvalues < initial_count);
        REQUIRE(final_stats.total_swept == swept);
    }
    
    SECTION("GC statistics") {
        // 创建一些Upvalue
        for (int i = 0; i < 5; ++i) {
            stack.Push(LuaValue::String("gc_test_" + std::to_string(i)));
            manager.CreateUpvalue(&stack, i);
        }
        
        auto pre_gc_stats = manager.GetStatistics();
        
        // 执行完整GC周期
        manager.MarkReachableUpvalues();
        manager.SweepUnmarkedUpvalues();
        
        auto post_gc_stats = manager.GetStatistics();
        
        // GC统计应该更新
        REQUIRE(post_gc_stats.gc_cycles > pre_gc_stats.gc_cycles);
    }
}

TEST_CASE("UpvalueManager - Performance Metrics", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Operation timing") {
        auto stats = manager.GetStatistics();
        
        // 操作时间应该被跟踪
        REQUIRE(stats.avg_create_time >= 0.0);
        REQUIRE(stats.avg_close_time >= 0.0);
        
        // 执行一些操作
        stack.Push(LuaValue::Number(1.0));
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        manager.CloseUpvalues(&stack, 0);
        
        auto updated_stats = manager.GetStatistics();
        
        // 统计应该更新
        REQUIRE(updated_stats.total_create_operations > stats.total_create_operations);
        REQUIRE(updated_stats.total_close_operations > stats.total_close_operations);
    }
    
    SECTION("Memory usage tracking") {
        auto initial_memory = manager.GetMemoryUsage();
        
        // 创建多个Upvalue
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        for (int i = 0; i < 50; ++i) {
            stack.Push(LuaValue::Number(i));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        auto peak_memory = manager.GetMemoryUsage();
        REQUIRE(peak_memory > initial_memory);
        
        // 清理
        upvalues.clear();
        manager.SweepUnmarkedUpvalues();
        
        auto final_memory = manager.GetMemoryUsage();
        REQUIRE(final_memory <= peak_memory);
    }
    
    SECTION("Cache performance") {
        LuaValue value = LuaValue::String("cache_test");
        stack.Push(value);
        
        auto initial_stats = manager.GetStatistics();
        
        // 多次创建相同位置的Upvalue
        for (int i = 0; i < 10; ++i) {
            auto upvalue = manager.CreateUpvalue(&stack, 0);
        }
        
        auto final_stats = manager.GetStatistics();
        
        // 应该有缓存命中
        REQUIRE(final_stats.cache_hits > initial_stats.cache_hits);
        REQUIRE(final_stats.cache_hit_ratio > 0.0);
    }
}

TEST_CASE("UpvalueManager - Error Handling", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    
    SECTION("Invalid parameters") {
        // Null stack
        REQUIRE_THROWS_AS(manager.CreateUpvalue(nullptr, 0), UpvalueError);
        REQUIRE_THROWS_AS(manager.CloseUpvalues(nullptr, 0), UpvalueError);
    }
    
    SECTION("Exception safety") {
        LuaStack stack(256);
        stack.Push(LuaValue::Number(1));
        
        // 创建Upvalue
        auto upvalue = manager.CreateUpvalue(&stack, 0);
        REQUIRE(manager.ValidateIntegrity());
        
        // 尝试无效操作
        try {
            manager.CreateUpvalue(&stack, 10);  // 无效索引
        } catch (...) {
            // 异常后完整性应该保持
            REQUIRE(manager.ValidateIntegrity());
        }
    }
    
    SECTION("State consistency after errors") {
        LuaStack stack(256);
        
        auto initial_stats = manager.GetStatistics();
        
        // 尝试多个无效操作
        for (int i = 0; i < 5; ++i) {
            try {
                manager.CreateUpvalue(&stack, i);  // 栈为空，应该失败
            } catch (...) {
                // 忽略预期的异常
            }
        }
        
        // 统计不应该被无效操作影响
        auto final_stats = manager.GetStatistics();
        REQUIRE(final_stats.total_upvalues == initial_stats.total_upvalues);
        REQUIRE(manager.ValidateIntegrity());
    }
}

TEST_CASE("UpvalueManager - Complex Scenarios", "[unit][upvalue_manager]") {
    UpvalueManager manager;
    LuaStack stack(256);
    
    SECTION("Mixed operations scenario") {
        std::vector<std::shared_ptr<Upvalue>> upvalues;
        
        // 创建多层Upvalue
        for (int i = 0; i < 10; ++i) {
            stack.Push(LuaValue::Number(i * 10));
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        // 创建一些共享Upvalue
        for (int i = 0; i < 5; ++i) {
            upvalues.push_back(manager.CreateUpvalue(&stack, i));
        }
        
        // 部分关闭
        manager.CloseUpvalues(&stack, 5);
        
        // 验证状态
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_upvalues == 10);  // 物理Upvalue数量
        REQUIRE(stats.open_upvalues == 5);
        REQUIRE(stats.closed_upvalues == 5);
        REQUIRE(stats.shared_upvalues >= 5);
        
        REQUIRE(manager.ValidateIntegrity());
    }
    
    SECTION("Stress test") {
        const int NUM_OPERATIONS = 1000;
        
        // 大量创建和关闭操作
        for (int i = 0; i < NUM_OPERATIONS; ++i) {
            if (stack.GetSize() < 100) {
                stack.Push(LuaValue::Number(i));
            }
            
            if (stack.GetSize() > 0) {
                Size index = i % stack.GetSize();
                auto upvalue = manager.CreateUpvalue(&stack, index);
                
                if (i % 10 == 0 && stack.GetSize() > 5) {
                    manager.CloseUpvalues(&stack, stack.GetSize() - 5);
                }
            }
        }
        
        // 系统应该保持稳定
        REQUIRE(manager.ValidateIntegrity());
        
        auto stats = manager.GetStatistics();
        REQUIRE(stats.total_create_operations >= NUM_OPERATIONS);
        REQUIRE(stats.cache_hit_ratio >= 0.0);
    }
}

} // namespace lua_cpp