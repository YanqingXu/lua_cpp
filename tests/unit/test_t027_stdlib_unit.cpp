/**
 * @file test_t027_stdlib_unit.cpp
 * @brief T027标准库完整单元测试套件
 * @description 测试EnhancedVirtualMachine集成的标准库功能
 * - Base库：type, tostring, tonumber, print, rawget/rawset等
 * - String库：len, sub, upper, lower, find, format等
 * - Table库：insert, remove, sort, concat等  
 * - Math库：sin, cos, sqrt, random等
 * @author Lua C++ Project Team
 * @date 2025-01-28
 * @version T027 - Complete Standard Library Implementation
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

// 测试目标：T027标准库实现
#include "src/vm/enhanced_virtual_machine.h"
#include "src/stdlib/stdlib.h"
#include "src/stdlib/base_lib.h" 
#include "src/stdlib/string_lib.h"
#include "src/stdlib/table_lib.h"
#include "src/stdlib/math_lib.h"

using namespace lua_cpp;
using ::testing::_;
using ::testing::Return;
using ::testing::AtLeast;

namespace lua_cpp {
namespace test {

/* ========================================================================== */
/* 测试基础设施 */
/* ========================================================================== */

/**
 * @brief T027标准库测试基类
 */
class T027StandardLibraryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建增强虚拟机（包含T027标准库）
        vm_ = std::make_unique<EnhancedVirtualMachine>();
        
        // 获取标准库实例
        stdlib_ = vm_->GetStandardLibrary();
        ASSERT_NE(stdlib_, nullptr) << "标准库应该已初始化";
        
        // 获取各个子库
        base_lib_ = stdlib_->GetBaseLibrary();
        string_lib_ = stdlib_->GetStringLibrary(); 
        table_lib_ = stdlib_->GetTableLibrary();
        math_lib_ = stdlib_->GetMathLibrary();
        
        ASSERT_NE(base_lib_, nullptr) << "Base库应该存在";
        ASSERT_NE(string_lib_, nullptr) << "String库应该存在";
        ASSERT_NE(table_lib_, nullptr) << "Table库应该存在";
        ASSERT_NE(math_lib_, nullptr) << "Math库应该存在";
    }
    
    void TearDown() override {
        vm_.reset();
    }
    
    // 辅助方法：调用库函数并获取结果
    std::vector<LuaValue> CallFunction(LibraryModule* lib, const std::string& name, 
                                     const std::vector<LuaValue>& args = {}) {
        return lib->CallFunction(name, args);
    }
    
    // 辅助方法：验证单个返回值
    template<typename T>
    void ExpectSingleResult(const std::vector<LuaValue>& results, const T& expected) {
        ASSERT_EQ(results.size(), 1) << "应该返回一个结果";
        
        if constexpr (std::is_same_v<T, std::string>) {
            EXPECT_EQ(results[0].ToString(), expected);
        } else if constexpr (std::is_same_v<T, double>) {
            EXPECT_DOUBLE_EQ(results[0].ToNumber(), expected);
        } else if constexpr (std::is_same_v<T, int>) {
            EXPECT_EQ(results[0].ToNumber(), static_cast<double>(expected));
        } else if constexpr (std::is_same_v<T, bool>) {
            EXPECT_EQ(results[0].ToBoolean(), expected);
        }
    }
    
    // 辅助方法：创建字符串值
    LuaValue MakeString(const std::string& str) {
        return LuaValue(str);
    }
    
    // 辅助方法：创建数字值
    LuaValue MakeNumber(double num) {
        return LuaValue(num);
    }
    
    // 辅助方法：创建表值
    LuaValue MakeTable() {
        return LuaValue(std::make_shared<LuaTable>());
    }
    
protected:
    std::unique_ptr<EnhancedVirtualMachine> vm_;
    StandardLibrary* stdlib_;
    BaseLibrary* base_lib_;
    StringLibrary* string_lib_;
    TableLibrary* table_lib_;
    MathLibrary* math_lib_;
};

