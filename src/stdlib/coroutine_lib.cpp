/**
 * @file coroutine_lib.cpp
 * @brief T028 Lua协程标准库实现
 * 
 * @author Lua C++ Project Team
 * @date 2025-10-11
 */

#include "coroutine_lib.h"
#include "vm/enhanced_virtual_machine.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {
namespace stdlib {

/* ========================================================================== */
/* 工具函数 */
/* ========================================================================== */

std::string CoroutineStateToString(CoroutineState state) {
    switch (state) {
        case CoroutineState::SUSPENDED: return "suspended";
        case CoroutineState::RUNNING:   return "running";
        case CoroutineState::NORMAL:    return "normal";
        case CoroutineState::DEAD:      return "dead";
        default:                        return "unknown";
    }
}

/* ========================================================================== */
/* LuaCoroutine实现 */
/* ========================================================================== */

// promise_type::get_return_object实现
LuaCoroutine LuaCoroutine::promise_type::get_return_object() {
    return LuaCoroutine{
        std::coroutine_handle<promise_type>::from_promise(*this)
    };
}

LuaCoroutine::LuaCoroutine(std::coroutine_handle<promise_type> handle)
    : handle_(handle) {
    stats_.created_time = std::chrono::steady_clock::now();
    stats_.last_resume_time = stats_.created_time;
}

LuaCoroutine::~LuaCoroutine() {
    if (handle_) {
        handle_.destroy();
    }
}

LuaCoroutine::LuaCoroutine(LuaCoroutine&& other) noexcept
    : handle_(std::exchange(other.handle_, nullptr))
    , stats_(std::move(other.stats_)) {
}

LuaCoroutine& LuaCoroutine::operator=(LuaCoroutine&& other) noexcept {
    if (this != &other) {
        if (handle_) {
            handle_.destroy();
        }
        handle_ = std::exchange(other.handle_, nullptr);
        stats_ = std::move(other.stats_);
    }
    return *this;
}

std::vector<LuaValue> LuaCoroutine::Resume(const std::vector<LuaValue>& args) {
    // 检查协程是否有效
    if (!handle_) {
        throw CoroutineStateError("Cannot resume destroyed coroutine");
    }
    
    // 检查协程是否已完成
    if (handle_.done()) {
        throw CoroutineStateError("Cannot resume dead coroutine");
    }
    
    // 检查协程状态
    auto& promise = handle_.promise();
    if (promise.state_ != CoroutineState::SUSPENDED) {
        throw CoroutineStateError("Cannot resume non-suspended coroutine");
    }
    
    // 更新统计信息
    auto resume_start = std::chrono::steady_clock::now();
    stats_.resume_count++;
    stats_.last_resume_time = resume_start;
    
    // 设置resume参数
    promise.resume_values_ = args;
    promise.resume_count_++;
    promise.state_ = CoroutineState::RUNNING;
    
    // 恢复执行
    handle_.resume();
    
    // 计算运行时间
    auto resume_end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        resume_end - resume_start
    ).count();
    stats_.total_run_time_ms += duration / 1000.0;
    
    // 检查异常
    if (promise.exception_) {
        std::rethrow_exception(promise.exception_);
    }
    
    // 返回结果
    if (handle_.done() || promise.state_ == CoroutineState::DEAD) {
        // 协程结束，返回return值
        return std::move(promise.return_values_);
    } else {
        // 协程yield，返回yield值
        return std::move(promise.yield_values_);
    }
}

CoroutineState LuaCoroutine::GetState() const {
    if (!handle_) {
        return CoroutineState::DEAD;
    }
    if (handle_.done()) {
        return CoroutineState::DEAD;
    }
    return handle_.promise().state_;
}

bool LuaCoroutine::IsDone() const {
    return !handle_ || handle_.done();
}

/* ========================================================================== */
/* CoroutineLibrary实现 */
/* ========================================================================== */

CoroutineLibrary::CoroutineLibrary(EnhancedVirtualMachine* vm)
    : vm_(vm) {
}

