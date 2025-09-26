#!/usr/bin/env pwsh
# T026性能测试脚本

Write-Host "========================================" -ForegroundColor Green
Write-Host "       T026 Performance Testing         " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green
Write-Host ""

# 检查构建目录
$buildDir = "build"
if (-not (Test-Path $buildDir)) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# 配置CMake（如果需要）
if (-not (Test-Path "$buildDir/CMakeCache.txt")) {
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    Set-Location $buildDir
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON -DBUILD_BENCHMARKS=ON
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed!" -ForegroundColor Red
        Set-Location ..
        exit 1
    }
    Set-Location ..
}

# 构建项目
Write-Host "Building project..." -ForegroundColor Yellow
Set-Location $buildDir
cmake --build . --config Release
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed!" -ForegroundColor Red
    Set-Location ..
    exit 1
}
Set-Location ..

Write-Host "Build completed successfully!" -ForegroundColor Green
Write-Host ""

# 运行性能分析器
$analyzerPath = "$buildDir/tests/T026_tests/T026_performance_analyzer.exe"
if (Test-Path $analyzerPath) {
    Write-Host "Running T026 Performance Analyzer..." -ForegroundColor Cyan
    Write-Host ""
    & $analyzerPath
    Write-Host ""
} else {
    Write-Host "Performance analyzer not found at: $analyzerPath" -ForegroundColor Yellow
    Write-Host "Skipping performance analysis..." -ForegroundColor Yellow
}

# 运行基准测试
$benchmarkPath = "$buildDir/tests/T026_tests/T026_benchmark_tests.exe"
if (Test-Path $benchmarkPath) {
    Write-Host "Running T026 Benchmark Tests..." -ForegroundColor Cyan
    Write-Host ""
    & $benchmarkPath --benchmark_format=console --benchmark_color=true
    Write-Host ""
} else {
    Write-Host "Benchmark tests not found at: $benchmarkPath" -ForegroundColor Yellow
    Write-Host "Skipping benchmark tests..." -ForegroundColor Yellow
}

# 运行单元测试（如果存在）
$testPaths = @(
    "$buildDir/tests/T026_tests/T026_contract_tests.exe",
    "$buildDir/tests/T026_tests/T026_unit_tests.exe",
    "$buildDir/tests/T026_tests/T026_integration_tests.exe"
)

foreach ($testPath in $testPaths) {
    if (Test-Path $testPath) {
        $testName = Split-Path -Leaf $testPath
        Write-Host "Running $testName..." -ForegroundColor Cyan
        & $testPath
        if ($LASTEXITCODE -eq 0) {
            Write-Host "$testName passed!" -ForegroundColor Green
        } else {
            Write-Host "$testName failed!" -ForegroundColor Red
        }
        Write-Host ""
    }
}

Write-Host "========================================" -ForegroundColor Green
Write-Host "     T026 Performance Testing Done     " -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Green

# 生成性能报告摘要
if (Test-Path "T026_performance_report.txt") {
    Write-Host ""
    Write-Host "Performance Report Summary:" -ForegroundColor Magenta
    Write-Host "----------------------------------------" -ForegroundColor Magenta
    
    $reportContent = Get-Content "T026_performance_report.txt"
    $inSummary = $false
    
    foreach ($line in $reportContent) {
        if ($line -match "Average Performance Improvement:") {
            Write-Host $line -ForegroundColor Green
            $inSummary = $true
        } elseif ($inSummary -and $line -match "T026 Status:") {
            Write-Host $line -ForegroundColor Yellow
            break
        }
    }
    
    Write-Host ""
    Write-Host "Full report available in: T026_performance_report.txt" -ForegroundColor Cyan
}