/* ========================================================================== */
/* Base库测试 */
/* ========================================================================== */

class BaseLibraryTest : public T027StandardLibraryTest {};

TEST_F(BaseLibraryTest, TypeFunction) {
    // 测试type函数对各种类型的识别
    auto result = CallFunction(base_lib_, "type", {LuaValue()});  // nil
    ExpectSingleResult(result, std::string("nil"));
    
    result = CallFunction(base_lib_, "type", {LuaValue(true)});
    ExpectSingleResult(result, std::string("boolean"));
    
    result = CallFunction(base_lib_, "type", {LuaValue(42.0)});
    ExpectSingleResult(result, std::string("number"));
    
    result = CallFunction(base_lib_, "type", {MakeString("hello")});
    ExpectSingleResult(result, std::string("string"));
    
    result = CallFunction(base_lib_, "type", {MakeTable()});
    ExpectSingleResult(result, std::string("table"));
}

TEST_F(BaseLibraryTest, ToStringFunction) {
    // 测试tostring函数
    auto result = CallFunction(base_lib_, "tostring", {LuaValue()});
    ExpectSingleResult(result, std::string("nil"));
    
    result = CallFunction(base_lib_, "tostring", {LuaValue(true)});
    ExpectSingleResult(result, std::string("true"));
    
    result = CallFunction(base_lib_, "tostring", {LuaValue(false)});
    ExpectSingleResult(result, std::string("false"));
    
    result = CallFunction(base_lib_, "tostring", {LuaValue(123.0)});
    ExpectSingleResult(result, std::string("123"));
    
    result = CallFunction(base_lib_, "tostring", {MakeString("test")});
    ExpectSingleResult(result, std::string("test"));
}

TEST_F(BaseLibraryTest, ToNumberFunction) {
    // 测试tonumber函数
    auto result = CallFunction(base_lib_, "tonumber", {MakeString("123")});
    ExpectSingleResult(result, 123.0);
    
    result = CallFunction(base_lib_, "tonumber", {MakeString("3.14")});
    ExpectSingleResult(result, 3.14);
    
    result = CallFunction(base_lib_, "tonumber", {MakeString("hello")});
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].IsNil());
    
    // 带进制的转换
    result = CallFunction(base_lib_, "tonumber", {MakeString("FF"), LuaValue(16.0)});
    ExpectSingleResult(result, 255.0);
    
    result = CallFunction(base_lib_, "tonumber", {MakeString("1010"), LuaValue(2.0)});
    ExpectSingleResult(result, 10.0);
}

TEST_F(BaseLibraryTest, RawgetRawsetFunctions) {
    // 创建测试表
    auto table = MakeTable();
    auto table_ptr = table.GetTable();
    
    // 使用rawset设置值
    auto result = CallFunction(base_lib_, "rawset", {table, MakeString("key"), MakeString("value")});
    EXPECT_EQ(result.size(), 1);
    
    // 使用rawget获取值
    result = CallFunction(base_lib_, "rawget", {table, MakeString("key")});
    ExpectSingleResult(result, std::string("value"));
    
    // 获取不存在的键
    result = CallFunction(base_lib_, "rawget", {table, MakeString("nonexistent")});
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].IsNil());
}

/* ========================================================================== */
/* String库测试 */
/* ========================================================================== */

class StringLibraryTest : public T027StandardLibraryTest {};

TEST_F(StringLibraryTest, LenFunction) {
    // 测试string.len函数
    auto result = CallFunction(string_lib_, "len", {MakeString("hello")});
    ExpectSingleResult(result, 5);
    
    result = CallFunction(string_lib_, "len", {MakeString("")});
    ExpectSingleResult(result, 0);
    
    result = CallFunction(string_lib_, "len", {MakeString("测试")});
    EXPECT_GT(result[0].ToNumber(), 2); // UTF-8编码长度大于字符数
}

