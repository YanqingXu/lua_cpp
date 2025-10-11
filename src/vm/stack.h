#pragma once

#include "core/lua_common.h"
#include "types/value.h"
#include "core/lua_errors.h"
#include <vector>
#include <memory>

namespace lua_cpp {

/* ========================================================================== */
/* 堆栈错误类型 */
/* ========================================================================== */

/**
 * @brief 堆栈溢出错误
 */
class StackOverflowError : public LuaError {
public:
    explicit StackOverflowError(const std::string& message = "Stack overflow")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief 堆栈下溢错误
 */
class StackUnderflowError : public LuaError {
public:
    explicit StackUnderflowError(const std::string& message = "Stack underflow")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/**
 * @brief 堆栈索引错误
 */
class StackIndexError : public LuaError {
public:
    explicit StackIndexError(const std::string& message = "Invalid stack index")
        : LuaError(message, ErrorType::RUNTIME_ERROR) {}
};

/* ========================================================================== */
/* 堆栈配置 */
/* ========================================================================== */

/**
 * @brief 堆栈配置常量
 */
constexpr Size VM_MIN_STACK_SIZE = 20;      // 最小堆栈大小
constexpr Size VM_DEFAULT_STACK_SIZE = 256; // 默认堆栈大小
constexpr Size VM_MAX_STACK_SIZE = 65536;   // 最大堆栈大小
constexpr Size VM_STACK_GROW_FACTOR = 2;    // 堆栈增长因子

/* ========================================================================== */
/* Lua值堆栈类 */
/* ========================================================================== */

/**
 * @brief Lua值堆栈类
 * 
 * 管理Lua虚拟机的值堆栈，提供高效的值存储和访问
 * 支持动态扩展和边界检查
 */
class LuaStack {
public:
    /**
     * @brief 构造函数
     * @param initial_size 初始堆栈大小
     * @param max_size 最大堆栈大小
     */
    explicit LuaStack(Size initial_size = VM_DEFAULT_STACK_SIZE, 
                     Size max_size = VM_MAX_STACK_SIZE);
    
    /**
     * @brief 析构函数
     */
    ~LuaStack() = default;
    
    // 禁用拷贝，允许移动
    LuaStack(const LuaStack&) = delete;
    LuaStack& operator=(const LuaStack&) = delete;
    LuaStack(LuaStack&&) = default;
    LuaStack& operator=(LuaStack&&) = default;
    
    /* ====================================================================== */
    /* 基本堆栈操作 */
    /* ====================================================================== */
    
    /**
     * @brief 推入值到栈顶
     * @param value 要推入的值
     */
    void Push(const LuaValue& value);
    
    /**
     * @brief 推入值到栈顶（移动语义）
     * @param value 要推入的值
     */
    void Push(LuaValue&& value);
    
    /**
     * @brief 弹出栈顶值
     * @return 弹出的值
     * @throws StackUnderflowError 如果堆栈为空
     */
    LuaValue Pop();
    
    /**
     * @brief 查看栈顶值（不弹出）
     * @return 栈顶值的引用
     * @throws StackUnderflowError 如果堆栈为空
     */
    const LuaValue& Top() const;
    
    /**
     * @brief 查看栈顶值（可修改，不弹出）
     * @return 栈顶值的引用
     * @throws StackUnderflowError 如果堆栈为空
     */
    LuaValue& Top();
    
    /* ====================================================================== */
    /* 索引访问 */
    /* ====================================================================== */
    
    /**
     * @brief 获取指定索引的值
     * @param index 堆栈索引（0为栈底）
     * @return 值的常量引用
     * @throws StackIndexError 如果索引无效
     */
    const LuaValue& Get(Size index) const;
    
    /**
     * @brief 获取指定索引的值（可修改）
     * @param index 堆栈索引（0为栈底）
     * @return 值的引用
     * @throws StackIndexError 如果索引无效
     */
    LuaValue& Get(Size index);
    
    /**
     * @brief 设置指定索引的值
     * @param index 堆栈索引（0为栈底）
     * @param value 新值
     * @throws StackIndexError 如果索引无效
     */
    void Set(Size index, const LuaValue& value);
    
    /**
     * @brief 设置指定索引的值（移动语义）
     * @param index 堆栈索引（0为栈底）
     * @param value 新值
     * @throws StackIndexError 如果索引无效
     */
    void Set(Size index, LuaValue&& value);
    
    /**
     * @brief 操作符重载：索引访问
     */
    const LuaValue& operator[](Size index) const { return Get(index); }
    LuaValue& operator[](Size index) { return Get(index); }
    
    /* ====================================================================== */
    /* 堆栈状态查询 */
    /* ====================================================================== */
    
    /**
     * @brief 获取当前堆栈大小（元素数量）
     */
    Size GetTop() const { return top_; }
    
    /**
     * @brief 获取堆栈容量
     */
    Size GetCapacity() const { return stack_.size(); }
    
