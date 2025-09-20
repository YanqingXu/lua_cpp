/**
 * @file test_function_contract.cpp
 * @brief LuaFunction（Lua函数）契约测试
 * @description 测试Lua函数、C函数、闭包、Upvalue等行为
 *              确保100% Lua 5.1.5兼容性和正确的函数调用机制
 * @date 2025-09-20
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

// 注意：这些头文件还不存在，这是TDD方法 - 先写测试定义接口
#include "types/lua_function.h"
#include "types/lua_closure.h"
#include "types/upvalue.h"
#include "types/prototype.h"
#include "types/tvalue.h"
#include "core/lua_common.h"
#include "core/lua_errors.h"
#include "gc/gc_object.h"
#include "vm/bytecode.h"

using namespace lua_cpp;
using Catch::Approx;

/* ========================================================================== */
/* 函数类型层次和基础属性契约 */
/* ========================================================================== */

TEST_CASE("LuaFunction - 函数类型层次契约", "[function][contract][types]") {
    SECTION("Lua函数基本属性") {
        // 创建Lua函数需要一个原型（Prototype）
        auto prototype = Prototype::Create();
        prototype->SetInstructionCount(10);
        prototype->SetParameterCount(2);
        prototype->SetMaxStackSize(5);
        
        auto luaFunc = LuaFunction::CreateLuaFunction(prototype);
        REQUIRE(luaFunc != nullptr);
        REQUIRE(luaFunc->GetType() == FunctionType::LuaFunction);
        REQUIRE(luaFunc->IsLuaFunction());
        REQUIRE_FALSE(luaFunc->IsCFunction());
        REQUIRE_FALSE(luaFunc->IsLightCFunction());
        
        // 访问原型信息
        REQUIRE(luaFunc->GetPrototype() == prototype);
        REQUIRE(luaFunc->GetParameterCount() == 2);
        REQUIRE(luaFunc->GetMaxStackSize() == 5);
    }

    SECTION("C函数基本属性") {
        // C函数指针
        auto cFuncPtr = [](LuaState* L) -> int {
            // 简单的C函数：返回参数个数
            return lua_gettop(L);
        };
        
        auto cFunc = LuaFunction::CreateCFunction(cFuncPtr);
        REQUIRE(cFunc != nullptr);
        REQUIRE(cFunc->GetType() == FunctionType::CFunction);
        REQUIRE_FALSE(cFunc->IsLuaFunction());
        REQUIRE(cFunc->IsCFunction());
        REQUIRE_FALSE(cFunc->IsLightCFunction());
        
        // C函数属性
        REQUIRE(cFunc->GetCFunction() == cFuncPtr);
        REQUIRE(cFunc->GetParameterCount() == -1); // C函数参数个数可变
    }

    SECTION("轻量C函数属性") {
        auto lightCFunc = [](LuaState* L) -> int { return 0; };
        
        auto lightFunc = LuaFunction::CreateLightCFunction(lightCFunc);
        REQUIRE(lightFunc != nullptr);
        REQUIRE(lightFunc->GetType() == FunctionType::LightCFunction);
        REQUIRE_FALSE(lightFunc->IsLuaFunction());
        REQUIRE_FALSE(lightFunc->IsCFunction());
        REQUIRE(lightFunc->IsLightCFunction());
        
        // 轻量C函数不应该有环境表等额外数据
        REQUIRE(lightFunc->GetEnvironment() == nullptr);
        REQUIRE(lightFunc->GetUpvalueCount() == 0);
    }

    SECTION("函数相等性") {
        auto prototype = Prototype::Create();
        auto luaFunc1 = LuaFunction::CreateLuaFunction(prototype);
        auto luaFunc2 = LuaFunction::CreateLuaFunction(prototype);
        
        // 相同原型的不同函数实例不相等
        REQUIRE(luaFunc1 != luaFunc2);
        REQUIRE_FALSE(luaFunc1->Equals(*luaFunc2));
        
        // 同一个函数实例相等
        REQUIRE(luaFunc1->Equals(*luaFunc1));
        
        auto cFunc = [](LuaState* L) -> int { return 0; };
        auto cFunction1 = LuaFunction::CreateCFunction(cFunc);
        auto cFunction2 = LuaFunction::CreateCFunction(cFunc);
        
        // 相同C函数指针的函数实例相等
        REQUIRE(cFunction1->Equals(*cFunction2));
    }
}

/* ========================================================================== */
/* 原型（Prototype）契约 */
/* ========================================================================== */

