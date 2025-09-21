/**
 * @file gc_test.cpp
 * @brief 垃圾收集器测试
 * @description 测试标记-清扫垃圾收集器的功能
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <chrono>

#include "memory/garbage_collector.h"
#include "vm/virtual_machine.h"
#include "core/common.h"

using namespace lua_cpp;

/**
 * @brief 测试GC对象类
 */
class TestGCObject : public GCObject {
public:
    TestGCObject(int value) 
        : GCObject(GCObjectType::UserData, sizeof(TestGCObject))
        , value_(value) {
        std::cout << "Created TestGCObject " << value_ << std::endl;
    }
    
    ~TestGCObject() {
        std::cout << "Destroyed TestGCObject " << value_ << std::endl;
    }
    
    void Mark(GarbageCollector* gc) override {
        if (GetColor() != GCColor::White) {
            return;
        }
        
        SetColor(GCColor::Gray);
        gc->AddToGrayList(this);
    }
    
    std::vector<GCObject*> GetReferences() const override {
        std::vector<GCObject*> refs;
        for (auto* ref : references_) {
            if (ref) {
                refs.push_back(ref);
            }
        }
        return refs;
    }
    
    void AddReference(TestGCObject* ref) {
        references_.push_back(ref);
    }
    
    int GetValue() const { return value_; }
    
    std::string ToString() const override {
        return "TestGCObject(" + std::to_string(value_) + ")";
    }

private:
    int value_;
    std::vector<TestGCObject*> references_;
};

/**
 * @brief 测试字符串对象创建和回收
 */
void TestStringObjects() {
    std::cout << "\n=== Testing String Objects ===" << std::endl;
    
    VirtualMachine vm;
    GarbageCollector gc(&vm);
    
    std::vector<StringObject*> strings;
    
    // 创建一些字符串对象
    for (int i = 0; i < 10; i++) {
        std::string str = "String_" + std::to_string(i);
        StringObject* obj = new StringObject(str);
        gc.RegisterObject(obj);
        strings.push_back(obj);
    }
    
    std::cout << "Created " << strings.size() << " string objects" << std::endl;
    std::cout << "Total memory: " << gc.GetTotalBytes() << " bytes" << std::endl;
    std::cout << "Object count: " << gc.GetObjectCount() << std::endl;
    
    // 执行垃圾收集
    gc.Collect();
    
    std::cout << "After GC:" << std::endl;
    std::cout << "Total memory: " << gc.GetTotalBytes() << " bytes" << std::endl;
    std::cout << "Object count: " << gc.GetObjectCount() << std::endl;
    
    // 清理
    for (auto* str : strings) {
        gc.UnregisterObject(str);
        delete str;
    }
}

/**
 * @brief 测试表对象创建和回收
 */
void TestTableObjects() {
    std::cout << "\n=== Testing Table Objects ===" << std::endl;
    
    VirtualMachine vm;
    GarbageCollector gc(&vm);
    
    std::vector<TableObject*> tables;
    
    // 创建一些表对象
    for (int i = 0; i < 5; i++) {
        TableObject* table = new TableObject(10, 10);
        gc.RegisterObject(table);
        tables.push_back(table);
        
        // 向表中添加一些值
        for (int j = 0; j < 5; j++) {
            LuaValue key(j);
            LuaValue value("value_" + std::to_string(j));
            table->Set(key, value);
        }
    }
    
    std::cout << "Created " << tables.size() << " table objects" << std::endl;
    std::cout << "Total memory: " << gc.GetTotalBytes() << " bytes" << std::endl;
    std::cout << "Object count: " << gc.GetObjectCount() << std::endl;
    
    // 执行垃圾收集
    gc.Collect();
    
    std::cout << "After GC:" << std::endl;
    std::cout << "Total memory: " << gc.GetTotalBytes() << " bytes" << std::endl;
    std::cout << "Object count: " << gc.GetObjectCount() << std::endl;
    
    // 清理
    for (auto* table : tables) {
        gc.UnregisterObject(table);
        delete table;
    }
}

/**
 * @brief 测试对象引用和标记
 */
