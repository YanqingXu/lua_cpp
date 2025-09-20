/**
 * @file test_tvalue_contract.cpp
 * @brief TValue（Lua值表示）契约测试
 * @description 测试Lua值的所有行为契约，确保100% Lua 5.1.5兼容性
 *              包括类型判断、值存储、类型转换、比较操作等核心功能
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "types/tvalue.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* TValue基础构造和类型检查契约 */
/* ========================================================================== */

TEST_CASE("TValue - 基础构造契约", "[tvalue][contract][core]") {
    SECTION("默认构造应该创建nil值") {
        TValue value;
        REQUIRE(value.GetType() == LuaType::Nil);
        REQUIRE(value.IsNil());
        REQUIRE_FALSE(value.IsBoolean());
        REQUIRE_FALSE(value.IsNumber());
        REQUIRE_FALSE(value.IsString());
        REQUIRE_FALSE(value.IsTable());
        REQUIRE_FALSE(value.IsFunction());
        REQUIRE_FALSE(value.IsUserdata());
        REQUIRE_FALSE(value.IsThread());
        REQUIRE_FALSE(value.IsLightUserdata());
    }

    SECTION("nil值构造") {
        TValue value = TValue::CreateNil();
        REQUIRE(value.GetType() == LuaType::Nil);
        REQUIRE(value.IsNil());
    }

    SECTION("boolean值构造") {
        TValue trueValue = TValue::CreateBoolean(true);
        TValue falseValue = TValue::CreateBoolean(false);
        
        REQUIRE(trueValue.GetType() == LuaType::Boolean);
        REQUIRE(falseValue.GetType() == LuaType::Boolean);
        REQUIRE(trueValue.IsBoolean());
        REQUIRE(falseValue.IsBoolean());
        REQUIRE(trueValue.GetBoolean() == true);
        REQUIRE(falseValue.GetBoolean() == false);
    }

    SECTION("number值构造") {
        TValue intValue = TValue::CreateNumber(42.0);
        TValue floatValue = TValue::CreateNumber(3.14159);
        TValue negativeValue = TValue::CreateNumber(-123.456);
        
        REQUIRE(intValue.GetType() == LuaType::Number);
        REQUIRE(floatValue.GetType() == LuaType::Number);
        REQUIRE(negativeValue.GetType() == LuaType::Number);
        
        REQUIRE(intValue.IsNumber());
        REQUIRE(floatValue.IsNumber());
        REQUIRE(negativeValue.IsNumber());
        
        REQUIRE(intValue.GetNumber() == Approx(42.0));
        REQUIRE(floatValue.GetNumber() == Approx(3.14159));
        REQUIRE(negativeValue.GetNumber() == Approx(-123.456));
    }

    SECTION("特殊number值") {
        TValue infValue = TValue::CreateNumber(std::numeric_limits<double>::infinity());
        TValue negInfValue = TValue::CreateNumber(-std::numeric_limits<double>::infinity());
        TValue nanValue = TValue::CreateNumber(std::numeric_limits<double>::quiet_NaN());
        
        REQUIRE(infValue.IsNumber());
        REQUIRE(negInfValue.IsNumber());
        REQUIRE(nanValue.IsNumber());
        
        REQUIRE(std::isinf(infValue.GetNumber()));
        REQUIRE(std::isinf(negInfValue.GetNumber()));
        REQUIRE(std::isnan(nanValue.GetNumber()));
    }
}

/* ========================================================================== */
/* 类型转换和强制转换契约 */
/* ========================================================================== */