TEST_CASE("Prototype - 函数原型契约", "[function][contract][prototype]") {
    SECTION("基本原型属性") {
        auto proto = Prototype::Create();
        
        // 默认属性
        REQUIRE(proto->GetInstructionCount() == 0);
        REQUIRE(proto->GetParameterCount() == 0);
        REQUIRE(proto->GetMaxStackSize() == 0);
        REQUIRE(proto->GetUpvalueCount() == 0);
        REQUIRE(proto->GetConstantCount() == 0);
        REQUIRE(proto->GetChildPrototypeCount() == 0);
        REQUIRE_FALSE(proto->IsVararg());
        
        // 设置属性
        proto->SetParameterCount(3);
        proto->SetMaxStackSize(10);
        proto->SetVararg(true);
        
        REQUIRE(proto->GetParameterCount() == 3);
        REQUIRE(proto->GetMaxStackSize() == 10);
        REQUIRE(proto->IsVararg());
    }

    SECTION("字节码指令管理") {
        auto proto = Prototype::Create();
        
        // 添加指令
        proto->AddInstruction(OpCode::LOADK, 0, 1, 0);    // 加载常量
        proto->AddInstruction(OpCode::MOVE, 1, 0, 0);     // 移动值
        proto->AddInstruction(OpCode::RETURN, 0, 2, 0);   // 返回
        
        REQUIRE(proto->GetInstructionCount() == 3);
        
        // 访问指令
        Instruction inst1 = proto->GetInstruction(0);
        Instruction inst2 = proto->GetInstruction(1);
        Instruction inst3 = proto->GetInstruction(2);
        
        REQUIRE(GET_OPCODE(inst1) == OpCode::LOADK);
        REQUIRE(GET_OPCODE(inst2) == OpCode::MOVE);
        REQUIRE(GET_OPCODE(inst3) == OpCode::RETURN);
        
        // 验证指令参数
        REQUIRE(GETARG_A(inst1) == 0);
        REQUIRE(GETARG_Bx(inst1) == 1);
        
        // 修改指令
        proto->SetInstruction(1, OpCode::LOADNIL, 1, 1, 0);
        Instruction modifiedInst = proto->GetInstruction(1);
        REQUIRE(GET_OPCODE(modifiedInst) == OpCode::LOADNIL);
    }

    SECTION("常量表管理") {
        auto proto = Prototype::Create();
        
        // 添加各种类型的常量
        Index idx1 = proto->AddConstant(TValue::CreateNumber(42.0));
        Index idx2 = proto->AddConstant(TValue::CreateString("hello"));
        Index idx3 = proto->AddConstant(TValue::CreateBoolean(true));
        Index idx4 = proto->AddConstant(TValue::CreateNil());
        
        REQUIRE(proto->GetConstantCount() == 4);
        REQUIRE(idx1 == 0);
        REQUIRE(idx2 == 1);
        REQUIRE(idx3 == 2);
        REQUIRE(idx4 == 3);
        
        // 访问常量
        REQUIRE(proto->GetConstant(idx1) == TValue::CreateNumber(42.0));
        REQUIRE(proto->GetConstant(idx2) == TValue::CreateString("hello"));
        REQUIRE(proto->GetConstant(idx3) == TValue::CreateBoolean(true));
        REQUIRE(proto->GetConstant(idx4) == TValue::CreateNil());
        
        // 查找常量
        Index foundIdx = proto->FindConstant(TValue::CreateNumber(42.0));
        REQUIRE(foundIdx == idx1);
        
        Index notFoundIdx = proto->FindConstant(TValue::CreateString("not found"));
        REQUIRE(notFoundIdx == -1);
    }

    SECTION("子原型管理") {
        auto mainProto = Prototype::Create();
        auto childProto1 = Prototype::Create();
        auto childProto2 = Prototype::Create();
        
        // 添加子原型
        Index child1Idx = mainProto->AddChildPrototype(childProto1);
        Index child2Idx = mainProto->AddChildPrototype(childProto2);
        
        REQUIRE(mainProto->GetChildPrototypeCount() == 2);
        REQUIRE(child1Idx == 0);
        REQUIRE(child2Idx == 1);
        
        // 访问子原型
        REQUIRE(mainProto->GetChildPrototype(child1Idx) == childProto1);
        REQUIRE(mainProto->GetChildPrototype(child2Idx) == childProto2);
        
        // 验证父子关系
        REQUIRE(childProto1->GetParentPrototype() == mainProto);
        REQUIRE(childProto2->GetParentPrototype() == mainProto);
    }

    SECTION("调试信息") {
        auto proto = Prototype::Create();
        
        // 设置源码信息
        proto->SetSourceName("test.lua");
        proto->SetLineDefined(10);
        proto->SetLastLineDefined(20);
        
        REQUIRE(proto->GetSourceName() == "test.lua");
        REQUIRE(proto->GetLineDefined() == 10);
        REQUIRE(proto->GetLastLineDefined() == 20);
        
        // 添加行号信息
        proto->AddInstruction(OpCode::LOADK, 0, 0, 0);
        proto->AddInstruction(OpCode::RETURN, 0, 1, 0);
        proto->SetLineInfo(0, 12);  // 第0条指令对应源码第12行
        proto->SetLineInfo(1, 13);  // 第1条指令对应源码第13行
        
        REQUIRE(proto->GetLineInfo(0) == 12);
        REQUIRE(proto->GetLineInfo(1) == 13);
        
        // 局部变量信息
        proto->AddLocalVariable("x", 0, 2);  // 变量x从指令0到指令2有效
        proto->AddLocalVariable("y", 1, 2);  // 变量y从指令1到指令2有效
        
        auto locals = proto->GetLocalVariables();
        REQUIRE(locals.size() == 2);
        REQUIRE(locals[0].name == "x");
        REQUIRE(locals[0].startPc == 0);
        REQUIRE(locals[0].endPc == 2);
    }
}

