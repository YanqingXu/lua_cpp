#!/bin/bash

# lua_with_cpp质量检查脚本
# 用于验证lua_cpp的代码质量是否符合lua_with_cpp的标准

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
LUA_WITH_CPP_PATH="${PROJECT_ROOT}/../lua_with_cpp"

echo "🏗️ lua_with_cpp质量检查开始..."

# 检查参考项目路径
if [ ! -d "$LUA_WITH_CPP_PATH" ]; then
    echo "❌ 错误: 找不到lua_with_cpp项目路径: $LUA_WITH_CPP_PATH"
    echo "请确保lua_with_cpp项目位于正确的相对路径"
    exit 1
fi

# 创建质量检查报告目录
QUALITY_DIR="${PROJECT_ROOT}/verification/quality_checks"
mkdir -p "$QUALITY_DIR"

echo "📊 开始代码质量分析..."

# 1. 代码格式检查
echo ""
echo "1. 代码格式检查 (clang-format)"
echo "----------------------------------------"

format_issues=0
if command -v clang-format >/dev/null 2>&1; then
    echo "检查C++代码格式..."
    find "${PROJECT_ROOT}/src" -name "*.cpp" -o -name "*.h" -o -name "*.hpp" | while read -r file; do
        if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
            echo "❌ 格式问题: $file"
            format_issues=$((format_issues + 1))
        fi
    done
    
    if [ $format_issues -eq 0 ]; then
        echo "✅ 代码格式检查通过"
    else
        echo "⚠️  发现 $format_issues 个格式问题"
    fi
else
    echo "⚠️  clang-format未安装，跳过格式检查"
fi

# 2. 静态分析检查
echo ""
echo "2. 静态分析检查 (clang-tidy)"
echo "----------------------------------------"

if command -v clang-tidy >/dev/null 2>&1; then
    echo "执行静态分析..."
    
    # 创建clang-tidy配置（基于lua_with_cpp的标准）
    cat > "${PROJECT_ROOT}/.clang-tidy" << 'EOF'
Checks: >
  *,
  -fuchsia-*,
  -google-build-using-namespace,
  -llvm-header-guard,
  -misc-non-private-member-variables-in-classes,
  -modernize-use-trailing-return-type,
  -readability-named-parameter,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers

WarningsAsErrors: ''
HeaderFilterRegex: '.*'
FormatStyle: 'file'

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelCase
  - key: readability-identifier-naming.VariableCase
    value: camelBack
  - key: readability-identifier-naming.ConstantCase
    value: UPPER_CASE
EOF

    tidy_output="${QUALITY_DIR}/clang_tidy_report.txt"
    find "${PROJECT_ROOT}/src" -name "*.cpp" | head -5 | xargs clang-tidy > "$tidy_output" 2>&1 || true
    
    if [ -s "$tidy_output" ]; then
        echo "⚠️  静态分析发现问题，详见: $tidy_output"
        head -20 "$tidy_output"
    else
        echo "✅ 静态分析检查通过"
    fi
else
    echo "⚠️  clang-tidy未安装，跳过静态分析"
fi

# 3. 现代C++特性使用检查
echo ""
echo "3. 现代C++特性使用检查"
echo "----------------------------------------"

modern_cpp_score=0
total_checks=0

# 检查智能指针使用
total_checks=$((total_checks + 1))
if grep -r "std::unique_ptr\|std::shared_ptr" "${PROJECT_ROOT}/src" >/dev/null 2>&1; then
    echo "✅ 使用了智能指针"
    modern_cpp_score=$((modern_cpp_score + 1))
else
    echo "❌ 未发现智能指针使用"
fi

# 检查RAII模式
total_checks=$((total_checks + 1))
if grep -r "class.*{" "${PROJECT_ROOT}/src" | grep -c "~" >/dev/null 2>&1; then
    echo "✅ 使用了RAII模式（析构函数）"
    modern_cpp_score=$((modern_cpp_score + 1))
else
    echo "❌ 未发现RAII模式使用"
fi

# 检查constexpr使用
total_checks=$((total_checks + 1))
if grep -r "constexpr" "${PROJECT_ROOT}/src" >/dev/null 2>&1; then
    echo "✅ 使用了constexpr"
    modern_cpp_score=$((modern_cpp_score + 1))