std::vector<LuaValue> CoroutineLibrary::CallFunction(
    const std::string& name,
    const std::vector<LuaValue>& args
) {
    if (name == "create") {
        if (args.empty()) {
            throw LuaError(ErrorType::Type, "bad argument #1 to 'create' (function expected)");
        }
        return {Create(args[0])};
        
    } else if (name == "resume") {
        if (args.empty()) {
            throw LuaError(ErrorType::Type, "bad argument #1 to 'resume' (coroutine expected)");
        }
        std::vector<LuaValue> resume_args(args.begin() + 1, args.end());
        return Resume(args[0], resume_args);
        
    } else if (name == "yield") {
        return Yield(args);
        
    } else if (name == "status") {
        if (args.empty()) {
            throw LuaError(ErrorType::Type, "bad argument #1 to 'status' (coroutine expected)");
        }
        return {LuaValue(Status(args[0]))};
        
    } else if (name == "running") {
        return {Running()};
        
    } else if (name == "wrap") {
        if (args.empty()) {
            throw LuaError(ErrorType::Type, "bad argument #1 to 'wrap' (function expected)");
        }
        return {Wrap(args[0])};
        
    } else {
        throw LuaError(ErrorType::Runtime, "unknown coroutine function: " + name);
    }
}

std::vector<std::string> CoroutineLibrary::GetFunctionNames() const {
    return {
        "create",
        "resume",
        "yield",
        "status",
        "running",
        "wrap"
    };
}

LuaValue CoroutineLibrary::Create(const LuaValue& func) {
    // 验证参数
    if (!func.IsFunction()) {
        throw LuaError(ErrorType::Type, "bad argument to 'create' (function expected)");
    }
    
    // 生成协程ID
    size_t id = GenerateCoroutineId();
    
    // 注意：这里我们需要一个实际的协程函数实现
    // 由于当前没有完整的VM执行函数机制，我们创建一个简单的示例协程
    // 实际实现需要与VM集成
    
    // 创建协程对象（占位实现）
    auto coroutine = std::make_shared<LuaCoroutine>(
        [](const LuaValue& f) -> LuaCoroutine {
            // 这是一个简化的协程函数示例
            // 实际实现需要调用VM执行Lua函数
            co_return std::vector<LuaValue>{};
        }(func)
    );
    
    // 存储协程
    coroutines_[id] = coroutine;
    
    // 返回协程对象（使用ID表示）
    return LuaValue::CreateUserData(reinterpret_cast<void*>(id));
}

std::vector<LuaValue> CoroutineLibrary::Resume(
    const LuaValue& co,
    const std::vector<LuaValue>& args
) {
    // 验证并获取协程
    auto coroutine = ValidateAndGetCoroutine(co);
    
    // 保存上一个协程ID
    auto prev_coroutine = current_coroutine_id_;
    
    // 设置当前协程
    size_t co_id = reinterpret_cast<size_t>(co.GetUserData());
    
    // 如果有上一个协程，设置其状态为NORMAL
    if (prev_coroutine.has_value()) {
        auto prev_coro = coroutines_[*prev_coroutine];
        if (prev_coro && prev_coro->GetState() == CoroutineState::RUNNING) {
            // 注意：这里需要直接访问promise来修改状态
            // 在实际实现中可能需要提供额外的接口
        }
    }
    
    current_coroutine_id_ = co_id;
    
    // 执行resume
    std::vector<LuaValue> result;
    bool success = true;
    std::string error_message;
    
    try {
        result = coroutine->Resume(args);
    } catch (const CoroutineError& e) {
        success = false;
        error_message = e.what();
    } catch (const LuaError& e) {
        success = false;
        error_message = e.what();
    } catch (const std::exception& e) {
        success = false;
        error_message = std::string("coroutine error: ") + e.what();
    }
    
    // 恢复上一个协程
    current_coroutine_id_ = prev_coroutine;
    
    // 构造返回值
    std::vector<LuaValue> full_result;
    full_result.push_back(LuaValue(success));
    
    if (success) {
        full_result.insert(full_result.end(), result.begin(), result.end());
    } else {
        full_result.push_back(LuaValue(error_message));
    }
    
    return full_result;
}