    /**
     * @brief 获取最大堆栈大小
     */
    Size GetMaxSize() const { return max_size_; }
    
    /**
     * @brief 检查堆栈是否为空
     */
    bool IsEmpty() const { return top_ == 0; }
    
    /**
     * @brief 检查堆栈是否已满
     */
    bool IsFull() const { return top_ >= max_size_; }
    
    /**
     * @brief 获取剩余容量
     */
    Size GetAvailableSpace() const { return max_size_ - top_; }
    
    /* ====================================================================== */
    /* 堆栈管理 */
    /* ====================================================================== */
    
    /**
     * @brief 设置堆栈顶部位置
     * @param new_top 新的栈顶位置
     * @note 如果new_top小于当前位置，多余的元素会被丢弃
     * @note 如果new_top大于当前位置，新位置会被nil填充
     */
    void SetTop(Size new_top);
    
    /**
     * @brief 确保堆栈有足够空间
     * @param required_space 需要的空间大小
     * @throws StackOverflowError 如果无法提供足够空间
     */
    void EnsureSpace(Size required_space);
    
    /**
     * @brief 扩展堆栈容量
     * @param new_capacity 新容量
     * @throws StackOverflowError 如果超过最大大小
     */
    void Grow(Size new_capacity);
    
    /**
     * @brief 清空堆栈
     */
    void Clear();
    
    /**
     * @brief 重置堆栈到初始状态
     */
    void Reset();
    
    /* ====================================================================== */
    /* 批量操作 */
    /* ====================================================================== */
    
    /**
     * @brief 推入多个值
     * @param values 要推入的值列表
     */
    void PushMultiple(const std::vector<LuaValue>& values);
    
    /**
     * @brief 弹出多个值
     * @param count 要弹出的值数量
     * @return 弹出的值列表（从栈顶到栈底的顺序）
     */
    std::vector<LuaValue> PopMultiple(Size count);
    
    /**
     * @brief 复制栈顶的N个值
     * @param count 要复制的值数量
     */
    void DuplicateTop(Size count = 1);
    
    /**
     * @brief 交换两个栈位置的值
     * @param index1 第一个位置
     * @param index2 第二个位置
     */
    void Swap(Size index1, Size index2);
    
    /**
     * @brief 旋转栈顶的N个值
     * @param count 参与旋转的值数量
     * @param direction 旋转方向（正数向右，负数向左）
     */
    void Rotate(Size count, int direction);
    
    /* ====================================================================== */
    /* 调试和诊断 */
    /* ====================================================================== */
    
    /**
     * @brief 获取堆栈使用统计
     */
    struct StackStats {
        Size current_size;      // 当前大小
        Size capacity;          // 当前容量
        Size max_size;          // 最大大小
        Size peak_usage;        // 峰值使用量
        Size grow_count;        // 扩展次数
    };
    
    /**
     * @brief 获取堆栈统计信息
     */
    StackStats GetStats() const;
    
    /**
     * @brief 获取堆栈内容的字符串表示（用于调试）
     * @param max_elements 最多显示的元素数量
     */
    std::string ToString(Size max_elements = 10) const;
    
    /**
     * @brief 验证堆栈完整性
     * @return 如果堆栈状态有效返回true
     */
    bool ValidateIntegrity() const;

private:
    /* ====================================================================== */
    /* 内部方法 */
    /* ====================================================================== */
    
    /**
     * @brief 检查索引是否有效
     * @param index 要检查的索引
     * @throws StackIndexError 如果索引无效
     */
    void CheckIndex(Size index) const;
    
    /**
     * @brief 检查是否有足够空间
     * @param required_space 需要的空间
     * @throws StackOverflowError 如果空间不足且无法扩展
     */
    void CheckSpace(Size required_space);
    
    /**
     * @brief 计算新的堆栈容量
     * @param required_size 需要的最小大小
     * @return 新容量
     */
    Size CalculateNewCapacity(Size required_size) const;
    
    /* ====================================================================== */
    /* 成员变量 */
    /* ====================================================================== */
    
    std::vector<LuaValue> stack_;   // 堆栈存储
    Size top_;                      // 栈顶位置（元素数量）
    Size max_size_;                 // 最大堆栈大小
    Size initial_size_;             // 初始大小
    
    // 统计信息
    mutable Size peak_usage_;       // 峰值使用量
    mutable Size grow_count_;       // 扩展次数
};

/* ========================================================================== */
/* 堆栈工厂函数 */
/* ========================================================================== */

/**
 * @brief 创建标准大小的堆栈
 * @return 堆栈实例
 */
std::unique_ptr<LuaStack> CreateStandardStack();

/**
 * @brief 创建小型堆栈（用于嵌入式环境）
 * @return 堆栈实例
 */
std::unique_ptr<LuaStack> CreateSmallStack();

/**
 * @brief 创建大型堆栈（用于大量数据处理）
 * @return 堆栈实例
 */
std::unique_ptr<LuaStack> CreateLargeStack();

} // namespace lua_cpp