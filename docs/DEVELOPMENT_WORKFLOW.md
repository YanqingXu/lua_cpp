# lua_cpp Development Workflow

**Version**: 2.0  
**Date**: 2025-09-22  
**Status**: Active  

## Overview

This document defines the standardized development workflow for lua_cpp project, incorporating Spec-Kit methodology with dual reference project verification.

## Workflow Architecture

### Core Principles

1. **Specification-Driven Development**: Every change begins with a clear specification
2. **Dual Reference Verification**: All implementations verified against lua_c_analysis and lua_with_cpp
3. **Incremental Progress**: Features developed in small, verifiable increments
4. **Quality Gates**: Multiple verification checkpoints throughout development
5. **Continuous Integration**: Automated testing and validation at every step

### Development Phases

```mermaid
graph TD
    A[Feature Request] --> B[/specify]
    B --> C[/plan] 
    C --> D[/tasks]
    D --> E[/implement]
    E --> F[Quality Gates]
    F --> G{Pass?}
    G -->|Yes| H[Feature Complete]
    G -->|No| I[Refinement]
    I --> D
```

## Workflow Commands

### 1. `/constitution` - Project Foundation
**Purpose**: Establish project principles and standards  
**Usage**: `constitution <topic>`  
**Output**: Updated constitution.md with project guidelines

**Example**:
```bash
# Review memory management principles
constitution memory-management

# Update testing standards  
constitution testing-standards
```

### 2. `/specify` - Feature Specification
**Purpose**: Create detailed feature specifications  
**Usage**: `specify <feature-id>`  
**Output**: Enhanced spec.md with complete requirements

**Process**:
1. Analyze feature requirements
2. Define acceptance criteria  
3. Identify integration points
4. Specify lua_c_analysis compatibility requirements
5. Define lua_with_cpp quality standards

**Example**:
```bash
# Specify lexer enhancement feature
specify 002-lexer-enhancement
```

### 3. `/plan` - Implementation Planning
**Purpose**: Develop detailed implementation strategy  
**Usage**: `plan <feature-id>`  
**Output**: Comprehensive plan.md with implementation approach

**Planning Elements**:
- **Technical Architecture**: Component design and interfaces
- **Implementation Phases**: Step-by-step development plan
- **Risk Analysis**: Potential issues and mitigation strategies
- **Resource Requirements**: Time, dependencies, prerequisites
- **Verification Strategy**: Testing and validation approach

**Example**:
```bash
# Plan parser refactor implementation
plan 003-parser-refactor
```

### 4. `/tasks` - Task Generation
**Purpose**: Break down plan into actionable tasks  
**Usage**: `tasks <feature-id>`  
**Output**: Detailed tasks.md with implementation checklist

**Task Structure**:
- **Implementation Tasks**: Code development activities
- **Testing Tasks**: Verification and validation activities  
- **Documentation Tasks**: Documentation updates
- **Integration Tasks**: Component integration activities
- **Verification Tasks**: Reference project validation

**Example**:
```bash
# Generate tasks for VM instruction set
tasks 004-vm-instruction-set
```

### 5. `/implement` - Feature Implementation
**Purpose**: Execute feature implementation with continuous verification  
**Usage**: `implement <feature-id>`  
**Output**: Complete feature implementation with verification results

**Implementation Process**:
1. Execute tasks in specified order
2. Run verification after each significant change
3. Update documentation as implementation progresses
4. Perform continuous integration checks
5. Complete quality gate validation

**Example**:
```bash
# Implement memory management feature
implement 005-memory-management
```

## Quality Gates

### Gate 1: Specification Review
**Checkpoint**: After `/specify`  
**Requirements**:
- [ ] Clear, measurable acceptance criteria
- [ ] lua_c_analysis compatibility requirements defined
- [ ] lua_with_cpp quality standards identified  
- [ ] Integration points documented
- [ ] Testing strategy outlined

### Gate 2: Plan Validation
**Checkpoint**: After `/plan`  
**Requirements**:
- [ ] Implementation approach is sound
- [ ] Risk mitigation strategies defined
- [ ] Resource requirements reasonable
- [ ] Verification strategy comprehensive
- [ ] Dependencies identified and managed

### Gate 3: Task Readiness
**Checkpoint**: After `/tasks`  
**Requirements**:
- [ ] All tasks are actionable and specific
- [ ] Task dependencies properly ordered
- [ ] Verification tasks included
- [ ] Documentation tasks specified
- [ ] Acceptance criteria mapped to tasks

