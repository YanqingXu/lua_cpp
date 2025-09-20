/**
 * @file test_string_contract.cpp
 * @brief LuaString（Lua字符串）契约测试
 * @description 测试字符串池化、比较、哈希、内存管理等行为
 *              确保100% Lua 5.1.5兼容性和高效的字符串处理
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "types/lua_string.h"
#include "types/string_pool.h"
#include "types/tvalue.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"
#include "gc/gc_object.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* LuaString基础构造和属性契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 基础构造契约", "[string][contract][core]") {
    SECTION("从C字符串创建") {
        auto str = LuaString::Create("hello world");
        REQUIRE(str != nullptr);
        REQUIRE(str->GetLength() == 11);
        REQUIRE(str->GetData() != nullptr);
        REQUIRE(std::string(str->GetData(), str->GetLength()) == "hello world");
        REQUIRE(str->GetCString() == std::string("hello world"));
    }

    SECTION("从std::string创建") {
        std::string stdStr = "lua string test";
        auto str = LuaString::Create(stdStr);
        REQUIRE(str != nullptr);
        REQUIRE(str->GetLength() == stdStr.length());
        REQUIRE(str->GetCString() == stdStr);
    }

    SECTION("从字符数组和长度创建") {
        const char* data = "embedded\0null\0bytes";
        Size length = 18; // 包含嵌入的null字节
        auto str = LuaString::Create(data, length);
        
        REQUIRE(str != nullptr);
        REQUIRE(str->GetLength() == length);
        REQUIRE(std::memcmp(str->GetData(), data, length) == 0);
        
        // 验证包含null字节的字符串处理
        REQUIRE(str->GetData()[8] == '\0');
        REQUIRE(str->GetData()[13] == '\0');
    }

    SECTION("空字符串创建") {
        auto emptyStr = LuaString::Create("");
        REQUIRE(emptyStr != nullptr);
        REQUIRE(emptyStr->GetLength() == 0);
        REQUIRE(emptyStr->GetCString() == "");
        
        // 空字符串的数据指针应该有效
        REQUIRE(emptyStr->GetData() != nullptr);
    }

    SECTION("长字符串创建") {
        // 创建超过典型小字符串优化阈值的字符串
        std::string longStr(10000, 'x');
        auto str = LuaString::Create(longStr);
        
        REQUIRE(str != nullptr);
        REQUIRE(str->GetLength() == 10000);
        REQUIRE(str->GetCString() == longStr);
        REQUIRE(str->IsLongString());
        REQUIRE_FALSE(str->IsShortString());
    }

    SECTION("字符串不可变性") {
        auto str = LuaString::Create("immutable");
        const char* originalData = str->GetData();
        Size originalLength = str->GetLength();
        
        // 多次访问应该返回相同的数据
        REQUIRE(str->GetData() == originalData);
        REQUIRE(str->GetLength() == originalLength);
        REQUIRE(str->GetCString() == "immutable");
        
        // 数据应该是常量（通过类型系统保证）
        static_assert(std::is_const_v<std::remove_pointer_t<decltype(str->GetData())>>, 
                     "String data should be const");
    }
}

/* ========================================================================== */
/* 字符串池化（String Interning）契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 字符串池化契约", "[string][contract][interning]") {
    SECTION("相同内容字符串共享实例") {
        auto str1 = LuaString::Create("shared string");
        auto str2 = LuaString::Create("shared string");
        
        // 相同内容的字符串应该是同一个实例
        REQUIRE(str1 == str2); // 指针相等
        REQUIRE(str1.get() == str2.get());
        
        // 确保这是真正的共享，而不是内容相等
        REQUIRE(str1->GetData() == str2->GetData()); // 数据指针也相同
    }

    SECTION("不同内容字符串不共享") {
        auto str1 = LuaString::Create("string one");
        auto str2 = LuaString::Create("string two");
        
        REQUIRE(str1 != str2); // 指针不等
        REQUIRE(str1.get() != str2.get());
        REQUIRE(str1->GetData() != str2->GetData());
    }

    SECTION("空字符串池化") {
        auto empty1 = LuaString::Create("");
        auto empty2 = LuaString::Create("");
        auto empty3 = LuaString::Create(nullptr, 0);
        
        // 所有空字符串应该共享同一个实例
        REQUIRE(empty1 == empty2);
        REQUIRE(empty2 == empty3);
        REQUIRE(empty1.get() == empty2.get());
        REQUIRE(empty2.get() == empty3.get());
    }

    SECTION("大小写敏感的池化") {
        auto lower = LuaString::Create("case sensitive");
        auto upper = LuaString::Create("Case Sensitive");
        auto mixed = LuaString::Create("CASE SENSITIVE");
        
        // 大小写不同的字符串应该是不同的实例
        REQUIRE(lower != upper);
        REQUIRE(upper != mixed);
        REQUIRE(lower != mixed);
    }

    SECTION("嵌入null字节的池化") {
        const char* data1 = "null\0byte\0test";
        const char* data2 = "null\0byte\0test";
        Size length = 15;
        
        auto str1 = LuaString::Create(data1, length);
        auto str2 = LuaString::Create(data2, length);
        
        // 包含null字节的相同内容字符串也应该池化
        REQUIRE(str1 == str2);
        REQUIRE(str1.get() == str2.get());
    }

    SECTION("字符串池查找接口") {
        auto original = LuaString::Create("lookup test");
        
        // 通过池查找应该返回相同实例
        auto found = StringPool::GetInstance().Find("lookup test");
        REQUIRE(found == original);
        
        // 查找不存在的字符串
        auto notFound = StringPool::GetInstance().Find("does not exist");
        REQUIRE(notFound == nullptr);
    }
}

/* ========================================================================== */
/* 哈希计算契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 哈希计算契约", "[string][contract][hash]") {
    SECTION("一致性哈希计算") {
        auto str = LuaString::Create("hash consistency test");
        
        HashValue hash1 = str->GetHash();
        HashValue hash2 = str->GetHash();
        
        // 多次调用应该返回相同的哈希值
        REQUIRE(hash1 == hash2);
        REQUIRE(hash1 != 0); // 哈希值不应该是0（除非特殊情况）
    }

    SECTION("不同字符串不同哈希") {
        auto str1 = LuaString::Create("string one");
        auto str2 = LuaString::Create("string two");
        auto str3 = LuaString::Create("very different content");
        
        HashValue hash1 = str1->GetHash();
        HashValue hash2 = str2->GetHash();
        HashValue hash3 = str3->GetHash();
        
        // 不同字符串应该有不同的哈希值（高概率）
        REQUIRE(hash1 != hash2);
        REQUIRE(hash2 != hash3);
        REQUIRE(hash1 != hash3);
    }

    SECTION("相同内容相同哈希") {
        auto str1 = LuaString::Create("same content");
        auto str2 = LuaString::Create("same content");
        
        // 池化后的字符串应该有相同的哈希值
        REQUIRE(str1 == str2); // 确保是池化的
        REQUIRE(str1->GetHash() == str2->GetHash());
    }

    SECTION("空字符串哈希") {
        auto emptyStr = LuaString::Create("");
        HashValue emptyHash = emptyStr->GetHash();
        
        // 空字符串应该有确定的哈希值
        REQUIRE(emptyHash != 0); // 或者根据实现定义的特定值
        
        // 多个空字符串实例应该有相同的哈希
        auto anotherEmpty = LuaString::Create("");
        REQUIRE(anotherEmpty->GetHash() == emptyHash);
    }

    SECTION("嵌入null字节的哈希") {
        const char* data1 = "null\0in\0middle";
        const char* data2 = "null in middle"; // 没有null字节
        Size length1 = 14;
        Size length2 = 14;
        
        auto str1 = LuaString::Create(data1, length1);
        auto str2 = LuaString::Create(data2, length2);
        
        // 包含null字节和不包含的字符串应该有不同的哈希
        REQUIRE(str1->GetHash() != str2->GetHash());
    }

    SECTION("哈希分布质量") {
        std::set<HashValue> hashes;
        constexpr int testCount = 1000;
        
        // 生成大量不同的字符串并检查哈希分布
        for (int i = 0; i < testCount; ++i) {
            std::string testStr = "test_string_" + std::to_string(i);
            auto str = LuaString::Create(testStr);
            hashes.insert(str->GetHash());
        }
        
        // 期望大部分哈希值都不同（良好的分布）
        double uniqueRatio = static_cast<double>(hashes.size()) / testCount;
        REQUIRE(uniqueRatio > 0.95); // 至少95%的哈希值唯一
    }
}

/* ========================================================================== */
/* 字符串比较契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 字符串比较契约", "[string][contract][comparison]") {
    SECTION("相等性比较") {
        auto str1 = LuaString::Create("equal test");
        auto str2 = LuaString::Create("equal test");
        auto str3 = LuaString::Create("different");
        
        // 相同内容的字符串相等
        REQUIRE(str1->Equals(*str2));
        REQUIRE(str2->Equals(*str1));
        
        // 不同内容的字符串不相等
        REQUIRE_FALSE(str1->Equals(*str3));
        REQUIRE_FALSE(str3->Equals(*str1));
        
        // 池化的字符串可以通过指针比较
        REQUIRE(str1 == str2); // 快速指针比较
    }

    SECTION("字典序比较") {
        auto str1 = LuaString::Create("abc");
        auto str2 = LuaString::Create("def");
        auto str3 = LuaString::Create("abc");
        auto str4 = LuaString::Create("ab");
        
        // 字典序比较
        REQUIRE(str1->Compare(*str2) < 0); // "abc" < "def"
        REQUIRE(str2->Compare(*str1) > 0); // "def" > "abc"
        REQUIRE(str1->Compare(*str3) == 0); // "abc" == "abc"
        REQUIRE(str1->Compare(*str4) > 0); // "abc" > "ab"
        REQUIRE(str4->Compare(*str1) < 0); // "ab" < "abc"
    }

    SECTION("大小写敏感比较") {
        auto lower = LuaString::Create("lowercase");
        auto upper = LuaString::Create("LOWERCASE");
        auto mixed = LuaString::Create("LowerCase");
        
        // Lua字符串比较是大小写敏感的
        REQUIRE_FALSE(lower->Equals(*upper));
        REQUIRE_FALSE(lower->Equals(*mixed));
        REQUIRE_FALSE(upper->Equals(*mixed));
        
        // 大小写影响字典序
        REQUIRE(upper->Compare(*lower) != 0);
    }

    SECTION("嵌入null字节比较") {
        const char* data1 = "null\0byte";
        const char* data2 = "null\0byte";
        const char* data3 = "null byte"; // 空格代替null
        Size length = 9;
        
        auto str1 = LuaString::Create(data1, length);
        auto str2 = LuaString::Create(data2, length);
        auto str3 = LuaString::Create(data3, length);
        
        // 包含相同null字节的字符串相等
        REQUIRE(str1->Equals(*str2));
        REQUIRE(str1->Compare(*str2) == 0);
        
        // null字节和空格不相等
        REQUIRE_FALSE(str1->Equals(*str3));
        REQUIRE(str1->Compare(*str3) != 0);
    }

    SECTION("长度不同的比较") {
        auto short_str = LuaString::Create("short");
        auto long_str = LuaString::Create("much longer string");
        auto prefix = LuaString::Create("much");
        
        REQUIRE_FALSE(short_str->Equals(*long_str));
        REQUIRE(short_str->Compare(*long_str) != 0);
        
        // 前缀比较
        REQUIRE_FALSE(prefix->Equals(*long_str));
        REQUIRE(prefix->Compare(*long_str) < 0); // 较短的字符串小于较长的
    }

    SECTION("操作符重载") {
        auto str1 = LuaString::Create("operator");
        auto str2 = LuaString::Create("operator");
        auto str3 = LuaString::Create("different");
        
        // 相等操作符
        REQUIRE(*str1 == *str2);
        REQUIRE_FALSE(*str1 == *str3);
        
        // 不等操作符
        REQUIRE_FALSE(*str1 != *str2);
        REQUIRE(*str1 != *str3);
        
        // 比较操作符
        REQUIRE_FALSE(*str1 < *str2);
        REQUIRE(*str3 < *str1); // "different" < "operator"
        REQUIRE(*str1 <= *str2);
        REQUIRE(*str1 >= *str2);
    }
}

/* ========================================================================== */
/* 内存管理和生命周期契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 内存管理契约", "[string][contract][memory]") {
    SECTION("引用计数生命周期") {
        std::weak_ptr<LuaString> weakRef;
        
        {
            auto str = LuaString::Create("reference counting test");
            weakRef = str;
            REQUIRE_FALSE(weakRef.expired());
            
            // 字符串应该保持存活
            REQUIRE(str->GetLength() > 0);
            REQUIRE_FALSE(weakRef.expired());
        }
        
        // 注意：由于字符串池化，字符串可能仍然存活
        // 这取决于具体的池化策略实现
        /*
        REQUIRE(weakRef.expired());
        */
    }

    SECTION("短字符串优化") {
        // 短字符串应该有优化的内存布局
        auto shortStr = LuaString::Create("short");
        REQUIRE(shortStr->IsShortString());
        REQUIRE_FALSE(shortStr->IsLongString());
        
        // 短字符串的内存使用应该更紧凑
        Size shortMemory = shortStr->GetMemorySize();
        
        auto longStr = LuaString::Create(std::string(1000, 'x'));
        REQUIRE(longStr->IsLongString());
        REQUIRE_FALSE(longStr->IsShortString());
        
        Size longMemory = longStr->GetMemorySize();
        
        // 长字符串的每字符平均内存开销应该更小
        double shortOverhead = static_cast<double>(shortMemory) / shortStr->GetLength();
        double longOverhead = static_cast<double>(longMemory) / longStr->GetLength();
        REQUIRE(longOverhead < shortOverhead);
    }

    SECTION("内存对齐") {
        auto str = LuaString::Create("alignment test");
        
        // 字符串对象应该正确对齐
        REQUIRE(IsAligned(str.get(), alignof(LuaString)));
        REQUIRE(IsAligned(str->GetData(), alignof(char)));
        
        // 内存大小应该是对齐的
        Size memSize = str->GetMemorySize();
        REQUIRE(memSize % LUA_CPP_MEMORY_ALIGN == 0);
    }

    SECTION("大量字符串创建") {
        std::vector<std::shared_ptr<LuaString>> strings;
        constexpr int count = 10000;
        
        // 创建大量不同的字符串
        for (int i = 0; i < count; ++i) {
            std::string content = "string_" + std::to_string(i);
            strings.push_back(LuaString::Create(content));
        }
        
        // 验证所有字符串都正确创建
        REQUIRE(strings.size() == count);
        for (int i = 0; i < count; ++i) {
            std::string expected = "string_" + std::to_string(i);
            REQUIRE(strings[i]->GetCString() == expected);
        }
        
        // 清理应该能正常进行
        strings.clear();
    }

    SECTION("循环引用防护") {
        // 字符串本身不包含对其他对象的引用，不会有循环引用问题
        // 但测试字符串池的内存管理
        
        auto str1 = LuaString::Create("no cycles");
        std::weak_ptr<LuaString> weak1 = str1;
        
        // 创建相同内容的字符串（应该返回池化的实例）
        auto str2 = LuaString::Create("no cycles");
        REQUIRE(str1 == str2);
        
        str1.reset();
        // str2仍然持有引用，对象应该存活
        REQUIRE_FALSE(weak1.expired());
        
        str2.reset();
        // 现在对象可能被回收（取决于池化策略）
    }
}