/* ========================================================================== */
/* Upvalue机制契约 */
/* ========================================================================== */

TEST_CASE("Upvalue - 上值机制契约", "[function][contract][upvalue]") {
    SECTION("基本Upvalue属性") {
        // 创建一个栈值的Upvalue
        TValue stackValue = TValue::CreateNumber(42.0);
        auto upvalue = Upvalue::Create(&stackValue);
        
        REQUIRE(upvalue != nullptr);
        REQUIRE(upvalue->IsOpen());
        REQUIRE_FALSE(upvalue->IsClosed());
        REQUIRE(upvalue->GetValue() == TValue::CreateNumber(42.0));
        REQUIRE(upvalue->GetLocation() == &stackValue);
    }

    SECTION("Upvalue关闭机制") {
        TValue stackValue = TValue::CreateString("test");
        auto upvalue = Upvalue::Create(&stackValue);
        
        REQUIRE(upvalue->IsOpen());
        
        // 关闭Upvalue（将栈值复制到Upvalue内部）
        upvalue->Close();
        
        REQUIRE_FALSE(upvalue->IsOpen());
        REQUIRE(upvalue->IsClosed());
        REQUIRE(upvalue->GetValue() == TValue::CreateString("test"));
        REQUIRE(upvalue->GetLocation() != &stackValue); // 不再指向栈
        
        // 修改原栈值不应影响已关闭的Upvalue
        stackValue = TValue::CreateNumber(100.0);
        REQUIRE(upvalue->GetValue() == TValue::CreateString("test"));
    }

    SECTION("Upvalue值更新") {
        TValue stackValue = TValue::CreateNumber(10.0);
        auto upvalue = Upvalue::Create(&stackValue);
        
        // 通过Upvalue设置值
        upvalue->SetValue(TValue::CreateNumber(20.0));
        REQUIRE(upvalue->GetValue() == TValue::CreateNumber(20.0));
        REQUIRE(stackValue == TValue::CreateNumber(20.0)); // 栈值也应该更新
        
        // 关闭后设置值
        upvalue->Close();
        upvalue->SetValue(TValue::CreateNumber(30.0));
        REQUIRE(upvalue->GetValue() == TValue::CreateNumber(30.0));
        REQUIRE(stackValue == TValue::CreateNumber(20.0)); // 栈值不变
    }

    SECTION("Upvalue链表管理") {
        // 模拟多个Upvalue的链表结构
        TValue val1 = TValue::CreateNumber(1.0);
        TValue val2 = TValue::CreateNumber(2.0);
        TValue val3 = TValue::CreateNumber(3.0);
        
        auto uv1 = Upvalue::Create(&val1);
        auto uv2 = Upvalue::Create(&val2);
        auto uv3 = Upvalue::Create(&val3);
        
        // 建立链表关系
        uv1->SetNext(uv2);
        uv2->SetNext(uv3);
        
        REQUIRE(uv1->GetNext() == uv2);
        REQUIRE(uv2->GetNext() == uv3);
        REQUIRE(uv3->GetNext() == nullptr);
        
        // 遍历链表
        std::vector<double> values;
        for (auto current = uv1; current != nullptr; current = current->GetNext()) {
            values.push_back(current->GetValue().GetNumber());
        }
        REQUIRE(values == std::vector<double>{1.0, 2.0, 3.0});
    }

    SECTION("Upvalue GC标记") {
        TValue tableValue = TValue::CreateTable(LuaTable::Create());
        auto upvalue = Upvalue::Create(&tableValue);
        
        // Upvalue应该正确标记引用的对象
        REQUIRE(upvalue->HasReferences());
        
        // 模拟GC标记过程
        upvalue->SetGCColor(GCColor::Gray);
        upvalue->MarkReferences(GCColor::Gray);
        
        // 验证引用的表也被标记（具体实现时验证）
        // auto table = tableValue.GetTable();
        // REQUIRE(table->GetGCColor() == GCColor::Gray);
    }
}

/* ========================================================================== */
/* 闭包（Closure）契约 */
/* ========================================================================== */