TEST_F(StringLibraryTest, SubFunction) {
    // 测试string.sub函数
    auto result = CallFunction(string_lib_, "sub", {MakeString("hello"), LuaValue(2.0)});
    ExpectSingleResult(result, std::string("ello"));
    
    result = CallFunction(string_lib_, "sub", {MakeString("hello"), LuaValue(2.0), LuaValue(4.0)});
    ExpectSingleResult(result, std::string("ell"));
    
    // 负索引
    result = CallFunction(string_lib_, "sub", {MakeString("hello"), LuaValue(-2.0)});
    ExpectSingleResult(result, std::string("lo"));
    
    // 超出范围
    result = CallFunction(string_lib_, "sub", {MakeString("hello"), LuaValue(10.0)});
    ExpectSingleResult(result, std::string(""));
}

TEST_F(StringLibraryTest, UpperLowerFunctions) {
    // 测试string.upper和string.lower
    auto result = CallFunction(string_lib_, "upper", {MakeString("Hello World")});
    ExpectSingleResult(result, std::string("HELLO WORLD"));
    
    result = CallFunction(string_lib_, "lower", {MakeString("Hello World")});
    ExpectSingleResult(result, std::string("hello world"));
    
    result = CallFunction(string_lib_, "upper", {MakeString("")});
    ExpectSingleResult(result, std::string(""));
}

TEST_F(StringLibraryTest, FindFunction) {
    // 测试string.find函数
    auto result = CallFunction(string_lib_, "find", {MakeString("hello world"), MakeString("world")});
    ASSERT_GE(result.size(), 1);
    EXPECT_EQ(result[0].ToNumber(), 7.0); // "world"从位置7开始
    
    result = CallFunction(string_lib_, "find", {MakeString("hello world"), MakeString("foo")});
    ASSERT_EQ(result.size(), 1);
    EXPECT_TRUE(result[0].IsNil());
    
    // 带起始位置的查找
    result = CallFunction(string_lib_, "find", {MakeString("hello hello"), MakeString("hello"), LuaValue(2.0)});
    ASSERT_GE(result.size(), 1);
    EXPECT_EQ(result[0].ToNumber(), 7.0); // 第二个"hello"
}

TEST_F(StringLibraryTest, FormatFunction) {
    // 测试string.format函数
    auto result = CallFunction(string_lib_, "format", {MakeString("Hello %s"), MakeString("World")});
    ExpectSingleResult(result, std::string("Hello World"));
    
    result = CallFunction(string_lib_, "format", {MakeString("%d + %d = %d"), 
                                                 LuaValue(1.0), LuaValue(2.0), LuaValue(3.0)});
    ExpectSingleResult(result, std::string("1 + 2 = 3"));
    
    result = CallFunction(string_lib_, "format", {MakeString("%.2f"), LuaValue(3.14159)});
    ExpectSingleResult(result, std::string("3.14"));
}

/* ========================================================================== */
/* Table库测试 */
/* ========================================================================== */

class TableLibraryTest : public T027StandardLibraryTest {};

TEST_F(TableLibraryTest, InsertFunction) {
    // 创建测试表并添加初始元素
    auto table = MakeTable();
    auto table_ptr = table.GetTable();
    table_ptr->SetElement(1, LuaValue(10.0));
    table_ptr->SetElement(2, LuaValue(20.0));
    table_ptr->SetElement(3, LuaValue(30.0));
    
    // 在末尾插入
    auto result = CallFunction(table_lib_, "insert", {table, LuaValue(40.0)});
    EXPECT_EQ(table_ptr->GetArrayLength(), 4);
    EXPECT_EQ(table_ptr->GetElement(4).ToNumber(), 40.0);
    
    // 在指定位置插入
    result = CallFunction(table_lib_, "insert", {table, LuaValue(2.0), LuaValue(15.0)});
    EXPECT_EQ(table_ptr->GetArrayLength(), 5);
    EXPECT_EQ(table_ptr->GetElement(2).ToNumber(), 15.0);
    EXPECT_EQ(table_ptr->GetElement(3).ToNumber(), 20.0); // 原来位置2的元素被移动到位置3
}

