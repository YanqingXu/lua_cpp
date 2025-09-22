#!/bin/bash

# lua_cpp Spec-Kit 指令统一验证脚本
# 验证项目是否只有唯一的一套spec-kit指令

set -e

PROJECT_ROOT="$(dirname "$0")/../.."
CLAUDE_COMMANDS_DIR="$PROJECT_ROOT/.claude/commands"

echo "=== lua_cpp Spec-Kit 指令统一验证 ==="
echo "验证唯一指令集位置: $CLAUDE_COMMANDS_DIR"

# 检查必需的指令文件
REQUIRED_COMMANDS=("constitution.md" "specify.md" "plan.md" "tasks.md" "implement.md")
echo ""
echo "检查必需指令文件:"

all_present=true
for cmd in "${REQUIRED_COMMANDS[@]}"; do
    cmd_file="$CLAUDE_COMMANDS_DIR/$cmd"
    if [[ -f "$cmd_file" ]]; then
        echo "✅ $cmd - 存在"
    else
        echo "❌ $cmd - 缺失"
        all_present=false
    fi
done

# 检查是否有重复的指令配置
echo ""
echo "检查重复指令配置:"

duplicate_found=false

# 检查可能的重复位置
POTENTIAL_DUPLICATES=(
    ".github/prompts"
    "commands" 
    "templates/commands"
)

for dir in "${POTENTIAL_DUPLICATES[@]}"; do
    full_path="$PROJECT_ROOT/$dir"
    if [[ -d "$full_path" ]]; then
        echo "⚠️  发现重复目录: $dir"
        duplicate_found=true
        
        # 列出该目录中的文件
        echo "   包含文件:"
        for cmd in "${REQUIRED_COMMANDS[@]}"; do
            if [[ -f "$full_path/$cmd" ]]; then
                echo "   - $cmd"
            fi
        done
    else
        echo "✅ 无重复目录: $dir"
    fi
done

# 检查指令文件内容完整性
echo ""
echo "检查指令文件内容完整性:"

content_valid=true
for cmd in "${REQUIRED_COMMANDS[@]}"; do
    cmd_file="$CLAUDE_COMMANDS_DIR/$cmd"
    if [[ -f "$cmd_file" ]]; then
        # 检查文件是否有基本的yaml front matter
        if head -n 1 "$cmd_file" | grep -q "^---"; then
            echo "✅ $cmd - 格式正确"
        else
            echo "❌ $cmd - 格式错误 (缺少YAML front matter)"
            content_valid=false
        fi
        
        # 检查文件大小是否合理（至少500字节）
        file_size=$(wc -c < "$cmd_file")
        if [[ $file_size -gt 500 ]]; then
            echo "✅ $cmd - 内容充实 ($file_size 字节)"
        else
            echo "⚠️  $cmd - 内容可能不完整 ($file_size 字节)"
        fi
    fi
done

# 验证lua_cpp项目特化
echo ""
echo "验证lua_cpp项目特化:"

project_specific=true
for cmd in "${REQUIRED_COMMANDS[@]}"; do
    cmd_file="$CLAUDE_COMMANDS_DIR/$cmd"
    if [[ -f "$cmd_file" ]]; then
        # 检查是否包含lua_cpp相关内容
        if grep -q -i "lua_cpp\|lua.*c++\|modern.*c++.*lua" "$cmd_file"; then
            echo "✅ $cmd - 包含项目特化内容"
        else
            echo "⚠️  $cmd - 可能缺少项目特化内容"
            project_specific=false
        fi
        
        # 检查是否引用双重参考项目
        if grep -q -i "lua_c_analysis\|lua_with_cpp" "$cmd_file"; then
            echo "✅ $cmd - 包含双重参考项目引用"
        else
            echo "⚠️  $cmd - 缺少双重参考项目引用"
        fi
    fi
done

# 生成验证报告
echo ""
echo "=== 验证报告汇总 ==="

if $all_present && ! $duplicate_found && $content_valid && $project_specific; then
    echo "🎉 验证通过！"
    echo "✅ 所有必需指令文件存在"
    echo "✅ 无重复指令配置"
    echo "✅ 指令内容格式正确"
    echo "✅ 包含项目特化内容"
    echo ""
    echo "lua_cpp项目现在拥有统一的spec-kit指令集。"
    echo "位置: .claude/commands/"
    echo "使用方法: 在Claude中输入 /constitution, /specify, /plan, /tasks, /implement"
    exit 0
else
    echo "❌ 验证失败！"
    
    if ! $all_present; then
        echo "❌ 缺少必需的指令文件"
    fi
    
    if $duplicate_found; then
        echo "❌ 存在重复的指令配置"
    fi
    
    if ! $content_valid; then
        echo "❌ 指令内容格式不正确"
    fi
    
    if ! $project_specific; then
        echo "❌ 缺少项目特化内容"
    fi
    
    echo ""
    echo "请修复上述问题后重新运行验证。"
    exit 1
fi