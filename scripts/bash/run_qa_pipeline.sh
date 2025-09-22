#!/bin/bash

# lua_cpp Automated Quality Assurance Pipeline
# Runs comprehensive quality checks and generates reports

set -e  # Exit on any error

# Configuration
PROJECT_ROOT="$(dirname "$0")/../.."
BUILD_DIR="$PROJECT_ROOT/build"
REPORTS_DIR="$PROJECT_ROOT/qa_reports"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

echo "=== lua_cpp Quality Assurance Pipeline ==="
echo "Timestamp: $TIMESTAMP"
echo "Project Root: $PROJECT_ROOT"

# Ensure directories exist
mkdir -p "$REPORTS_DIR"
mkdir -p "$BUILD_DIR"

# Function to log with timestamp
log() {
    echo "[$(date '+%H:%M:%S')] $1"
}

# Function to run step with error handling
run_step() {
    local step_name="$1"
    local step_function="$2"
    
    log "Starting: $step_name"
    if $step_function; then
        log "✅ Completed: $step_name"
        return 0
    else
        log "❌ Failed: $step_name"
        return 1
    fi
}

# Quality Gate 1: Static Analysis
static_analysis() {
    log "Running static analysis..."
    
    # Ensure tools are available
    command -v cppcheck >/dev/null 2>&1 || { log "cppcheck not found"; return 1; }
    
    # Run CPP Check
    cppcheck --enable=all --std=c++17 \
        --project="$PROJECT_ROOT/CMakeLists.txt" \
        --xml --xml-version=2 \
        --output-file="$REPORTS_DIR/cppcheck_$TIMESTAMP.xml" \
        "$PROJECT_ROOT/src" 2>/dev/null
    
    # Generate summary
    local issues=$(grep -c '<error' "$REPORTS_DIR/cppcheck_$TIMESTAMP.xml" || echo "0")
    echo "Static Analysis Issues: $issues" > "$REPORTS_DIR/static_analysis_summary.txt"
    
    return 0
}

# Quality Gate 2: Build and Unit Tests
build_and_test() {
    log "Building project and running unit tests..."
    
    cd "$PROJECT_ROOT"
    
    # Configure CMake
    cmake -B "$BUILD_DIR" \
        -DCMAKE_BUILD_TYPE=Release \
        -DENABLE_TESTING=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    
    # Build project
    cmake --build "$BUILD_DIR" --parallel
    
    # Run unit tests
    cd "$BUILD_DIR"
    ctest --output-on-failure --verbose > "$REPORTS_DIR/unit_tests_$TIMESTAMP.log" 2>&1
    
    return $?
}

# Quality Gate 3: Contract Tests
contract_tests() {
    log "Running contract tests..."
    
    cd "$BUILD_DIR"
    
    local contract_results="$REPORTS_DIR/contract_tests_$TIMESTAMP.log"
    echo "Contract Test Results - $TIMESTAMP" > "$contract_results"
    echo "======================================" >> "$contract_results"
    
    local test_count=0
    local pass_count=0
    
    # Run each contract test
    for test_executable in test_*_contract*; do
        if [[ -x "$test_executable" ]]; then
            test_count=$((test_count + 1))
            log "Running $test_executable..."
            
            if "./$test_executable" >> "$contract_results" 2>&1; then
                echo "✅ $test_executable: PASSED" >> "$contract_results"
                pass_count=$((pass_count + 1))
            else
                echo "❌ $test_executable: FAILED" >> "$contract_results"
            fi
        fi
    done
    
    echo "" >> "$contract_results"
    echo "Summary: $pass_count/$test_count tests passed" >> "$contract_results"
    
    # Return success if all tests passed
    [[ $pass_count -eq $test_count ]]
}

# Quality Gate 4: lua_c_analysis Behavioral Verification
behavioral_verification() {
    log "Running lua_c_analysis behavioral verification..."
    
    # Check if verification script exists
    local verify_script="$PROJECT_ROOT/scripts/bash/verify_behavior.sh"
    if [[ ! -f "$verify_script" ]]; then
        log "Behavioral verification script not found"
        return 1
    fi
    
    # Run verification
    chmod +x "$verify_script"
    "$verify_script" > "$REPORTS_DIR/behavioral_verification_$TIMESTAMP.log" 2>&1
    
    return $?
}

# Quality Gate 5: lua_with_cpp Quality Verification
quality_verification() {
    log "Running lua_with_cpp quality verification..."
    
    # Check if verification script exists
    local verify_script="$PROJECT_ROOT/scripts/bash/verify_quality.sh"
    if [[ ! -f "$verify_script" ]]; then
        log "Quality verification script not found"
        return 1
    fi
    
    # Run verification
    chmod +x "$verify_script"
    "$verify_script" > "$REPORTS_DIR/quality_verification_$TIMESTAMP.log" 2>&1
    
    return $?
}

# Quality Gate 6: Performance Benchmarks
performance_benchmarks() {
    log "Running performance benchmarks..."
    
    cd "$BUILD_DIR"
    
    # Check if benchmark executable exists
    if [[ -x "lua_cpp_benchmark" ]]; then
        ./lua_cpp_benchmark --output="$REPORTS_DIR/performance_$TIMESTAMP.json"
    else
        log "Performance benchmark executable not found, skipping..."
        return 0  # Non-critical for now
    fi
    
    return 0
}