TEST_CASE("TValue - 类型转换契约", "[tvalue][contract][conversion]") {
    SECTION("ToBoolean转换 - Lua真值语义") {
        // Lua中只有nil和false为假，其他都为真
        REQUIRE_FALSE(TValue::CreateNil().ToBoolean());
        REQUIRE_FALSE(TValue::CreateBoolean(false).ToBoolean());
        
        REQUIRE(TValue::CreateBoolean(true).ToBoolean());
        REQUIRE(TValue::CreateNumber(0.0).ToBoolean());  // Lua中0为真
        REQUIRE(TValue::CreateNumber(42.0).ToBoolean());
        REQUIRE(TValue::CreateNumber(-1.0).ToBoolean());
        
        // 注意：字符串、表等对象需要在实现时确保为真
        // REQUIRE(TValue::CreateString("").ToBoolean());    // 空字符串也为真
        // REQUIRE(TValue::CreateString("false").ToBoolean()); // 字符串"false"也为真
    }

    SECTION("ToNumber转换") {
        // boolean到number
        REQUIRE(TValue::CreateBoolean(true).ToNumber() == Approx(1.0));
        REQUIRE(TValue::CreateBoolean(false).ToNumber() == Approx(0.0));
        
        // number到number（无变化）
        REQUIRE(TValue::CreateNumber(42.5).ToNumber() == Approx(42.5));
        
        // nil和其他类型到number应该失败或返回0（根据Lua 5.1.5语义）
        REQUIRE_THROWS_AS(TValue::CreateNil().ToNumber(), TypeError);
    }

    SECTION("ToString转换") {
        // number到string
        TValue num42 = TValue::CreateNumber(42.0);
        TValue numPi = TValue::CreateNumber(3.14159);
        
        REQUIRE(num42.ToString() == "42");
        REQUIRE(numPi.ToString().substr(0, 4) == "3.14"); // 精度可能有变化
        
        // boolean到string
        REQUIRE(TValue::CreateBoolean(true).ToString() == "true");
        REQUIRE(TValue::CreateBoolean(false).ToString() == "false");
        
        // nil到string
        REQUIRE(TValue::CreateNil().ToString() == "nil");
    }

    SECTION("TryToNumber - 字符串到数字转换") {
        // 这些测试需要在string实现后添加
        // 预留接口设计验证
        /*
        double result;
        REQUIRE(TValue::CreateString("42").TryToNumber(result));
        REQUIRE(result == Approx(42.0));
        
        REQUIRE(TValue::CreateString("3.14").TryToNumber(result));
        REQUIRE(result == Approx(3.14));
        
        REQUIRE_FALSE(TValue::CreateString("hello").TryToNumber(result));
        REQUIRE_FALSE(TValue::CreateString("").TryToNumber(result));
        */
    }
}

/* ========================================================================== */
/* 相等性和比较契约 */
/* ========================================================================== */

TEST_CASE("TValue - 相等性比较契约", "[tvalue][contract][equality]") {
    SECTION("相同类型相等性") {
        // nil相等性
        REQUIRE(TValue::CreateNil() == TValue::CreateNil());
        
        // boolean相等性
        REQUIRE(TValue::CreateBoolean(true) == TValue::CreateBoolean(true));
        REQUIRE(TValue::CreateBoolean(false) == TValue::CreateBoolean(false));
        REQUIRE_FALSE(TValue::CreateBoolean(true) == TValue::CreateBoolean(false));
        
        // number相等性
        REQUIRE(TValue::CreateNumber(42.0) == TValue::CreateNumber(42.0));
        REQUIRE(TValue::CreateNumber(0.0) == TValue::CreateNumber(0.0));
        REQUIRE_FALSE(TValue::CreateNumber(42.0) == TValue::CreateNumber(43.0));
        
        // 特殊number值
        auto nan1 = TValue::CreateNumber(std::numeric_limits<double>::quiet_NaN());
        auto nan2 = TValue::CreateNumber(std::numeric_limits<double>::quiet_NaN());
        REQUIRE_FALSE(nan1 == nan2); // NaN != NaN in Lua
        
        auto inf1 = TValue::CreateNumber(std::numeric_limits<double>::infinity());
        auto inf2 = TValue::CreateNumber(std::numeric_limits<double>::infinity());
        REQUIRE(inf1 == inf2); // Inf == Inf
    }

    SECTION("不同类型相等性") {
        // 不同类型永远不相等（Lua 5.1.5语义）
        REQUIRE_FALSE(TValue::CreateNil() == TValue::CreateBoolean(false));
        REQUIRE_FALSE(TValue::CreateBoolean(true) == TValue::CreateNumber(1.0));
        REQUIRE_FALSE(TValue::CreateNumber(0.0) == TValue::CreateBoolean(false));
        
        // 即使"语义上相同"也不相等
        REQUIRE_FALSE(TValue::CreateNumber(1.0) == TValue::CreateBoolean(true));
        REQUIRE_FALSE(TValue::CreateNumber(0.0) == TValue::CreateNil());
    }

    SECTION("不等性操作符") {
        REQUIRE_FALSE(TValue::CreateNil() != TValue::CreateNil());
        REQUIRE(TValue::CreateBoolean(true) != TValue::CreateBoolean(false));
        REQUIRE(TValue::CreateNumber(1.0) != TValue::CreateNumber(2.0));
        REQUIRE(TValue::CreateNil() != TValue::CreateNumber(0.0));
    }
}