### Gate 4: Implementation Verification
**Checkpoint**: During `/implement`  
**Requirements**:
- [ ] All contract tests pass
- [ ] lua_c_analysis behavioral verification passes
- [ ] lua_with_cpp quality standards met
- [ ] Performance benchmarks maintained
- [ ] Documentation updated

### Gate 5: Feature Acceptance
**Checkpoint**: After `/implement`  
**Requirements**:
- [ ] All acceptance criteria met
- [ ] Integration tests pass
- [ ] Performance requirements satisfied
- [ ] Code review completed
- [ ] Documentation comprehensive

## Verification Framework

### lua_c_analysis Verification
**Purpose**: Ensure behavioral compatibility with Lua 5.1.5  
**Frequency**: After each significant change  
**Script**: `scripts/verify_behavior.sh`

**Verification Types**:
- **Functional Verification**: Output comparison with reference implementation
- **Edge Case Testing**: Boundary condition validation
- **Error Handling**: Exception and error code verification
- **Performance Baseline**: Basic performance comparison

### lua_with_cpp Quality Verification  
**Purpose**: Ensure modern C++ architectural standards  
**Frequency**: Before feature completion  
**Script**: `scripts/verify_quality.sh`

**Quality Checks**:
- **Code Structure**: Modern C++ patterns and idioms
- **Memory Safety**: RAII compliance and leak detection
- **Type Safety**: Strong typing and const correctness
- **Performance**: Optimization and efficiency validation

### Automated Testing Pipeline
**Purpose**: Continuous validation throughout development  
**Trigger**: On every commit and pull request

**Pipeline Stages**:
1. **Build Verification**: Compilation and linking validation
2. **Unit Testing**: Component-level test execution
3. **Integration Testing**: System-level test execution
4. **Contract Testing**: Interface contract validation
5. **Behavioral Verification**: lua_c_analysis comparison
6. **Quality Assessment**: lua_with_cpp standards check

## Development Environment

### Required Tools
- **C++ Compiler**: Modern C++17/20 compatible compiler
- **CMake**: Build system management
- **Git**: Version control
- **Bash/PowerShell**: Script execution environment
- **Testing Framework**: Contract testing infrastructure

### Recommended IDE Setup
- **VS Code**: Primary development environment
- **Extensions**: C/C++, CMake Tools, GitLens
- **Settings**: Configured for lua_cpp project standards
- **Tasks**: Integrated Spec-Kit command execution

### Environment Variables
```bash
# Project configuration
export LUA_CPP_ROOT="/path/to/lua_cpp"
export LUA_C_ANALYSIS_ROOT="/path/to/lua_c_analysis"  
export LUA_WITH_CPP_ROOT="/path/to/lua_with_cpp"

# Build configuration
export CMAKE_BUILD_TYPE="Debug"
export CXX_STANDARD="17"
```

## Best Practices

### Feature Development
1. **Start Small**: Begin with minimal viable implementation
2. **Iterate Frequently**: Make small, verifiable changes
3. **Test Early**: Write tests before implementation when possible
4. **Document Continuously**: Update documentation as code evolves
5. **Verify Often**: Run verification scripts frequently

### Code Quality
1. **Follow Standards**: Adhere to lua_with_cpp architectural patterns
2. **Modern C++**: Use contemporary C++ features and idioms
3. **RAII Principles**: Ensure proper resource management
4. **Const Correctness**: Apply const wherever appropriate
5. **Clear Interfaces**: Design clean, intuitive APIs

### Collaboration
1. **Clear Commits**: Write descriptive commit messages
2. **Feature Branches**: Use separate branches for each feature
3. **Code Reviews**: Require review before merging
4. **Documentation**: Keep documentation current and comprehensive
5. **Communication**: Discuss design decisions openly

## Troubleshooting

### Common Issues

**Build Failures**:
- Check compiler compatibility (C++17/20 required)
- Verify CMake configuration
- Ensure all dependencies available

**Test Failures**:
- Run verification scripts to identify specific issues
- Check lua_c_analysis compatibility
- Validate against lua_with_cpp quality standards

**Integration Problems**:
- Review interface definitions
- Check dependency management
- Validate component interactions

### Support Resources
- **Project Documentation**: `docs/` directory
- **Architecture Guides**: `docs/guides/` directory  
- **Reference Projects**: lua_c_analysis and lua_with_cpp
- **Issue Tracking**: GitHub Issues
- **Code Review**: Pull Request discussions

---

**Workflow Version**: 2.0  
**Last Updated**: 2025-09-22  
**Next Review**: 2025-10-22  
**Maintained By**: lua_cpp Development Team