TEST_F(TableLibraryTest, RemoveFunction) {
    // 创建测试表
    auto table = MakeTable();
    auto table_ptr = table.GetTable();
    table_ptr->SetElement(1, LuaValue(10.0));
    table_ptr->SetElement(2, LuaValue(20.0));
    table_ptr->SetElement(3, LuaValue(30.0));
    table_ptr->SetElement(4, LuaValue(40.0));
    
    // 移除末尾元素
    auto result = CallFunction(table_lib_, "remove", {table});
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].ToNumber(), 40.0);
    EXPECT_EQ(table_ptr->GetArrayLength(), 3);
    
    // 移除指定位置元素
    result = CallFunction(table_lib_, "remove", {table, LuaValue(2.0)});
    ASSERT_EQ(result.size(), 1);
    EXPECT_EQ(result[0].ToNumber(), 20.0);
    EXPECT_EQ(table_ptr->GetArrayLength(), 2);
    EXPECT_EQ(table_ptr->GetElement(2).ToNumber(), 30.0); // 原来位置3的元素移动到位置2
}

TEST_F(TableLibraryTest, ConcatFunction) {
    // 创建字符串数组
    auto table = MakeTable();
    auto table_ptr = table.GetTable();
    table_ptr->SetElement(1, MakeString("hello"));
    table_ptr->SetElement(2, MakeString("world"));
    table_ptr->SetElement(3, MakeString("test"));
    
    // 无分隔符连接
    auto result = CallFunction(table_lib_, "concat", {table});
    ExpectSingleResult(result, std::string("helloworldtest"));
    
    // 带分隔符连接
    result = CallFunction(table_lib_, "concat", {table, MakeString(" ")});
    ExpectSingleResult(result, std::string("hello world test"));
    
    // 指定范围连接
    result = CallFunction(table_lib_, "concat", {table, MakeString("-"), LuaValue(1.0), LuaValue(2.0)});
    ExpectSingleResult(result, std::string("hello-world"));
}

TEST_F(TableLibraryTest, SortFunction) {
    // 创建数字数组
    auto table = MakeTable();
    auto table_ptr = table.GetTable();
    table_ptr->SetElement(1, LuaValue(3.0));
    table_ptr->SetElement(2, LuaValue(1.0));
    table_ptr->SetElement(3, LuaValue(4.0));
    table_ptr->SetElement(4, LuaValue(2.0));
    
    // 默认升序排序
    auto result = CallFunction(table_lib_, "sort", {table});
    
    // 验证排序结果
    EXPECT_EQ(table_ptr->GetElement(1).ToNumber(), 1.0);
    EXPECT_EQ(table_ptr->GetElement(2).ToNumber(), 2.0);
    EXPECT_EQ(table_ptr->GetElement(3).ToNumber(), 3.0);
    EXPECT_EQ(table_ptr->GetElement(4).ToNumber(), 4.0);
}

/* ========================================================================== */
/* Math库测试 */
/* ========================================================================== */

class MathLibraryTest : public T027StandardLibraryTest {};

TEST_F(MathLibraryTest, BasicMathFunctions) {
    // 测试基础数学函数
    auto result = CallFunction(math_lib_, "abs", {LuaValue(-5.0)});
    ExpectSingleResult(result, 5.0);
    
    result = CallFunction(math_lib_, "abs", {LuaValue(5.0)});
    ExpectSingleResult(result, 5.0);
    
    result = CallFunction(math_lib_, "floor", {LuaValue(3.7)});
    ExpectSingleResult(result, 3.0);
    
    result = CallFunction(math_lib_, "floor", {LuaValue(-3.7)});
    ExpectSingleResult(result, -4.0);
    
    result = CallFunction(math_lib_, "ceil", {LuaValue(3.2)});
    ExpectSingleResult(result, 4.0);
    
    result = CallFunction(math_lib_, "ceil", {LuaValue(-3.2)});
    ExpectSingleResult(result, -3.0);
}