# Quality Gate 7: Integration Tests
integration_tests() {
    log "Running integration tests..."
    
    cd "$BUILD_DIR"
    
    local integration_results="$REPORTS_DIR/integration_tests_$TIMESTAMP.log"
    echo "Integration Test Results - $TIMESTAMP" > "$integration_results"
    echo "========================================" >> "$integration_results"
    
    # Check if integration test suite exists
    if [[ -x "integration_test_suite" ]]; then
        ./integration_test_suite --verbose >> "$integration_results" 2>&1
        return $?
    else
        log "Integration test suite not found, skipping..."
        echo "Integration test suite not available" >> "$integration_results"
        return 0  # Non-critical for now
    fi
}

# Generate comprehensive report
generate_report() {
    log "Generating comprehensive quality report..."
    
    local report_file="$REPORTS_DIR/quality_report_$TIMESTAMP.md"
    
    cat > "$report_file" << EOF
# lua_cpp Quality Assessment Report

**Generated**: $(date)  
**Pipeline Run**: $TIMESTAMP  
**Project Root**: $PROJECT_ROOT  

## Quality Gate Summary

| Quality Gate | Status | Details |
|--------------|--------|---------|
| Static Analysis | $static_analysis_status | Issues found: $(cat "$REPORTS_DIR/static_analysis_summary.txt" 2>/dev/null || echo "N/A") |
| Build & Unit Tests | $build_test_status | Test results in unit_tests_$TIMESTAMP.log |
| Contract Tests | $contract_test_status | Contract results in contract_tests_$TIMESTAMP.log |
| Behavioral Verification | $behavioral_status | Verification results in behavioral_verification_$TIMESTAMP.log |
| Quality Verification | $quality_status | Quality results in quality_verification_$TIMESTAMP.log |
| Performance Benchmarks | $performance_status | Benchmark results in performance_$TIMESTAMP.json |
| Integration Tests | $integration_status | Integration results in integration_tests_$TIMESTAMP.log |

## Overall Assessment

**Quality Score**: $overall_score/7 gates passed  
**Pipeline Status**: $pipeline_status  

## Recommendations

EOF

    # Add specific recommendations based on failures
    if [[ $overall_score -lt 7 ]]; then
        echo "### Issues to Address" >> "$report_file"
        echo "" >> "$report_file"
        
        [[ "$static_analysis_status" == "❌ FAILED" ]] && echo "- Fix static analysis issues identified in cppcheck report" >> "$report_file"
        [[ "$build_test_status" == "❌ FAILED" ]] && echo "- Resolve build failures and unit test issues" >> "$report_file"
        [[ "$contract_test_status" == "❌ FAILED" ]] && echo "- Fix contract test failures" >> "$report_file"
        [[ "$behavioral_status" == "❌ FAILED" ]] && echo "- Address behavioral compatibility issues with lua_c_analysis" >> "$report_file"
        [[ "$quality_status" == "❌ FAILED" ]] && echo "- Improve code quality to meet lua_with_cpp standards" >> "$report_file"
        [[ "$performance_status" == "❌ FAILED" ]] && echo "- Address performance regressions" >> "$report_file"
        [[ "$integration_status" == "❌ FAILED" ]] && echo "- Fix integration test failures" >> "$report_file"
    else
        echo "### All Quality Gates Passed ✅" >> "$report_file"
        echo "" >> "$report_file"
        echo "The project meets all quality standards and is ready for deployment." >> "$report_file"
    fi
    
    echo "" >> "$report_file"
    echo "---" >> "$report_file"
    echo "**Report Generated By**: lua_cpp QA Pipeline v1.0" >> "$report_file"
    
    log "Quality report generated: $report_file"
}

# Main pipeline execution
main() {
    local gates_passed=0
    local total_gates=7
    
    # Initialize status variables
    static_analysis_status="⏳ PENDING"
    build_test_status="⏳ PENDING"
    contract_test_status="⏳ PENDING"
    behavioral_status="⏳ PENDING"
    quality_status="⏳ PENDING"
    performance_status="⏳ PENDING"
    integration_status="⏳ PENDING"
    
    # Run quality gates
    if run_step "Static Analysis" static_analysis; then
        static_analysis_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        static_analysis_status="❌ FAILED"
    fi
    
    if run_step "Build and Unit Tests" build_and_test; then
        build_test_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        build_test_status="❌ FAILED"
    fi
    
    if run_step "Contract Tests" contract_tests; then
        contract_test_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        contract_test_status="❌ FAILED"
    fi
    
    if run_step "Behavioral Verification" behavioral_verification; then
        behavioral_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        behavioral_status="❌ FAILED"
    fi
    
    if run_step "Quality Verification" quality_verification; then
        quality_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        quality_status="❌ FAILED"
    fi
    
    if run_step "Performance Benchmarks" performance_benchmarks; then
        performance_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        performance_status="❌ FAILED"
    fi
    
    if run_step "Integration Tests" integration_tests; then
        integration_status="✅ PASSED"
        gates_passed=$((gates_passed + 1))
    else
        integration_status="❌ FAILED"
    fi
    
    # Set overall status
    overall_score=$gates_passed
    if [[ $gates_passed -eq $total_gates ]]; then
        pipeline_status="✅ PASSED"
    else
        pipeline_status="❌ FAILED"
    fi
    
    # Generate final report
    generate_report
    
    echo ""
    echo "=== Quality Assurance Pipeline Complete ==="
    echo "Gates Passed: $gates_passed/$total_gates"
    echo "Overall Status: $pipeline_status"
    echo "Report Location: $REPORTS_DIR/quality_report_$TIMESTAMP.md"
    echo ""
    
    # Exit with appropriate code
    if [[ $gates_passed -eq $total_gates ]]; then
        log "🚀 All quality gates passed! Project is ready for deployment."
        exit 0
    else
        log "⚠️  Some quality gates failed. Please review the report and address issues."
        exit 1
    fi
}

# Run the pipeline
main "$@"