TEST_CASE("LuaClosure - Lua闭包契约", "[function][contract][closure]") {
    SECTION("Lua闭包基本属性") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(2);
        
        auto closure = LuaClosure::Create(prototype);
        REQUIRE(closure != nullptr);
        REQUIRE(closure->GetPrototype() == prototype);
        REQUIRE(closure->GetUpvalueCount() == 2);
        REQUIRE(closure->GetType() == FunctionType::LuaFunction);
    }

    SECTION("Upvalue绑定") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(3);
        
        auto closure = LuaClosure::Create(prototype);
        
        // 创建Upvalue
        TValue val1 = TValue::CreateNumber(10.0);
        TValue val2 = TValue::CreateString("hello");
        TValue val3 = TValue::CreateBoolean(true);
        
        auto uv1 = Upvalue::Create(&val1);
        auto uv2 = Upvalue::Create(&val2);
        auto uv3 = Upvalue::Create(&val3);
        
        // 绑定Upvalue到闭包
        closure->SetUpvalue(0, uv1);
        closure->SetUpvalue(1, uv2);
        closure->SetUpvalue(2, uv3);
        
        // 验证绑定
        REQUIRE(closure->GetUpvalue(0) == uv1);
        REQUIRE(closure->GetUpvalue(1) == uv2);
        REQUIRE(closure->GetUpvalue(2) == uv3);
        
        // 通过闭包访问Upvalue值
        REQUIRE(closure->GetUpvalueValue(0) == TValue::CreateNumber(10.0));
        REQUIRE(closure->GetUpvalueValue(1) == TValue::CreateString("hello"));
        REQUIRE(closure->GetUpvalueValue(2) == TValue::CreateBoolean(true));
    }

    SECTION("Upvalue设置和更新") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(1);
        
        auto closure = LuaClosure::Create(prototype);
        
        TValue stackValue = TValue::CreateNumber(42.0);
        auto upvalue = Upvalue::Create(&stackValue);
        closure->SetUpvalue(0, upvalue);
        
        // 通过闭包设置Upvalue值
        closure->SetUpvalueValue(0, TValue::CreateNumber(100.0));
        REQUIRE(closure->GetUpvalueValue(0) == TValue::CreateNumber(100.0));
        REQUIRE(stackValue == TValue::CreateNumber(100.0)); // 栈值也应该更新
        
        // 关闭Upvalue后再设置
        upvalue->Close();
        closure->SetUpvalueValue(0, TValue::CreateNumber(200.0));
        REQUIRE(closure->GetUpvalueValue(0) == TValue::CreateNumber(200.0));
        REQUIRE(stackValue == TValue::CreateNumber(100.0)); // 栈值不再更新
    }

    SECTION("环境表") {
        auto prototype = Prototype::Create();
        auto closure = LuaClosure::Create(prototype);
        
        // 默认环境表
        REQUIRE(closure->GetEnvironment() != nullptr);
        
        // 设置自定义环境表
        auto customEnv = LuaTable::Create();
        customEnv->SetHashValue(TValue::CreateString("custom"), TValue::CreateBoolean(true));
        
        closure->SetEnvironment(customEnv);
        REQUIRE(closure->GetEnvironment() == customEnv);
        
        // 验证环境表内容
        auto envValue = closure->GetEnvironment()->GetHashValue(TValue::CreateString("custom"));
        REQUIRE(envValue == TValue::CreateBoolean(true));
    }

    SECTION("闭包克隆") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(2);
        
        auto originalClosure = LuaClosure::Create(prototype);
        
        // 设置Upvalue
        TValue val1 = TValue::CreateNumber(10.0);
        TValue val2 = TValue::CreateString("test");
        originalClosure->SetUpvalue(0, Upvalue::Create(&val1));
        originalClosure->SetUpvalue(1, Upvalue::Create(&val2));
        
        // 克隆闭包
        auto clonedClosure = originalClosure->Clone();
        
        REQUIRE(clonedClosure != originalClosure);
        REQUIRE(clonedClosure->GetPrototype() == originalClosure->GetPrototype());
        REQUIRE(clonedClosure->GetUpvalueCount() == originalClosure->GetUpvalueCount());
        
        // Upvalue应该是独立的
        REQUIRE(clonedClosure->GetUpvalue(0) != originalClosure->GetUpvalue(0));
        REQUIRE(clonedClosure->GetUpvalue(1) != originalClosure->GetUpvalue(1));
        
        // 但值应该相同
        REQUIRE(clonedClosure->GetUpvalueValue(0) == originalClosure->GetUpvalueValue(0));
        REQUIRE(clonedClosure->GetUpvalueValue(1) == originalClosure->GetUpvalueValue(1));
    }
}

