/**
 * @file test_table_contract.cpp
 * @brief LuaTable（Lua表）契约测试
 * @description 测试Lua表的完整行为：索引访问、数组/哈希部分、元方法、弱引用、遍历等
 *              确保100% Lua 5.1.5兼容性和性能要求
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "types/lua_table.h"
#include "types/tvalue.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"
#include "gc/gc_object.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* LuaTable基础构造和状态契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 基础构造契约", "[table][contract][core]") {
    SECTION("默认构造空表") {
        auto table = LuaTable::Create();
        REQUIRE(table != nullptr);
        REQUIRE(table->GetLength() == 0);
        REQUIRE(table->GetArraySize() == 0);
        REQUIRE(table->GetHashSize() == 0);
        REQUIRE(table->IsEmpty());
        REQUIRE_FALSE(table->HasMetatable());
    }

    SECTION("预分配构造") {
        auto table = LuaTable::Create(10, 5); // 10个数组元素，5个哈希元素
        REQUIRE(table != nullptr);
        REQUIRE(table->GetLength() == 0); // 长度基于实际元素，不是容量
        REQUIRE(table->GetArrayCapacity() >= 10);
        REQUIRE(table->GetHashCapacity() >= 5);
        REQUIRE(table->IsEmpty());
    }

    SECTION("拷贝构造") {
        auto original = LuaTable::Create();
        original->SetArrayValue(1, TValue::CreateNumber(42.0));
        original->SetHashValue(TValue::CreateString("key"), TValue::CreateString("value"));
        
        auto copy = LuaTable::Create(*original);
        REQUIRE(copy != nullptr);
        REQUIRE(copy != original); // 不同的对象
        REQUIRE(copy->GetLength() == original->GetLength());
        REQUIRE(copy->GetArrayValue(1) == TValue::CreateNumber(42.0));
        REQUIRE(copy->GetHashValue(TValue::CreateString("key")) == TValue::CreateString("value"));
    }
}

/* ========================================================================== */
/* 数组部分访问契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 数组部分契约", "[table][contract][array]") {
    SECTION("数组索引访问（1-based）") {
        auto table = LuaTable::Create();
        
        // Lua数组索引从1开始
        table->SetArrayValue(1, TValue::CreateNumber(10.0));
        table->SetArrayValue(2, TValue::CreateNumber(20.0));
        table->SetArrayValue(3, TValue::CreateNumber(30.0));
        
        REQUIRE(table->GetArrayValue(1) == TValue::CreateNumber(10.0));
        REQUIRE(table->GetArrayValue(2) == TValue::CreateNumber(20.0));
        REQUIRE(table->GetArrayValue(3) == TValue::CreateNumber(30.0));
        REQUIRE(table->GetLength() == 3);
    }

    SECTION("非连续索引处理") {
        auto table = LuaTable::Create();
        
        // 设置非连续索引
        table->SetArrayValue(1, TValue::CreateNumber(10.0));
        table->SetArrayValue(3, TValue::CreateNumber(30.0)); // 跳过索引2
        table->SetArrayValue(5, TValue::CreateNumber(50.0)); // 跳过索引4
        
        REQUIRE(table->GetArrayValue(1) == TValue::CreateNumber(10.0));
        REQUIRE(table->GetArrayValue(2).IsNil()); // 未设置的索引为nil
        REQUIRE(table->GetArrayValue(3) == TValue::CreateNumber(30.0));
        REQUIRE(table->GetArrayValue(4).IsNil());
        REQUIRE(table->GetArrayValue(5) == TValue::CreateNumber(50.0));
        
        // 长度计算基于连续部分
        REQUIRE(table->GetLength() == 1); // 只有索引1连续
    }

    SECTION("数组边界和零索引") {
        auto table = LuaTable::Create();
        
        // 索引0不是数组部分
        table->SetValue(TValue::CreateNumber(0.0), TValue::CreateString("zero"));
        REQUIRE(table->GetValue(TValue::CreateNumber(0.0)) == TValue::CreateString("zero"));
        REQUIRE(table->GetArrayValue(0).IsNil()); // 数组访问返回nil
        REQUIRE(table->GetLength() == 0); // 不影响数组长度
        
        // 负数索引也不是数组部分
        table->SetValue(TValue::CreateNumber(-1.0), TValue::CreateString("negative"));
        REQUIRE(table->GetLength() == 0);
    }

    SECTION("数组自动扩展") {
        auto table = LuaTable::Create();
        
        // 设置大索引值应该自动扩展数组
        table->SetArrayValue(100, TValue::CreateString("hundred"));
        REQUIRE(table->GetArrayValue(100) == TValue::CreateString("hundred"));
        REQUIRE(table->GetArrayCapacity() >= 100);
        
        // 中间未设置的索引应该是nil
        for (int i = 1; i < 100; ++i) {
            REQUIRE(table->GetArrayValue(i).IsNil());
        }
    }

    SECTION("nil值设置和删除") {
        auto table = LuaTable::Create();
        
        // 设置值
        table->SetArrayValue(1, TValue::CreateNumber(10.0));
        table->SetArrayValue(2, TValue::CreateNumber(20.0));
        table->SetArrayValue(3, TValue::CreateNumber(30.0));
        REQUIRE(table->GetLength() == 3);
        
        // 设置为nil相当于删除
        table->SetArrayValue(2, TValue::CreateNil());
        REQUIRE(table->GetArrayValue(2).IsNil());
        REQUIRE(table->GetLength() == 1); // 连续长度变为1
        
        // 重新设置值
        table->SetArrayValue(2, TValue::CreateNumber(25.0));
        REQUIRE(table->GetLength() == 3); // 恢复连续性
    }
}

/* ========================================================================== */
/* 哈希部分访问契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 哈希部分契约", "[table][contract][hash]") {
    SECTION("基本键值对操作") {
        auto table = LuaTable::Create();
        
        // 字符串键
        auto strKey = TValue::CreateString("name");
        auto strValue = TValue::CreateString("lua");
        table->SetHashValue(strKey, strValue);
        REQUIRE(table->GetHashValue(strKey) == strValue);
        
        // 数值键
        auto numKey = TValue::CreateNumber(3.14);
        auto numValue = TValue::CreateBoolean(true);
        table->SetHashValue(numKey, numValue);
        REQUIRE(table->GetHashValue(numKey) == numValue);
        
        // 布尔键
        auto boolKey = TValue::CreateBoolean(false);
        auto boolValue = TValue::CreateNumber(42.0);
        table->SetHashValue(boolKey, boolValue);
        REQUIRE(table->GetHashValue(boolKey) == boolValue);
    }

    SECTION("键的相等性和哈希") {
        auto table = LuaTable::Create();
        
        // 相等的键应该指向同一个值
        auto key1 = TValue::CreateNumber(42.0);
        auto key2 = TValue::CreateNumber(42.0);
        auto value = TValue::CreateString("test");
        
        table->SetHashValue(key1, value);
        REQUIRE(table->GetHashValue(key2) == value); // key1 == key2
        
        // 不同类型即使"值相同"也不相等
        auto numKey = TValue::CreateNumber(1.0);
        auto strKey = TValue::CreateString("1");
        table->SetHashValue(numKey, TValue::CreateString("number"));
        table->SetHashValue(strKey, TValue::CreateString("string"));
        
        REQUIRE(table->GetHashValue(numKey) == TValue::CreateString("number"));
        REQUIRE(table->GetHashValue(strKey) == TValue::CreateString("string"));
    }

    SECTION("无效键处理") {
        auto table = LuaTable::Create();
        
        // nil键应该抛出异常或被忽略
        auto nilKey = TValue::CreateNil();
        REQUIRE_THROWS_AS(table->SetHashValue(nilKey, TValue::CreateString("value")), TypeError);
        
        // NaN键应该抛出异常
        auto nanKey = TValue::CreateNumber(std::numeric_limits<double>::quiet_NaN());
        REQUIRE_THROWS_AS(table->SetHashValue(nanKey, TValue::CreateString("value")), TypeError);
    }

    SECTION("哈希冲突处理") {
        auto table = LuaTable::Create();
        
        // 添加多个键值对测试哈希冲突处理
        for (int i = 0; i < 100; ++i) {
            auto key = TValue::CreateString("key" + std::to_string(i));
            auto value = TValue::CreateNumber(static_cast<double>(i));
            table->SetHashValue(key, value);
        }
        
        // 验证所有键值对都能正确访问
        for (int i = 0; i < 100; ++i) {
            auto key = TValue::CreateString("key" + std::to_string(i));
            auto expected = TValue::CreateNumber(static_cast<double>(i));
            REQUIRE(table->GetHashValue(key) == expected);
        }
    }

    SECTION("哈希删除操作") {
        auto table = LuaTable::Create();
        
        auto key1 = TValue::CreateString("key1");
        auto key2 = TValue::CreateString("key2");
        table->SetHashValue(key1, TValue::CreateNumber(1.0));
        table->SetHashValue(key2, TValue::CreateNumber(2.0));
        
        REQUIRE(table->GetHashSize() == 2);
        
        // 设置为nil删除键
        table->SetHashValue(key1, TValue::CreateNil());
        REQUIRE(table->GetHashValue(key1).IsNil());
        REQUIRE(table->GetHashValue(key2) == TValue::CreateNumber(2.0));
        REQUIRE(table->GetHashSize() == 1);
    }
}

/* ========================================================================== */
/* 统一访问接口契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 统一访问接口契约", "[table][contract][unified]") {
    SECTION("数组和哈希统一访问") {
        auto table = LuaTable::Create();
        
        // 通过统一接口设置数组元素
        table->SetValue(TValue::CreateNumber(1.0), TValue::CreateString("first"));
        table->SetValue(TValue::CreateNumber(2.0), TValue::CreateString("second"));
        
        // 通过统一接口设置哈希元素
        table->SetValue(TValue::CreateString("name"), TValue::CreateString("lua"));
        
        // 通过统一接口访问
        REQUIRE(table->GetValue(TValue::CreateNumber(1.0)) == TValue::CreateString("first"));
        REQUIRE(table->GetValue(TValue::CreateNumber(2.0)) == TValue::CreateString("second"));
        REQUIRE(table->GetValue(TValue::CreateString("name")) == TValue::CreateString("lua"));
        
        // 不存在的键返回nil
        REQUIRE(table->GetValue(TValue::CreateString("unknown")).IsNil());
    }

    SECTION("索引运算符重载") {
        auto table = LuaTable::Create();
        
        // 使用[]操作符
        (*table)[TValue::CreateNumber(1.0)] = TValue::CreateString("array");
        (*table)[TValue::CreateString("hash")] = TValue::CreateString("value");
        
        REQUIRE((*table)[TValue::CreateNumber(1.0)] == TValue::CreateString("array"));
        REQUIRE((*table)[TValue::CreateString("hash")] == TValue::CreateString("value"));
        
        // const版本
        const auto& constTable = *table;
        REQUIRE(constTable[TValue::CreateNumber(1.0)] == TValue::CreateString("array"));
    }

    SECTION("边界情况处理") {
        auto table = LuaTable::Create();
        
        // 大整数作为键（超出数组范围）
        auto bigInt = TValue::CreateNumber(1e15);
        table->SetValue(bigInt, TValue::CreateString("big"));
        REQUIRE(table->GetValue(bigInt) == TValue::CreateString("big"));
        
        // 小数作为键
        auto decimal = TValue::CreateNumber(1.5);
        table->SetValue(decimal, TValue::CreateString("decimal"));
        REQUIRE(table->GetValue(decimal) == TValue::CreateString("decimal"));
        
        // 这些都应该进入哈希部分，不影响数组长度
        REQUIRE(table->GetLength() == 0);
    }
}

/* ========================================================================== */
/* 长度计算契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 长度计算契约", "[table][contract][length]") {
    SECTION("标准长度计算") {
        auto table = LuaTable::Create();
        
        // 连续数组的长度
        table->SetArrayValue(1, TValue::CreateString("a"));
        REQUIRE(table->GetLength() == 1);
        
        table->SetArrayValue(2, TValue::CreateString("b"));
        REQUIRE(table->GetLength() == 2);
        
        table->SetArrayValue(3, TValue::CreateString("c"));
        REQUIRE(table->GetLength() == 3);
    }

    SECTION("非连续数组长度") {
        auto table = LuaTable::Create();
        
        // 有间隙的数组
        table->SetArrayValue(1, TValue::CreateString("a"));
        table->SetArrayValue(3, TValue::CreateString("c")); // 跳过2
        
        // 长度是最大连续索引
        REQUIRE(table->GetLength() == 1);
        
        // 填补间隙
        table->SetArrayValue(2, TValue::CreateString("b"));
        REQUIRE(table->GetLength() == 3);
    }

    SECTION("边界长度计算") {
        auto table = LuaTable::Create();
        
        // 长度为0的表
        REQUIRE(table->GetLength() == 0);
        
        // 只有哈希部分的表
        table->SetHashValue(TValue::CreateString("key"), TValue::CreateString("value"));
        REQUIRE(table->GetLength() == 0);
        
        // 0索引不计入长度
        table->SetValue(TValue::CreateNumber(0.0), TValue::CreateString("zero"));
        REQUIRE(table->GetLength() == 0);
    }

    SECTION("长度操作符重载") {
        auto table = LuaTable::Create();
        
        table->SetArrayValue(1, TValue::CreateString("a"));
        table->SetArrayValue(2, TValue::CreateString("b"));
        
        // # 操作符
        REQUIRE(table->GetLength() == 2);
        
        // 删除末尾元素
        table->SetArrayValue(2, TValue::CreateNil());
        REQUIRE(table->GetLength() == 1);
        
        // 删除所有元素
        table->SetArrayValue(1, TValue::CreateNil());
        REQUIRE(table->GetLength() == 0);
    }
}

/* ========================================================================== */
/* 元表和元方法契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 元表契约", "[table][contract][metatable]") {
    SECTION("元表设置和获取") {
        auto table = LuaTable::Create();
        auto metatable = LuaTable::Create();
        
        REQUIRE_FALSE(table->HasMetatable());
        REQUIRE(table->GetMetatable() == nullptr);
        
        table->SetMetatable(metatable);
        REQUIRE(table->HasMetatable());
        REQUIRE(table->GetMetatable() == metatable);
        
        // 清除元表
        table->SetMetatable(nullptr);
        REQUIRE_FALSE(table->HasMetatable());
        REQUIRE(table->GetMetatable() == nullptr);
    }

    SECTION("__index元方法") {
        auto table = LuaTable::Create();
        auto metatable = LuaTable::Create();
        auto indexTable = LuaTable::Create();
        
        // 设置__index为另一个表
        metatable->SetHashValue(TValue::CreateString("__index"), TValue::CreateTable(indexTable));
        table->SetMetatable(metatable);
        
        // 在index表中设置值
        indexTable->SetHashValue(TValue::CreateString("inherited"), TValue::CreateString("value"));
        
        // 通过原表访问应该找到inherited值
        // 注意：这需要在实际实现中通过元方法机制实现
        /*
        REQUIRE(table->GetValue(TValue::CreateString("inherited")) == TValue::CreateString("value"));
        */
    }

    SECTION("__newindex元方法") {
        auto table = LuaTable::Create();
        auto metatable = LuaTable::Create();
        auto targetTable = LuaTable::Create();
        
        // 设置__newindex重定向新索引到另一个表
        metatable->SetHashValue(TValue::CreateString("__newindex"), TValue::CreateTable(targetTable));
        table->SetMetatable(metatable);
        
        // 设置新键应该重定向到target表
        // 注意：这需要在实际实现中通过元方法机制实现
        /*
        table->SetValue(TValue::CreateString("newkey"), TValue::CreateString("newvalue"));
        REQUIRE(targetTable->GetValue(TValue::CreateString("newkey")) == TValue::CreateString("newvalue"));
        REQUIRE(table->GetRawValue(TValue::CreateString("newkey")).IsNil()); // 原表中没有
        */
    }

    SECTION("__len元方法") {
        auto table = LuaTable::Create();
        auto metatable = LuaTable::Create();
        
        // 设置自定义长度函数
        metatable->SetHashValue(TValue::CreateString("__len"), TValue::CreateFunction(/* custom length function */));
        table->SetMetatable(metatable);
        
        // 长度计算应该调用__len元方法
        // 注意：这需要在实际实现中支持
        /*
        REQUIRE(table->GetLength() == custom_length_result);
        */
    }
}

