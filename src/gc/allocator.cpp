#include "allocator.hpp"
#include <cstdlib>
#include <new>
#include <cstring>
#include <algorithm>

namespace Lua {
namespace GC {

Allocator::Allocator()
    : m_totalAllocated(0), m_allocCount(0), m_deallocCount(0) {
    // 初始化所有空闲内存池链表为空
    for (int i = 0; i < static_cast<int>(SizeClass::Huge); ++i) {
        m_freeList[i] = nullptr;
    }
    
    // 设置默认的内存不足处理函数
    m_outOfMemoryHandler = []() {
        throw std::bad_alloc();
    };
}

Allocator::~Allocator() {
    // 释放所有内存池中的内存块
    for (int i = 0; i < static_cast<int>(SizeClass::Huge); ++i) {
        BlockHeader* current = m_freeList[i];
        while (current) {
            BlockHeader* next = current->next;
            std::free(current);
            current = next;
        }
    }
}

void* Allocator::allocate(usize size) {
    if (size == 0) {
        return nullptr;
    }
    
    // 计算需要的总大小，包括头部
    usize totalSize = size + sizeof(BlockHeader);
    
    // 确定大小类别
    SizeClass sizeClass = getSizeClass(size);
    
    // 分配内存
    void* memory = nullptr;
    if (sizeClass == SizeClass::Huge) {
        memory = allocateLarge(totalSize);
    } else {
        memory = allocateFromPool(size, sizeClass);
    }
    
    // 更新统计信息
    if (memory) {
        m_totalAllocated += size;
        m_allocCount++;
    } else {
        // 内存分配失败，调用处理函数
        if (m_outOfMemoryHandler) {
            m_outOfMemoryHandler();
        }
    }
    
    return memory;
}

void Allocator::deallocate(void* ptr, usize size) {
    if (!ptr) {
        return;
    }
    
    // 获取内存块头部
    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<char*>(ptr) - sizeof(BlockHeader)
    );
    
    // 确认大小匹配
    if (header->size != size) {
        // 大小不匹配，可能是内存损坏
        throw std::runtime_error("Memory corruption detected: size mismatch in deallocate");
    }
    
    // 根据大小类别进行释放
    if (header->sizeClass == SizeClass::Huge) {
        deallocateLarge(header);
    } else {
        returnToPool(ptr, header);
    }
    