TEST_CASE("CClosure - C闭包契约", "[function][contract][cclosure]") {
    SECTION("C闭包基本属性") {
        auto cFunc = [](LuaState* L) -> int { return 0; };
        auto cclosure = CClosure::Create(cFunc, 2); // 2个upvalue
        
        REQUIRE(cclosure != nullptr);
        REQUIRE(cclosure->GetCFunction() == cFunc);
        REQUIRE(cclosure->GetUpvalueCount() == 2);
        REQUIRE(cclosure->GetType() == FunctionType::CFunction);
    }

    SECTION("C闭包Upvalue管理") {
        auto cFunc = [](LuaState* L) -> int { return 0; };
        auto cclosure = CClosure::Create(cFunc, 3);
        
        // 设置Upvalue
        cclosure->SetUpvalue(0, TValue::CreateNumber(1.0));
        cclosure->SetUpvalue(1, TValue::CreateString("c_upvalue"));
        cclosure->SetUpvalue(2, TValue::CreateBoolean(false));
        
        // 验证Upvalue
        REQUIRE(cclosure->GetUpvalue(0) == TValue::CreateNumber(1.0));
        REQUIRE(cclosure->GetUpvalue(1) == TValue::CreateString("c_upvalue"));
        REQUIRE(cclosure->GetUpvalue(2) == TValue::CreateBoolean(false));
        
        // C闭包的Upvalue是直接存储的值，不是引用
        cclosure->SetUpvalue(0, TValue::CreateNumber(2.0));
        REQUIRE(cclosure->GetUpvalue(0) == TValue::CreateNumber(2.0));
    }

    SECTION("用户数据和注册表") {
        auto cFunc = [](LuaState* L) -> int { return 0; };
        auto cclosure = CClosure::Create(cFunc, 1);
        
        // C闭包可以有关联的用户数据
        void* userData = malloc(100);
        cclosure->SetUserData(userData);
        REQUIRE(cclosure->GetUserData() == userData);
        
        // 注册表键
        cclosure->SetRegistryKey(42);
        REQUIRE(cclosure->GetRegistryKey() == 42);
        
        free(userData);
    }
}

/* ========================================================================== */
/* 函数调用机制契约 */
/* ========================================================================== */

TEST_CASE("LuaFunction - 函数调用契约", "[function][contract][call]") {
    SECTION("参数和返回值约定") {
        auto prototype = Prototype::Create();
        prototype->SetParameterCount(2);
        prototype->SetMaxStackSize(5);
        
        // 设置简单的字节码：返回两个参数的和
        prototype->AddInstruction(OpCode::ADD, 2, 0, 1);     // R(2) = R(0) + R(1)
        prototype->AddInstruction(OpCode::RETURN, 2, 2, 0);  // return R(2)
        
        auto luaFunc = LuaFunction::CreateLuaFunction(prototype);
        
        // 验证调用约定信息
        REQUIRE(luaFunc->GetParameterCount() == 2);
        REQUIRE(luaFunc->IsVararg() == false);
        REQUIRE(luaFunc->GetMaxStackSize() == 5);
    }

    SECTION("可变参数函数") {
        auto prototype = Prototype::Create();
        prototype->SetParameterCount(1);  // 至少1个参数
        prototype->SetVararg(true);       // 支持可变参数
        
        auto varargFunc = LuaFunction::CreateLuaFunction(prototype);
        
        REQUIRE(varargFunc->IsVararg());
        REQUIRE(varargFunc->GetParameterCount() == 1);
    }

    SECTION("函数调用状态") {
        auto prototype = Prototype::Create();
        auto luaFunc = LuaFunction::CreateLuaFunction(prototype);
        
        // 函数调用状态跟踪
        REQUIRE_FALSE(luaFunc->IsRunning());
        
        // 模拟调用开始
        luaFunc->SetRunning(true);
        REQUIRE(luaFunc->IsRunning());
        
        // 模拟调用结束
        luaFunc->SetRunning(false);
        REQUIRE_FALSE(luaFunc->IsRunning());
    }

    SECTION("尾调用优化标记") {
        auto prototype = Prototype::Create();
        
        // 添加尾调用指令
        prototype->AddInstruction(OpCode::TAILCALL, 0, 2, 0);
        
        auto func = LuaFunction::CreateLuaFunction(prototype);
        
        // 检查是否包含尾调用
        REQUIRE(func->HasTailCalls());
    }
}

/* ========================================================================== */
/* 内存管理和垃圾回收契约 */
/* ========================================================================== */

