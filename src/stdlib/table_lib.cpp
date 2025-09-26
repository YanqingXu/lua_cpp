/**
 * @file table_lib.cpp
 * @brief T027 Lua表库实现
 * 
 * 实现Lua 5.1.5表库的所有函数
 * 
 * @author Lua C++ Project Team
 * @date 2025-09-26
 * @version 1.0
 */

#include "table_lib.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {
namespace stdlib {

// ============================================================================
// TableLibrary类实现
// ============================================================================

TableLibrary::TableLibrary() = default;

std::vector<LibFunction> TableLibrary::GetFunctions() const {
    std::vector<LibFunction> functions;
    
    // 数组操作
    REGISTER_FUNCTION(functions, "insert", lua_table_insert, "在表中插入元素");
    REGISTER_FUNCTION(functions, "remove", lua_table_remove, "从表中移除元素");
    REGISTER_FUNCTION(functions, "sort", lua_table_sort, "对表进行排序");
    REGISTER_FUNCTION(functions, "concat", lua_table_concat, "连接表元素为字符串");
    
    // 表工具
    REGISTER_FUNCTION(functions, "maxn", lua_table_maxn, "获取表的最大数字键");
    REGISTER_FUNCTION(functions, "foreach", lua_table_foreach, "遍历表的所有元素");
    REGISTER_FUNCTION(functions, "foreachi", lua_table_foreachi, "遍历表的数组部分");
    
    return functions;
}

void TableLibrary::RegisterModule(EnhancedVirtualMachine* vm) {
    if (!vm) return;
    
    auto functions = GetFunctions();
    
    // 创建table表
    LuaTable table_table;
    
    for (const auto& func : functions) {
        LuaValue func_value = LuaValue::CreateFunction(
            [f = func.func](EnhancedVirtualMachine* vm) -> std::vector<LuaValue> {
                int result_count = f(vm);
                auto& stack = vm->GetStack();
                
                std::vector<LuaValue> results;
                if (result_count > 0 && static_cast<size_t>(result_count) <= stack.size()) {
                    results.reserve(result_count);
                    for (int i = stack.size() - result_count; i < static_cast<int>(stack.size()); i++) {
                        results.push_back(stack[i]);
                    }
                    // 清理栈上的返回值
                    for (int i = 0; i < result_count; i++) {
                        stack.pop_back();
                    }
                }
                
                return results;
            }
        );
        
        table_table.SetField(LuaValue::CreateString(func.name), func_value);
    }
    
    // 注册table表到全局环境
    auto& globals = vm->GetGlobalEnvironment();
    globals.SetField("table", LuaValue::CreateTable(table_table));
}

void TableLibrary::Initialize(EnhancedVirtualMachine* vm) {
    (void)vm;  // 表库暂时不需要特殊初始化
}

void TableLibrary::Cleanup(EnhancedVirtualMachine* vm) {
    (void)vm;  // 表库暂时不需要特殊清理
}

// ============================================================================
// 表库函数实现
// ============================================================================

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_insert) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 2 || vm->GetStack().size() > 3) {
        ErrorHelper::ArgError("table.insert", -1, "expected 2 or 3 arguments");
    }
    
    helper.CheckArgType(1, LuaValueType::TABLE, "table.insert");
    
    LuaTable& table = const_cast<LuaTable&>(vm->GetStack()[0].AsTable());
    
    if (vm->GetStack().size() == 2) {
        // table.insert(table, value) - 在末尾插入
        const LuaValue& value = vm->GetStack()[1];
        size_t len = GetArrayLength(table);
        
        LuaValue index_key = LuaValue::CreateNumber(static_cast<double>(len + 1));
        table.SetField(index_key, value);
        
    } else {
        // table.insert(table, pos, value) - 在指定位置插入
        int pos = helper.GetIntArg(2);
        const LuaValue& value = vm->GetStack()[2];
        size_t len = GetArrayLength(table);
        
        if (pos < 1 || pos > static_cast<int>(len + 1)) {
            ErrorHelper::ArgError("table.insert", 2, "position out of bounds");
        }
        
        // 移动现有元素
        for (int i = static_cast<int>(len); i >= pos; i--) {
            LuaValue old_key = LuaValue::CreateNumber(static_cast<double>(i));
            LuaValue new_key = LuaValue::CreateNumber(static_cast<double>(i + 1));
            
            LuaValue old_value = table.GetField(old_key);
            table.SetField(new_key, old_value);
        }
        
        // 插入新值
        LuaValue pos_key = LuaValue::CreateNumber(static_cast<double>(pos));
        table.SetField(pos_key, value);
    }
    
    // 清理参数
    vm->GetStack().clear();
    
    return 0;  // table.insert不返回值
}

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_remove) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 1 || vm->GetStack().size() > 2) {
        ErrorHelper::ArgError("table.remove", -1, "expected 1 or 2 arguments");
    }
    
    helper.CheckArgType(1, LuaValueType::TABLE, "table.remove");
    
    LuaTable& table = const_cast<LuaTable&>(vm->GetStack()[0].AsTable());
    size_t len = GetArrayLength(table);
    
    if (len == 0) {
        // 空表，返回nil
        vm->GetStack().clear();
        vm->GetStack().push_back(LuaValue::CreateNil());
        return 1;
    }
    
    int pos;
    if (vm->GetStack().size() == 1) {
        // table.remove(table) - 移除最后一个元素
        pos = static_cast<int>(len);
    } else {
        // table.remove(table, pos) - 移除指定位置的元素
        pos = helper.GetIntArg(2);
    }
    
    if (pos < 1 || pos > static_cast<int>(len)) {
        // 位置超出范围，返回nil
        vm->GetStack().clear();
        vm->GetStack().push_back(LuaValue::CreateNil());
        return 1;
    }
    
    // 获取要移除的值
    LuaValue pos_key = LuaValue::CreateNumber(static_cast<double>(pos));
    LuaValue removed_value = table.GetField(pos_key);
    
    // 移动后续元素
    for (int i = pos + 1; i <= static_cast<int>(len); i++) {
        LuaValue old_key = LuaValue::CreateNumber(static_cast<double>(i));
        LuaValue new_key = LuaValue::CreateNumber(static_cast<double>(i - 1));
        
        LuaValue value = table.GetField(old_key);
        table.SetField(new_key, value);
    }
    
    // 移除最后一个元素
    LuaValue last_key = LuaValue::CreateNumber(static_cast<double>(len));
    table.SetField(last_key, LuaValue::CreateNil());
    
    // 清理参数
    vm->GetStack().clear();
    
    // 返回移除的值
    vm->GetStack().push_back(removed_value);
    
    return 1;
}

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_concat) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 1 || vm->GetStack().size() > 4) {
        ErrorHelper::ArgError("table.concat", -1, "expected 1-4 arguments");
    }
    
    helper.CheckArgType(1, LuaValueType::TABLE, "table.concat");
    
    const LuaTable& table = vm->GetStack()[0].AsTable();
    std::string sep = helper.GetStringArg(2, "");
    int start = helper.GetIntArg(3, 1);
    int end = helper.GetIntArg(4, static_cast<int>(GetArrayLength(table)));
    
    std::ostringstream result;
    bool first = true;
    
    for (int i = start; i <= end; i++) {
        LuaValue key = LuaValue::CreateNumber(static_cast<double>(i));
        LuaValue value = table.GetField(key);
        
        if (value.GetType() == LuaValueType::NIL) {
            continue;  // 跳过nil值
        }
        
        if (!first && !sep.empty()) {
            result << sep;
        }
        first = false;
        
        // 转换为字符串
        if (value.GetType() == LuaValueType::STRING) {
            result << value.AsString();
        } else if (value.GetType() == LuaValueType::NUMBER) {
            double num = value.AsNumber();
            if (num == std::floor(num)) {
                result << static_cast<long long>(num);
            } else {
                result << num;
            }
        } else {
            ErrorHelper::ArgError("table.concat", 1, 
                "invalid value (" + std::string("string or number expected") + ")");
        }
    }
    
    // 清理参数
    vm->GetStack().clear();
    
    // 返回连接后的字符串
    vm->GetStack().push_back(LuaValue::CreateString(result.str()));
    
    return 1;
}

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_sort) {
    StackHelper helper(vm);
    
    if (vm->GetStack().size() < 1 || vm->GetStack().size() > 2) {
        ErrorHelper::ArgError("table.sort", -1, "expected 1 or 2 arguments");
    }
    
    helper.CheckArgType(1, LuaValueType::TABLE, "table.sort");
    
    LuaTable& table = const_cast<LuaTable&>(vm->GetStack()[0].AsTable());
    size_t len = GetArrayLength(table);
    
    if (len <= 1) {
        // 空表或单元素表，无需排序
        vm->GetStack().clear();
        return 0;
    }
    
    CompareFunction compare = DefaultCompare;
    
    // TODO: 处理自定义比较函数
    if (vm->GetStack().size() == 2) {
        if (vm->GetStack()[1].GetType() != LuaValueType::FUNCTION) {
            ErrorHelper::ArgError("table.sort", 2, "function expected");
        }
        // 暂时使用默认比较函数
    }
    
    // 执行快速排序
    QuickSort(table, compare, 1, static_cast<int>(len));
    
    // 清理参数
    vm->GetStack().clear();
    
    return 0;  // table.sort不返回值
}

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_maxn) {
    StackHelper helper(vm);
    helper.CheckArgCount(1, "table.maxn");
    helper.CheckArgType(1, LuaValueType::TABLE, "table.maxn");
    
    const LuaTable& table = vm->GetStack()[0].AsTable();
    double max_key = GetMaxNumericKey(table);
    
    // 清理参数
    vm->GetStack().clear();
    
    // 返回最大数字键
    vm->GetStack().push_back(LuaValue::CreateNumber(max_key));
    
    return 1;
}

