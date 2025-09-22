# lua_c_analysis行为验证脚本 (PowerShell版本)
# 用于验证lua_cpp的实现与lua_c_analysis的行为一致性

param(
    [string]$TestPattern = "*"
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$LuaCAnalysisPath = Join-Path (Split-Path -Parent $ProjectRoot) "lua_c_analysis"

Write-Host "🔍 lua_c_analysis行为验证开始..." -ForegroundColor Green

# 检查参考项目路径
if (-not (Test-Path $LuaCAnalysisPath)) {
    Write-Error "找不到lua_c_analysis项目路径: $LuaCAnalysisPath"
    exit 1
}

# 检查lua_cpp构建状态
$LuaCppBinary = Join-Path $ProjectRoot "build\bin\lua_cpp.exe"
if (-not (Test-Path $LuaCppBinary)) {
    Write-Error "找不到lua_cpp二进制文件: $LuaCppBinary"
    exit 1
}

# 检查lua_c_analysis构建状态
$LuaCBinary = Join-Path $LuaCAnalysisPath "lua.exe"
if (-not (Test-Path $LuaCBinary)) {
    Write-Warning "找不到lua_c_analysis二进制文件，尝试构建..."
    # 在Windows上可能需要不同的构建方法
    Write-Warning "请手动构建lua_c_analysis项目"
}

# 创建测试目录
$TestDir = Join-Path $ProjectRoot "verification\behavior_tests"
if (-not (Test-Path $TestDir)) {
    New-Item -Path $TestDir -ItemType Directory -Force | Out-Null
}

Write-Host "📝 生成行为验证测试用例..." -ForegroundColor Cyan

# 基础语法测试
$BasicSyntaxTest = @'
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
'@

$BasicSyntaxTest | Out-File -FilePath (Join-Path $TestDir "basic_syntax.lua") -Encoding UTF8

# 表操作测试
$TableOperationsTest = @'
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
'@

$TableOperationsTest | Out-File -FilePath (Join-Path $TestDir "table_operations.lua") -Encoding UTF8

# 字符串操作测试
$StringOperationsTest = @'
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
'@

$StringOperationsTest | Out-File -FilePath (Join-Path $TestDir "string_operations.lua") -Encoding UTF8

# 执行验证测试
Write-Host "🚀 执行行为验证测试..." -ForegroundColor Yellow

$TestCount = 0
$PassCount = 0
$FailCount = 0

$TestFiles = Get-ChildItem -Path $TestDir -Filter "*.lua" | Where-Object { $_.Name -like $TestPattern }

foreach ($TestFile in $TestFiles) {
    $TestName = $TestFile.BaseName
    Write-Host ""
    Write-Host "测试: $TestName" -ForegroundColor White
    Write-Host "----------------------------------------"
    
    $TestCount++
    
    # 获取lua_c_analysis输出
    Write-Host "🔍 lua_c_analysis输出:" -ForegroundColor Blue
    $LuaCOutputFile = Join-Path $TestDir "${TestName}_lua_c.out"
    
    try {
        if (Test-Path $LuaCBinary) {
            & $LuaCBinary $TestFile.FullName > $LuaCOutputFile 2>&1
            if ($LASTEXITCODE -ne 0) {
                throw "lua_c_analysis执行失败"
            }
            Get-Content $LuaCOutputFile
        } else {
            Write-Warning "lua_c_analysis二进制文件不存在，跳过对比"
            "# lua_c_analysis不可用" | Out-File $LuaCOutputFile
        }
    } catch {
        Write-Host "❌ lua_c_analysis执行失败: $_" -ForegroundColor Red
        $FailCount++
        continue
    }
    
    # 获取lua_cpp输出
    Write-Host ""
    Write-Host "🏗️ lua_cpp输出:" -ForegroundColor Green
    $LuaCppOutputFile = Join-Path $TestDir "${TestName}_lua_cpp.out"
    
    try {
        & $LuaCppBinary $TestFile.FullName > $LuaCppOutputFile 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "lua_cpp执行失败"
        }
        Get-Content $LuaCppOutputFile
    } catch {
        Write-Host "❌ lua_cpp执行失败: $_" -ForegroundColor Red
        Get-Content $LuaCppOutputFile -ErrorAction SilentlyContinue
        $FailCount++
        continue
    }
    
    # 比较输出
    Write-Host ""
    Write-Host "📊 输出比较:" -ForegroundColor Cyan
    $DiffFile = Join-Path $TestDir "${TestName}_diff.out"
    
    if ((Get-Content $LuaCOutputFile -Raw) -eq (Get-Content $LuaCppOutputFile -Raw)) {
        Write-Host "✅ 输出一致" -ForegroundColor Green
        $PassCount++
    } else {
        Write-Host "❌ 输出不一致" -ForegroundColor Red
        Write-Host "差异详情:" -ForegroundColor Yellow
        
        # 简单的差异显示
        $LuaCContent = Get-Content $LuaCOutputFile
        $LuaCppContent = Get-Content $LuaCppOutputFile
        
        "--- lua_c_analysis" | Out-File $DiffFile
        "+++ lua_cpp" | Add-Content $DiffFile
        
        for ($i = 0; $i -lt [Math]::Max($LuaCContent.Count, $LuaCppContent.Count); $i++) {
            $LuaCLine = if ($i -lt $LuaCContent.Count) { $LuaCContent[$i] } else { "" }
            $LuaCppLine = if ($i -lt $LuaCppContent.Count) { $LuaCppContent[$i] } else { "" }
            
            if ($LuaCLine -ne $LuaCppLine) {
                "- $LuaCLine" | Add-Content $DiffFile
                "+ $LuaCppLine" | Add-Content $DiffFile
            }
        }
        
        Get-Content $DiffFile | Select-Object -First 10
        $FailCount++
    }
}

# 生成验证报告
Write-Host ""
Write-Host "=========================================" -ForegroundColor White
Write-Host "🔍 lua_c_analysis行为验证报告" -ForegroundColor Green
Write-Host "=========================================" -ForegroundColor White
Write-Host "总测试数: $TestCount"
Write-Host "通过测试: $PassCount" -ForegroundColor Green
Write-Host "失败测试: $FailCount" -ForegroundColor Red

if ($TestCount -gt 0) {
    $PassRate = [Math]::Round(($PassCount * 100.0 / $TestCount), 1)
    Write-Host "通过率: $PassRate%"
}

if ($FailCount -eq 0) {
    Write-Host "🎉 所有测试通过！lua_cpp与lua_c_analysis行为一致" -ForegroundColor Green
    exit 0
} else {
    Write-Host "⚠️  有测试失败，需要修复lua_cpp实现" -ForegroundColor Yellow
    exit 1
}