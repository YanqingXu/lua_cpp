#!/bin/bash

# lua_c_analysis行为验证脚本
# 用于验证lua_cpp的实现与lua_c_analysis的行为一致性

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
LUA_C_ANALYSIS_PATH="${PROJECT_ROOT}/../lua_c_analysis"

echo "🔍 lua_c_analysis行为验证开始..."

# 检查参考项目路径
if [ ! -d "$LUA_C_ANALYSIS_PATH" ]; then
    echo "❌ 错误: 找不到lua_c_analysis项目路径: $LUA_C_ANALYSIS_PATH"
    echo "请确保lua_c_analysis项目位于正确的相对路径"
    exit 1
fi

# 检查lua_cpp构建状态
LUA_CPP_BINARY="${PROJECT_ROOT}/build/bin/lua_cpp"
if [ ! -f "$LUA_CPP_BINARY" ]; then
    echo "❌ 错误: 找不到lua_cpp二进制文件: $LUA_CPP_BINARY"
    echo "请先构建lua_cpp项目"
    exit 1
fi

# 检查lua_c_analysis构建状态
LUA_C_BINARY="${LUA_C_ANALYSIS_PATH}/lua"
if [ ! -f "$LUA_C_BINARY" ]; then
    echo "⚠️  警告: 找不到lua_c_analysis二进制文件，尝试构建..."
    cd "$LUA_C_ANALYSIS_PATH"
    make || {
        echo "❌ 错误: 无法构建lua_c_analysis"
        exit 1
    }
    cd "$PROJECT_ROOT"
fi

# 创建测试目录
TEST_DIR="${PROJECT_ROOT}/verification/behavior_tests"
mkdir -p "$TEST_DIR"

echo "📝 生成行为验证测试用例..."

# 基础语法测试
cat > "$TEST_DIR/basic_syntax.lua" << 'EOF'
-- 基础语法测试
print("Hello, Lua!")

-- 变量和类型
local x = 42
local y = "string"
local z = true
local t = {1, 2, 3}

print(type(x), type(y), type(z), type(t))

-- 控制流
for i = 1, 3 do
    print("Loop:", i)
end

if x > 40 then
    print("Greater than 40")
end

-- 函数
function test_func(a, b)
    return a + b
end

print("Function result:", test_func(10, 20))
EOF

# 表操作测试
cat > "$TEST_DIR/table_operations.lua" << 'EOF'
-- 表操作测试
local t = {}
t[1] = "first"
t[2] = "second"
t["key"] = "value"

print("Table length:", #t)
print("Table key access:", t["key"])

-- 表遍历
for k, v in pairs(t) do
    print("Key:", k, "Value:", v)
end

-- 元表测试
local mt = {
    __index = function(t, k)
        return "default_" .. tostring(k)
    end
}

setmetatable(t, mt)
print("Missing key:", t["missing"])
EOF

# 字符串操作测试
cat > "$TEST_DIR/string_operations.lua" << 'EOF'
-- 字符串操作测试
local s1 = "Hello"
local s2 = "World"
local s3 = s1 .. " " .. s2

print("Concatenation:", s3)
print("Length:", string.len(s3))
print("Uppercase:", string.upper(s3))
print("Substring:", string.sub(s3, 1, 5))

-- 字符串查找
local pos = string.find(s3, "World")
print("Find position:", pos)
EOF

# 执行验证测试
echo "🚀 执行行为验证测试..."

test_count=0
pass_count=0
fail_count=0

for test_file in "$TEST_DIR"/*.lua; do
    test_name=$(basename "$test_file" .lua)
    echo ""
    echo "测试: $test_name"
    echo "----------------------------------------"
    
    test_count=$((test_count + 1))
    
    # 获取lua_c_analysis输出
    echo "🔍 lua_c_analysis输出:"
    lua_c_output_file="${TEST_DIR}/${test_name}_lua_c.out"
    "$LUA_C_BINARY" "$test_file" > "$lua_c_output_file" 2>&1 || {
        echo "❌ lua_c_analysis执行失败"
        cat "$lua_c_output_file"
        fail_count=$((fail_count + 1))
        continue
    }
    cat "$lua_c_output_file"
    
    # 获取lua_cpp输出
    echo ""
    echo "🏗️ lua_cpp输出:"
    lua_cpp_output_file="${TEST_DIR}/${test_name}_lua_cpp.out"
    "$LUA_CPP_BINARY" "$test_file" > "$lua_cpp_output_file" 2>&1 || {
        echo "❌ lua_cpp执行失败"
        cat "$lua_cpp_output_file"
        fail_count=$((fail_count + 1))
        continue
    }
    cat "$lua_cpp_output_file"
    
    # 比较输出
    echo ""
    echo "📊 输出比较:"
    if diff -u "$lua_c_output_file" "$lua_cpp_output_file" > "${TEST_DIR}/${test_name}_diff.out"; then
        echo "✅ 输出一致"
        pass_count=$((pass_count + 1))
    else
        echo "❌ 输出不一致"
        echo "差异详情:"
        cat "${TEST_DIR}/${test_name}_diff.out"
        fail_count=$((fail_count + 1))
    fi
done

# 生成验证报告
echo ""
echo "========================================="
echo "🔍 lua_c_analysis行为验证报告"
echo "========================================="
echo "总测试数: $test_count"
echo "通过测试: $pass_count"
echo "失败测试: $fail_count"
echo "通过率: $(( pass_count * 100 / test_count ))%"

if [ $fail_count -eq 0 ]; then
    echo "🎉 所有测试通过！lua_cpp与lua_c_analysis行为一致"
    exit 0
else
    echo "⚠️  有测试失败，需要修复lua_cpp实现"
    exit 1
fi