TEST_CASE("TValue - 大小比较契约", "[tvalue][contract][comparison]") {
    SECTION("number比较") {
        TValue num1 = TValue::CreateNumber(1.0);
        TValue num2 = TValue::CreateNumber(2.0);
        TValue num3 = TValue::CreateNumber(1.0);
        
        REQUIRE(num1 < num2);
        REQUIRE(num2 > num1);
        REQUIRE(num1 <= num2);
        REQUIRE(num2 >= num1);
        REQUIRE(num1 <= num3);
        REQUIRE(num1 >= num3);
        REQUIRE_FALSE(num1 > num2);
        REQUIRE_FALSE(num2 < num1);
    }

    SECTION("特殊number比较") {
        auto inf = TValue::CreateNumber(std::numeric_limits<double>::infinity());
        auto negInf = TValue::CreateNumber(-std::numeric_limits<double>::infinity());
        auto nan = TValue::CreateNumber(std::numeric_limits<double>::quiet_NaN());
        auto normal = TValue::CreateNumber(42.0);
        
        REQUIRE(negInf < normal);
        REQUIRE(normal < inf);
        REQUIRE(negInf < inf);
        
        // NaN比较总是false
        REQUIRE_FALSE(nan < normal);
        REQUIRE_FALSE(normal < nan);
        REQUIRE_FALSE(nan == nan);
        REQUIRE_FALSE(nan <= normal);
        REQUIRE_FALSE(nan >= normal);
    }

    SECTION("string比较（预留）") {
        // 字符串按字典序比较 - 需要string实现
        /*
        TValue str1 = TValue::CreateString("abc");
        TValue str2 = TValue::CreateString("def");
        TValue str3 = TValue::CreateString("abc");
        
        REQUIRE(str1 < str2);
        REQUIRE(str2 > str1);
        REQUIRE(str1 <= str3);
        REQUIRE(str1 >= str3);
        */
    }

    SECTION("不同类型比较应该抛出异常") {
        TValue num = TValue::CreateNumber(42.0);
        TValue boolean = TValue::CreateBoolean(true);
        TValue nil = TValue::CreateNil();
        
        REQUIRE_THROWS_AS(num < boolean, TypeError);
        REQUIRE_THROWS_AS(boolean > nil, TypeError);
        REQUIRE_THROWS_AS(nil <= num, TypeError);
        // 注意：某些实现可能选择返回false而不是抛出异常
        // 这需要根据具体的Lua 5.1.5行为来决定
    }
}

/* ========================================================================== */
/* 内存和垃圾回收契约 */
/* ========================================================================== */