TEST_F(MathLibraryTest, MinMaxFunctions) {
    // 测试min/max函数
    auto result = CallFunction(math_lib_, "min", {LuaValue(1.0), LuaValue(3.0), LuaValue(2.0)});
    ExpectSingleResult(result, 1.0);
    
    result = CallFunction(math_lib_, "max", {LuaValue(1.0), LuaValue(3.0), LuaValue(2.0)});
    ExpectSingleResult(result, 3.0);
    
    // 单个参数
    result = CallFunction(math_lib_, "min", {LuaValue(42.0)});
    ExpectSingleResult(result, 42.0);
}

TEST_F(MathLibraryTest, PowerAndRootFunctions) {
    // 测试幂和根函数
    auto result = CallFunction(math_lib_, "pow", {LuaValue(2.0), LuaValue(3.0)});
    ExpectSingleResult(result, 8.0);
    
    result = CallFunction(math_lib_, "sqrt", {LuaValue(16.0)});
    ExpectSingleResult(result, 4.0);
    
    result = CallFunction(math_lib_, "sqrt", {LuaValue(2.0)});
    ASSERT_EQ(result.size(), 1);
    EXPECT_NEAR(result[0].ToNumber(), std::sqrt(2.0), 1e-10);
}

TEST_F(MathLibraryTest, TrigonometricFunctions) {
    const double PI = 3.14159265358979323846;
    
    // 测试三角函数
    auto result = CallFunction(math_lib_, "sin", {LuaValue(0.0)});
    EXPECT_NEAR(result[0].ToNumber(), 0.0, 1e-10);
    
    result = CallFunction(math_lib_, "cos", {LuaValue(0.0)});
    EXPECT_NEAR(result[0].ToNumber(), 1.0, 1e-10);
    
    result = CallFunction(math_lib_, "sin", {LuaValue(PI / 2)});
    EXPECT_NEAR(result[0].ToNumber(), 1.0, 1e-10);
    
    result = CallFunction(math_lib_, "cos", {LuaValue(PI)});
    EXPECT_NEAR(result[0].ToNumber(), -1.0, 1e-10);
}

TEST_F(MathLibraryTest, LogarithmicFunctions) {
    // 测试对数函数
    auto result = CallFunction(math_lib_, "log", {LuaValue(std::exp(1.0))});
    EXPECT_NEAR(result[0].ToNumber(), 1.0, 1e-10);
    
    result = CallFunction(math_lib_, "log10", {LuaValue(100.0)});
    EXPECT_NEAR(result[0].ToNumber(), 2.0, 1e-10);
    
    result = CallFunction(math_lib_, "exp", {LuaValue(0.0)});
    EXPECT_NEAR(result[0].ToNumber(), 1.0, 1e-10);
}

TEST_F(MathLibraryTest, RandomFunctions) {
    // 测试随机数函数
    
    // 设置种子并测试可重现性
    auto result = CallFunction(math_lib_, "randomseed", {LuaValue(12345.0)});
    auto r1 = CallFunction(math_lib_, "random", {});
    
    CallFunction(math_lib_, "randomseed", {LuaValue(12345.0)});
    auto r2 = CallFunction(math_lib_, "random", {});
    
    EXPECT_EQ(r1[0].ToNumber(), r2[0].ToNumber());
    
    // 测试范围随机数
    result = CallFunction(math_lib_, "random", {LuaValue(1.0), LuaValue(10.0)});
    ASSERT_EQ(result.size(), 1);
    double rand_val = result[0].ToNumber();
    EXPECT_GE(rand_val, 1.0);
    EXPECT_LE(rand_val, 10.0);
    
    result = CallFunction(math_lib_, "random", {LuaValue(5.0)});
    ASSERT_EQ(result.size(), 1);
    rand_val = result[0].ToNumber();
    EXPECT_GE(rand_val, 1.0);
    EXPECT_LE(rand_val, 5.0);
}

