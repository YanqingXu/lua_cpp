# Implementation Plan: Interface Unification

**Branch**: `001-interface-unification` | **Date**: 2025-09-22 | **Spec**: [spec.md](./spec.md)
**Input**: Feature specification from `/specs/001-interface-unification/spec.md`

## Summary
Unify conflicting type definitions (LuaType, OpCode, ErrorType, RegisterIndex) across lua_cpp modules by creating a centralized type system that maintains lua_c_analysis behavioral compatibility and follows lua_with_cpp architectural patterns.

## Technical Context
**Language/Version**: C++17 minimum, C++20 features recommended  
**Primary Dependencies**: Existing lua_cpp modules, reference projects  
**Storage**: N/A (interface definitions only)  
**Testing**: Contract tests, integration tests, behavioral verification  
**Target Platform**: Cross-platform (Windows, Linux, macOS)
**Project Type**: single (refactoring existing codebase)  
**Performance Goals**: Zero runtime overhead, improved compile times
**Constraints**: Must not break existing contracts, maintain behavioral compatibility
**Scale/Scope**: All 6 core modules (GC, VM, Parser, Compiler, Types, API)

## Constitution Check
*Based on lua_cpp project constitution v1.1.0*

### ✅ Lua 5.1.5 Compatibility Gate
- [ ] Behavioral compatibility maintained (verified via lua_c_analysis tests)
- [ ] No changes to external API contracts
- [ ] Type semantics remain identical to reference implementation

### ✅ Modern C++ Gate  
- [ ] Using C++17 scoped enums and type safety
- [ ] RAII principles for resource management
- [ ] constexpr for compile-time optimizations
- [ ] Zero undefined behavior

### ✅ Architecture Gate
- [ ] Maintains modular boundaries per lua_with_cpp patterns
- [ ] Clear separation of concerns
- [ ] Single responsibility principle
- [ ] Minimal coupling between modules

**Gate Status**: PASS - No violations detected

## Project Structure

### Documentation (this feature)
```
specs/001-interface-unification/
├── spec.md                    # Feature specification
├── plan.md                    # This file (/plan command output)
├── research.md                # Technical research and decisions
├── data-model.md              # Type system data model
├── contracts/                 # Interface contracts
│   ├── core_types.h          # Unified type definitions
│   ├── module_interfaces.h   # Standard module interfaces
│   └── type_traits.h         # Type utilities and metaprogramming
└── tasks.md                   # Implementation tasks (/tasks command - NOT created by /plan)
```

### Source Code Impact (repository root)
```
src/
├── core/
│   ├── types.h               # 🔄 UNIFIED - Central type definitions
│   ├── interfaces.h          # ✨ NEW - Standard module interfaces  
│   └── type_traits.h         # ✨ NEW - Type utilities
├── gc/
│   ├── gc_standalone.h       # 🔄 MODIFIED - Use unified types
│   └── gc_test_suite.cpp     # 🔄 UPDATED - Interface changes
├── vm/
│   ├── vm_types.h            # 🗑️ REMOVED - Merged into core/types.h
│   └── vm_executor.h         # 🔄 MODIFIED - Use unified types
├── compiler/
│   ├── bytecode.h            # 🔄 MODIFIED - Use unified OpCode
│   └── compiler.h            # 🔄 MODIFIED - Interface updates
└── specs/contracts/          # 🔄 UPDATED - Reflect unified types
```

## Phase 0: Research & Analysis
*Extracting and resolving technical unknowns*

### Current Conflict Analysis
1. **LuaType conflicts**: 
   - Research: Map existing definitions across modules
   - Decision: Consolidate into single enum class in core/types.h
   
2. **OpCode conflicts**:
   - Research: Compare VM and Compiler OpCode definitions
   - Decision: Create authoritative definition matching lua_c_analysis

3. **ErrorType inconsistencies**:
   - Research: Inventory error handling patterns
   - Decision: Unified error taxonomy with module-specific extensions

4. **RegisterIndex variations**:
   - Research: Usage patterns in VM and GC modules
   - Decision: Strong typedef with bounds checking

### Reference Project Integration
1. **lua_c_analysis behavioral mapping**:
   - Task: "Map C type definitions to C++ equivalents"
   - Task: "Verify enum value consistency with reference"
   
2. **lua_with_cpp architectural analysis**:
   - Task: "Analyze existing interface patterns"
   - Task: "Identify reusable design patterns"

**Output**: research.md with all technical decisions documented

## Phase 1: Design & Contracts
*Prerequisites: research.md complete*

### 1. Unified Type System Design
**Output**: `data-model.md`
- Core type definitions with lua_c_analysis mappings
- Module interface specifications
- Type conversion and validation rules
- Backward compatibility strategy

### 2. Interface Contracts Generation
**Output**: `contracts/` directory
- `core_types.h`: Authoritative type definitions
- `module_interfaces.h`: Standard module interfaces
- `type_traits.h`: Template utilities for type safety

### 3. Contract Test Updates
**Output**: Updated contract tests
- Modify existing tests to use unified types
- Add interface validation tests
- Ensure lua_c_analysis behavioral compatibility

### 4. Migration Strategy
**Output**: `migration_guide.md`
- Step-by-step refactoring process
- Automated migration scripts where possible
- Rollback procedures and safety checks

**Output**: Unified interface contracts, validated migration plan

## Phase 2: Task Planning Approach
*This section describes what the /tasks command will do - DO NOT execute during /plan*

**Task Generation Strategy**:
- Load interface contracts from Phase 1 design docs
- Generate tasks for each module migration (GC, VM, Compiler, etc.)
- Each module → interface migration task
- Each conflict → resolution task [P]
- Validation tasks to ensure no regression

**Ordering Strategy**:
- TDD order: Update contract tests before implementation
- Dependency order: Core types → Module interfaces → Integration
- Mark [P] for parallel module migrations (independent)

**Estimated Output**: 15-20 numbered, ordered tasks in tasks.md

## Phase 3+: Future Implementation
*These phases are beyond the scope of the /plan command*

**Phase 3**: Task execution (/tasks command creates tasks.md)  
**Phase 4**: Implementation (migrate modules to unified interfaces)  
**Phase 5**: Validation (run dual verification, integration tests)

## Complexity Tracking
*Fill ONLY if Constitution Check has violations that must be justified*

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| None detected | N/A | N/A |

## Progress Tracking

**Phase Status**:
- [ ] Phase 0: Research complete (/plan command)
- [ ] Phase 1: Design complete (/plan command)
- [ ] Phase 2: Task planning complete (/plan command - describe approach only)
- [ ] Phase 3: Tasks generated (/tasks command)
- [ ] Phase 4: Implementation complete
- [ ] Phase 5: Validation passed

**Gate Status**:
- [ ] Initial Constitution Check: PASS
- [ ] Post-Design Constitution Check: PASS
- [ ] All technical unknowns resolved
- [ ] lua_c_analysis behavioral mapping complete
- [ ] lua_with_cpp architectural alignment verified

---
*Based on Constitution v1.1.0 - See `/memory/constitution.md`*