TEST_CASE("TValue - 内存管理契约", "[tvalue][contract][memory]") {
    SECTION("值类型内存模型") {
        TValue value1 = TValue::CreateNumber(42.0);
        TValue value2 = value1; // 拷贝构造
        
        // 对于基础类型，应该是值语义
        REQUIRE(value1 == value2);
        REQUIRE(value1.GetNumber() == value2.GetNumber());
        
        // 修改一个不应影响另一个（对于基础类型）
        value2 = TValue::CreateNumber(100.0);
        REQUIRE(value1.GetNumber() == Approx(42.0));
        REQUIRE(value2.GetNumber() == Approx(100.0));
    }

    SECTION("赋值语义") {
        TValue value1 = TValue::CreateBoolean(true);
        TValue value2 = TValue::CreateNil();
        
        value2 = value1; // 赋值
        REQUIRE(value1 == value2);
        REQUIRE(value1.IsBoolean());
        REQUIRE(value2.IsBoolean());
        REQUIRE(value1.GetBoolean() == value2.GetBoolean());
    }

    SECTION("移动语义（C++11+）") {
        TValue value1 = TValue::CreateNumber(42.0);
        TValue value2 = std::move(value1);
        
        REQUIRE(value2.IsNumber());
        REQUIRE(value2.GetNumber() == Approx(42.0));
        // value1的状态在移动后是未定义的，但应该是安全的
    }

    SECTION("GC标记接口（预留）") {
        // TValue需要支持GC标记，但具体实现依赖于GC系统
        /*
        TValue value = TValue::CreateNumber(42.0);
        
        // 基础类型不需要GC
        REQUIRE_FALSE(value.NeedsGC());
        REQUIRE(value.GetGCColor() == GCColor::White0); // 或其他合适的默认值
        
        // 引用类型需要GC（在实现时验证）
        // TValue table = TValue::CreateTable(...);
        // REQUIRE(table.NeedsGC());
        */
    }
}

/* ========================================================================== */
/* 类型安全和错误处理契约 */
/* ========================================================================== */

TEST_CASE("TValue - 类型安全契约", "[tvalue][contract][safety]") {
    SECTION("类型检查getter") {
        TValue nilValue = TValue::CreateNil();
        TValue boolValue = TValue::CreateBoolean(true);
        TValue numValue = TValue::CreateNumber(42.0);
        
        // 正确类型访问
        REQUIRE_NOTHROW(boolValue.GetBoolean());
        REQUIRE_NOTHROW(numValue.GetNumber());
        
        // 错误类型访问应该抛出异常
        REQUIRE_THROWS_AS(nilValue.GetBoolean(), TypeError);
        REQUIRE_THROWS_AS(nilValue.GetNumber(), TypeError);
        REQUIRE_THROWS_AS(boolValue.GetNumber(), TypeError);
        REQUIRE_THROWS_AS(numValue.GetBoolean(), TypeError);
    }

    SECTION("安全getter（不抛出异常）") {
        TValue nilValue = TValue::CreateNil();
        TValue boolValue = TValue::CreateBoolean(true);
        TValue numValue = TValue::CreateNumber(42.0);
        
        bool boolResult;
        double numResult;
        
        // 成功的安全访问
        REQUIRE(boolValue.TryGetBoolean(boolResult));
        REQUIRE(boolResult == true);
        
        REQUIRE(numValue.TryGetNumber(numResult));
        REQUIRE(numResult == Approx(42.0));
        
        // 失败的安全访问
        REQUIRE_FALSE(nilValue.TryGetBoolean(boolResult));
        REQUIRE_FALSE(nilValue.TryGetNumber(numResult));
        REQUIRE_FALSE(boolValue.TryGetNumber(numResult));
        REQUIRE_FALSE(numValue.TryGetBoolean(boolResult));
    }

    SECTION("类型转换异常处理") {
        TValue nilValue = TValue::CreateNil();
        
        // 无法转换的类型应该抛出适当的异常
        REQUIRE_THROWS_AS(nilValue.ToNumber(), TypeError);
        
        // 异常应该包含有用的信息
        try {
            nilValue.ToNumber();
            FAIL("Expected TypeError");
        } catch (const TypeError& e) {
            REQUIRE(e.GetExpectedType() == LuaType::Number);
            REQUIRE(e.GetActualType() == LuaType::Nil);
            REQUIRE_FALSE(std::string(e.what()).empty());
        }
    }
}

