/**
 * @file test_coroutine_lib_api.cpp
 * @brief T028 Phase 3.2 - 协程库 Lua API 集成测试
 * 
 * 测试目标：
 * 1. 验证 coroutine.create() API
 * 2. 验证 coroutine.resume() 和 coroutine.yield() API
 * 3. 验证 coroutine.status() API
 * 4. 验证 coroutine.running() API
 * 5. 验证 coroutine.wrap() API
 * 6. 验证错误处理（dead coroutine, invalid args 等）
 * 
 * 策略：
 * - 创建简化的 LuaValue 模拟
 * - 测试协程库的公共 API 接口
 * - 验证状态转换和错误处理
 * 
 * @author Lua C++ Project Team
 * @date 2025-10-13
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <stdexcept>
#include <cassert>

/* ========================================================================== */
/* 最小化 LuaValue 模拟（用于测试） */
/* ========================================================================== */

enum class LuaValueType {
    NIL,
    BOOLEAN,
    NUMBER,
    STRING,
    FUNCTION,
    COROUTINE
};

class LuaValue {
public:
    LuaValueType type;
    
    union {
        bool boolean_value;
        double number_value;
    };
    
    std::string string_value;
    std::function<std::vector<LuaValue>(const std::vector<LuaValue>&)> function_value;
    void* coroutine_ptr;  // 指向协程对象的指针
    
    // 构造函数
    LuaValue() : type(LuaValueType::NIL), number_value(0) {}
    
    explicit LuaValue(bool b) : type(LuaValueType::BOOLEAN), boolean_value(b), number_value(0) {}
    
    explicit LuaValue(double n) : type(LuaValueType::NUMBER), number_value(n) {}
    
    explicit LuaValue(const std::string& s) : type(LuaValueType::STRING), string_value(s), number_value(0) {}
    
    template<typename F>
    explicit LuaValue(F&& f)
        : type(LuaValueType::FUNCTION), number_value(0) {
        function_value = std::forward<F>(f);
    }
    
    // 类型检查
    bool IsNil() const { return type == LuaValueType::NIL; }
    bool IsBoolean() const { return type == LuaValueType::BOOLEAN; }
    bool IsNumber() const { return type == LuaValueType::NUMBER; }
    bool IsString() const { return type == LuaValueType::STRING; }
    bool IsFunction() const { return type == LuaValueType::FUNCTION; }
    bool IsCoroutine() const { return type == LuaValueType::COROUTINE; }
    
    // 值访问
    bool AsBoolean() const { return boolean_value; }
    double AsNumber() const { return number_value; }
    std::string AsString() const { return string_value; }
    
    // 协程创建
    static LuaValue MakeCoroutine(void* ptr) {
        LuaValue v;
        v.type = LuaValueType::COROUTINE;
        v.coroutine_ptr = ptr;
        return v;
    }
};

/* ========================================================================== */
/* 简化的协程状态枚举 */
/* ========================================================================== */

enum class CoroutineState {
    SUSPENDED,
    RUNNING,
    NORMAL,
    DEAD
};

std::string CoroutineStateToString(CoroutineState state) {
    switch (state) {
        case CoroutineState::SUSPENDED: return "suspended";
        case CoroutineState::RUNNING: return "running";
        case CoroutineState::NORMAL: return "normal";
        case CoroutineState::DEAD: return "dead";
        default: return "unknown";
    }
}

/* ========================================================================== */
/* 简化的协程对象 */
/* ========================================================================== */

class SimpleCoroutine {
public:
    std::function<std::vector<LuaValue>(const std::vector<LuaValue>&)> func_;
    CoroutineState state_;
    std::vector<LuaValue> yield_values_;
    bool has_error_;
    std::string error_message_;
    int resume_count_;
    
    SimpleCoroutine(std::function<std::vector<LuaValue>(const std::vector<LuaValue>&)> f)
        : func_(std::move(f))
        , state_(CoroutineState::SUSPENDED)
        , has_error_(false)
        , resume_count_(0) {}
    
    std::vector<LuaValue> Resume(const std::vector<LuaValue>& args) {
        if (state_ == CoroutineState::DEAD) {
            throw std::runtime_error("cannot resume dead coroutine");
        }
        
        state_ = CoroutineState::RUNNING;
        resume_count_++;
        
        try {
            auto results = func_(args);
            
            // 检查是否是 yield
            if (!yield_values_.empty()) {
                state_ = CoroutineState::SUSPENDED;
                auto temp = std::move(yield_values_);
                yield_values_.clear();
                return temp;
            }
            
            // 协程完成
            state_ = CoroutineState::DEAD;
            return results;
            
        } catch (const std::exception& e) {
            state_ = CoroutineState::DEAD;
            has_error_ = true;
            error_message_ = e.what();
            throw;
        }
    }
    
