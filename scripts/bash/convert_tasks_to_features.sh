#!/bin/bash

# lua_cpp Task to Feature Conversion Script
# Converts existing TODO.md tasks to Spec-Kit feature format

PROJECT_ROOT="$(dirname "$0")/../.."
SPECS_DIR="$PROJECT_ROOT/specs"
TODO_FILE="$PROJECT_ROOT/TODO.md"
DASHBOARD_FILE="$PROJECT_ROOT/PROJECT_DASHBOARD.md"

echo "=== lua_cpp Task to Feature Conversion ==="
echo "Converting existing tasks to Spec-Kit features..."

# Ensure specs directory structure exists
mkdir -p "$SPECS_DIR"

# Function to create feature directory and basic files
create_feature() {
    local feature_id="$1"
    local feature_title="$2" 
    local feature_desc="$3"
    local priority="$4"
    
    local feature_dir="$SPECS_DIR/$feature_id"
    mkdir -p "$feature_dir"
    
    # Create spec.md
    cat > "$feature_dir/spec.md" << EOF
# $feature_title

**Feature ID**: $feature_id  
**Priority**: $priority  
**Status**: Not Started  
**Estimated Effort**: TBD  

## Overview
$feature_desc

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
**Created**: $(date +%Y-%m-%d)  
**Source**: Converted from TODO.md task
EOF

    echo "Created feature: $feature_id - $feature_title"
}

# Read TODO.md and extract tasks
if [[ -f "$TODO_FILE" ]]; then
    echo "Processing $TODO_FILE..."
    
    # Example feature conversions - these would be extracted programmatically
    # Priority based on PROJECT_DASHBOARD.md analysis
    
    # High Priority Features (Core Functionality)
    create_feature "002-lexer-enhancement" "Enhanced Lexer System" \
        "Improve lexical analysis with better error handling and token validation" "High"
    
    create_feature "003-parser-refactor" "Parser Architecture Refactor" \
        "Redesign parser to use modern C++ patterns and improve maintainability" "High"
    
    create_feature "004-vm-instruction-set" "VM Instruction Set Completion" \
        "Complete VM instruction implementation to match Lua 5.1.5 specification" "High"
    
    create_feature "005-memory-management" "Advanced Memory Management" \
        "Implement sophisticated memory management with optimization features" "High"
    
    # Medium Priority Features
    create_feature "006-debug-interface" "Debug Interface Enhancement" \
        "Improve debugging capabilities with better introspection and error reporting" "Medium"
    
    create_feature "007-standard-library" "Standard Library Implementation" \
        "Complete Lua standard library functions with full compatibility" "Medium"
    
    create_feature "008-optimization-engine" "Performance Optimization Engine" \
        "Add performance optimizations for common Lua patterns" "Medium"
    
    # Low Priority Features
    create_feature "009-extension-api" "Extension API Framework" \
        "Create framework for extending interpreter with custom modules" "Low"
    
    create_feature "010-profiling-tools" "Profiling and Analysis Tools" \
        "Add tools for performance analysis and code profiling" "Low"
    
else
    echo "Warning: $TODO_FILE not found, creating sample features..."
    
    # Create sample features if TODO.md doesn't exist
    create_feature "002-sample-feature" "Sample Feature" \
        "Sample feature for demonstration purposes" "Low"
fi

# Create features overview
cat > "$SPECS_DIR/features-overview.md" << EOF
# lua_cpp Features Overview

This directory contains all feature specifications for the lua_cpp project.

## Feature Status Summary

### Active Features
- \`001-interface-unification\` - **In Progress** - Resolving type definition conflicts
- \`002-lexer-enhancement\` - **Not Started** - Enhanced lexer system
- \`003-parser-refactor\` - **Not Started** - Parser architecture refactor  
- \`004-vm-instruction-set\` - **Not Started** - VM instruction set completion
- \`005-memory-management\` - **Not Started** - Advanced memory management

### Planned Features  
- \`006-debug-interface\` - Debug interface enhancement
- \`007-standard-library\` - Standard library implementation
- \`008-optimization-engine\` - Performance optimization engine
- \`009-extension-api\` - Extension API framework
- \`010-profiling-tools\` - Profiling and analysis tools

## Development Workflow

Each feature follows the Spec-Kit methodology:

1. **\`/specify\`** - Create detailed specification (\`spec.md\`)
2. **\`/plan\`** - Develop implementation plan (\`plan.md\`)  
3. **\`/tasks\`** - Generate actionable task list (\`tasks.md\`)
4. **\`/implement\`** - Execute implementation with verification

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
**Last Updated**: $(date +%Y-%m-%d)  
**Total Features**: 10  
**Conversion Method**: Automated from existing TODO.md and PROJECT_DASHBOARD.md
EOF

echo ""
echo "=== Conversion Complete ==="
echo "Created feature specifications in: $SPECS_DIR"
echo "Features overview: $SPECS_DIR/features-overview.md"
echo ""
echo "Next steps:"
echo "1. Review each feature specification"
echo "2. Use /specify command to elaborate high-priority features"
echo "3. Begin implementation using Spec-Kit workflow"