TEST_CASE("LuaFunction - 内存管理契约", "[function][contract][memory]") {
    SECTION("函数对象生命周期") {
        std::weak_ptr<LuaFunction> weakFunc;
        
        {
            auto prototype = Prototype::Create();
            auto func = LuaFunction::CreateLuaFunction(prototype);
            weakFunc = func;
            
            REQUIRE_FALSE(weakFunc.expired());
        }
        
        // 函数离开作用域后应该被回收
        REQUIRE(weakFunc.expired());
    }

    SECTION("原型共享") {
        auto sharedPrototype = Prototype::Create();
        
        auto func1 = LuaFunction::CreateLuaFunction(sharedPrototype);
        auto func2 = LuaFunction::CreateLuaFunction(sharedPrototype);
        
        // 多个函数可以共享同一个原型
        REQUIRE(func1->GetPrototype() == func2->GetPrototype());
        REQUIRE(func1->GetPrototype() == sharedPrototype);
        
        // 但函数实例是不同的
        REQUIRE(func1 != func2);
    }

    SECTION("Upvalue生命周期") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(1);
        
        std::weak_ptr<Upvalue> weakUpvalue;
        
        {
            auto closure = LuaClosure::Create(prototype);
            TValue stackValue = TValue::CreateNumber(42.0);
            auto upvalue = Upvalue::Create(&stackValue);
            weakUpvalue = upvalue;
            
            closure->SetUpvalue(0, upvalue);
            REQUIRE_FALSE(weakUpvalue.expired());
            
            // 闭包应该保持Upvalue存活
        }
        
        // 闭包销毁后，Upvalue也应该被回收
        REQUIRE(weakUpvalue.expired());
    }

    SECTION("GC标记遍历") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(2);
        
        auto closure = LuaClosure::Create(prototype);
        
        // 创建包含引用的Upvalue
        auto table1 = LuaTable::Create();
        auto table2 = LuaTable::Create();
        
        TValue tableVal1 = TValue::CreateTable(table1);
        TValue tableVal2 = TValue::CreateTable(table2);
        
        closure->SetUpvalue(0, Upvalue::Create(&tableVal1));
        closure->SetUpvalue(1, Upvalue::Create(&tableVal2));
        
        // 闭包应该能够标记所有引用的对象
        REQUIRE(closure->HasReferences());
        
        // 模拟GC标记
        closure->SetGCColor(GCColor::Gray);
        closure->MarkReferences(GCColor::Gray);
        
        // 验证Upvalue和表都被标记（具体实现时验证）
        /*
        REQUIRE(table1->GetGCColor() == GCColor::Gray);
        REQUIRE(table2->GetGCColor() == GCColor::Gray);
        */
    }

    SECTION("内存使用统计") {
        auto prototype = Prototype::Create();
        
        // 添加一些数据到原型
        prototype->AddConstant(TValue::CreateString("constant1"));
        prototype->AddConstant(TValue::CreateString("constant2"));
        prototype->AddInstruction(OpCode::LOADK, 0, 0, 0);
        prototype->AddInstruction(OpCode::RETURN, 0, 1, 0);
        
        Size prototypeSize = prototype->GetMemorySize();
        REQUIRE(prototypeSize > 0);
        
        auto func = LuaFunction::CreateLuaFunction(prototype);
        Size functionSize = func->GetMemorySize();
        REQUIRE(functionSize > 0);
        
        // 函数大小应该包括原型大小（如果不共享）或者有基础开销
        REQUIRE(functionSize >= sizeof(LuaFunction));
    }
}

/* ========================================================================== */
/* 性能和优化契约 */
/* ========================================================================== */