/* ========================================================================== */
/* 垃圾回收集成契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 垃圾回收契约", "[string][contract][gc]") {
    SECTION("GC对象接口") {
        auto str = LuaString::Create("gc integration test");
        
        // 字符串应该实现GCObject接口
        REQUIRE(str->GetGCType() == GCObjectType::String);
        REQUIRE(str->GetGCColor() != GCColor::Black); // 初始不是黑色
        
        // 字符串不包含其他GC对象的引用
        REQUIRE_FALSE(str->HasReferences());
    }

    SECTION("GC标记过程") {
        auto str = LuaString::Create("mark test");
        
        // 设置GC颜色
        str->SetGCColor(GCColor::Gray);
        REQUIRE(str->GetGCColor() == GCColor::Gray);
        
        str->SetGCColor(GCColor::Black);
        REQUIRE(str->GetGCColor() == GCColor::Black);
        
        // 字符串标记不需要遍历子对象
        str->MarkReferences(GCColor::Gray); // 应该是空操作
    }

    SECTION("弱引用支持") {
        auto str = LuaString::Create("weak reference test");
        std::weak_ptr<LuaString> weakStr = str;
        
        REQUIRE_FALSE(weakStr.expired());
        
        // 模拟GC回收
        str.reset();
        
        // 字符串可能仍在池中存活，这取决于实现策略
        // 这个测试主要验证弱引用机制工作正常
        if (weakStr.expired()) {
            // 字符串被回收
            REQUIRE(weakStr.lock() == nullptr);
        } else {
            // 字符串仍在池中
            REQUIRE(weakStr.lock() != nullptr);
        }
    }

    SECTION("GC统计信息") {
        Size initialMemory = StringPool::GetInstance().GetTotalMemory();
        Size initialCount = StringPool::GetInstance().GetStringCount();
        
        // 创建一些字符串
        std::vector<std::shared_ptr<LuaString>> strings;
        for (int i = 0; i < 100; ++i) {
            strings.push_back(LuaString::Create("gc_test_" + std::to_string(i)));
        }
        
        Size afterCreation = StringPool::GetInstance().GetTotalMemory();
        Size afterCreationCount = StringPool::GetInstance().GetStringCount();
        
        // 内存和计数应该增加
        REQUIRE(afterCreation > initialMemory);
        REQUIRE(afterCreationCount > initialCount);
        
        // 清理字符串
        strings.clear();
        
        // 强制GC（如果支持）
        StringPool::GetInstance().CollectGarbage();
        
        Size afterGC = StringPool::GetInstance().GetTotalMemory();
        Size afterGCCount = StringPool::GetInstance().GetStringCount();
        
        // GC后内存应该释放（如果字符串不在池中持久化）
        REQUIRE(afterGC <= afterCreation);
        REQUIRE(afterGCCount <= afterCreationCount);
    }
}

/* ========================================================================== */
/* 性能和效率契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 性能契约", "[string][contract][performance]") {
    SECTION("创建性能") {
        constexpr int iterations = 10000;
        std::vector<std::shared_ptr<LuaString>> strings;
        strings.reserve(iterations);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 创建大量不同的字符串
        for (int i = 0; i < iterations; ++i) {
            strings.push_back(LuaString::Create("perf_test_" + std::to_string(i)));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 字符串创建应该很快
        REQUIRE(duration.count() < 50000); // 少于50ms
        
        // 验证创建正确
        REQUIRE(strings.size() == iterations);
        REQUIRE(strings[0]->GetCString() == "perf_test_0");
        REQUIRE(strings[iterations-1]->GetCString() == "perf_test_" + std::to_string(iterations-1));
    }

    SECTION("池化查找性能") {
        // 预先创建一些字符串
        std::vector<std::string> testStrings;
        for (int i = 0; i < 1000; ++i) {
            testStrings.push_back("lookup_test_" + std::to_string(i));
            LuaString::Create(testStrings.back()); // 添加到池中
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 大量池化查找
        volatile int found = 0;
        for (int rep = 0; rep < 100; ++rep) {
            for (const auto& testStr : testStrings) {
                auto result = StringPool::GetInstance().Find(testStr);
                if (result != nullptr) ++found;
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 池化查找应该很快
        REQUIRE(duration.count() < 10000); // 少于10ms
        REQUIRE(found > 0); // 防止优化掉
    }

    SECTION("哈希计算性能") {
        auto longStr = LuaString::Create(std::string(100000, 'x'));
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 多次哈希计算（应该缓存）
        volatile HashValue hash = 0;
        for (int i = 0; i < 10000; ++i) {
            hash ^= longStr->GetHash();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 缓存的哈希访问应该很快
        REQUIRE(duration.count() < 1000); // 少于1ms
        REQUIRE(hash != 0); // 防止优化掉
    }

    SECTION("比较性能") {
        auto str1 = LuaString::Create("comparison performance test string");
        auto str2 = LuaString::Create("comparison performance test string");
        auto str3 = LuaString::Create("different string for comparison");
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 大量比较操作
        volatile int equal = 0, different = 0;
        for (int i = 0; i < 100000; ++i) {
            if (str1->Equals(*str2)) ++equal;
            if (str1->Equals(*str3)) ++different;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 字符串比较应该很快（池化的字符串可以用指针比较）
        REQUIRE(duration.count() < 5000); // 少于5ms
        REQUIRE(equal > 0 && different >= 0); // 防止优化掉
    }

    SECTION("内存效率") {
        // 测试短字符串的内存效率
        auto shortStr = LuaString::Create("short");
        Size shortSize = shortStr->GetMemorySize();
        Size shortLen = shortStr->GetLength();
        
        // 短字符串的内存开销应该合理
        REQUIRE(shortSize <= shortLen + 32); // 不超过内容长度+32字节开销
        
        // 测试长字符串的内存效率
        auto longStr = LuaString::Create(std::string(10000, 'L'));
        Size longSize = longStr->GetMemorySize();
        Size longLen = longStr->GetLength();
        
        // 长字符串的开销比例应该很小
        double overhead = static_cast<double>(longSize - longLen) / longLen;
        REQUIRE(overhead < 0.1); // 开销不超过10%
    }
}

/* ========================================================================== */
/* Lua 5.1.5兼容性契约 */
/* ========================================================================== */