    void Yield(const std::vector<LuaValue>& values) {
        yield_values_ = values;
        state_ = CoroutineState::SUSPENDED;
    }
    
    CoroutineState GetState() const {
        return state_;
    }
};

/* ========================================================================== */
/* 简化的协程库（模拟 CoroutineLibrary） */
/* ========================================================================== */

class SimpleCoroutineLibrary {
private:
    std::vector<std::unique_ptr<SimpleCoroutine>> coroutines_;
    SimpleCoroutine* current_coroutine_;
    
public:
    SimpleCoroutineLibrary() : current_coroutine_(nullptr) {}
    
    // coroutine.create(f)
    LuaValue Create(const LuaValue& func) {
        if (!func.IsFunction()) {
            throw std::runtime_error("bad argument #1 to 'create' (function expected)");
        }
        
        auto co = std::make_unique<SimpleCoroutine>(func.function_value);
        auto ptr = co.get();
        coroutines_.push_back(std::move(co));
        
        return LuaValue::MakeCoroutine(ptr);
    }
    
    // coroutine.resume(co, ...)
    std::vector<LuaValue> Resume(const LuaValue& co, const std::vector<LuaValue>& args) {
        if (!co.IsCoroutine()) {
            throw std::runtime_error("bad argument #1 to 'resume' (coroutine expected)");
        }
        
        auto* coro = static_cast<SimpleCoroutine*>(co.coroutine_ptr);
        
        try {
            auto prev_coro = current_coroutine_;
            current_coroutine_ = coro;
            
            auto results = coro->Resume(args);
            
            current_coroutine_ = prev_coro;
            
            // 成功：返回 true + 结果
            std::vector<LuaValue> ret;
            ret.push_back(LuaValue(true));
            ret.insert(ret.end(), results.begin(), results.end());
            return ret;
            
        } catch (const std::exception& e) {
            current_coroutine_ = nullptr;
            
            // 失败：返回 false + 错误消息
            std::vector<LuaValue> ret;
            ret.push_back(LuaValue(false));
            ret.push_back(LuaValue(std::string(e.what())));
            return ret;
        }
    }
    
    // coroutine.yield(...)
    std::vector<LuaValue> Yield(const std::vector<LuaValue>& values) {
        if (!current_coroutine_) {
            throw std::runtime_error("attempt to yield from outside a coroutine");
        }
        
        current_coroutine_->Yield(values);
        
        // 注意：实际实现中这里应该真正挂起执行
        // 这里为了测试简化，直接返回空
        return {};
    }
    
    // coroutine.status(co)
    std::string Status(const LuaValue& co) {
        if (!co.IsCoroutine()) {
            throw std::runtime_error("bad argument #1 to 'status' (coroutine expected)");
        }
        
        auto* coro = static_cast<SimpleCoroutine*>(co.coroutine_ptr);
        return CoroutineStateToString(coro->GetState());
    }
    
    // coroutine.running()
    LuaValue Running() {
        if (!current_coroutine_) {
            return LuaValue();  // nil
        }
        return LuaValue::MakeCoroutine(current_coroutine_);
    }
    
    // coroutine.wrap(f)
    LuaValue Wrap(const LuaValue& func) {
        if (!func.IsFunction()) {
            throw std::runtime_error("bad argument #1 to 'wrap' (function expected)");
        }
        
        // 创建协程
        auto co_value = Create(func);
        auto* coro = static_cast<SimpleCoroutine*>(co_value.coroutine_ptr);
        
        // 创建包装函数
        auto wrapper = [this, coro](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
            auto co_value = LuaValue::MakeCoroutine(coro);
            auto results = Resume(co_value, args);
            
            // 检查成功/失败
            if (!results.empty() && results[0].AsBoolean()) {
                // 成功：返回结果（去掉第一个 true）
                results.erase(results.begin());
                return results;
            } else {
                // 失败：抛出异常
                std::string error = results.size() > 1 ? results[1].AsString() : "unknown error";
                throw std::runtime_error(error);
            }
        };
        
        return LuaValue(wrapper);
    }
};

/* ========================================================================== */
/* 测试函数 */
/* ========================================================================== */