else
    echo "❌ 未发现constexpr使用"
fi

# 检查auto关键字
total_checks=$((total_checks + 1))
if grep -r "auto " "${PROJECT_ROOT}/src" >/dev/null 2>&1; then
    echo "✅ 使用了auto关键字"
    modern_cpp_score=$((modern_cpp_score + 1))
else
    echo "❌ 未发现auto关键字使用"
fi

# 检查范围for循环
total_checks=$((total_checks + 1))
if grep -r "for.*:" "${PROJECT_ROOT}/src" >/dev/null 2>&1; then
    echo "✅ 使用了范围for循环"
    modern_cpp_score=$((modern_cpp_score + 1))
else
    echo "❌ 未发现范围for循环使用"
fi

echo "现代C++特性得分: $modern_cpp_score/$total_checks"

# 4. 架构模式检查
echo ""
echo "4. 架构模式检查"
echo "----------------------------------------"

# 检查模块化结构
if [ -d "${PROJECT_ROOT}/src/core" ] && [ -d "${PROJECT_ROOT}/src/vm" ] && [ -d "${PROJECT_ROOT}/src/gc" ]; then
    echo "✅ 模块化结构良好"
else
    echo "❌ 缺少清晰的模块化结构"
fi

# 检查接口分离
if find "${PROJECT_ROOT}/src" -name "*.h" -o -name "*.hpp" | grep -c "interface\|api" >/dev/null 2>&1; then
    echo "✅ 使用了接口分离"
else
    echo "❌ 缺少明确的接口定义"
fi

# 检查测试覆盖
if [ -d "${PROJECT_ROOT}/tests" ]; then
    test_files=$(find "${PROJECT_ROOT}/tests" -name "*.cpp" | wc -l)
    echo "✅ 测试文件数量: $test_files"
else
    echo "❌ 缺少测试目录"
fi

# 5. 与lua_with_cpp的架构对比
echo ""
echo "5. 与lua_with_cpp架构对比"
echo "----------------------------------------"

if [ -d "$LUA_WITH_CPP_PATH/src" ]; then
    echo "参考项目结构分析:"
    echo "lua_with_cpp模块:"
    ls -1 "$LUA_WITH_CPP_PATH/src" 2>/dev/null | head -10 | sed 's/^/  - /'
    
    echo ""
    echo "lua_cpp模块:"
    ls -1 "${PROJECT_ROOT}/src" 2>/dev/null | head -10 | sed 's/^/  - /'
    
    # 比较关键模块是否存在
    key_modules=("core" "vm" "gc" "api" "types")
    missing_modules=()
    
    for module in "${key_modules[@]}"; do
        if [ ! -d "${PROJECT_ROOT}/src/$module" ]; then
            missing_modules+=("$module")
        fi
    done
    
    if [ ${#missing_modules[@]} -eq 0 ]; then
        echo "✅ 所有关键模块都存在"
    else
        echo "❌ 缺少关键模块: ${missing_modules[*]}"
    fi
else
    echo "⚠️  无法访问lua_with_cpp结构"
fi

# 生成质量报告
echo ""
echo "========================================="
echo "🏗️ lua_with_cpp质量检查报告"
echo "========================================="

# 计算总体质量得分
quality_score=0
max_score=5

if [ $format_issues -eq 0 ]; then quality_score=$((quality_score + 1)); fi
if [ $modern_cpp_score -ge 3 ]; then quality_score=$((quality_score + 1)); fi
if [ -d "${PROJECT_ROOT}/src/core" ]; then quality_score=$((quality_score + 1)); fi
if [ -d "${PROJECT_ROOT}/tests" ]; then quality_score=$((quality_score + 1)); fi
if [ ${#missing_modules[@]} -eq 0 ]; then quality_score=$((quality_score + 1)); fi

echo "总体质量得分: $quality_score/$max_score"
echo "现代C++特性使用: $modern_cpp_score/$total_checks"

if [ $quality_score -ge 4 ]; then
    echo "🎉 质量检查通过！符合lua_with_cpp标准"
    exit 0
else
    echo "⚠️  质量需要改进，建议参考lua_with_cpp的实现"
    exit 1
fi