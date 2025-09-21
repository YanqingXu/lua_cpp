/**
 * @file stack.cpp
 * @brief Lua堆栈实现
 * @description 实现Lua虚拟机的值堆栈管理
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "stack.h"
#include <algorithm>
#include <sstream>

namespace lua_cpp {

/* ========================================================================== */
/* LuaStack构造和析构 */
/* ========================================================================== */

LuaStack::LuaStack(Size initial_size, Size max_size)
    : max_size_(max_size), current_top_(0) {
    
    if (initial_size < VM_MIN_STACK_SIZE) {
        initial_size = VM_MIN_STACK_SIZE;
    }
    
    if (initial_size > max_size) {
        initial_size = max_size;
    }
    
    stack_.reserve(initial_size);
    stack_.resize(initial_size, LuaValue());
}

/* ========================================================================== */
/* 基本堆栈操作 */
/* ========================================================================== */

void LuaStack::Push(const LuaValue& value) {
    EnsureCapacity(current_top_ + 1);
    
    if (current_top_ >= stack_.size()) {
        stack_.push_back(value);
    } else {
        stack_[current_top_] = value;
    }
    
    current_top_++;
}

void LuaStack::Push(LuaValue&& value) {
    EnsureCapacity(current_top_ + 1);
    
    if (current_top_ >= stack_.size()) {
        stack_.push_back(std::move(value));
    } else {
        stack_[current_top_] = std::move(value);
    }
    
    current_top_++;
}

LuaValue LuaStack::Pop() {
    if (current_top_ == 0) {
        throw StackUnderflowError("Cannot pop from empty stack");
    }
    
    current_top_--;
    return std::move(stack_[current_top_]);
}

const LuaValue& LuaStack::Top() const {
    if (current_top_ == 0) {
        throw StackUnderflowError("Cannot access top of empty stack");
    }
    
    return stack_[current_top_ - 1];
}

LuaValue& LuaStack::Top() {
    if (current_top_ == 0) {
        throw StackUnderflowError("Cannot access top of empty stack");
    }
    
    return stack_[current_top_ - 1];
}

void LuaStack::SetTop(Size new_top) {
    if (new_top > max_size_) {
        throw StackOverflowError("Stack top exceeds maximum size");
    }
    
    if (new_top > stack_.size()) {
        // 扩展栈
        stack_.resize(new_top, LuaValue());
    }
    
    current_top_ = new_top;
}

/* ========================================================================== */
/* 索引访问 */
/* ========================================================================== */

const LuaValue& LuaStack::Get(Size index) const {
    if (index >= current_top_) {
        throw StackIndexError("Stack index out of range: " + std::to_string(index));
    }
    
    return stack_[index];
}

LuaValue& LuaStack::Get(Size index) {
    if (index >= current_top_) {
        throw StackIndexError("Stack index out of range: " + std::to_string(index));
    }
    
    return stack_[index];
}

void LuaStack::Set(Size index, const LuaValue& value) {
    if (index >= max_size_) {
        throw StackIndexError("Stack index exceeds maximum size: " + std::to_string(index));
    }
    
    // 如果索引超出当前大小，扩展栈
    if (index >= stack_.size()) {
        stack_.resize(index + 1, LuaValue());
    }
    
    stack_[index] = value;
    
    // 更新栈顶指针
    if (index >= current_top_) {
        current_top_ = index + 1;
    }
}

void LuaStack::Set(Size index, LuaValue&& value) {
    if (index >= max_size_) {
        throw StackIndexError("Stack index exceeds maximum size: " + std::to_string(index));
    }
    
    // 如果索引超出当前大小，扩展栈
    if (index >= stack_.size()) {
        stack_.resize(index + 1, LuaValue());
    }
    
    stack_[index] = std::move(value);
    
    // 更新栈顶指针
    if (index >= current_top_) {
        current_top_ = index + 1;
    }
}

/* ========================================================================== */
/* Lua式索引访问 */
/* ========================================================================== */

const LuaValue& LuaStack::GetLuaIndex(int index) const {
    Size actual_index = ConvertLuaIndex(index);
    return Get(actual_index);
}

LuaValue& LuaStack::GetLuaIndex(int index) {
    Size actual_index = ConvertLuaIndex(index);
    return Get(actual_index);
}

void LuaStack::SetLuaIndex(int index, const LuaValue& value) {
    Size actual_index = ConvertLuaIndex(index);
    Set(actual_index, value);
}

void LuaStack::SetLuaIndex(int index, LuaValue&& value) {
    Size actual_index = ConvertLuaIndex(index);
    Set(actual_index, std::move(value));
}

/* ========================================================================== */
/* 批量操作 */
/* ========================================================================== */

void LuaStack::PushMultiple(const std::vector<LuaValue>& values) {
    EnsureCapacity(current_top_ + values.size());
    
    for (const auto& value : values) {
        Push(value);
    }
}

