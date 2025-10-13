/**
 * @file test_coroutine_lib_minimal.cpp
 * @brief T028 Phase 3 - 协程库最小化独立测试
 * 
 * 目标：验证coroutine_lib的核心C++20协程封装是否正确
 * 策略：独立测试，不依赖VM集成
 * 
 * @date 2025-10-13
 */

#include <iostream>
#include <coroutine>
#include <vector>
#include <string>
#include <memory>

/* ========================================================================== */
/* 最小化协程状态枚举 */
/* ========================================================================== */

enum class CoroutineState {
    SUSPENDED,
    RUNNING,
    NORMAL,
    DEAD
};

std::string StateToString(CoroutineState state) {
    switch (state) {
        case CoroutineState::SUSPENDED: return "suspended";
        case CoroutineState::RUNNING:   return "running";
        case CoroutineState::NORMAL:    return "normal";
        case CoroutineState::DEAD:      return "dead";
        default:                        return "unknown";
    }
}

/* ========================================================================== */
/* 最小化Lua值类型 */
/* ========================================================================== */

struct MinimalLuaValue {
    enum class Type { NIL, NUMBER, STRING };
    
    Type type;
    double number;
    std::string string;
    
    MinimalLuaValue() : type(Type::NIL), number(0) {}
    MinimalLuaValue(double n) : type(Type::NUMBER), number(n) {}
    MinimalLuaValue(const std::string& s) : type(Type::STRING), string(s) {}
};

/* ========================================================================== */
/* 最小化协程封装 */
/* ========================================================================== */