// ============================================================================
// 内部辅助函数实现
// ============================================================================

size_t TableLibrary::GetArrayLength(const LuaTable& table) {
    size_t length = 0;
    
    // 计算连续的整数索引长度
    for (size_t i = 1; ; i++) {
        LuaValue key = LuaValue::CreateNumber(static_cast<double>(i));
        LuaValue value = table.GetField(key);
        
        if (value.GetType() == LuaValueType::NIL) {
            break;
        }
        
        length = i;
    }
    
    return length;
}

double TableLibrary::GetMaxNumericKey(const LuaTable& table) {
    double max_key = 0.0;
    
    // 遍历表中的所有键，找到最大的数字键
    // TODO: 实现表的键遍历功能
    // 暂时使用简单方法检查前1000个可能的键
    for (int i = 1; i <= 1000; i++) {
        LuaValue key = LuaValue::CreateNumber(static_cast<double>(i));
        LuaValue value = table.GetField(key);
        
        if (value.GetType() != LuaValueType::NIL) {
            max_key = static_cast<double>(i);
        }
    }
    
    return max_key;
}

bool TableLibrary::DefaultCompare(const LuaValue& a, const LuaValue& b) {
    // 默认排序：数字 < 字符串，同类型按值比较
    if (a.GetType() != b.GetType()) {
        return static_cast<int>(a.GetType()) < static_cast<int>(b.GetType());
    }
    
    switch (a.GetType()) {
        case LuaValueType::NUMBER:
            return a.AsNumber() < b.AsNumber();
        case LuaValueType::STRING:
            return a.AsString() < b.AsString();
        case LuaValueType::BOOLEAN:
            return !a.AsBoolean() && b.AsBoolean();  // false < true
        default:
            return false;  // 其他类型认为相等
    }
}

