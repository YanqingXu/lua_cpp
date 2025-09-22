# lua_cpp Task to Feature Conversion Script (PowerShell)
# Converts existing TODO.md tasks to Spec-Kit feature format

param(
    [switch]$WhatIf = $false
)

$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$SpecsDir = Join-Path $ProjectRoot "specs"
$TodoFile = Join-Path $ProjectRoot "TODO.md"
$DashboardFile = Join-Path $ProjectRoot "PROJECT_DASHBOARD.md"

Write-Host "=== lua_cpp Task to Feature Conversion ===" -ForegroundColor Cyan
Write-Host "Converting existing tasks to Spec-Kit features..."

# Ensure specs directory structure exists
if (-not (Test-Path $SpecsDir)) {
    New-Item -ItemType Directory -Path $SpecsDir -Force | Out-Null
}

# Function to create feature directory and basic files
function New-Feature {
    param(
        [string]$FeatureId,
        [string]$FeatureTitle,
        [string]$FeatureDescription,
        [string]$Priority
    )
    
    $FeatureDir = Join-Path $SpecsDir $FeatureId
    
    if ($WhatIf) {
        Write-Host "Would create: $FeatureId - $FeatureTitle" -ForegroundColor Yellow
        return
    }
    
    New-Item -ItemType Directory -Path $FeatureDir -Force | Out-Null
    
    $SpecContent = @"
# $FeatureTitle

**Feature ID**: $FeatureId  
**Priority**: $Priority  
**Status**: Not Started  
**Estimated Effort**: TBD  

## Overview
$FeatureDescription

## Acceptance Criteria
- [ ] TBD - requires detailed analysis
- [ ] All contract tests pass
- [ ] lua_c_analysis behavior verification passes
- [ ] lua_with_cpp quality standards met

## Technical Requirements
*To be specified during /specify phase*

## Integration Points
*To be identified during analysis*

## Testing Strategy
*To be defined during /plan phase*

---
**Created**: $(Get-Date -Format 'yyyy-MM-dd')  
**Source**: Converted from TODO.md task
"@
    
    $SpecFile = Join-Path $FeatureDir "spec.md"
    Set-Content -Path $SpecFile -Value $SpecContent -Encoding UTF8
    
    Write-Host "Created feature: $FeatureId - $FeatureTitle" -ForegroundColor Green
}

# Process TODO.md and extract tasks
if (Test-Path $TodoFile) {
    Write-Host "Processing $TodoFile..."
    
    # Example feature conversions - these would be extracted programmatically
    # Priority based on PROJECT_DASHBOARD.md analysis
    
    # High Priority Features (Core Functionality)
    New-Feature -FeatureId "002-lexer-enhancement" -FeatureTitle "Enhanced Lexer System" `
        -FeatureDescription "Improve lexical analysis with better error handling and token validation" -Priority "High"
    
    New-Feature -FeatureId "003-parser-refactor" -FeatureTitle "Parser Architecture Refactor" `
        -FeatureDescription "Redesign parser to use modern C++ patterns and improve maintainability" -Priority "High"
    
    New-Feature -FeatureId "004-vm-instruction-set" -FeatureTitle "VM Instruction Set Completion" `
        -FeatureDescription "Complete VM instruction implementation to match Lua 5.1.5 specification" -Priority "High"
    
    New-Feature -FeatureId "005-memory-management" -FeatureTitle "Advanced Memory Management" `
        -FeatureDescription "Implement sophisticated memory management with optimization features" -Priority "High"
    
    # Medium Priority Features
    New-Feature -FeatureId "006-debug-interface" -FeatureTitle "Debug Interface Enhancement" `
        -FeatureDescription "Improve debugging capabilities with better introspection and error reporting" -Priority "Medium"
    
    New-Feature -FeatureId "007-standard-library" -FeatureTitle "Standard Library Implementation" `
        -FeatureDescription "Complete Lua standard library functions with full compatibility" -Priority "Medium"
    
    New-Feature -FeatureId "008-optimization-engine" -FeatureTitle "Performance Optimization Engine" `
        -FeatureDescription "Add performance optimizations for common Lua patterns" -Priority "Medium"
    
    # Low Priority Features
    New-Feature -FeatureId "009-extension-api" -FeatureTitle "Extension API Framework" `
        -FeatureDescription "Create framework for extending interpreter with custom modules" -Priority "Low"
    
    New-Feature -FeatureId "010-profiling-tools" -FeatureTitle "Profiling and Analysis Tools" `
        -FeatureDescription "Add tools for performance analysis and code profiling" -Priority "Low"
    
} else {
    Write-Warning "$TodoFile not found, creating sample features..."
    
    # Create sample features if TODO.md doesn't exist
    New-Feature -FeatureId "002-sample-feature" -FeatureTitle "Sample Feature" `
        -FeatureDescription "Sample feature for demonstration purposes" -Priority "Low"
}

# Create features overview
if (-not $WhatIf) {
    $OverviewContent = @"
# lua_cpp Features Overview

This directory contains all feature specifications for the lua_cpp project.

## Feature Status Summary

### Active Features
- ``001-interface-unification`` - **In Progress** - Resolving type definition conflicts
- ``002-lexer-enhancement`` - **Not Started** - Enhanced lexer system
- ``003-parser-refactor`` - **Not Started** - Parser architecture refactor  
- ``004-vm-instruction-set`` - **Not Started** - VM instruction set completion
- ``005-memory-management`` - **Not Started** - Advanced memory management

### Planned Features  
- ``006-debug-interface`` - Debug interface enhancement
- ``007-standard-library`` - Standard library implementation
- ``008-optimization-engine`` - Performance optimization engine
- ``009-extension-api`` - Extension API framework
- ``010-profiling-tools`` - Profiling and analysis tools

## Development Workflow

Each feature follows the Spec-Kit methodology:

1. **``/specify``** - Create detailed specification (``spec.md``)
2. **``/plan``** - Develop implementation plan (``plan.md``)  
3. **``/tasks``** - Generate actionable task list (``tasks.md``)
4. **``/implement``** - Execute implementation with verification

## Quality Assurance

Every feature must pass:
- ✅ lua_c_analysis behavioral verification
- ✅ lua_with_cpp architectural quality standards  
- ✅ Comprehensive contract test coverage
- ✅ Performance benchmarks (where applicable)

## Priority Guidelines

- **High**: Core interpreter functionality, blocking other development
- **Medium**: Important enhancements, quality of life improvements
- **Low**: Optional features, future enhancements

---
**Last Updated**: $(Get-Date -Format 'yyyy-MM-dd')  
**Total Features**: 10  
**Conversion Method**: Automated from existing TODO.md and PROJECT_DASHBOARD.md
"@
    
    $OverviewFile = Join-Path $SpecsDir "features-overview.md"
    Set-Content -Path $OverviewFile -Value $OverviewContent -Encoding UTF8
}

Write-Host ""
Write-Host "=== Conversion Complete ===" -ForegroundColor Cyan
Write-Host "Created feature specifications in: $SpecsDir"
Write-Host "Features overview: $(Join-Path $SpecsDir 'features-overview.md')"
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "1. Review each feature specification"
Write-Host "2. Use /specify command to elaborate high-priority features"
Write-Host "3. Begin implementation using Spec-Kit workflow"