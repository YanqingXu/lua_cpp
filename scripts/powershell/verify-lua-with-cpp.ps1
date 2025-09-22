# lua_with_cpp质量检查脚本 (PowerShell版本)
# 用于验证lua_cpp的代码质量是否符合lua_with_cpp的标准

param(
    [switch]$Detailed = $false
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$LuaWithCppPath = Join-Path (Split-Path -Parent $ProjectRoot) "lua_with_cpp"

Write-Host "🏗️ lua_with_cpp质量检查开始..." -ForegroundColor Green

# 检查参考项目路径
if (-not (Test-Path $LuaWithCppPath)) {
    Write-Warning "找不到lua_with_cpp项目路径: $LuaWithCppPath"
    Write-Host "将基于内置标准进行质量检查"
}

# 创建质量检查报告目录
$QualityDir = Join-Path $ProjectRoot "verification\quality_checks"
if (-not (Test-Path $QualityDir)) {
    New-Item -Path $QualityDir -ItemType Directory -Force | Out-Null
}

Write-Host "📊 开始代码质量分析..." -ForegroundColor Cyan

# 1. 代码格式检查
Write-Host ""
Write-Host "1. 代码格式检查" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$FormatIssues = 0
$CppFiles = Get-ChildItem -Path (Join-Path $ProjectRoot "src") -Include "*.cpp", "*.h", "*.hpp" -Recurse -ErrorAction SilentlyContinue

if ($CppFiles.Count -eq 0) {
    Write-Host "⚠️  未找到C++源文件" -ForegroundColor Yellow
} else {
    Write-Host "检查 $($CppFiles.Count) 个C++文件..."
    
    # 检查基本格式规范
    foreach ($File in $CppFiles) {
        $Content = Get-Content $File.FullName -Raw
        
        # 检查行尾空格
        if ($Content -match '\s+$') {
            Write-Host "❌ 行尾空格: $($File.Name)" -ForegroundColor Red
            $FormatIssues++
        }
        
        # 检查制表符vs空格
        if ($Content -match '\t') {
            Write-Host "⚠️  包含制表符: $($File.Name)" -ForegroundColor Yellow
        }
    }
    
    if ($FormatIssues -eq 0) {
        Write-Host "✅ 代码格式检查通过" -ForegroundColor Green
    } else {
        Write-Host "⚠️  发现 $FormatIssues 个格式问题" -ForegroundColor Yellow
    }
}

# 2. 现代C++特性使用检查
Write-Host ""
Write-Host "2. 现代C++特性使用检查" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$ModernCppScore = 0
$TotalChecks = 0

$SrcContent = ""
if (Test-Path (Join-Path $ProjectRoot "src")) {
    $SrcFiles = Get-ChildItem -Path (Join-Path $ProjectRoot "src") -Include "*.cpp", "*.h", "*.hpp" -Recurse
    foreach ($File in $SrcFiles) {
        $SrcContent += Get-Content $File.FullName -Raw
    }
}

# 检查智能指针使用
$TotalChecks++
if ($SrcContent -match "std::unique_ptr|std::shared_ptr") {
    Write-Host "✅ 使用了智能指针" -ForegroundColor Green
    $ModernCppScore++
} else {
    Write-Host "❌ 未发现智能指针使用" -ForegroundColor Red
}

# 检查RAII模式
$TotalChecks++
if ($SrcContent -match "~\w+\(\)") {
    Write-Host "✅ 使用了RAII模式（析构函数）" -ForegroundColor Green
    $ModernCppScore++
} else {
    Write-Host "❌ 未发现RAII模式使用" -ForegroundColor Red
}

# 检查constexpr使用
$TotalChecks++
if ($SrcContent -match "constexpr") {
    Write-Host "✅ 使用了constexpr" -ForegroundColor Green
    $ModernCppScore++
} else {
    Write-Host "❌ 未发现constexpr使用" -ForegroundColor Red
}

# 检查auto关键字
$TotalChecks++
if ($SrcContent -match "\bauto\b") {
    Write-Host "✅ 使用了auto关键字" -ForegroundColor Green
    $ModernCppScore++
} else {
    Write-Host "❌ 未发现auto关键字使用" -ForegroundColor Red
}

# 检查范围for循环
$TotalChecks++
if ($SrcContent -match "for\s*\([^)]*:\s*[^)]*\)") {
    Write-Host "✅ 使用了范围for循环" -ForegroundColor Green
    $ModernCppScore++
} else {
    Write-Host "❌ 未发现范围for循环使用" -ForegroundColor Red
}

Write-Host "现代C++特性得分: $ModernCppScore/$TotalChecks"

# 3. 架构模式检查
Write-Host ""
Write-Host "3. 架构模式检查" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$ArchitectureScore = 0
$ArchitectureChecks = 0

# 检查模块化结构
$ArchitectureChecks++
$RequiredDirs = @("core", "vm", "gc", "api", "types")
$ExistingDirs = @()
$SrcPath = Join-Path $ProjectRoot "src"

if (Test-Path $SrcPath) {
    $ExistingDirs = Get-ChildItem -Path $SrcPath -Directory | ForEach-Object { $_.Name }
}

$MissingDirs = $RequiredDirs | Where-Object { $_ -notin $ExistingDirs }

if ($MissingDirs.Count -eq 0) {
    Write-Host "✅ 模块化结构良好" -ForegroundColor Green
    $ArchitectureScore++
} else {
    Write-Host "❌ 缺少关键模块: $($MissingDirs -join ', ')" -ForegroundColor Red
}

# 检查接口分离
$ArchitectureChecks++
$InterfaceFiles = Get-ChildItem -Path $SrcPath -Include "*interface*", "*api*" -Recurse -ErrorAction SilentlyContinue
if ($InterfaceFiles.Count -gt 0) {
    Write-Host "✅ 使用了接口分离" -ForegroundColor Green
    $ArchitectureScore++
} else {
    Write-Host "❌ 缺少明确的接口定义" -ForegroundColor Red
}

# 检查测试覆盖
$ArchitectureChecks++
$TestsPath = Join-Path $ProjectRoot "tests"
if (Test-Path $TestsPath) {
    $TestFiles = Get-ChildItem -Path $TestsPath -Include "*.cpp" -Recurse
    Write-Host "✅ 测试文件数量: $($TestFiles.Count)" -ForegroundColor Green
    $ArchitectureScore++
} else {
    Write-Host "❌ 缺少测试目录" -ForegroundColor Red
}

Write-Host "架构模式得分: $ArchitectureScore/$ArchitectureChecks"

# 4. 与lua_with_cpp的架构对比
Write-Host ""
Write-Host "4. 与lua_with_cpp架构对比" -ForegroundColor Yellow
Write-Host "----------------------------------------"

if (Test-Path $LuaWithCppPath) {
    Write-Host "参考项目结构分析:"
    
    $RefSrcPath = Join-Path $LuaWithCppPath "src"
    if (Test-Path $RefSrcPath) {
        Write-Host "lua_with_cpp模块:"
        Get-ChildItem -Path $RefSrcPath -Directory | Select-Object -First 10 | ForEach-Object {
            Write-Host "  - $($_.Name)"
        }
    }
    
    Write-Host ""
    Write-Host "lua_cpp模块:"
    if (Test-Path $SrcPath) {
        Get-ChildItem -Path $SrcPath -Directory | Select-Object -First 10 | ForEach-Object {
            Write-Host "  - $($_.Name)"
        }
    }
    
    # 结构对比
    if ($MissingDirs.Count -eq 0) {
        Write-Host "✅ 所有关键模块都存在" -ForegroundColor Green
    } else {
        Write-Host "❌ 缺少关键模块: $($MissingDirs -join ', ')" -ForegroundColor Red
    }
} else {
    Write-Host "⚠️  无法访问lua_with_cpp结构，使用内置标准" -ForegroundColor Yellow
}

# 5. 文档和注释检查
Write-Host ""
Write-Host "5. 文档和注释检查" -ForegroundColor Yellow
Write-Host "----------------------------------------"

$DocScore = 0
$DocChecks = 0

# 检查README
$DocChecks++
if (Test-Path (Join-Path $ProjectRoot "README.md")) {
    Write-Host "✅ 存在README文档" -ForegroundColor Green
    $DocScore++
} else {
    Write-Host "❌ 缺少README文档" -ForegroundColor Red
}

# 检查代码注释密度
$DocChecks++
if ($SrcContent -match "//|/\*") {
    Write-Host "✅ 代码包含注释" -ForegroundColor Green
    $DocScore++
} else {
    Write-Host "❌ 代码缺少注释" -ForegroundColor Red
}

Write-Host "文档质量得分: $DocScore/$DocChecks"

# 生成质量报告
Write-Host ""
Write-Host "=========================================" -ForegroundColor White
Write-Host "🏗️ lua_with_cpp质量检查报告" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor White

# 计算总体质量得分
$QualityScore = 0
$MaxScore = 5

if ($FormatIssues -eq 0) { $QualityScore++ }
if ($ModernCppScore -ge 3) { $QualityScore++ }
if ($ArchitectureScore -ge 2) { $QualityScore++ }
if ($MissingDirs.Count -eq 0) { $QualityScore++ }
if ($DocScore -ge 1) { $QualityScore++ }

Write-Host "总体质量得分: $QualityScore/$MaxScore"
Write-Host "现代C++特性使用: $ModernCppScore/$TotalChecks"
Write-Host "架构模式得分: $ArchitectureScore/$ArchitectureChecks"
Write-Host "文档质量得分: $DocScore/$DocChecks"

# 详细报告保存
$ReportFile = Join-Path $QualityDir "quality_report.txt"
$ReportContent = @"
lua_cpp质量检查报告
生成时间: $(Get-Date)

总体质量得分: $QualityScore/$MaxScore
现代C++特性: $ModernCppScore/$TotalChecks
架构模式: $ArchitectureScore/$ArchitectureChecks
文档质量: $DocScore/$DocChecks
格式问题: $FormatIssues

建议改进:
$(if ($ModernCppScore -lt 3) { "- 增加现代C++特性使用" })
$(if ($ArchitectureScore -lt 2) { "- 改善模块化架构设计" })
$(if ($FormatIssues -gt 0) { "- 修复代码格式问题" })
$(if ($DocScore -lt 1) { "- 添加文档和注释" })
"@

$ReportContent | Out-File -FilePath $ReportFile -Encoding UTF8
Write-Host "详细报告已保存至: $ReportFile"

if ($QualityScore -ge 4) {
    Write-Host "🎉 质量检查通过！符合现代C++标准" -ForegroundColor Green
    exit 0
} else {
    Write-Host "⚠️  质量需要改进，建议参考lua_with_cpp的实现" -ForegroundColor Yellow
    exit 1
}