/* ========================================================================== */
/* 集成测试 */
/* ========================================================================== */

class StandardLibraryIntegrationTest : public T027StandardLibraryTest {};

TEST_F(StandardLibraryIntegrationTest, VirtualMachineIntegration) {
    // 测试虚拟机集成
    EXPECT_TRUE(vm_->IsT026Enabled());
    EXPECT_NE(vm_->GetStandardLibrary(), nullptr);
    
    // 测试T026兼容性
    vm_->SwitchToLegacyMode();
    EXPECT_NE(vm_->GetStandardLibrary(), nullptr); // 标准库在传统模式下仍可用
    
    vm_->SwitchToEnhancedMode();
    EXPECT_TRUE(vm_->IsT026Enabled());
}

TEST_F(StandardLibraryIntegrationTest, CrossLibraryOperations) {
    // 跨库操作测试：使用多个库协作完成复杂任务
    
    // 创建数字字符串数组并排序
    auto table = MakeTable();
    auto table_ptr = table.GetTable();
    
    // 使用string.format创建格式化数字
    auto num1 = CallFunction(string_lib_, "format", {MakeString("%.1f"), LuaValue(3.7)});
    auto num2 = CallFunction(string_lib_, "format", {MakeString("%.1f"), LuaValue(1.2)});
    auto num3 = CallFunction(string_lib_, "format", {MakeString("%.1f"), LuaValue(2.8)});
    
    table_ptr->SetElement(1, num1[0]);
    table_ptr->SetElement(2, num2[0]);
    table_ptr->SetElement(3, num3[0]);
    
    // 使用table.concat连接字符串
    auto result = CallFunction(table_lib_, "concat", {table, MakeString(", ")});
    
    // 验证结果包含所有数字
    std::string concat_result = result[0].ToString();
    EXPECT_NE(concat_result.find("3.7"), std::string::npos);
    EXPECT_NE(concat_result.find("1.2"), std::string::npos);
    EXPECT_NE(concat_result.find("2.8"), std::string::npos);
}

TEST_F(StandardLibraryIntegrationTest, PerformanceTest) {
    // 简单性能测试：执行大量标准库调用
    const int iterations = 1000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i) {
        // 混合使用各种库函数
        CallFunction(math_lib_, "sin", {LuaValue(i * 0.01)});
        CallFunction(string_lib_, "format", {MakeString("%d"), LuaValue(i)});
        CallFunction(base_lib_, "type", {LuaValue(i)});
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // 性能应该在合理范围内（这里设置一个宽松的限制）
    EXPECT_LT(duration.count(), 1000) << "1000次标准库调用应该在1秒内完成";
}

/* ========================================================================== */
/* 主测试运行器 */
/* ========================================================================== */

} // namespace test
} // namespace lua_cpp

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "=== T027标准库单元测试开始 ===" << std::endl;
    std::cout << "测试目标: EnhancedVirtualMachine + 标准库集成" << std::endl;
    std::cout << "测试覆盖: Base, String, Table, Math 四个库模块" << std::endl;
    std::cout << std::endl;
    
    int result = RUN_ALL_TESTS();
    
    if (result == 0) {
        std::cout << std::endl;
        std::cout << "🎉 T027标准库单元测试全部通过！" << std::endl;
        std::cout << "✅ 所有库模块功能正常" << std::endl;
        std::cout << "✅ VM集成成功" << std::endl;
        std::cout << "✅ 跨库操作正常" << std::endl;
        std::cout << "✅ 性能表现良好" << std::endl;
    } else {
        std::cout << std::endl;
        std::cout << "❌ T027标准库测试发现问题" << std::endl;
        std::cout << "请检查失败的测试用例并修复相关代码" << std::endl;
    }
    
    return result;
}