void TableLibrary::QuickSort(LuaTable& table, CompareFunction compare, int start, int end) {
    if (start < end) {
        int pivot = Partition(table, compare, start, end);
        QuickSort(table, compare, start, pivot - 1);
        QuickSort(table, compare, pivot + 1, end);
    }
}

int TableLibrary::Partition(LuaTable& table, CompareFunction compare, int low, int high) {
    // 选择最后一个元素作为枢轴
    LuaValue pivot_key = LuaValue::CreateNumber(static_cast<double>(high));
    LuaValue pivot = table.GetField(pivot_key);
    
    int i = low - 1;  // 较小元素的索引
    
    for (int j = low; j < high; j++) {
        LuaValue j_key = LuaValue::CreateNumber(static_cast<double>(j));
        LuaValue j_value = table.GetField(j_key);
        
        if (compare(j_value, pivot)) {
            i++;
            SwapTableElements(table, i, j);
        }
    }
    
    SwapTableElements(table, i + 1, high);
    return i + 1;
}

void TableLibrary::SwapTableElements(LuaTable& table, int i, int j) {
    LuaValue i_key = LuaValue::CreateNumber(static_cast<double>(i));
    LuaValue j_key = LuaValue::CreateNumber(static_cast<double>(j));
    
    LuaValue i_value = table.GetField(i_key);
    LuaValue j_value = table.GetField(j_key);
    
    table.SetField(i_key, j_value);
    table.SetField(j_key, i_value);
}

// ============================================================================
// 简化实现的其他函数
// ============================================================================

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_foreach) {
    StackHelper helper(vm);
    // TODO: 实现完整的foreach函数
    vm->GetStack().clear();
    return 0;
}

LUA_STDLIB_FUNCTION(TableLibrary::lua_table_foreachi) {
    StackHelper helper(vm);
    // TODO: 实现完整的foreachi函数
    vm->GetStack().clear();
    return 0;
}

} // namespace stdlib
} // namespace lua_cpp