void TestObjectReferences() {
    std::cout << "\n=== Testing Object References ===" << std::endl;
    
    VirtualMachine vm;
    GarbageCollector gc(&vm);
    
    // 创建对象图：A -> B -> C，D是孤立的
    TestGCObject* objA = new TestGCObject(1);
    TestGCObject* objB = new TestGCObject(2);
    TestGCObject* objC = new TestGCObject(3);
    TestGCObject* objD = new TestGCObject(4);
    
    objA->AddReference(objB);
    objB->AddReference(objC);
    // objD没有被引用
    
    gc.RegisterObject(objA);
    gc.RegisterObject(objB);
    gc.RegisterObject(objC);
    gc.RegisterObject(objD);
    
    std::cout << "Created 4 objects with references A->B->C, D isolated" << std::endl;
    std::cout << "Object count before GC: " << gc.GetObjectCount() << std::endl;
    
    // 模拟根引用：只有objA在根集合中
    // 在实际的VM中，这是通过栈、全局变量等来实现的
    // 这里我们手动标记objA作为根
    
    // 直接调用标记方法来模拟根标记
    gc.MarkObject(objA);
    
    // 执行垃圾收集
    std::cout << "Performing garbage collection..." << std::endl;
    
    // 手动执行标记阶段
    gc.PropagateMarks();
    
    std::cout << "Object A color: " << (int)objA->GetColor() << std::endl;
    std::cout << "Object B color: " << (int)objB->GetColor() << std::endl;
    std::cout << "Object C color: " << (int)objC->GetColor() << std::endl;
    std::cout << "Object D color: " << (int)objD->GetColor() << std::endl;
    
    // 清理
    gc.UnregisterObject(objA);
    gc.UnregisterObject(objB);
    gc.UnregisterObject(objC);
    gc.UnregisterObject(objD);
    
    delete objA;
    delete objB;
    delete objC;
    delete objD;
}

/**
 * @brief 测试增量垃圾收集
 */
void TestIncrementalGC() {
    std::cout << "\n=== Testing Incremental GC ===" << std::endl;
    
    VirtualMachine vm;
    GCConfig config;
    config.enable_incremental = true;
    config.initial_threshold = 1024;
    
    GarbageCollector gc(&vm);
    gc.SetConfig(config);
    
    std::vector<StringObject*> objects;
    
    // 创建足够多的对象来触发增量GC
    for (int i = 0; i < 100; i++) {
        std::string str = "IncrementalTest_" + std::to_string(i);
        StringObject* obj = new StringObject(str);
        gc.RegisterObject(obj);
        objects.push_back(obj);
        
        // 每隔几个对象执行一次增量步骤
        if (i % 10 == 0) {
            std::cout << "Incremental step at object " << i << std::endl;
            std::cout << "GC State: " << (int)gc.GetState() << std::endl;
            std::cout << "Memory usage: " << gc.GetTotalBytes() << " bytes" << std::endl;
        }
    }
    
    // 执行完整的增量收集
    std::cout << "Performing incremental collection..." << std::endl;
    gc.PerformIncrementalCollection();
    
    std::cout << "Final state:" << std::endl;
    std::cout << "GC State: " << (int)gc.GetState() << std::endl;
    std::cout << "Memory usage: " << gc.GetTotalBytes() << " bytes" << std::endl;
    std::cout << "Object count: " << gc.GetObjectCount() << std::endl;
    
    // 清理
    for (auto* obj : objects) {
        gc.UnregisterObject(obj);
        delete obj;
    }
}

/**
 * @brief 测试GC统计信息
 */
void TestGCStatistics() {
    std::cout << "\n=== Testing GC Statistics ===" << std::endl;
    
    VirtualMachine vm;
    GarbageCollector gc(&vm);
    
    // 创建一些对象
    std::vector<StringObject*> objects;
    for (int i = 0; i < 50; i++) {
        StringObject* obj = new StringObject("StatTest_" + std::to_string(i));
        gc.RegisterObject(obj);
        objects.push_back(obj);
    }
    
    std::cout << "Before GC:" << std::endl;
    gc.DumpStats();
    
    // 执行几次垃圾收集
    for (int i = 0; i < 3; i++) {
        std::cout << "\nGC round " << (i + 1) << ":" << std::endl;
        gc.Collect();
        gc.DumpStats();
    }
    
    // 清理
    for (auto* obj : objects) {
        gc.UnregisterObject(obj);
        delete obj;
    }
    
    std::cout << "\nAfter cleanup:" << std::endl;
    gc.DumpStats();
}