/* ========================================================================== */
/* 特殊值和边界情况契约 */
/* ========================================================================== */

TEST_CASE("TValue - 边界情况契约", "[tvalue][contract][edge-cases]") {
    SECTION("数值精度和范围") {
        // 测试极大和极小值
        double maxValue = std::numeric_limits<double>::max();
        double minValue = std::numeric_limits<double>::lowest();
        double epsilon = std::numeric_limits<double>::epsilon();
        
        TValue maxVal = TValue::CreateNumber(maxValue);
        TValue minVal = TValue::CreateNumber(minValue);
        TValue epsVal = TValue::CreateNumber(epsilon);
        
        REQUIRE(maxVal.GetNumber() == Approx(maxValue));
        REQUIRE(minVal.GetNumber() == Approx(minValue));
        REQUIRE(epsVal.GetNumber() == Approx(epsilon));
    }

    SECTION("零值处理") {
        TValue posZero = TValue::CreateNumber(0.0);
        TValue negZero = TValue::CreateNumber(-0.0);
        
        // IEEE 754: +0.0 == -0.0
        REQUIRE(posZero == negZero);
        
        // 但在某些操作中可能需要区分
        REQUIRE(std::signbit(posZero.GetNumber()) == false);
        REQUIRE(std::signbit(negZero.GetNumber()) == true);
    }

    SECTION("布尔值边界") {
        // 确保布尔值只能是true或false
        TValue trueVal = TValue::CreateBoolean(true);
        TValue falseVal = TValue::CreateBoolean(false);
        
        REQUIRE(trueVal.GetBoolean() == true);
        REQUIRE(falseVal.GetBoolean() == false);
        
        // 内部表示应该是标准的
        REQUIRE(trueVal != falseVal);
    }

    SECTION("类型枚举完整性") {
        // 确保所有Lua类型都能被正确识别
        auto validTypes = {
            LuaType::Nil, LuaType::Boolean, LuaType::Number,
            LuaType::String, LuaType::Table, LuaType::Function,
            LuaType::Userdata, LuaType::Thread, LuaType::LightUserdata
        };
        
        for (auto type : validTypes) {
            REQUIRE(IsValidLuaType(type));
            REQUIRE_FALSE(GetLuaTypeName(type).empty());
        }
        
        // 无效类型应该被检测出来
        REQUIRE_FALSE(IsValidLuaType(static_cast<LuaType>(255)));
    }
}

/* ========================================================================== */
/* 性能和效率契约 */
/* ========================================================================== */