/* ========================================================================== */
/* 遍历和迭代契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 遍历契约", "[table][contract][iteration]") {
    SECTION("数组部分遍历") {
        auto table = LuaTable::Create();
        
        // 设置数组元素
        table->SetArrayValue(1, TValue::CreateString("first"));
        table->SetArrayValue(2, TValue::CreateString("second"));
        table->SetArrayValue(3, TValue::CreateString("third"));
        
        // 数组遍历
        std::vector<TValue> values;
        table->ForEachArrayElement([&values](Index index, const TValue& value) {
            values.push_back(value);
            return true; // 继续遍历
        });
        
        REQUIRE(values.size() == 3);
        REQUIRE(values[0] == TValue::CreateString("first"));
        REQUIRE(values[1] == TValue::CreateString("second"));
        REQUIRE(values[2] == TValue::CreateString("third"));
    }

    SECTION("哈希部分遍历") {
        auto table = LuaTable::Create();
        
        // 设置哈希元素
        table->SetHashValue(TValue::CreateString("name"), TValue::CreateString("lua"));
        table->SetHashValue(TValue::CreateString("version"), TValue::CreateNumber(5.1));
        table->SetHashValue(TValue::CreateBoolean(true), TValue::CreateString("boolean_key"));
        
        // 哈希遍历
        std::map<std::string, TValue> pairs;
        table->ForEachHashElement([&pairs](const TValue& key, const TValue& value) {
            pairs[key.ToString()] = value;
            return true; // 继续遍历
        });
        
        REQUIRE(pairs.size() == 3);
        REQUIRE(pairs["name"] == TValue::CreateString("lua"));
        REQUIRE(pairs["5.1"] == TValue::CreateNumber(5.1));
        REQUIRE(pairs["true"] == TValue::CreateString("boolean_key"));
    }

    SECTION("完整表遍历") {
        auto table = LuaTable::Create();
        
        // 混合设置数组和哈希元素
        table->SetArrayValue(1, TValue::CreateString("array1"));
        table->SetArrayValue(2, TValue::CreateString("array2"));
        table->SetHashValue(TValue::CreateString("hash1"), TValue::CreateString("hashvalue1"));
        table->SetHashValue(TValue::CreateString("hash2"), TValue::CreateString("hashvalue2"));
        
        // 完整遍历
        std::vector<std::pair<TValue, TValue>> allPairs;
        table->ForEachElement([&allPairs](const TValue& key, const TValue& value) {
            allPairs.emplace_back(key, value);
            return true; // 继续遍历
        });
        
        REQUIRE(allPairs.size() == 4);
        
        // 验证包含所有键值对（顺序可能不确定）
        bool foundArray1 = false, foundArray2 = false, foundHash1 = false, foundHash2 = false;
        for (const auto& pair : allPairs) {
            if (pair.first == TValue::CreateNumber(1.0)) foundArray1 = true;
            if (pair.first == TValue::CreateNumber(2.0)) foundArray2 = true;
            if (pair.first == TValue::CreateString("hash1")) foundHash1 = true;
            if (pair.first == TValue::CreateString("hash2")) foundHash2 = true;
        }
        REQUIRE(foundArray1);
        REQUIRE(foundArray2);
        REQUIRE(foundHash1);
        REQUIRE(foundHash2);
    }

    SECTION("早期终止遍历") {
        auto table = LuaTable::Create();
        
        // 添加多个元素
        for (int i = 1; i <= 10; ++i) {
            table->SetArrayValue(i, TValue::CreateNumber(static_cast<double>(i)));
        }
        
        // 只遍历前3个元素
        int count = 0;
        table->ForEachArrayElement([&count](Index index, const TValue& value) {
            ++count;
            return count < 3; // 当count达到3时停止
        });
        
        REQUIRE(count == 3);
    }
}

/* ========================================================================== */
/* 内存管理和垃圾回收契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 内存管理契约", "[table][contract][memory]") {
    SECTION("引用计数和生命周期") {
        std::weak_ptr<LuaTable> weakRef;
        
        {
            auto table = LuaTable::Create();
            weakRef = table;
            REQUIRE_FALSE(weakRef.expired());
            
            // 表应该保持存活
            table->SetArrayValue(1, TValue::CreateString("test"));
            REQUIRE_FALSE(weakRef.expired());
        }
        
        // 离开作用域后表应该被销毁
        REQUIRE(weakRef.expired());
    }

    SECTION("循环引用处理") {
        auto table1 = LuaTable::Create();
        auto table2 = LuaTable::Create();
        
        // 创建循环引用
        table1->SetHashValue(TValue::CreateString("ref"), TValue::CreateTable(table2));
        table2->SetHashValue(TValue::CreateString("ref"), TValue::CreateTable(table1));
        
        // 表应该能够正确处理循环引用（通过GC）
        // 这需要垃圾回收器的支持
        /*
        std::weak_ptr<LuaTable> weak1 = table1;
        std::weak_ptr<LuaTable> weak2 = table2;
        
        table1.reset();
        table2.reset();
        
        // 运行GC后循环引用应该被打破
        RunGarbageCollection();
        REQUIRE(weak1.expired());
        REQUIRE(weak2.expired());
        */
    }

    SECTION("弱引用表") {
        auto table = LuaTable::Create();
        
        // 设置弱引用模式
        table->SetWeakMode(WeakMode::Keys);   // 键为弱引用
        table->SetWeakMode(WeakMode::Values); // 值为弱引用
        table->SetWeakMode(WeakMode::Both);   // 键值都为弱引用
        
        REQUIRE(table->GetWeakMode() == WeakMode::Both);
        
        // 弱引用表中的对象应该可以被GC回收
        /*
        auto weakKey = CreateGCObject();
        auto weakValue = CreateGCObject();
        table->SetHashValue(TValue::CreateObject(weakKey), TValue::CreateObject(weakValue));
        
        std::weak_ptr<GCObject> weakKeyRef = weakKey;
        std::weak_ptr<GCObject> weakValueRef = weakValue;
        
        weakKey.reset();
        weakValue.reset();
        
        RunGarbageCollection();
        
        REQUIRE(weakKeyRef.expired());
        REQUIRE(weakValueRef.expired());
        REQUIRE(table->IsEmpty()); // 弱引用对象被回收后表变空
        */
    }

    SECTION("GC标记遍历") {
        auto table = LuaTable::Create();
        auto childTable1 = LuaTable::Create();
        auto childTable2 = LuaTable::Create();
        
        // 建立引用关系
        table->SetHashValue(TValue::CreateString("child1"), TValue::CreateTable(childTable1));
        table->SetArrayValue(1, TValue::CreateTable(childTable2));
        
        // GC标记应该遍历所有引用的对象
        /*
        table->MarkGC(GCColor::Gray);
        
        // 子对象应该被标记
        REQUIRE(childTable1->GetGCColor() != GCColor::White0);
        REQUIRE(childTable2->GetGCColor() != GCColor::White0);
        */
    }
}