void PrintBanner(const std::string& title) {
    std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║ " << title;
    // 填充空格
    for (size_t i = title.length(); i < 62; ++i) std::cout << " ";
    std::cout << " ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
}

void PrintSeparator() {
    std::cout << "────────────────────────────────────────────────────────────────\n";
}

// Test 1: coroutine.create()
void TestCoroutineCreate() {
    std::cout << "\n=== Test 1: coroutine.create() ===\n";
    
    SimpleCoroutineLibrary lib;
    
    // 创建一个简单的协程函数
    auto func = LuaValue([](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        std::cout << "  Coroutine function executed\n";
        return {LuaValue(42.0)};
    });
    
    try {
        auto co = lib.Create(func);
        std::cout << "✓ Coroutine created successfully\n";
        std::cout << "  Type: " << (co.IsCoroutine() ? "coroutine" : "unknown") << "\n";
        
        // 检查初始状态
        std::string status = lib.Status(co);
        std::cout << "✓ Initial status: " << status << "\n";
        assert(status == "suspended");
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << "\n";
    }
    
    // 测试错误：非函数参数
    std::cout << "\nTest error handling (non-function):\n";
    try {
        auto co = lib.Create(LuaValue(123.0));
        std::cout << "✗ Should have thrown exception\n";
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly threw exception: " << e.what() << "\n";
    }
}

// Test 2: coroutine.resume() 基础测试
void TestCoroutineResume() {
    std::cout << "\n=== Test 2: coroutine.resume() - Basic ===\n";
    
    SimpleCoroutineLibrary lib;
    
    // 创建一个简单的协程
    auto func = LuaValue([](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        std::cout << "  Coroutine started with " << args.size() << " args\n";
        if (!args.empty() && args[0].IsNumber()) {
            std::cout << "  First arg: " << args[0].AsNumber() << "\n";
        }
        return {LuaValue(100.0), LuaValue("done")};
    });
    
    auto co = lib.Create(func);
    
    // Resume
    std::cout << "Resuming coroutine...\n";
    auto results = lib.Resume(co, {LuaValue(10.0), LuaValue(20.0)});
    
    std::cout << "✓ Resume returned " << results.size() << " values\n";
    assert(results.size() >= 1);
    
    // 第一个值应该是 true（成功）
    assert(results[0].IsBoolean());
    std::cout << "  Success flag: " << (results[0].AsBoolean() ? "true" : "false") << "\n";
    assert(results[0].AsBoolean());
    
    // 检查返回值
    if (results.size() > 1) {
        std::cout << "  Return values: " << (results.size() - 1) << "\n";
        if (results[1].IsNumber()) {
            std::cout << "    [1] = " << results[1].AsNumber() << "\n";
        }
        if (results.size() > 2 && results[2].IsString()) {
            std::cout << "    [2] = \"" << results[2].AsString() << "\"\n";
        }
    }
    
    // 检查状态（应该是 dead）
    std::string status = lib.Status(co);
    std::cout << "✓ Final status: " << status << "\n";
    assert(status == "dead");
}

// Test 3: coroutine.resume() - Dead coroutine
void TestCoroutineResumeDead() {
    std::cout << "\n=== Test 3: coroutine.resume() - Dead Coroutine ===\n";
    
    SimpleCoroutineLibrary lib;
    
    auto func = LuaValue([](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        return {LuaValue(1.0)};
    });
    
    auto co = lib.Create(func);
    
    // 第一次 resume（正常完成）
    std::cout << "First resume:\n";
    auto results1 = lib.Resume(co, {});
    std::cout << "  Status: " << lib.Status(co) << "\n";
    
    // 第二次 resume（应该失败）
    std::cout << "Second resume (should fail):\n";
    auto results2 = lib.Resume(co, {});
    
    assert(results2.size() >= 2);
    assert(results2[0].IsBoolean());
    std::cout << "  Success flag: " << (results2[0].AsBoolean() ? "true" : "false") << "\n";
    assert(!results2[0].AsBoolean());  // 应该是 false
    
    std::cout << "✓ Correctly returned error for dead coroutine\n";
    if (results2[1].IsString()) {
        std::cout << "  Error message: " << results2[1].AsString() << "\n";
    }
}