std::vector<LuaValue> LuaStack::PopMultiple(Size count) {
    if (count > current_top_) {
        throw StackUnderflowError("Cannot pop " + std::to_string(count) + 
                                 " values from stack with " + std::to_string(current_top_) + " elements");
    }
    
    std::vector<LuaValue> result;
    result.reserve(count);
    
    for (Size i = 0; i < count; i++) {
        result.push_back(Pop());
    }
    
    // 反转顺序（因为Pop是从栈顶开始的）
    std::reverse(result.begin(), result.end());
    
    return result;
}

void LuaStack::Insert(Size index, const LuaValue& value) {
    if (index > current_top_) {
        throw StackIndexError("Insert index out of range: " + std::to_string(index));
    }
    
    EnsureCapacity(current_top_ + 1);
    
    // 确保栈有足够大小
    if (current_top_ >= stack_.size()) {
        stack_.resize(current_top_ + 1, LuaValue());
    }
    
    // 向后移动元素
    for (Size i = current_top_; i > index; i--) {
        stack_[i] = std::move(stack_[i - 1]);
    }
    
    stack_[index] = value;
    current_top_++;
}

void LuaStack::Remove(Size index) {
    if (index >= current_top_) {
        throw StackIndexError("Remove index out of range: " + std::to_string(index));
    }
    
    // 向前移动元素
    for (Size i = index; i < current_top_ - 1; i++) {
        stack_[i] = std::move(stack_[i + 1]);
    }
    
    current_top_--;
}

/* ========================================================================== */
/* 栈状态查询 */
/* ========================================================================== */

Size LuaStack::Size() const {
    return current_top_;
}

Size LuaStack::Capacity() const {
    return stack_.size();
}

Size LuaStack::MaxSize() const {
    return max_size_;
}

bool LuaStack::IsEmpty() const {
    return current_top_ == 0;
}

bool LuaStack::IsFull() const {
    return current_top_ >= max_size_;
}

/* ========================================================================== */
/* 栈管理 */
/* ========================================================================== */

void LuaStack::Clear() {
    current_top_ = 0;
    // 不需要清理内存，只需重置栈顶指针
}

void LuaStack::Reserve(Size capacity) {
    if (capacity > max_size_) {
        throw StackOverflowError("Reserve capacity exceeds maximum size");
    }
    
    stack_.reserve(capacity);
}

void LuaStack::Resize(Size new_size) {
    if (new_size > max_size_) {
        throw StackOverflowError("Resize exceeds maximum size");
    }
    
    stack_.resize(new_size, LuaValue());
    
    if (current_top_ > new_size) {
        current_top_ = new_size;
    }
}

void LuaStack::ShrinkToFit() {
    // 缩小到当前使用的大小，但保持最小大小
    Size target_size = std::max(current_top_, VM_MIN_STACK_SIZE);
    
    if (stack_.size() > target_size * 2) {
        stack_.resize(target_size);
        stack_.shrink_to_fit();
    }
}

/* ========================================================================== */
/* 调试和诊断 */
/* ========================================================================== */

std::string LuaStack::ToString() const {
    std::ostringstream oss;
    oss << "LuaStack[size=" << current_top_ << ", capacity=" << stack_.size() << "]:\n";
    
    for (Size i = 0; i < current_top_; i++) {
        oss << "  [" << i << "] = " << stack_[i].ToString();
        if (i == current_top_ - 1) {
            oss << " <- top";
        }
        oss << "\n";
    }
    
    return oss.str();
}

void LuaStack::Dump() const {
    std::cout << ToString() << std::endl;
}

bool LuaStack::CheckConsistency() const {
    // 检查栈的一致性
    if (current_top_ > stack_.size()) {
        return false;
    }
    
    if (current_top_ > max_size_) {
        return false;
    }
    
    return true;
}

/* ========================================================================== */
/* 内部辅助方法 */
/* ========================================================================== */

void LuaStack::EnsureCapacity(Size required_capacity) {
    if (required_capacity > max_size_) {
        throw StackOverflowError("Stack overflow: required " + std::to_string(required_capacity) + 
                                 ", maximum " + std::to_string(max_size_));
    }
    
    if (required_capacity > stack_.size()) {
        Size new_capacity = stack_.size() * VM_STACK_GROW_FACTOR;
        
        if (new_capacity < required_capacity) {
            new_capacity = required_capacity;
        }
        
        if (new_capacity > max_size_) {
            new_capacity = max_size_;
        }
        
        stack_.resize(new_capacity, LuaValue());
    }
}

Size LuaStack::ConvertLuaIndex(int index) const {
    if (index > 0) {
        // 正索引：1-based转0-based
        if (index > static_cast<int>(current_top_)) {
            throw StackIndexError("Positive Lua index out of range: " + std::to_string(index));
        }
        return static_cast<Size>(index - 1);
    } else if (index < 0) {
        // 负索引：从栈顶倒数
        Size abs_index = static_cast<Size>(-index);
        if (abs_index > current_top_) {
            throw StackIndexError("Negative Lua index out of range: " + std::to_string(index));
        }
        return current_top_ - abs_index;
    } else {
        // index == 0，在Lua中无效
        throw StackIndexError("Lua index cannot be 0");
    }
}

} // namespace lua_cpp