TEST_CASE("LuaString - Lua 5.1.5兼容性契约", "[string][contract][compatibility]") {
    SECTION("字符串相等语义") {
        auto str1 = LuaString::Create("lua equality test");
        auto str2 = LuaString::Create("lua equality test");
        auto str3 = LuaString::Create("different content");
        
        // Lua中相同内容的字符串相等
        REQUIRE(str1->Equals(*str2));
        REQUIRE_FALSE(str1->Equals(*str3));
        
        // 池化确保相同内容的字符串是同一个实例
        REQUIRE(str1 == str2);
    }

    SECTION("字符串长度操作") {
        auto str = LuaString::Create("length test");
        
        // Lua的#操作符返回字符串长度
        REQUIRE(str->GetLength() == 11);
        
        // 包含null字节的字符串
        const char* nullStr = "null\0byte\0string";
        auto strWithNull = LuaString::Create(nullStr, 17);
        REQUIRE(strWithNull->GetLength() == 17); // 完整长度，包括null字节
    }

    SECTION("字符串连接语义") {
        auto str1 = LuaString::Create("Hello ");
        auto str2 = LuaString::Create("World");
        
        // 字符串连接应该创建新字符串
        auto concatenated = LuaString::Concatenate(*str1, *str2);
        REQUIRE(concatenated->GetCString() == "Hello World");
        REQUIRE(concatenated->GetLength() == 11);
        
        // 原字符串不变
        REQUIRE(str1->GetCString() == "Hello ");
        REQUIRE(str2->GetCString() == "World");
    }

    SECTION("字符串转数字") {
        // 可以转换为数字的字符串
        auto numStr1 = LuaString::Create("42");
        auto numStr2 = LuaString::Create("3.14159");
        auto numStr3 = LuaString::Create("  -123.45  "); // 带空格
        
        double result;
        REQUIRE(numStr1->ToNumber(result));
        REQUIRE(result == Approx(42.0));
        
        REQUIRE(numStr2->ToNumber(result));
        REQUIRE(result == Approx(3.14159));
        
        REQUIRE(numStr3->ToNumber(result));
        REQUIRE(result == Approx(-123.45));
        
        // 不能转换的字符串
        auto nonNum1 = LuaString::Create("hello");
        auto nonNum2 = LuaString::Create("12.34.56");
        auto nonNum3 = LuaString::Create("");
        
        REQUIRE_FALSE(nonNum1->ToNumber(result));
        REQUIRE_FALSE(nonNum2->ToNumber(result));
        REQUIRE_FALSE(nonNum3->ToNumber(result));
    }

    SECTION("字符串比较顺序") {
        // Lua 5.1.5字符串按字典序比较
        auto a = LuaString::Create("a");
        auto b = LuaString::Create("b");
        auto aa = LuaString::Create("aa");
        auto ab = LuaString::Create("ab");
        
        REQUIRE(a->Compare(*b) < 0); // "a" < "b"
        REQUIRE(b->Compare(*a) > 0); // "b" > "a"
        REQUIRE(a->Compare(*aa) < 0); // "a" < "aa"
        REQUIRE(aa->Compare(*ab) < 0); // "aa" < "ab"
    }

    SECTION("Unicode和多字节字符") {
        // Lua 5.1.5将字符串视为字节序列
        auto utf8Str = LuaString::Create("Hello 世界"); // UTF-8中文
        
        // 长度是字节数，不是字符数
        Size byteLength = utf8Str->GetLength();
        REQUIRE(byteLength > 8); // "Hello "是6字节，"世界"是6字节（UTF-8）
        
        // 字符串内容按字节处理
        const char* data = utf8Str->GetData();
        REQUIRE(data[0] == 'H');
        REQUIRE(data[5] == ' ');
        // UTF-8字节序列
        REQUIRE(static_cast<unsigned char>(data[6]) == 0xE4); // "世"的第一个字节
    }

    SECTION("字符串作为表键") {
        // 在Lua中，字符串经常用作表的键
        auto key1 = LuaString::Create("table_key");
        auto key2 = LuaString::Create("table_key");
        auto key3 = LuaString::Create("different_key");
        
        // 相同内容的字符串应该产生相同的哈希值
        REQUIRE(key1->GetHash() == key2->GetHash());
        REQUIRE(key1->GetHash() != key3->GetHash());
        
        // 池化确保可以用指针比较
        REQUIRE(key1 == key2);
        REQUIRE(key1 != key3);
    }

    SECTION("字符串元表") {
        // Lua 5.1.5中所有字符串共享同一个元表
        auto str1 = LuaString::Create("string with metatable");
        auto str2 = LuaString::Create("another string");
        
        // 字符串类型的元表访问（这需要在实际实现中支持）
        /*
        auto metatable1 = str1->GetMetatable();
        auto metatable2 = str2->GetMetatable();
        
        // 所有字符串应该共享相同的元表
        REQUIRE(metatable1 == metatable2);
        REQUIRE(metatable1 != nullptr);
        */
    }
}