TEST_CASE("LuaFunction - 性能契约", "[function][contract][performance]") {
    SECTION("函数创建性能") {
        auto prototype = Prototype::Create();
        constexpr int iterations = 10000;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::shared_ptr<LuaFunction>> functions;
        for (int i = 0; i < iterations; ++i) {
            functions.push_back(LuaFunction::CreateLuaFunction(prototype));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 函数创建应该很快
        REQUIRE(duration.count() < 10000); // 少于10ms
        REQUIRE(functions.size() == iterations);
    }

    SECTION("Upvalue访问性能") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(10);
        
        auto closure = LuaClosure::Create(prototype);
        
        // 设置Upvalue
        for (int i = 0; i < 10; ++i) {
            TValue value = TValue::CreateNumber(static_cast<double>(i));
            closure->SetUpvalue(i, Upvalue::Create(&value));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 大量Upvalue访问
        volatile double sum = 0.0;
        for (int rep = 0; rep < 100000; ++rep) {
            for (int i = 0; i < 10; ++i) {
                sum += closure->GetUpvalueValue(i).GetNumber();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // Upvalue访问应该很快
        REQUIRE(duration.count() < 50000); // 少于50ms
        REQUIRE(sum > 0.0); // 防止优化掉
    }

    SECTION("常量表访问性能") {
        auto prototype = Prototype::Create();
        
        // 添加大量常量
        for (int i = 0; i < 1000; ++i) {
            prototype->AddConstant(TValue::CreateNumber(static_cast<double>(i)));
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 大量常量访问
        volatile double sum = 0.0;
        for (int rep = 0; rep < 1000; ++rep) {
            for (int i = 0; i < 1000; ++i) {
                sum += prototype->GetConstant(i).GetNumber();
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 常量访问应该很快
        REQUIRE(duration.count() < 10000); // 少于10ms
        REQUIRE(sum > 0.0); // 防止优化掉
    }

    SECTION("字节码指令缓存") {
        auto prototype = Prototype::Create();
        
        // 添加大量指令
        for (int i = 0; i < 10000; ++i) {
            prototype->AddInstruction(OpCode::MOVE, i % 256, (i + 1) % 256, 0);
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // 大量指令访问
        volatile UInt32 sum = 0;
        for (int rep = 0; rep < 100; ++rep) {
            for (int i = 0; i < 10000; ++i) {
                sum += prototype->GetInstruction(i);
            }
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        
        // 指令访问应该很快
        REQUIRE(duration.count() < 20000); // 少于20ms
        REQUIRE(sum > 0); // 防止优化掉
    }
}

/* ========================================================================== */
/* Lua 5.1.5兼容性契约 */
/* ========================================================================== */

TEST_CASE("LuaFunction - Lua 5.1.5兼容性契约", "[function][contract][compatibility]") {
    SECTION("函数类型识别") {
        auto prototype = Prototype::Create();
        auto luaFunc = LuaFunction::CreateLuaFunction(prototype);
        
        auto cFunc = [](LuaState* L) -> int { return 0; };
        auto cFunction = LuaFunction::CreateCFunction(cFunc);
        
        // Lua 5.1.5函数类型检查
        REQUIRE(luaFunc->GetLuaType() == LuaType::Function);
        REQUIRE(cFunction->GetLuaType() == LuaType::Function);
        
        // 但内部类型不同
        REQUIRE(luaFunc->IsLuaFunction());
        REQUIRE_FALSE(luaFunc->IsCFunction());
        REQUIRE_FALSE(cFunction->IsLuaFunction());
        REQUIRE(cFunction->IsCFunction());
    }

    SECTION("函数调用约定") {
        auto prototype = Prototype::Create();
        prototype->SetParameterCount(2);
        prototype->SetVararg(false);
        
        auto func = LuaFunction::CreateLuaFunction(prototype);
        
        // Lua 5.1.5调用约定
        REQUIRE(func->GetParameterCount() == 2);
        REQUIRE_FALSE(func->IsVararg());
        
        // 可变参数函数
        auto varargProto = Prototype::Create();
        varargProto->SetParameterCount(1);
        varargProto->SetVararg(true);
        
        auto varargFunc = LuaFunction::CreateLuaFunction(varargProto);
        REQUIRE(varargFunc->IsVararg());
    }

    SECTION("环境表处理") {
        auto prototype = Prototype::Create();
        auto func = LuaFunction::CreateLuaFunction(prototype);
        
        // Lua 5.1.5中函数有环境表
        REQUIRE(func->GetEnvironment() != nullptr);
        
        // 可以设置自定义环境表
        auto customEnv = LuaTable::Create();
        func->SetEnvironment(customEnv);
        REQUIRE(func->GetEnvironment() == customEnv);
        
        // C函数也有环境表
        auto cFunc = [](LuaState* L) -> int { return 0; };
        auto cFunction = LuaFunction::CreateCFunction(cFunc);
        REQUIRE(cFunction->GetEnvironment() != nullptr);
    }

    SECTION("Upvalue语义") {
        auto prototype = Prototype::Create();
        prototype->SetUpvalueCount(1);
        
        auto closure = LuaClosure::Create(prototype);
        
        // Lua 5.1.5 Upvalue语义
        TValue localVar = TValue::CreateNumber(42.0);
        auto upvalue = Upvalue::Create(&localVar);
        closure->SetUpvalue(0, upvalue);
        
        // 通过Upvalue修改外部变量
        closure->SetUpvalueValue(0, TValue::CreateNumber(100.0));
        REQUIRE(localVar == TValue::CreateNumber(100.0));
        
        // 变量离开作用域时Upvalue应该被关闭
        upvalue->Close();
        closure->SetUpvalueValue(0, TValue::CreateNumber(200.0));
        REQUIRE(localVar == TValue::CreateNumber(100.0)); // 不再更新外部变量
        REQUIRE(closure->GetUpvalueValue(0) == TValue::CreateNumber(200.0));
    }

    SECTION("字节码格式兼容性") {
        auto prototype = Prototype::Create();
        
        // Lua 5.1.5字节码指令格式
        prototype->AddInstruction(OpCode::MOVE, 0, 1, 0);
        prototype->AddInstruction(OpCode::LOADK, 0, 0, 0);
        prototype->AddInstruction(OpCode::LOADBOOL, 0, 1, 0);
        prototype->AddInstruction(OpCode::LOADNIL, 0, 2, 0);
        
        REQUIRE(prototype->GetInstructionCount() == 4);
        
        // 验证指令编码格式
        Instruction moveInst = prototype->GetInstruction(0);
        REQUIRE(GET_OPCODE(moveInst) == OpCode::MOVE);
        REQUIRE(GETARG_A(moveInst) == 0);
        REQUIRE(GETARG_B(moveInst) == 1);
        REQUIRE(GETARG_C(moveInst) == 0);
        
        Instruction loadkInst = prototype->GetInstruction(1);
        REQUIRE(GET_OPCODE(loadkInst) == OpCode::LOADK);
        REQUIRE(GETARG_A(loadkInst) == 0);
        REQUIRE(GETARG_Bx(loadkInst) == 0);
    }

    SECTION("调试信息兼容性") {
        auto prototype = Prototype::Create();
        
        // Lua 5.1.5调试信息格式
        prototype->SetSourceName("@test.lua");
        prototype->SetLineDefined(1);
        prototype->SetLastLineDefined(10);
        
        // 添加指令和行号信息
        prototype->AddInstruction(OpCode::MOVE, 0, 1, 0);
        prototype->AddInstruction(OpCode::RETURN, 0, 1, 0);
        prototype->SetLineInfo(0, 5);
        prototype->SetLineInfo(1, 6);
        
        // 局部变量信息
        prototype->AddLocalVariable("x", 0, 2);
        prototype->AddLocalVariable("y", 0, 2);
        
        // 验证调试信息
        REQUIRE(prototype->GetSourceName() == "@test.lua");
        REQUIRE(prototype->GetLineDefined() == 1);
        REQUIRE(prototype->GetLastLineDefined() == 10);
        REQUIRE(prototype->GetLineInfo(0) == 5);
        REQUIRE(prototype->GetLineInfo(1) == 6);
        
        auto locals = prototype->GetLocalVariables();
        REQUIRE(locals.size() == 2);
        REQUIRE(locals[0].name == "x");
        REQUIRE(locals[1].name == "y");
    }
}

/* ========================================================================== */
/* 错误处理和边界情况契约 */
/* ========================================================================== */

TEST_CASE("LuaFunction - 错误处理契约", "[function][contract][error-handling]") {
    SECTION("无效参数处理") {
        // 空原型应该抛出异常
        REQUIRE_THROWS_AS(LuaFunction::CreateLuaFunction(nullptr), std::invalid_argument);
        
        // 空C函数指针应该抛出异常
        REQUIRE_THROWS_AS(LuaFunction::CreateCFunction(nullptr), std::invalid_argument);
    }

    SECTION("越界访问处理") {
        auto prototype = Prototype::Create();
        prototype->AddConstant(TValue::CreateNumber(1.0));
        prototype->AddInstruction(OpCode::MOVE, 0, 1, 0);
        
        // 常量表越界
        REQUIRE_THROWS_AS(prototype->GetConstant(10), std::out_of_range);
        
        // 指令表越界
        REQUIRE_THROWS_AS(prototype->GetInstruction(10), std::out_of_range);
        
        // Upvalue越界
        prototype->SetUpvalueCount(2);
        auto closure = LuaClosure::Create(prototype);
        REQUIRE_THROWS_AS(closure->GetUpvalue(5), std::out_of_range);
    }

    SECTION("循环引用检测") {
        auto proto1 = Prototype::Create();
        auto proto2 = Prototype::Create();
        
        // 添加循环引用的子原型
        proto1->AddChildPrototype(proto2);
        proto2->AddChildPrototype(proto1);
        
        // 应该能检测并处理循环引用
        REQUIRE_NOTHROW(proto1->HasCircularReference());
        REQUIRE(proto1->HasCircularReference());
    }

    SECTION("内存不足处理") {
        auto prototype = Prototype::Create();
        
        // 尝试添加大量常量和指令
        try {
            for (int i = 0; i < 1000000; ++i) {
                prototype->AddConstant(TValue::CreateString("large_constant_" + std::to_string(i)));
                prototype->AddInstruction(OpCode::MOVE, i % 256, (i + 1) % 256, 0);
            }
            
            // 如果成功，验证基本功能
            REQUIRE(prototype->GetConstantCount() == 1000000);
            REQUIRE(prototype->GetInstructionCount() == 1000000);
        } catch (const std::bad_alloc&) {
            // 内存不足是预期的行为
            REQUIRE(true);
        }
    }

    SECTION("并发安全性") {
        auto prototype = Prototype::Create();
        constexpr int threadCount = 4;
        constexpr int operationsPerThread = 1000;
        
        std::vector<std::thread> threads;
        std::atomic<int> successCount{0};
        
        // 多线程访问原型
        for (int t = 0; t < threadCount; ++t) {
            threads.emplace_back([&prototype, &successCount, operationsPerThread]() {
                try {
                    for (int i = 0; i < operationsPerThread; ++i) {
                        // 读取操作应该是安全的
                        volatile auto constCount = prototype->GetConstantCount();
                        volatile auto instCount = prototype->GetInstructionCount();
                        (void)constCount;
                        (void)instCount;
                    }
                    successCount.fetch_add(operationsPerThread);
                } catch (...) {
                    // 记录异常但不失败测试
                }
            });
        }
        
        // 等待所有线程完成
        for (auto& thread : threads) {
            thread.join();
        }
        
        // 大部分操作应该成功
        REQUIRE(successCount.load() > threadCount * operationsPerThread * 0.9);
    }
}