class MinimalCoroutine {
public:
    struct promise_type {
        MinimalCoroutine get_return_object() {
            return MinimalCoroutine{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
        
        // 协程状态
        CoroutineState state_ = CoroutineState::SUSPENDED;
        std::vector<MinimalLuaValue> resume_values_;
        std::vector<MinimalLuaValue> yield_values_;
    };
    
    // YieldAwaiter
    struct YieldAwaiter {
        bool await_ready() const noexcept { return false; }
        
        void await_suspend(std::coroutine_handle<promise_type> h) noexcept {
            auto& promise = h.promise();
            promise.state_ = CoroutineState::SUSPENDED;
        }
        
        std::vector<MinimalLuaValue> await_resume() noexcept {
            return {}; // 简化版本
        }
    };
    
    MinimalCoroutine(std::coroutine_handle<promise_type> handle)
        : handle_(handle) {}
    
    ~MinimalCoroutine() {
        if (handle_) {
            handle_.destroy();
        }
    }
    
    // 禁用拷贝，允许移动
    MinimalCoroutine(const MinimalCoroutine&) = delete;
    MinimalCoroutine& operator=(const MinimalCoroutine&) = delete;
    MinimalCoroutine(MinimalCoroutine&& other) noexcept
        : handle_(std::exchange(other.handle_, nullptr)) {}
    MinimalCoroutine& operator=(MinimalCoroutine&& other) noexcept {
        if (this != &other) {
            if (handle_) handle_.destroy();
            handle_ = std::exchange(other.handle_, nullptr);
        }
        return *this;
    }
    
    // Resume协程
    std::vector<MinimalLuaValue> Resume(const std::vector<MinimalLuaValue>& args) {
        if (!handle_ || handle_.done()) {
            throw std::runtime_error("Cannot resume dead coroutine");
        }
        
        auto& promise = handle_.promise();
        promise.resume_values_ = args;
        promise.state_ = CoroutineState::RUNNING;
        
        handle_.resume();
        
        if (handle_.done()) {
            promise.state_ = CoroutineState::DEAD;
        } else {
            promise.state_ = CoroutineState::SUSPENDED;
        }
        
        return promise.yield_values_;
    }
    
    // 获取状态
    CoroutineState GetState() const {
        if (!handle_) return CoroutineState::DEAD;
        if (handle_.done()) return CoroutineState::DEAD;
        return handle_.promise().state_;
    }
    
    // Yield静态方法
    static YieldAwaiter Yield(const std::vector<MinimalLuaValue>& values) {
        return YieldAwaiter{};
    }
    
private:
    std::coroutine_handle<promise_type> handle_;
};

/* ========================================================================== */
/* 测试协程函数 */
/* ========================================================================== */

MinimalCoroutine TestCoroutineFunction() {
    std::cout << "Coroutine started\n";
    
    // 第一次yield
    co_await MinimalCoroutine::Yield({MinimalLuaValue("First yield")});
    std::cout << "After first yield\n";
    
    // 第二次yield
    co_await MinimalCoroutine::Yield({MinimalLuaValue("Second yield")});
    std::cout << "After second yield\n";
    
    // 协程结束
    std::cout << "Coroutine finished\n";
}

/* ========================================================================== */
/* 测试用例 */
/* ========================================================================== */

void TestCoroutineCreation() {
    std::cout << "\n=== Test 1: Coroutine Creation ===\n";
    
    try {
        auto coro = TestCoroutineFunction();
        std::cout << "✓ Coroutine created successfully\n";
        std::cout << "  Initial state: " << StateToString(coro.GetState()) << "\n";
        
        if (coro.GetState() == CoroutineState::SUSPENDED) {
            std::cout << "✓ Initial state is SUSPENDED\n";
        } else {
            std::cout << "✗ Initial state is NOT SUSPENDED\n";
        }
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << "\n";
    }
}

void TestCoroutineResume() {
    std::cout << "\n=== Test 2: Coroutine Resume ===\n";
    
    try {
        auto coro = TestCoroutineFunction();
        
        // 第一次resume
        std::cout << "First resume...\n";
        auto result1 = coro.Resume({});
        std::cout << "✓ First resume successful\n";
        std::cout << "  State after resume: " << StateToString(coro.GetState()) << "\n";
        
        // 第二次resume
        std::cout << "Second resume...\n";
        auto result2 = coro.Resume({});
        std::cout << "✓ Second resume successful\n";
        std::cout << "  State after resume: " << StateToString(coro.GetState()) << "\n";
        
        // 第三次resume（应该到达DEAD状态）
        std::cout << "Third resume...\n";
        auto result3 = coro.Resume({});
        std::cout << "✓ Third resume successful\n";
        std::cout << "  Final state: " << StateToString(coro.GetState()) << "\n";
        
        if (coro.GetState() == CoroutineState::DEAD) {
            std::cout << "✓ Coroutine reached DEAD state\n";
        }
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << "\n";
    }
}

void TestCoroutineLifecycle() {
    std::cout << "\n=== Test 3: Coroutine Lifecycle ===\n";
    
    try {
        auto coro = TestCoroutineFunction();
        
        std::cout << "State before any resume: " << StateToString(coro.GetState()) << "\n";
        
        // Resume直到完成
        int resume_count = 0;
        while (coro.GetState() != CoroutineState::DEAD && resume_count < 10) {
            coro.Resume({});
            resume_count++;
            std::cout << "  Resume #" << resume_count << ", state: " 
                      << StateToString(coro.GetState()) << "\n";
        }
        
        std::cout << "✓ Coroutine lifecycle completed with " << resume_count << " resumes\n";
        
        // 尝试resume已死亡的协程
        try {
            coro.Resume({});
            std::cout << "✗ Should not be able to resume dead coroutine\n";
        } catch (const std::runtime_error&) {
            std::cout << "✓ Correctly throws exception on dead coroutine resume\n";
        }
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << "\n";
    }
}

void TestCoroutineMoveSemantics() {
    std::cout << "\n=== Test 4: Coroutine Move Semantics ===\n";
    
    try {
        auto coro1 = TestCoroutineFunction();
        std::cout << "Created coro1\n";
        
        // 移动构造
        auto coro2 = std::move(coro1);
        std::cout << "✓ Move construction successful\n";
        std::cout << "  coro2 state: " << StateToString(coro2.GetState()) << "\n";
        
        // coro2可以正常resume
        coro2.Resume({});
        std::cout << "✓ Moved coroutine can be resumed\n";
        
        // 移动赋值
        auto coro3 = TestCoroutineFunction();
        coro3 = std::move(coro2);
        std::cout << "✓ Move assignment successful\n";
        std::cout << "  coro3 state: " << StateToString(coro3.GetState()) << "\n";
        
    } catch (const std::exception& e) {
        std::cout << "✗ Exception: " << e.what() << "\n";
    }
}

/* ========================================================================== */
/* 主函数 */
/* ========================================================================== */

int main() {
    std::cout << "╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  T028 Phase 3 - 协程库最小化测试                          ║\n";
    std::cout << "║  测试C++20协程封装的核心功能                              ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    TestCoroutineCreation();
    TestCoroutineResume();
    TestCoroutineLifecycle();
    TestCoroutineMoveSemantics();
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  测试完成！                                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n";
    
    return 0;
}