TEST_CASE("TValue - 性能契约", "[tvalue][contract][performance]") {
    SECTION("基本操作性能要求") {
        // 这些是性能基准，实际值可能需要调整
        constexpr int iterations = 100000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 大量创建和销毁基本类型值
        for (int i = 0; i < iterations; ++i) {
            TValue val = TValue::CreateNumber(static_cast<double>(i));
            volatile double result = val.GetNumber(); // 防止优化
            (void)result;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 基本操作应该很快（这个阈值可能需要调整）
        REQUIRE(duration.count() < 10000); // 少于10ms
    }

    SECTION("内存使用效率") {
        // TValue应该尽可能紧凑
        REQUIRE(sizeof(TValue) <= 16); // 期望不超过16字节
        
        // 对齐要求
        REQUIRE(alignof(TValue) <= 8); // 期望不超过8字节对齐
    }

    SECTION("拷贝性能") {
        TValue original = TValue::CreateNumber(42.0);
        constexpr int copies = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < copies; ++i) {
            TValue copy = original; // 拷贝构造
            volatile bool result = copy.IsNumber(); // 防止优化
            (void)result;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 拷贝应该很快
        REQUIRE(duration.count() < 1000); // 少于1ms
    }
}

/* ========================================================================== */
/* Lua 5.1.5兼容性契约 */
/* ========================================================================== */

TEST_CASE("TValue - Lua 5.1.5兼容性契约", "[tvalue][contract][compatibility]") {
    SECTION("类型系统兼容性") {
        // 验证类型枚举与Lua 5.1.5一致
        REQUIRE(static_cast<int>(LuaType::Nil) == 0);
        REQUIRE(static_cast<int>(LuaType::Boolean) == 1);
        REQUIRE(static_cast<int>(LuaType::LightUserdata) == 2);
        REQUIRE(static_cast<int>(LuaType::Number) == 3);
        REQUIRE(static_cast<int>(LuaType::String) == 4);
        REQUIRE(static_cast<int>(LuaType::Table) == 5);
        REQUIRE(static_cast<int>(LuaType::Function) == 6);
        REQUIRE(static_cast<int>(LuaType::Userdata) == 7);
        REQUIRE(static_cast<int>(LuaType::Thread) == 8);
    }

    SECTION("数值表示兼容性") {
        // Lua 5.1.5使用double表示数值
        TValue val = TValue::CreateNumber(3.14159265359);
        REQUIRE(sizeof(val.GetNumber()) == sizeof(double));
        REQUIRE(val.GetNumber() == Approx(3.14159265359));
    }

    SECTION("真值判断兼容性") {
        // Lua 5.1.5真值语义：只有nil和false为假
        REQUIRE_FALSE(TValue::CreateNil().ToBoolean());
        REQUIRE_FALSE(TValue::CreateBoolean(false).ToBoolean());
        
        // 其他所有值都为真
        REQUIRE(TValue::CreateBoolean(true).ToBoolean());
        REQUIRE(TValue::CreateNumber(0.0).ToBoolean());    // 重要：0为真
        REQUIRE(TValue::CreateNumber(-1.0).ToBoolean());
        REQUIRE(TValue::CreateNumber(std::numeric_limits<double>::quiet_NaN()).ToBoolean()); // NaN为真
    }

    SECTION("比较操作兼容性") {
        // Lua 5.1.5比较语义
        TValue num1 = TValue::CreateNumber(1.0);
        TValue num2 = TValue::CreateNumber(2.0);
        
        REQUIRE(num1 < num2);
        REQUIRE_FALSE(num2 < num1);
        
        // 不同类型比较应该有明确的行为
        TValue boolean = TValue::CreateBoolean(true);
        TValue nil = TValue::CreateNil();
        
        // Lua 5.1.5: 不同类型比较结果定义明确
        REQUIRE_THROWS_AS(num1 < boolean, TypeError); // 或根据实际Lua行为调整
        REQUIRE_THROWS_AS(nil < boolean, TypeError);
    }
}

/* ========================================================================== */
/* 调试和诊断契约 */
/* ========================================================================== */

TEST_CASE("TValue - 调试支持契约", "[tvalue][contract][debug]") {
    SECTION("字符串表示") {
        REQUIRE(TValue::CreateNil().ToString() == "nil");
        REQUIRE(TValue::CreateBoolean(true).ToString() == "true");
        REQUIRE(TValue::CreateBoolean(false).ToString() == "false");
        REQUIRE(TValue::CreateNumber(42.0).ToString() == "42");
        REQUIRE(TValue::CreateNumber(3.14).ToString().substr(0, 4) == "3.14");
    }

    SECTION("类型名称") {
        REQUIRE(GetLuaTypeName(LuaType::Nil) == "nil");
        REQUIRE(GetLuaTypeName(LuaType::Boolean) == "boolean");
        REQUIRE(GetLuaTypeName(LuaType::Number) == "number");
        REQUIRE(GetLuaTypeName(LuaType::String) == "string");
        REQUIRE(GetLuaTypeName(LuaType::Table) == "table");
        REQUIRE(GetLuaTypeName(LuaType::Function) == "function");
        REQUIRE(GetLuaTypeName(LuaType::Userdata) == "userdata");
        REQUIRE(GetLuaTypeName(LuaType::Thread) == "thread");
        REQUIRE(GetLuaTypeName(LuaType::LightUserdata) == "userdata");
    }

    SECTION("错误消息质量") {
        TValue nilValue = TValue::CreateNil();
        
        try {
            nilValue.GetNumber();
            FAIL("Expected TypeError");
        } catch (const TypeError& e) {
            std::string message = e.what();
            
            // 错误消息应该包含有用信息
            REQUIRE(message.find("number") != std::string::npos);
            REQUIRE(message.find("nil") != std::string::npos);
            REQUIRE_FALSE(message.empty());
        }
    }
}

/* ========================================================================== */
/* 接口完整性验证 */
/* ========================================================================== */

TEST_CASE("TValue - 接口完整性", "[tvalue][contract][interface]") {
    SECTION("必需的静态方法") {
        // 验证所有创建方法都存在且可调用
        REQUIRE_NOTHROW(TValue::CreateNil());
        REQUIRE_NOTHROW(TValue::CreateBoolean(true));
        REQUIRE_NOTHROW(TValue::CreateNumber(42.0));
        
        // 预留：其他类型的创建方法
        /*
        REQUIRE_NOTHROW(TValue::CreateString("hello"));
        REQUIRE_NOTHROW(TValue::CreateTable());
        REQUIRE_NOTHROW(TValue::CreateLightUserdata(nullptr));
        */
    }

    SECTION("必需的实例方法") {
        TValue value = TValue::CreateNumber(42.0);
        
        // 类型检查方法
        REQUIRE_NOTHROW(value.GetType());
        REQUIRE_NOTHROW(value.IsNil());
        REQUIRE_NOTHROW(value.IsBoolean());
        REQUIRE_NOTHROW(value.IsNumber());
        REQUIRE_NOTHROW(value.IsString());
        REQUIRE_NOTHROW(value.IsTable());
        REQUIRE_NOTHROW(value.IsFunction());
        REQUIRE_NOTHROW(value.IsUserdata());
        REQUIRE_NOTHROW(value.IsThread());
        REQUIRE_NOTHROW(value.IsLightUserdata());
        
        // 值访问方法
        REQUIRE_NOTHROW(value.GetNumber());
        REQUIRE_THROWS(value.GetBoolean()); // 错误类型应该抛出异常
        
        // 转换方法
        REQUIRE_NOTHROW(value.ToBoolean());
        REQUIRE_NOTHROW(value.ToString());
        
        // 安全访问方法
        double numResult;
        bool boolResult;
        REQUIRE_NOTHROW(value.TryGetNumber(numResult));
        REQUIRE_NOTHROW(value.TryGetBoolean(boolResult));
    }

    SECTION("操作符重载完整性") {
        TValue val1 = TValue::CreateNumber(1.0);
        TValue val2 = TValue::CreateNumber(2.0);
        TValue val3 = TValue::CreateBoolean(true);
        
        // 比较操作符
        REQUIRE_NOTHROW(val1 == val2);
        REQUIRE_NOTHROW(val1 != val2);
        REQUIRE_NOTHROW(val1 < val2);
        REQUIRE_NOTHROW(val1 <= val2);
        REQUIRE_NOTHROW(val1 > val2);
        REQUIRE_NOTHROW(val1 >= val2);
        
        // 不同类型比较的错误处理
        REQUIRE_THROWS_AS(val1 < val3, TypeError);
    }
}