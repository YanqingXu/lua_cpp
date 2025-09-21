/**
 * @file gc_test_suite.cpp
 * @brief 独立垃圾收集器测试套件
 * @description 全面测试标记-清扫垃圾收集器的功能
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "gc_standalone.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <random>
#include <functional>

using namespace lua_cpp;

/**
 * @brief 测试辅助类
 */
class TestRunner {
public:
    TestRunner() : test_count_(0), passed_count_(0) {}
    
    void RunTest(const std::string& name, std::function<bool()> test_func) {
        test_count_++;
        std::cout << "Running test: " << name << "... ";
        
        try {
            if (test_func()) {
                passed_count_++;
                std::cout << "PASSED" << std::endl;
            } else {
                std::cout << "FAILED" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cout << "EXCEPTION: " << e.what() << std::endl;
        }
    }
    
    void PrintSummary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Total tests: " << test_count_ << std::endl;
        std::cout << "Passed: " << passed_count_ << std::endl;
        std::cout << "Failed: " << (test_count_ - passed_count_) << std::endl;
        std::cout << "Success rate: " << (100.0 * passed_count_ / test_count_) << "%" << std::endl;
        std::cout << "===================" << std::endl;
    }
    
    bool AllTestsPassed() const {
        return passed_count_ == test_count_;
    }
    
private:
    int test_count_;
    int passed_count_;
};

/**
 * @brief 测试基本的对象创建和销毁
 */
