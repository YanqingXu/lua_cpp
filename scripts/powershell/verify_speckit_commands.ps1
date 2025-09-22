# lua_cpp Spec-Kit 指令统一验证脚本 (PowerShell版本)
# 验证项目是否只有唯一的一套spec-kit指令

$ErrorActionPreference = "Stop"

$PROJECT_ROOT = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$CLAUDE_COMMANDS_DIR = Join-Path $PROJECT_ROOT ".claude" "commands"

Write-Host "=== lua_cpp Spec-Kit 指令统一验证 ===" -ForegroundColor Cyan
Write-Host "验证唯一指令集位置: $CLAUDE_COMMANDS_DIR" -ForegroundColor Yellow

# 检查必需的指令文件
$REQUIRED_COMMANDS = @("constitution.md", "specify.md", "plan.md", "tasks.md", "implement.md")
Write-Host ""
Write-Host "检查必需指令文件:" -ForegroundColor White

$all_present = $true
foreach ($cmd in $REQUIRED_COMMANDS) {
    $cmd_file = Join-Path $CLAUDE_COMMANDS_DIR $cmd
    if (Test-Path $cmd_file) {
        Write-Host "✅ $cmd - 存在" -ForegroundColor Green
    } else {
        Write-Host "❌ $cmd - 缺失" -ForegroundColor Red
        $all_present = $false
    }
}

# 检查是否有重复的指令配置
Write-Host ""
Write-Host "检查重复指令配置:" -ForegroundColor White

$duplicate_found = $false

# 检查可能的重复位置
$POTENTIAL_DUPLICATES = @(
    ".github\prompts",
    "commands", 
    "templates\commands"
)

foreach ($dir in $POTENTIAL_DUPLICATES) {
    $full_path = Join-Path $PROJECT_ROOT $dir
    if (Test-Path $full_path) {
        Write-Host "⚠️  发现重复目录: $dir" -ForegroundColor Yellow
        $duplicate_found = $true
        
        # 列出该目录中的文件
        Write-Host "   包含文件:" -ForegroundColor Gray
        foreach ($cmd in $REQUIRED_COMMANDS) {
            $cmd_path = Join-Path $full_path $cmd
            if (Test-Path $cmd_path) {
                Write-Host "   - $cmd" -ForegroundColor Gray
            }
        }
    } else {
        Write-Host "✅ 无重复目录: $dir" -ForegroundColor Green
    }
}

# 检查指令文件内容完整性
Write-Host ""
Write-Host "检查指令文件内容完整性:" -ForegroundColor White

$content_valid = $true
foreach ($cmd in $REQUIRED_COMMANDS) {
    $cmd_file = Join-Path $CLAUDE_COMMANDS_DIR $cmd
    if (Test-Path $cmd_file) {
        # 检查文件是否有基本的yaml front matter
        $first_line = Get-Content $cmd_file -First 1
        if ($first_line -match "^---") {
            Write-Host "✅ $cmd - 格式正确" -ForegroundColor Green
        } else {
            Write-Host "❌ $cmd - 格式错误 (缺少YAML front matter)" -ForegroundColor Red
            $content_valid = $false
        }
        
        # 检查文件大小是否合理（至少500字节）
        $file_size = (Get-Item $cmd_file).Length
        if ($file_size -gt 500) {
            Write-Host "✅ $cmd - 内容充实 ($file_size 字节)" -ForegroundColor Green
        } else {
            Write-Host "⚠️  $cmd - 内容可能不完整 ($file_size 字节)" -ForegroundColor Yellow
        }
    }
}

# 验证lua_cpp项目特化
Write-Host ""
Write-Host "验证lua_cpp项目特化:" -ForegroundColor White

$project_specific = $true
foreach ($cmd in $REQUIRED_COMMANDS) {
    $cmd_file = Join-Path $CLAUDE_COMMANDS_DIR $cmd
    if (Test-Path $cmd_file) {
        $content = Get-Content $cmd_file -Raw
        
        # 检查是否包含lua_cpp相关内容
        if ($content -match "(?i)lua_cpp|lua.*c\+\+|modern.*c\+\+.*lua") {
            Write-Host "✅ $cmd - 包含项目特化内容" -ForegroundColor Green
        } else {
            Write-Host "⚠️  $cmd - 可能缺少项目特化内容" -ForegroundColor Yellow
            $project_specific = $false
        }
        
        # 检查是否引用双重参考项目
        if ($content -match "(?i)lua_c_analysis|lua_with_cpp") {
            Write-Host "✅ $cmd - 包含双重参考项目引用" -ForegroundColor Green
        } else {
            Write-Host "⚠️  $cmd - 缺少双重参考项目引用" -ForegroundColor Yellow
        }
    }
}

# 生成验证报告
Write-Host ""
Write-Host "=== 验证报告汇总 ===" -ForegroundColor Cyan

if ($all_present -and (-not $duplicate_found) -and $content_valid -and $project_specific) {
    Write-Host "🎉 验证通过！" -ForegroundColor Green
    Write-Host "✅ 所有必需指令文件存在" -ForegroundColor Green
    Write-Host "✅ 无重复指令配置" -ForegroundColor Green
    Write-Host "✅ 指令内容格式正确" -ForegroundColor Green
    Write-Host "✅ 包含项目特化内容" -ForegroundColor Green
    Write-Host ""
    Write-Host "lua_cpp项目现在拥有统一的spec-kit指令集。" -ForegroundColor White
    Write-Host "位置: .claude/commands/" -ForegroundColor Yellow
    Write-Host "使用方法: 在Claude中输入 /constitution, /specify, /plan, /tasks, /implement" -ForegroundColor Yellow
    exit 0
} else {
    Write-Host "❌ 验证失败！" -ForegroundColor Red
    
    if (-not $all_present) {
        Write-Host "❌ 缺少必需的指令文件" -ForegroundColor Red
    }
    
    if ($duplicate_found) {
        Write-Host "❌ 存在重复的指令配置" -ForegroundColor Red
    }
    
    if (-not $content_valid) {
        Write-Host "❌ 指令内容格式不正确" -ForegroundColor Red
    }
    
    if (-not $project_specific) {
        Write-Host "❌ 缺少项目特化内容" -ForegroundColor Red
    }
    
    Write-Host ""
    Write-Host "请修复上述问题后重新运行验证。" -ForegroundColor Yellow
    exit 1
}