/* ========================================================================== */
/* 性能和优化契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - 性能契约", "[table][contract][performance]") {
    SECTION("数组访问性能") {
        auto table = LuaTable::Create();
        constexpr int size = 10000;
        
        // 预热
        for (int i = 1; i <= size; ++i) {
            table->SetArrayValue(i, TValue::CreateNumber(static_cast<double>(i)));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 数组访问应该是O(1)
        volatile double sum = 0.0;
        for (int i = 1; i <= size; ++i) {
            sum += table->GetArrayValue(i).GetNumber();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 访问10000个元素应该很快
        REQUIRE(duration.count() < 5000); // 少于5ms
        REQUIRE(sum > 0.0); // 防止优化掉
    }

    SECTION("哈希访问性能") {
        auto table = LuaTable::Create();
        constexpr int size = 1000;
        
        // 添加多个哈希元素
        for (int i = 0; i < size; ++i) {
            auto key = TValue::CreateString("key" + std::to_string(i));
            auto value = TValue::CreateNumber(static_cast<double>(i));
            table->SetHashValue(key, value);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 哈希访问应该接近O(1)
        volatile double sum = 0.0;
        for (int i = 0; i < size; ++i) {
            auto key = TValue::CreateString("key" + std::to_string(i));
            sum += table->GetHashValue(key).GetNumber();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 访问1000个哈希元素应该很快
        REQUIRE(duration.count() < 2000); // 少于2ms
        REQUIRE(sum > 0.0); // 防止优化掉
    }

    SECTION("内存使用效率") {
        auto table = LuaTable::Create();
        
        // 表的基本大小应该合理
        Size baseSize = table->GetMemorySize();
        REQUIRE(baseSize <= 128); // 期望空表不超过128字节
        
        // 添加元素后内存增长应该合理
        table->SetArrayValue(1, TValue::CreateNumber(1.0));
        Size withOneElement = table->GetMemorySize();
        REQUIRE(withOneElement - baseSize <= 32); // 单个元素增长不超过32字节
    }

    SECTION("重哈希性能") {
        auto table = LuaTable::Create(0, 4); // 小的初始哈希大小
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 添加大量元素触发重哈希
        for (int i = 0; i < 1000; ++i) {
            auto key = TValue::CreateString("key" + std::to_string(i));
            auto value = TValue::CreateNumber(static_cast<double>(i));
            table->SetHashValue(key, value);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 即使有重哈希，性能也应该可以接受
        REQUIRE(duration.count() < 10000); // 少于10ms
        
        // 验证所有数据仍然正确
        for (int i = 0; i < 1000; ++i) {
            auto key = TValue::CreateString("key" + std::to_string(i));
            REQUIRE(table->GetHashValue(key) == TValue::CreateNumber(static_cast<double>(i)));
        }
    }
}

/* ========================================================================== */
/* Lua 5.1.5兼容性契约 */
/* ========================================================================== */

