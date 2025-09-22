# lua_cpp Spec-Kit 指令统一验证脚本 (简化版)

$PROJECT_ROOT = "e:\Programming\spec-kit-lua\lua_cpp"
$CLAUDE_COMMANDS_DIR = "$PROJECT_ROOT\.claude\commands"

Write-Host "=== lua_cpp Spec-Kit 指令统一验证 ===" -ForegroundColor Cyan
Write-Host "验证唯一指令集位置: $CLAUDE_COMMANDS_DIR" -ForegroundColor Yellow

# 检查必需的指令文件
$REQUIRED_COMMANDS = @("constitution.md", "specify.md", "plan.md", "tasks.md", "implement.md")
Write-Host ""
Write-Host "检查必需指令文件:" -ForegroundColor White

$all_present = $true
foreach ($cmd in $REQUIRED_COMMANDS) {
    $cmd_file = "$CLAUDE_COMMANDS_DIR\$cmd"
    if (Test-Path $cmd_file) {
        $size = (Get-Item $cmd_file).Length
        Write-Host "✅ $cmd - 存在 ($size 字节)" -ForegroundColor Green
    } else {
        Write-Host "❌ $cmd - 缺失" -ForegroundColor Red
        $all_present = $false
    }
}

# 检查重复配置
Write-Host ""
Write-Host "检查重复指令配置:" -ForegroundColor White

$duplicate_dirs = @("$PROJECT_ROOT\.github\prompts", "$PROJECT_ROOT\commands")
$duplicate_found = $false

foreach ($dir in $duplicate_dirs) {
    if (Test-Path $dir) {
        Write-Host "⚠️  发现重复目录: $dir" -ForegroundColor Yellow
        $duplicate_found = $true
    } else {
        Write-Host "✅ 无重复目录: $dir" -ForegroundColor Green
    }
}

# 验证报告
Write-Host ""
Write-Host "=== 验证报告汇总 ===" -ForegroundColor Cyan

if ($all_present -and -not $duplicate_found) {
    Write-Host "🎉 验证通过！lua_cpp项目现在拥有统一的spec-kit指令集。" -ForegroundColor Green
    Write-Host "位置: .claude/commands/" -ForegroundColor Yellow
    Write-Host "使用方法: 在Claude中输入 /constitution, /specify, /plan, /tasks, /implement" -ForegroundColor Yellow
} else {
    Write-Host "❌ 验证失败！请检查上述问题。" -ForegroundColor Red
}