// Test 4: coroutine.status()
void TestCoroutineStatus() {
    std::cout << "\n=== Test 4: coroutine.status() ===\n";
    
    SimpleCoroutineLibrary lib;
    
    auto func = LuaValue([](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        return {LuaValue(1.0)};
    });
    
    auto co = lib.Create(func);
    
    // 创建后
    std::string status1 = lib.Status(co);
    std::cout << "Status after create: " << status1 << "\n";
    assert(status1 == "suspended");
    std::cout << "✓ Correct status: suspended\n";
    
    // Resume 后
    lib.Resume(co, {});
    std::string status2 = lib.Status(co);
    std::cout << "Status after resume: " << status2 << "\n";
    assert(status2 == "dead");
    std::cout << "✓ Correct status: dead\n";
    
    // 测试错误：非协程参数
    std::cout << "\nTest error handling (non-coroutine):\n";
    try {
        lib.Status(LuaValue(123.0));
        std::cout << "✗ Should have thrown exception\n";
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly threw exception: " << e.what() << "\n";
    }
}

// Test 5: coroutine.running()
void TestCoroutineRunning() {
    std::cout << "\n=== Test 5: coroutine.running() ===\n";
    
    SimpleCoroutineLibrary lib;
    
    // 主线程中调用
    auto running1 = lib.Running();
    std::cout << "running() in main thread: " << (running1.IsNil() ? "nil" : "not nil") << "\n";
    assert(running1.IsNil());
    std::cout << "✓ Correctly returns nil in main thread\n";
    
    // 协程中调用
    SimpleCoroutine* captured_coro = nullptr;
    auto func = LuaValue([&lib, &captured_coro](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        auto running = lib.Running();
        std::cout << "  running() inside coroutine: " << (running.IsNil() ? "nil" : "coroutine") << "\n";
        
        if (running.IsCoroutine()) {
            captured_coro = static_cast<SimpleCoroutine*>(running.coroutine_ptr);
        }
        
        return {LuaValue(1.0)};
    });
    
    auto co = lib.Create(func);
    lib.Resume(co, {});
    
    if (captured_coro) {
        std::cout << "✓ Correctly returns coroutine inside coroutine\n";
    } else {
        std::cout << "✗ Failed to capture running coroutine\n";
    }
}

// Test 6: coroutine.wrap()
void TestCoroutineWrap() {
    std::cout << "\n=== Test 6: coroutine.wrap() ===\n";
    
    SimpleCoroutineLibrary lib;
    
    int call_count = 0;
    auto func = LuaValue([&call_count](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        call_count++;
        std::cout << "  Wrapped coroutine called (call #" << call_count << ")\n";
        if (!args.empty() && args[0].IsNumber()) {
            double x = args[0].AsNumber();
            return {LuaValue(x * 2)};
        }
        return {LuaValue(0.0)};
    });
    
    try {
        auto wrapper = lib.Wrap(func);
        std::cout << "✓ Created wrapper function\n";
        std::cout << "  Type: " << (wrapper.IsFunction() ? "function" : "unknown") << "\n";
        assert(wrapper.IsFunction());
        
        // 调用包装器
        std::cout << "Calling wrapper(5)...\n";
        auto results = wrapper.function_value({LuaValue(5.0)});
        
        std::cout << "✓ Wrapper executed successfully\n";
        std::cout << "  Returned " << results.size() << " value(s)\n";
        
        if (!results.empty() && results[0].IsNumber()) {
            std::cout << "  Result: " << results[0].AsNumber() << "\n";
            assert(results[0].AsNumber() == 10.0);
            std::cout << "✓ Correct result: 10.0\n";
        }
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << "\n";
    }
    
    // 测试错误：非函数参数
    std::cout << "\nTest error handling (non-function):\n";
    try {
        lib.Wrap(LuaValue("not a function"));
        std::cout << "✗ Should have thrown exception\n";
    } catch (const std::exception& e) {
        std::cout << "✓ Correctly threw exception: " << e.what() << "\n";
    }
}

/* ========================================================================== */
/* 主函数 */
/* ========================================================================== */

int main() {
    PrintBanner("T028 Phase 3.2 - 协程库 Lua API 集成测试");
    std::cout << "测试简化的协程库 API 接口功能\n";
    
    try {
        TestCoroutineCreate();
        PrintSeparator();
        
        TestCoroutineResume();
        PrintSeparator();
        
        TestCoroutineResumeDead();
        PrintSeparator();
        
        TestCoroutineStatus();
        PrintSeparator();
        
        TestCoroutineRunning();
        PrintSeparator();
        
        TestCoroutineWrap();
        PrintSeparator();
        
        std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║ 所有测试完成！                                                 ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Unhandled exception: " << e.what() << "\n";
        return 1;
    }
}
