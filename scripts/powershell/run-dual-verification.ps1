# 双重验证机制统一执行脚本
# 同时运行lua_c_analysis行为验证和lua_with_cpp质量检查

param(
    [switch]$SkipBehavior = $false,
    [switch]$SkipQuality = $false,
    [switch]$ContinueOnError = $false
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)

Write-Host "🔍🏗️ lua_cpp双重验证机制启动" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor White

$TotalTests = 0
$PassedTests = 0
$FailedTests = 0

# 1. lua_c_analysis行为验证
if (-not $SkipBehavior) {
    Write-Host ""
    Write-Host "🔍 执行lua_c_analysis行为验证..." -ForegroundColor Blue
    Write-Host "----------------------------------------"
    
    $TotalTests++
    try {
        $BehaviorScript = Join-Path $PSScriptRoot "verify-lua-c-analysis.ps1"
        & $BehaviorScript
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✅ lua_c_analysis行为验证通过" -ForegroundColor Green
            $PassedTests++
        } else {
            throw "行为验证失败"
        }
    } catch {
        Write-Host "❌ lua_c_analysis行为验证失败: $_" -ForegroundColor Red
        $FailedTests++
        
        if (-not $ContinueOnError) {
            Write-Host "停止验证流程（使用 -ContinueOnError 继续）"
            exit 1
        }
    }
} else {
    Write-Host "⏭️  跳过lua_c_analysis行为验证" -ForegroundColor Yellow
}

# 2. lua_with_cpp质量检查
if (-not $SkipQuality) {
    Write-Host ""
    Write-Host "🏗️ 执行lua_with_cpp质量检查..." -ForegroundColor Green
    Write-Host "----------------------------------------"
    
    $TotalTests++
    try {
        $QualityScript = Join-Path $PSScriptRoot "verify-lua-with-cpp.ps1"
        & $QualityScript
        
        if ($LASTEXITCODE -eq 0) {
            Write-Host "✅ lua_with_cpp质量检查通过" -ForegroundColor Green
            $PassedTests++
        } else {
            throw "质量检查失败"
        }
    } catch {
        Write-Host "❌ lua_with_cpp质量检查失败: $_" -ForegroundColor Red
        $FailedTests++
        
        if (-not $ContinueOnError) {
            Write-Host "停止验证流程（使用 -ContinueOnError 继续）"
            exit 1
        }
    }
} else {
    Write-Host "⏭️  跳过lua_with_cpp质量检查" -ForegroundColor Yellow
}

# 3. 生成综合报告
Write-Host ""
Write-Host "========================================" -ForegroundColor White
Write-Host "📊 双重验证综合报告" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor White

Write-Host "验证项目总数: $TotalTests"
Write-Host "通过验证: $PassedTests" -ForegroundColor Green
Write-Host "失败验证: $FailedTests" -ForegroundColor Red

if ($TotalTests -gt 0) {
    $SuccessRate = [Math]::Round(($PassedTests * 100.0 / $TotalTests), 1)
    Write-Host "成功率: $SuccessRate%"
}

# 生成验证徽章
$BadgeText = ""
if ($PassedTests -eq $TotalTests) {
    $BadgeText = "🎉 全部验证通过！"
    $BadgeColor = "Green"
} elseif ($PassedTests -gt 0) {
    $BadgeText = "⚠️  部分验证通过"
    $BadgeColor = "Yellow"
} else {
    $BadgeText = "❌ 验证失败"
    $BadgeColor = "Red"
}

Write-Host ""
Write-Host $BadgeText -ForegroundColor $BadgeColor

# 生成建议
if ($FailedTests -gt 0) {
    Write-Host ""
    Write-Host "🔧 改进建议:" -ForegroundColor Cyan
    
    if (-not $SkipBehavior -and $FailedTests -gt 0) {
        Write-Host "  - 检查lua_cpp实现是否与lua_c_analysis行为一致"
        Write-Host "  - 参考lua_c_analysis的C源码注释进行修正"
    }
    
    if (-not $SkipQuality -and $FailedTests -gt 0) {
        Write-Host "  - 提升代码质量以符合lua_with_cpp标准"
        Write-Host "  - 增加现代C++特性使用"
        Write-Host "  - 改善模块化架构设计"
    }
}

# 保存综合报告
$ReportDir = Join-Path $ProjectRoot "verification"
if (-not (Test-Path $ReportDir)) {
    New-Item -Path $ReportDir -ItemType Directory -Force | Out-Null
}

$ReportFile = Join-Path $ReportDir "dual_verification_report.md"
$ReportContent = @"
# lua_cpp双重验证报告

**生成时间**: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")

## 验证概览

| 验证项目 | 状态 | 结果 |
|---------|------|------|
$(if (-not $SkipBehavior) { "| lua_c_analysis行为验证 | $(if ($PassedTests -ge 1 -and -not $SkipQuality) { '✅' } elseif ($PassedTests -eq $TotalTests) { '✅' } else { '❌' }) | $(if ($PassedTests -ge 1 -and -not $SkipQuality) { '通过' } elseif ($PassedTests -eq $TotalTests) { '通过' } else { '失败' }) |" })
$(if (-not $SkipQuality) { "| lua_with_cpp质量检查 | $(if ($PassedTests -eq $TotalTests) { '✅' } else { '❌' }) | $(if ($PassedTests -eq $TotalTests) { '通过' } else { '失败' }) |" })

## 统计信息

- **验证项目总数**: $TotalTests
- **通过验证**: $PassedTests
- **失败验证**: $FailedTests
- **成功率**: $(if ($TotalTests -gt 0) { [Math]::Round(($PassedTests * 100.0 / $TotalTests), 1) } else { 0 })%

## 验证状态

$BadgeText

## 下一步行动

$(if ($FailedTests -gt 0) {
"### 需要改进的方面

$(if (-not $SkipBehavior) { "- 🔍 **行为一致性**: 确保lua_cpp与lua_c_analysis的行为完全一致" })
$(if (-not $SkipQuality) { "- 🏗️ **代码质量**: 提升代码质量以符合lua_with_cpp标准" })

### 具体建议

1. 深入研读lua_c_analysis的实现细节和注释
2. 参考lua_with_cpp的现代C++架构设计
3. 增加测试覆盖率和代码注释
4. 遵循现代C++最佳实践"
} else {
"### 验证通过 🎉

lua_cpp项目成功通过了双重验证：
- ✅ 与lua_c_analysis的行为完全一致
- ✅ 符合lua_with_cpp的质量标准

项目已达到生产级别的质量要求。"
})

---
*报告由双重验证机制自动生成*
"@

$ReportContent | Out-File -FilePath $ReportFile -Encoding UTF8
Write-Host ""
Write-Host "📄 详细报告已保存至: $ReportFile" -ForegroundColor Gray

# 返回适当的退出码
if ($FailedTests -eq 0) {
    exit 0
} else {
    exit 1
}