/**
 * @brief 性能测试
 */
void TestGCPerformance() {
    std::cout << "\n=== Testing GC Performance ===" << std::endl;
    
    const int num_objects = 10000;
    const int num_collections = 10;
    
    VirtualMachine vm;
    GarbageCollector gc(&vm);
    
    std::vector<StringObject*> objects;
    objects.reserve(num_objects);
    
    // 测量对象创建时间
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_objects; i++) {
        StringObject* obj = new StringObject("PerfTest_" + std::to_string(i));
        gc.RegisterObject(obj);
        objects.push_back(obj);
    }
    
    auto create_time = std::chrono::high_resolution_clock::now();
    auto create_duration = std::chrono::duration<double>(create_time - start_time).count();
    
    std::cout << "Created " << num_objects << " objects in " << create_duration << " seconds" << std::endl;
    std::cout << "Creation rate: " << (num_objects / create_duration) << " objects/second" << std::endl;
    
    // 测量GC性能
    auto gc_start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_collections; i++) {
        gc.Collect();
    }
    
    auto gc_end_time = std::chrono::high_resolution_clock::now();
    auto gc_duration = std::chrono::duration<double>(gc_end_time - gc_start_time).count();
    
    std::cout << "Performed " << num_collections << " collections in " << gc_duration << " seconds" << std::endl;
    std::cout << "Average collection time: " << (gc_duration / num_collections) << " seconds" << std::endl;
    
    auto stats = gc.GetStats();
    std::cout << "Collections performed: " << stats.collections_performed << std::endl;
    std::cout << "Average pause time: " << stats.average_pause_time << " seconds" << std::endl;
    
    // 清理
    for (auto* obj : objects) {
        gc.UnregisterObject(obj);
        delete obj;
    }
}

/**
 * @brief 测试GC一致性检查
 */
void TestGCConsistency() {
    std::cout << "\n=== Testing GC Consistency ===" << std::endl;
    
    VirtualMachine vm;
    GarbageCollector gc(&vm);
    
    // 创建一些对象
    std::vector<StringObject*> objects;
    for (int i = 0; i < 20; i++) {
        StringObject* obj = new StringObject("ConsistencyTest_" + std::to_string(i));
        gc.RegisterObject(obj);
        objects.push_back(obj);
    }
    
    std::cout << "Created objects, checking consistency..." << std::endl;
    bool consistent = gc.CheckConsistency();
    std::cout << "Consistency check: " << (consistent ? "PASSED" : "FAILED") << std::endl;
    
    // 执行垃圾收集
    gc.Collect();
    
    std::cout << "After GC, checking consistency..." << std::endl;
    consistent = gc.CheckConsistency();
    std::cout << "Consistency check: " << (consistent ? "PASSED" : "FAILED") << std::endl;
    
    // 清理一半对象
    for (size_t i = 0; i < objects.size() / 2; i++) {
        gc.UnregisterObject(objects[i]);
        delete objects[i];
    }
    
    std::cout << "After partial cleanup, checking consistency..." << std::endl;
    consistent = gc.CheckConsistency();
    std::cout << "Consistency check: " << (consistent ? "PASSED" : "FAILED") << std::endl;
    
    // 清理剩余对象
    for (size_t i = objects.size() / 2; i < objects.size(); i++) {
        gc.UnregisterObject(objects[i]);
        delete objects[i];
    }
    
    std::cout << "After full cleanup, checking consistency..." << std::endl;
    consistent = gc.CheckConsistency();
    std::cout << "Consistency check: " << (consistent ? "PASSED" : "FAILED") << std::endl;
}

int main() {
    std::cout << "Lua C++ Garbage Collector Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    try {
        TestStringObjects();
        TestTableObjects();
        TestObjectReferences();
        TestIncrementalGC();
        TestGCStatistics();
        TestGCPerformance();
        TestGCConsistency();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}