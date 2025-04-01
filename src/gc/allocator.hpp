#pragma once

#include "types.hpp"

namespace Lua {
namespace GC {

/**
 * @brief 内存分配器类，负责管理Lua使用的内存
 * 
 * Allocator提供内存分配、释放和重分配功能，同时追踪内存使用情况。
 * 它支持不同大小内存块的分配，并实现了内存池策略以提高效率并减少碎片。
 */
class Allocator {
public:
    // 内存块大小类别，用于内存池管理
    enum class SizeClass {
        Tiny,      // 8-32字节
        Small,     // 33-128字节
        Medium,    // 129-512字节
        Large,     // 513-4096字节
        Huge       // >4096字节，直接使用系统分配
    };
    
    // 构造和析构
    Allocator();
    ~Allocator();
    
    // 禁止拷贝
    Allocator(const Allocator&) = delete;
    Allocator& operator=(const Allocator&) = delete;
    
    /**
     * @brief 分配内存
     * 
     * @param size 请求的内存大小（字节）
     * @return void* 分配的内存指针，失败时返回nullptr
     */
    void* allocate(usize size);
    
    /**
     * @brief 释放内存
     * 
     * @param ptr 要释放的内存指针
     * @param size 内存块大小（与分配时相同）
     */
    void deallocate(void* ptr, usize size);
    
    /**
     * @brief 重新分配内存块大小
     * 
     * @param ptr 原内存块指针
     * @param oldSize 原内存块大小
     * @param newSize 请求的新大小
     * @return void* 新分配的内存指针
     */
    void* reallocate(void* ptr, usize oldSize, usize newSize);
    
    /**
     * @brief 获取当前分配的总内存量
     * 
     * @return usize 分配的内存字节数
     */
    usize getTotalAllocated() const { return m_totalAllocated; }
    
    /**
     * @brief 获取分配操作的计数
     * 
     * @return usize 分配操作次数
     */
    usize getAllocationCount() const { return m_allocCount; }
    
    /**
     * @brief 获取释放操作的计数
     * 
     * @return usize 释放操作次数
     */
    usize getDeallocationCount() const { return m_deallocCount; }
    
    /**
     * @brief 设置出错处理函数
     * 
     * @param handler 错误处理函数
     */
    void setOutOfMemoryHandler(std::function<void()> handler) {
        m_outOfMemoryHandler = handler;
    }
    
private:
    /**
     * @brief 内存块头部结构，用于内存池管理
     */
    struct BlockHeader {
        usize size;           // 内存块大小
        SizeClass sizeClass;  // 大小类别
        bool inUse;           // 是否在使用中
        BlockHeader* next;    // 链表中的下一个块
    };
    
    // 确定内存大小对应的大小类别
    SizeClass getSizeClass(usize size) const;
    
    // 从内存池分配内存
    void* allocateFromPool(usize size, SizeClass sizeClass);
    
    // 向内存池返回内存
    void returnToPool(void* ptr, BlockHeader* header);
    
    // 直接从系统分配大块内存
    void* allocateLarge(usize size);
    
    // 释放直接分配的大块内存
    void deallocateLarge(void* ptr);
    
    // 各大小类别的空闲内存池
    BlockHeader* m_freeList[static_cast<int>(SizeClass::Huge)];
    
    // 内存使用统计
    usize m_totalAllocated;  // 总分配内存量
    usize m_allocCount;      // 分配操作计数
    usize m_deallocCount;    // 释放操作计数
    
    // 内存分配失败处理器
    std::function<void()> m_outOfMemoryHandler;
    
    // 内存池大小配置
    static constexpr usize TINY_BLOCK_SIZE = 32;
    static constexpr usize SMALL_BLOCK_SIZE = 128;
    static constexpr usize MEDIUM_BLOCK_SIZE = 512;
    static constexpr usize LARGE_BLOCK_SIZE = 4096;
    
    // 池块数量
    static constexpr usize POOL_BLOCK_COUNT = 16;
};

} // namespace GC
} // namespace Lua