/* ========================================================================== */
/* 错误处理和边界情况契约 */
/* ========================================================================== */

TEST_CASE("LuaString - 错误处理契约", "[string][contract][error-handling]") {
    SECTION("无效输入处理") {
        // null指针输入
        REQUIRE_THROWS_AS(LuaString::Create(nullptr), std::invalid_argument);
        
        // 空指针但非零长度
        REQUIRE_THROWS_AS(LuaString::Create(nullptr, 10), std::invalid_argument);
        
        // 负数长度（如果Size是无符号的，这可能不适用）
        // REQUIRE_THROWS_AS(LuaString::Create("test", -1), std::invalid_argument);
    }

    SECTION("极大字符串处理") {
        // 测试接近最大长度的字符串
        constexpr Size largeSize = 1000000; // 1MB字符串
        
        std::string largeContent(largeSize, 'X');
        auto largeStr = LuaString::Create(largeContent);
        
        REQUIRE(largeStr != nullptr);
        REQUIRE(largeStr->GetLength() == largeSize);
        REQUIRE(largeStr->IsLongString());
        
        // 验证内容正确
        REQUIRE(largeStr->GetCString() == largeContent);
    }

    SECTION("内存不足处理") {
        // 这个测试可能难以实现，因为很难模拟内存不足
        // 但应该确保在内存不足时有适当的错误处理
        /*
        try {
            // 尝试创建极大的字符串
            std::string hugeContent(SIZE_MAX / 2, 'X');
            auto hugeStr = LuaString::Create(hugeContent);
            // 如果成功，验证基本功能
            REQUIRE(hugeStr != nullptr);
        } catch (const std::bad_alloc&) {
            // 内存不足是预期的行为
            REQUIRE(true);
        }
        */
    }

    SECTION("并发安全性") {
        // 字符串池应该是线程安全的
        constexpr int threadCount = 4;
        constexpr int stringsPerThread = 1000;
        
        std::vector<std::thread> threads;
        std::vector<std::vector<std::shared_ptr<LuaString>>> results(threadCount);
        
        // 启动多个线程同时创建字符串
        for (int t = 0; t < threadCount; ++t) {
            threads.emplace_back([t, &results]() {
                for (int i = 0; i < stringsPerThread; ++i) {
                    std::string content = "thread_" + std::to_string(t) + "_string_" + std::to_string(i);
                    results[t].push_back(LuaString::Create(content));
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 验证结果
        for (int t = 0; t < threadCount; ++t) {
            REQUIRE(results[t].size() == stringsPerThread);
            for (int i = 0; i < stringsPerThread; ++i) {
                std::string expected = "thread_" + std::to_string(t) + "_string_" + std::to_string(i);
                REQUIRE(results[t][i]->GetCString() == expected);
            }
        }
        
        // 验证相同内容的字符串在不同线程中是池化的
        auto str1 = LuaString::Create("concurrent_test");
        auto str2 = LuaString::Create("concurrent_test");
        REQUIRE(str1 == str2);
    }

    SECTION("字符串池容量限制") {
        // 测试字符串池的容量管理
        Size initialCount = StringPool::GetInstance().GetStringCount();
        
        // 创建大量字符串
        std::vector<std::shared_ptr<LuaString>> strings;
        for (int i = 0; i < 10000; ++i) {
            strings.push_back(LuaString::Create("capacity_test_" + std::to_string(i)));
        }
        
        Size afterCreation = StringPool::GetInstance().GetStringCount();
        REQUIRE(afterCreation > initialCount);
        
        // 清理部分字符串
        strings.erase(strings.begin(), strings.begin() + 5000);
        
        // 强制GC
        StringPool::GetInstance().CollectGarbage();
        
        Size afterGC = StringPool::GetInstance().GetStringCount();
        
        // 验证池的状态合理
        REQUIRE(afterGC <= afterCreation);
    }
}