std::vector<LuaValue> CoroutineLibrary::Yield(const std::vector<LuaValue>& values) {
    // 检查是否在协程中
    if (!IsInCoroutine()) {
        throw CoroutineError("attempt to yield from outside a coroutine");
    }
    
    // 获取当前协程
    auto coroutine = coroutines_[*current_coroutine_id_];
    
    // 注意：实际的yield需要在协程函数内部使用co_yield
    // 这里我们需要与VM执行机制集成
    // 当前实现是一个占位符
    
    throw CoroutineError("yield implementation requires VM integration");
}

std::string CoroutineLibrary::Status(const LuaValue& co) {
    // 验证并获取协程
    auto coroutine = ValidateAndGetCoroutine(co);
    
    // 获取协程ID
    size_t co_id = reinterpret_cast<size_t>(co.GetUserData());
    
    // 检查是否是当前运行的协程
    if (current_coroutine_id_.has_value() && *current_coroutine_id_ == co_id) {
        return "running";
    }
    
    // 检查是否是正常状态（调用了其他协程）
    if (previous_coroutine_id_.has_value() && *previous_coroutine_id_ == co_id) {
        if (current_coroutine_id_.has_value()) {
            return "normal";
        }
    }
    
    // 返回协程自身的状态
    return CoroutineStateToString(coroutine->GetState());
}

LuaValue CoroutineLibrary::Running() {
    // 如果不在协程中，返回nil
    if (!IsInCoroutine()) {
        return LuaValue::Nil();
    }
    
    // 返回当前协程对象
    return LuaValue::CreateUserData(
        reinterpret_cast<void*>(*current_coroutine_id_)
    );
}

LuaValue CoroutineLibrary::Wrap(const LuaValue& func) {
    // 验证参数
    if (!func.IsFunction()) {
        throw LuaError(ErrorType::Type, "bad argument to 'wrap' (function expected)");
    }
    
    // 创建协程
    auto co = Create(func);
    
    // 创建包装函数
    auto wrapper = [this, co](const std::vector<LuaValue>& args) -> std::vector<LuaValue> {
        auto result = Resume(co, args);
        
        // 检查第一个返回值（成功/失败）
        if (result.empty() || !result[0].GetBoolean()) {
            // 失败，抛出错误
            std::string error_msg = "coroutine error";
            if (result.size() > 1) {
                error_msg = result[1].ToString();
            }
            throw LuaError(ErrorType::Runtime, error_msg);
        }
        
        // 成功，返回实际结果（去掉第一个true）
        return std::vector<LuaValue>(result.begin() + 1, result.end());
    };
    
    // 返回C函数对象
    // 注意：这需要VM支持创建C函数对象
    // 当前返回一个占位符
    return LuaValue::CreateCFunction(nullptr);
}

size_t CoroutineLibrary::GenerateCoroutineId() {
    return next_coroutine_id_++;
}

std::shared_ptr<LuaCoroutine> CoroutineLibrary::ValidateAndGetCoroutine(const LuaValue& co) {
    // 检查是否是userdata类型
    if (!co.IsUserData()) {
        throw LuaError(ErrorType::Type, "bad argument (coroutine expected)");
    }
    
    // 获取协程ID
    size_t id = reinterpret_cast<size_t>(co.GetUserData());
    
    // 查找协程
    auto it = coroutines_.find(id);
    if (it == coroutines_.end()) {
        throw LuaError(ErrorType::Type, "invalid coroutine");
    }
    
    return it->second;
}

/* ========================================================================== */
/* 工厂函数 */
/* ========================================================================== */

std::unique_ptr<CoroutineLibrary> CreateCoroutineLibrary(EnhancedVirtualMachine* vm) {
    return std::make_unique<CoroutineLibrary>(vm);
}

} // namespace stdlib
} // namespace lua_cpp