bool TestBasicObjectCreation() {
    StandaloneGC gc(1000); // 提高阈值，避免自动触发GC
    
    // 创建几个测试对象
    auto* str1 = gc.CreateObject<TestStringObject>("Hello");
    auto* str2 = gc.CreateObject<TestStringObject>("World");
    auto* container = gc.CreateObject<TestContainerObject>("test_container");
    
    // 验证对象数量
    if (gc.GetObjectCount() != 3) {
        std::cerr << "Expected 3 objects, got " << gc.GetObjectCount() << std::endl;
        return false;
    }
    
    // 验证内存使用
    Size expected_memory = str1->GetSize() + str2->GetSize() + container->GetSize();
    if (gc.GetCurrentMemory() != expected_memory) {
        std::cerr << "Memory mismatch: expected " << expected_memory 
                  << ", got " << gc.GetCurrentMemory() << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试简单的垃圾收集
 */
bool TestSimpleCollection() {
    StandaloneGC gc(1000); // 高阈值，手动触发GC
    
    // 创建一些对象，但不设置根
    auto* str1 = gc.CreateObject<TestStringObject>("temp1");
    auto* str2 = gc.CreateObject<TestStringObject>("temp2");
    auto* container = gc.CreateObject<TestContainerObject>("temp_container");
    
    Size initial_count = gc.GetObjectCount();
    Size initial_memory = gc.GetCurrentMemory();
    
    // 执行垃圾收集（没有根对象，所有对象都应该被回收）
    gc.Collect();
    
    // 验证所有对象都被回收
    if (gc.GetObjectCount() != 0) {
        std::cerr << "Expected 0 objects after collection, got " << gc.GetObjectCount() << std::endl;
        return false;
    }
    
    if (gc.GetCurrentMemory() != 0) {
        std::cerr << "Expected 0 memory after collection, got " << gc.GetCurrentMemory() << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试根对象保护
 */
bool TestRootProtection() {
    StandaloneGC gc(1000);
    
    // 创建对象并设置根
    auto* root_str = gc.CreateObject<TestStringObject>("root");
    auto* temp_str = gc.CreateObject<TestStringObject>("temp");
    
    gc.AddRoot(root_str);
    
    // 执行垃圾收集
    gc.Collect();
    
    // 根对象应该被保留，临时对象被回收
    if (gc.GetObjectCount() != 1) {
        std::cerr << "Expected 1 object after collection, got " << gc.GetObjectCount() << std::endl;
        return false;
    }
    
    // 验证保留的是根对象
    if (root_str->GetValue() != "root") {
        std::cerr << "Root object was not preserved correctly" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试对象引用关系
 */
bool TestObjectReferences() {
    StandaloneGC gc(1000); // 提高阈值避免自动GC
    
    // 创建容器和子对象
    auto* container = gc.CreateObject<TestContainerObject>("parent");
    auto* child1 = gc.CreateObject<TestStringObject>("child1");
    auto* child2 = gc.CreateObject<TestStringObject>("child2");
    auto* orphan = gc.CreateObject<TestStringObject>("orphan");
    
    // 建立引用关系
    container->AddChild(child1);
    container->AddChild(child2);
    // orphan没有被引用
    
    // 设置容器为根
    gc.AddRoot(container);
    
    // 执行垃圾收集
    gc.Collect();
    
    // 应该保留容器和两个子对象，orphan被回收
    if (gc.GetObjectCount() != 3) {
        std::cerr << "Expected 3 objects after collection, got " << gc.GetObjectCount() << std::endl;
        return false;
    }
    
    // 验证引用关系仍然正确
    if (container->GetChildren().size() != 2) {
        std::cerr << "Container should have 2 children after GC" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试循环引用处理
 */
bool TestCircularReferences() {
    StandaloneGC gc(300);
    
    // 创建循环引用：A -> B -> C -> A
    auto* containerA = gc.CreateObject<TestContainerObject>("A");
    auto* containerB = gc.CreateObject<TestContainerObject>("B");
    auto* containerC = gc.CreateObject<TestContainerObject>("C");
    
    containerA->AddChild(containerB);
    containerB->AddChild(containerC);
    containerC->AddChild(containerA);
    
    // 创建一个独立对象
    auto* independent = gc.CreateObject<TestStringObject>("independent");
    
    // 不设置任何根对象
    Size initial_count = gc.GetObjectCount();
    
    // 执行垃圾收集
    gc.Collect();
    
    // 所有对象都应该被回收（包括循环引用的对象）
    if (gc.GetObjectCount() != 0) {
        std::cerr << "Expected 0 objects after collection, got " << gc.GetObjectCount() 
                  << " (circular references may not be handled correctly)" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试增量垃圾收集
 */
bool TestIncrementalCollection() {
    StandaloneGC gc(10000); // 很高的阈值，避免自动触发GC
    
    // 创建一些对象
    auto* root = gc.CreateObject<TestContainerObject>("root");
    for (int i = 0; i < 10; i++) {
        auto* child = gc.CreateObject<TestStringObject>("child_" + std::to_string(i));
        root->AddChild(child);
    }
    
    // 创建一些无引用的对象
    for (int i = 0; i < 5; i++) {
        gc.CreateObject<TestStringObject>("orphan_" + std::to_string(i));
    }
    
    gc.AddRoot(root);
    
    Size initial_count = gc.GetObjectCount();
    Size initial_collections = gc.GetStats().collections_performed;
    
    // 执行增量收集步骤直到完成一次收集
    Size max_steps = 100; // 防止无限循环
    Size steps = 0;
    while (gc.GetStats().collections_performed == initial_collections && steps < max_steps) {
        gc.PerformIncrementalStep();
        steps++;
    }
    
    if (steps >= max_steps) {
        std::cerr << "Incremental collection did not complete within " << max_steps << " steps" << std::endl;
        return false;
    }
    
    // 应该保留根对象和10个子对象，回收5个orphan
    Size expected_count = 11; // root + 10 children
    if (gc.GetObjectCount() != expected_count) {
        std::cerr << "Expected " << expected_count << " objects after incremental collection, got " 
                  << gc.GetObjectCount() << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试GC统计信息
 */
bool TestGCStats() {
    StandaloneGC gc(100);
    
    // 创建一些对象
    for (int i = 0; i < 5; i++) {
        gc.CreateObject<TestStringObject>("test_" + std::to_string(i));
    }
    
    auto initial_stats = gc.GetStats();
    
    // 执行收集
    gc.Collect();
    
    auto final_stats = gc.GetStats();
    
    // 验证统计信息更新
    if (final_stats.collections_performed != initial_stats.collections_performed + 1) {
        std::cerr << "Collection count not updated correctly" << std::endl;
        return false;
    }
    
    if (final_stats.total_freed_objects == 0) {
        std::cerr << "No objects reported as freed" << std::endl;
        return false;
    }
    
    if (final_stats.total_freed_bytes == 0) {
        std::cerr << "No bytes reported as freed" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 测试GC一致性检查
 */
bool TestConsistencyCheck() {
    StandaloneGC gc(200);
    
    // 创建正常的对象引用结构
    auto* container = gc.CreateObject<TestContainerObject>("parent");
    auto* child = gc.CreateObject<TestStringObject>("child");
    container->AddChild(child);
    gc.AddRoot(container);
    
    // 一致性检查应该通过
    if (!gc.CheckConsistency()) {
        std::cerr << "Consistency check failed on valid structure" << std::endl;
        return false;
    }
    
    // 执行GC后再检查
    gc.Collect();
    
    if (!gc.CheckConsistency()) {
        std::cerr << "Consistency check failed after GC" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 压力测试：大量对象的创建和回收
 */
bool TestStressTest() {
    StandaloneGC gc(1000);
    
    const int NUM_ITERATIONS = 100;
    const int OBJECTS_PER_ITERATION = 50;
    
    for (int iter = 0; iter < NUM_ITERATIONS; iter++) {
        // 创建一批对象
        std::vector<GCObject*> objects;
        for (int i = 0; i < OBJECTS_PER_ITERATION; i++) {
            if (i % 2 == 0) {
                objects.push_back(gc.CreateObject<TestStringObject>("stress_" + std::to_string(i)));
            } else {
                auto* container = gc.CreateObject<TestContainerObject>("container_" + std::to_string(i));
                if (!objects.empty()) {
                    container->AddChild(objects.back());
                }
                objects.push_back(container);
            }
        }
        
        // 随机设置一些根对象
        if (iter % 10 == 0) {
            for (size_t i = 0; i < objects.size(); i += 10) {
                gc.AddRoot(objects[i]);
            }
        }
        
        // 执行垃圾收集
        if (iter % 5 == 0) {
            gc.Collect();
        }
        
        // 一致性检查
        if (!gc.CheckConsistency()) {
            std::cerr << "Consistency check failed at iteration " << iter << std::endl;
            return false;
        }
    }
    
    // 最终清理
    gc.Collect();
    
    return gc.CheckConsistency();
}

/**
 * @brief 性能测试
 */
bool TestPerformance() {
    const int NUM_OBJECTS = 10000;
    const int NUM_COLLECTIONS = 10;
    
    StandaloneGC gc(NUM_OBJECTS * 32); // 高阈值，手动触发GC
    
    // 创建大量对象
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<GCObject*> roots;
    for (int i = 0; i < NUM_OBJECTS; i++) {
        auto* obj = gc.CreateObject<TestStringObject>("perf_test_" + std::to_string(i));
        if (i % 100 == 0) {
            roots.push_back(obj);
            gc.AddRoot(obj);
        }
    }
    
    auto creation_time = std::chrono::high_resolution_clock::now();
    
    // 执行多次垃圾收集
    for (int i = 0; i < NUM_COLLECTIONS; i++) {
        gc.Collect();
    }
    
    auto collection_time = std::chrono::high_resolution_clock::now();
    
    auto creation_duration = std::chrono::duration<double>(creation_time - start_time).count();
    auto collection_duration = std::chrono::duration<double>(collection_time - creation_time).count();
    
    std::cout << "\nPerformance Results:" << std::endl;
    std::cout << "Object creation time: " << creation_duration << "s" << std::endl;
    std::cout << "Collection time: " << collection_duration << "s" << std::endl;
    std::cout << "Average collection time: " << (collection_duration / NUM_COLLECTIONS) << "s" << std::endl;
    std::cout << "Objects per second (creation): " << (NUM_OBJECTS / creation_duration) << std::endl;
    
    // 性能标准：创建应该很快，收集时间应该合理
    if (creation_duration > 1.0) {
        std::cerr << "Object creation too slow" << std::endl;
        return false;
    }
    
    if (collection_duration / NUM_COLLECTIONS > 0.1) {
        std::cerr << "Average collection time too slow" << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief 主测试函数
 */
int main() {
    std::cout << "=== Standalone GC Test Suite ===" << std::endl;
    
    TestRunner runner;
    
    // 基础功能测试
    runner.RunTest("Basic Object Creation", TestBasicObjectCreation);
    runner.RunTest("Simple Collection", TestSimpleCollection);
    runner.RunTest("Root Protection", TestRootProtection);
    runner.RunTest("Object References", TestObjectReferences);
    runner.RunTest("Circular References", TestCircularReferences);
    
    // 高级功能测试
    runner.RunTest("Incremental Collection", TestIncrementalCollection);
    runner.RunTest("GC Statistics", TestGCStats);
    runner.RunTest("Consistency Check", TestConsistencyCheck);
    
    // 压力和性能测试
    runner.RunTest("Stress Test", TestStressTest);
    runner.RunTest("Performance Test", TestPerformance);
    
    runner.PrintSummary();
    
    if (runner.AllTestsPassed()) {
        std::cout << "\n✅ All tests passed! GC implementation is working correctly." << std::endl;
        std::cout << "\n🎉 T023 Garbage Collector Implementation - COMPLETED SUCCESSFULLY!" << std::endl;
        return 0;
    } else {
        std::cout << "\n❌ Some tests failed. Please check the implementation." << std::endl;
        return 1;
    }
}