TEST_CASE("LuaTable - Lua 5.1.5兼容性契约", "[table][contract][compatibility]") {
    SECTION("数组索引兼容性") {
        auto table = LuaTable::Create();
        
        // Lua 5.1.5数组索引从1开始
        table->SetValue(TValue::CreateNumber(1.0), TValue::CreateString("first"));
        table->SetValue(TValue::CreateNumber(2.0), TValue::CreateString("second"));
        
        REQUIRE(table->GetLength() == 2);
        REQUIRE(table->GetValue(TValue::CreateNumber(1.0)) == TValue::CreateString("first"));
        REQUIRE(table->GetValue(TValue::CreateNumber(2.0)) == TValue::CreateString("second"));
        
        // 索引0不是数组部分
        table->SetValue(TValue::CreateNumber(0.0), TValue::CreateString("zero"));
        REQUIRE(table->GetLength() == 2); // 长度不变
    }

    SECTION("边界长度语义") {
        auto table = LuaTable::Create();
        
        // Lua 5.1.5的长度定义：最大的正整数索引n，使得t[n]不是nil且t[n+1]是nil
        table->SetArrayValue(1, TValue::CreateString("a"));
        table->SetArrayValue(2, TValue::CreateString("b"));
        table->SetArrayValue(3, TValue::CreateString("c"));
        REQUIRE(table->GetLength() == 3);
        
        // 中间有nil的情况
        table->SetArrayValue(2, TValue::CreateNil());
        REQUIRE(table->GetLength() == 1); // 长度到第一个nil为止
        
        // 恢复连续性
        table->SetArrayValue(2, TValue::CreateString("b2"));
        REQUIRE(table->GetLength() == 3);
    }

    SECTION("类型强制转换") {
        auto table = LuaTable::Create();
        
        // 数字字符串键的处理
        auto strKey = TValue::CreateString("42");
        auto numKey = TValue::CreateNumber(42.0);
        
        // 在Lua 5.1.5中，字符串"42"和数字42是不同的键
        table->SetValue(strKey, TValue::CreateString("string_key"));
        table->SetValue(numKey, TValue::CreateString("number_key"));
        
        REQUIRE(table->GetValue(strKey) == TValue::CreateString("string_key"));
        REQUIRE(table->GetValue(numKey) == TValue::CreateString("number_key"));
        REQUIRE(table->GetValue(strKey) != table->GetValue(numKey));
    }

    SECTION("表作为键的处理") {
        auto table = LuaTable::Create();
        auto keyTable1 = LuaTable::Create();
        auto keyTable2 = LuaTable::Create();
        
        // 不同的表对象作为键
        table->SetValue(TValue::CreateTable(keyTable1), TValue::CreateString("table1"));
        table->SetValue(TValue::CreateTable(keyTable2), TValue::CreateString("table2"));
        
        REQUIRE(table->GetValue(TValue::CreateTable(keyTable1)) == TValue::CreateString("table1"));
        REQUIRE(table->GetValue(TValue::CreateTable(keyTable2)) == TValue::CreateString("table2"));
        
        // 同一个表对象应该对应同一个键
        REQUIRE(table->GetValue(TValue::CreateTable(keyTable1)) == TValue::CreateString("table1"));
    }

    SECTION("next函数语义") {
        auto table = LuaTable::Create();
        
        // 设置一些键值对
        table->SetArrayValue(1, TValue::CreateString("first"));
        table->SetHashValue(TValue::CreateString("name"), TValue::CreateString("lua"));
        table->SetArrayValue(2, TValue::CreateString("second"));
        
        // next(table, nil) 应该返回第一个键值对
        auto firstPair = table->Next(TValue::CreateNil());
        REQUIRE_FALSE(firstPair.first.IsNil());
        REQUIRE_FALSE(firstPair.second.IsNil());
        
        // next(table, key) 应该返回下一个键值对
        auto secondPair = table->Next(firstPair.first);
        
        // 遍历整个表
        std::set<std::string> foundKeys;
        TValue currentKey = TValue::CreateNil();
        while (true) {
            auto pair = table->Next(currentKey);
            if (pair.first.IsNil()) break; // 遍历结束
            
            foundKeys.insert(pair.first.ToString());
            currentKey = pair.first;
        }
        
        REQUIRE(foundKeys.size() == 3);
        REQUIRE(foundKeys.count("1") == 1);
        REQUIRE(foundKeys.count("2") == 1);
        REQUIRE(foundKeys.count("name") == 1);
    }
}