    // 更新统计信息
    m_totalAllocated -= size;
    m_deallocCount++;
}

void* Allocator::reallocate(void* ptr, usize oldSize, usize newSize) {
    // 特殊情况处理
    if (!ptr) {
        return allocate(newSize);
    }
    
    if (newSize == 0) {
        deallocate(ptr, oldSize);
        return nullptr;
    }
    
    // 如果新旧大小相同，直接返回原指针
    if (oldSize == newSize) {
        return ptr;
    }
    
    // 分配新内存
    void* newPtr = allocate(newSize);
    if (!newPtr) {
        return nullptr;
    }
    
    // 复制数据
    memcpy(newPtr, ptr, std::min(oldSize, newSize));
    
    // 释放旧内存
    deallocate(ptr, oldSize);
    
    return newPtr;
}

Allocator::SizeClass Allocator::getSizeClass(usize size) const {
    if (size <= TINY_BLOCK_SIZE) {
        return SizeClass::Tiny;
    } else if (size <= SMALL_BLOCK_SIZE) {
        return SizeClass::Small;
    } else if (size <= MEDIUM_BLOCK_SIZE) {
        return SizeClass::Medium;
    } else if (size <= LARGE_BLOCK_SIZE) {
        return SizeClass::Large;
    } else {
        return SizeClass::Huge;
    }
}

void* Allocator::allocateFromPool(usize size, SizeClass sizeClass) {
    // 确定实际分配大小（向上取整到大小类别的最大尺寸）
    usize blockSize;
    switch (sizeClass) {
        case SizeClass::Tiny:
            blockSize = TINY_BLOCK_SIZE;
            break;
        case SizeClass::Small:
            blockSize = SMALL_BLOCK_SIZE;
            break;
        case SizeClass::Medium:
            blockSize = MEDIUM_BLOCK_SIZE;
            break;
        case SizeClass::Large:
            blockSize = LARGE_BLOCK_SIZE;
            break;
        default:
            return nullptr; // 不应该到达这里
    }
    
    int sizeClassIndex = static_cast<int>(sizeClass);
    
    // 检查是否有空闲块
    if (m_freeList[sizeClassIndex]) {
        // 从空闲列表中取出一个块
        BlockHeader* header = m_freeList[sizeClassIndex];
        m_freeList[sizeClassIndex] = header->next;
        
        // 设置块状态
        header->inUse = true;
        header->size = size;
        
        // 返回数据部分指针
        return reinterpret_cast<char*>(header) + sizeof(BlockHeader);
    }
    
    // 没有空闲块，分配一组新块
    usize poolSize = (blockSize + sizeof(BlockHeader)) * POOL_BLOCK_COUNT;
    char* poolMemory = reinterpret_cast<char*>(std::malloc(poolSize));
    if (!poolMemory) {
        return nullptr;
    }
    
    // 初始化新分配的内存池
    for (usize i = 0; i < POOL_BLOCK_COUNT - 1; ++i) {
        char* blockStart = poolMemory + i * (blockSize + sizeof(BlockHeader));
        BlockHeader* header = reinterpret_cast<BlockHeader*>(blockStart);
        header->size = blockSize;
        header->sizeClass = sizeClass;
        header->inUse = false;
        header->next = reinterpret_cast<BlockHeader*>(blockStart + blockSize + sizeof(BlockHeader));
        
        // 将第一个块外的所有块加入到空闲列表
        if (i == 0) {
            // 第一个块将被使用
            continue;
        }
        
        // 将这个块添加到空闲列表
        header->next = m_freeList[sizeClassIndex];
        m_freeList[sizeClassIndex] = header;
    }
    
    // 设置最后一个块
    char* lastBlockStart = poolMemory + (POOL_BLOCK_COUNT - 1) * (blockSize + sizeof(BlockHeader));
    BlockHeader* lastHeader = reinterpret_cast<BlockHeader*>(lastBlockStart);
    lastHeader->size = blockSize;
    lastHeader->sizeClass = sizeClass;
    lastHeader->inUse = false;
    lastHeader->next = nullptr;
    
    // 将最后一个块添加到空闲列表
    lastHeader->next = m_freeList[sizeClassIndex];
    m_freeList[sizeClassIndex] = lastHeader;
    
    // 使用第一个块
    BlockHeader* firstHeader = reinterpret_cast<BlockHeader*>(poolMemory);
    firstHeader->size = size;
    firstHeader->sizeClass = sizeClass;
    firstHeader->inUse = true;
    
    return poolMemory + sizeof(BlockHeader);
}

void Allocator::returnToPool(void* ptr, BlockHeader* header) {
    // 标记为未使用
    header->inUse = false;
    
    // 添加到对应大小类别的空闲列表
    int sizeClassIndex = static_cast<int>(header->sizeClass);
    header->next = m_freeList[sizeClassIndex];
    m_freeList[sizeClassIndex] = header;
}

void* Allocator::allocateLarge(usize size) {
    // 为大型内存直接分配
    void* memory = std::malloc(size);
    if (!memory) {
        return nullptr;
    }
    
    // 设置头部信息
    BlockHeader* header = reinterpret_cast<BlockHeader*>(memory);
    header->size = size - sizeof(BlockHeader);
    header->sizeClass = SizeClass::Huge;
    header->inUse = true;
    header->next = nullptr;
    
    return reinterpret_cast<char*>(memory) + sizeof(BlockHeader);
}

void Allocator::deallocateLarge(void* header) {
    // 直接释放大型内存
    std::free(header);
}

} // namespace GC
} // namespace Lua
