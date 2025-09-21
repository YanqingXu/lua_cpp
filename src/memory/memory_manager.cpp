/**
 * @file memory_manager.cpp
 * @brief 内存管理器实现
 * @description 实现统一的内存分配和管理功能
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "memory_manager.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <cstring>

namespace lua_cpp {

/* ========================================================================== */
/* SystemAllocator实现 */
/* ========================================================================== */

SystemAllocator::SystemAllocator() {
    ResetStats();
}

SystemAllocator::~SystemAllocator() {
    // 输出最终统计信息
    if (stats_.allocation_count > 0) {
        std::cout << "SystemAllocator final stats: "
                  << "allocated=" << stats_.total_allocated
                  << " freed=" << stats_.total_freed
                  << " peak=" << stats_.peak_usage << std::endl;
    }
}

void* SystemAllocator::Allocate(Size size, Size alignment) {
    if (size == 0) return nullptr;
    
    void* ptr = nullptr;
    
#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = nullptr;
    }
#endif
    
    if (ptr) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        stats_.total_allocated += size;
        stats_.current_usage += size;
        stats_.allocation_count++;
        
        if (stats_.current_usage > stats_.peak_usage) {
            stats_.peak_usage = stats_.current_usage;
        }
    }
    
    return ptr;
}

void SystemAllocator::Deallocate(void* ptr, Size size) {
    if (!ptr) return;
    
#ifdef _WIN32
    _aligned_free(ptr);
#else
    free(ptr);
#endif
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_.total_freed += size;
    if (stats_.current_usage >= size) {
        stats_.current_usage -= size;
    }
    stats_.deallocation_count++;
}

void* SystemAllocator::Reallocate(void* ptr, Size old_size, Size new_size, Size alignment) {
    if (new_size == 0) {
        Deallocate(ptr, old_size);
        return nullptr;
    }
    
    if (!ptr) {
        return Allocate(new_size, alignment);
    }
    
    void* new_ptr = Allocate(new_size, alignment);
    if (new_ptr && ptr) {
        std::memcpy(new_ptr, ptr, std::min(old_size, new_size));
        Deallocate(ptr, old_size);
    }
    
    return new_ptr;
}

MemoryStats SystemAllocator::GetStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return stats_;
}

void SystemAllocator::ResetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    stats_ = MemoryStats{};
}

/* ========================================================================== */
/* FixedPoolAllocator实现 */
/* ========================================================================== */

FixedPoolAllocator::FixedPoolAllocator(Size block_size, Size block_count)
    : block_size_(AlignTo(block_size, sizeof(void*)))
    , block_count_(block_count)
    , pool_memory_(nullptr)
    , free_list_(nullptr) {
    
    InitializePool();
    ResetStats();
}

FixedPoolAllocator::~FixedPoolAllocator() {
    CleanupPool();
}

void FixedPoolAllocator::InitializePool() {
    Size total_size = block_size_ * block_count_;
    pool_memory_ = std::malloc(total_size);
    
    if (!pool_memory_) {
        throw OutOfMemoryError("Failed to allocate memory pool");
    }
    
    // 初始化自由列表
    char* current = static_cast<char*>(pool_memory_);
    free_list_ = current;
    
    for (Size i = 0; i < block_count_ - 1; i++) {
        *reinterpret_cast<void**>(current) = current + block_size_;
        current += block_size_;
    }
    
    // 最后一个块指向nullptr
    *reinterpret_cast<void**>(current) = nullptr;
}

void FixedPoolAllocator::CleanupPool() {
    if (pool_memory_) {
        std::free(pool_memory_);
        pool_memory_ = nullptr;
        free_list_ = nullptr;
    }
}

void* FixedPoolAllocator::Allocate(Size size, Size alignment) {
    if (size > block_size_) {
        return nullptr; // 请求的大小超过块大小
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    if (!free_list_) {
        return nullptr; // 池已满
    }
    
    void* ptr = free_list_;
    free_list_ = *static_cast<void**>(free_list_);
    
    // 更新统计信息
    stats_.total_allocated += block_size_;
    stats_.current_usage += block_size_;
    stats_.allocation_count++;
    
    if (stats_.current_usage > stats_.peak_usage) {
        stats_.peak_usage = stats_.current_usage;
    }
    
    return ptr;
}

void FixedPoolAllocator::Deallocate(void* ptr, Size size) {
    if (!ptr || !OwnsPointer(ptr)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    // 将块加回自由列表
    *static_cast<void**>(ptr) = free_list_;
    free_list_ = ptr;
    
    // 更新统计信息
    stats_.total_freed += block_size_;
    if (stats_.current_usage >= block_size_) {
        stats_.current_usage -= block_size_;
    }
    stats_.deallocation_count++;
}

void* FixedPoolAllocator::Reallocate(void* ptr, Size old_size, Size new_size, Size alignment) {
    if (new_size <= block_size_) {
        return ptr; // 新大小仍然适合当前块
    }
    
    // 需要更大的块，使用新分配
    void* new_ptr = Allocate(new_size, alignment);
    if (new_ptr && ptr) {
        std::memcpy(new_ptr, ptr, std::min(old_size, new_size));
        Deallocate(ptr, old_size);
    }
    
    return new_ptr;
}

bool FixedPoolAllocator::OwnsPointer(void* ptr) const {
    if (!ptr || !pool_memory_) return false;
    
    char* pool_start = static_cast<char*>(pool_memory_);
    char* pool_end = pool_start + (block_size_ * block_count_);
    char* check_ptr = static_cast<char*>(ptr);
    
    return check_ptr >= pool_start && check_ptr < pool_end &&
           ((check_ptr - pool_start) % block_size_) == 0;
}

MemoryStats FixedPoolAllocator::GetStats() const {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    
    MemoryStats stats = stats_;
    
    // 计算自由块信息
    Size free_blocks = 0;
    void* current = free_list_;
    while (current) {
        free_blocks++;
        current = *static_cast<void**>(current);
    }
    
    stats.free_block_count = free_blocks;
    stats.largest_free_block = free_blocks > 0 ? block_size_ : 0;
    stats.smallest_free_block = free_blocks > 0 ? block_size_ : 0;
    stats.fragmentation_ratio = 1.0 - (static_cast<double>(free_blocks) / block_count_);
    
    return stats;
}

void FixedPoolAllocator::ResetStats() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    stats_ = MemoryStats{};
}

/* ========================================================================== */
/* StackAllocator实现 */
/* ========================================================================== */

StackAllocator::StackAllocator(Size capacity)
    : capacity_(capacity)
    , current_position_(0) {
    
    stack_memory_ = std::malloc(capacity);
    if (!stack_memory_) {
        throw OutOfMemoryError("Failed to allocate stack memory");
    }
    
    ResetStats();
}

StackAllocator::~StackAllocator() {
    if (stack_memory_) {
        std::free(stack_memory_);
    }
}

void* StackAllocator::Allocate(Size size, Size alignment) {
    if (size == 0) return nullptr;
    
    std::lock_guard<std::mutex> lock(stack_mutex_);
    
    // 计算对齐后的位置
    Size aligned_position = AlignTo(current_position_, alignment);
    Size new_position = aligned_position + size;
    
    if (new_position > capacity_) {
        return nullptr; // 栈空间不足
    }
    
    void* ptr = static_cast<char*>(stack_memory_) + aligned_position;
    current_position_ = new_position;
    
    // 更新统计信息
    stats_.total_allocated += size;
    stats_.current_usage = current_position_;
    stats_.allocation_count++;
    
    if (stats_.current_usage > stats_.peak_usage) {
        stats_.peak_usage = stats_.current_usage;
    }
    
    return ptr;
}

void StackAllocator::Deallocate(void* ptr, Size size) {
    // 栈分配器不支持单独释放
    // 只能通过回滚到标记点来释放
}

void* StackAllocator::Reallocate(void* ptr, Size old_size, Size new_size, Size alignment) {
    // 检查是否是栈顶分配
    std::lock_guard<std::mutex> lock(stack_mutex_);
    
    char* stack_start = static_cast<char*>(stack_memory_);
    char* ptr_addr = static_cast<char*>(ptr);
    
    if (ptr_addr + old_size == stack_start + current_position_) {
        // 是栈顶分配，可以就地扩展或收缩
        Size aligned_position = ptr_addr - stack_start;
        Size new_position = aligned_position + new_size;
        
        if (new_position <= capacity_) {
            current_position_ = new_position;
            stats_.current_usage = current_position_;
            return ptr;
        }
    }
    
    // 否则需要新分配
    void* new_ptr = Allocate(new_size, alignment);
    if (new_ptr && ptr) {
        std::memcpy(new_ptr, ptr, std::min(old_size, new_size));
    }
    
    return new_ptr;
}

StackAllocator::Marker StackAllocator::GetMarker() const {
    std::lock_guard<std::mutex> lock(stack_mutex_);
    return Marker{current_position_};
}

void StackAllocator::RollbackToMarker(const Marker& marker) {
    std::lock_guard<std::mutex> lock(stack_mutex_);
    
    if (marker.position <= current_position_) {
        Size freed_bytes = current_position_ - marker.position;
        current_position_ = marker.position;
        
        stats_.total_freed += freed_bytes;
        stats_.current_usage = current_position_;
        stats_.deallocation_count++;
    }
}

void StackAllocator::Clear() {
    std::lock_guard<std::mutex> lock(stack_mutex_);
    
    stats_.total_freed += current_position_;
    stats_.current_usage = 0;
    current_position_ = 0;
    
    if (stats_.deallocation_count == 0) {
        stats_.deallocation_count = 1;
    }
}

MemoryStats StackAllocator::GetStats() const {
    std::lock_guard<std::mutex> lock(stack_mutex_);
    
    MemoryStats stats = stats_;
    stats.largest_free_block = capacity_ - current_position_;
    stats.smallest_free_block = capacity_ - current_position_;
    stats.free_block_count = 1;
    stats.fragmentation_ratio = 0.0; // 栈分配器没有碎片
    
    return stats;
}

void StackAllocator::ResetStats() {
    std::lock_guard<std::mutex> lock(stack_mutex_);
    stats_ = MemoryStats{};
}

/* ========================================================================== */
/* MemoryManager实现 */
/* ========================================================================== */

MemoryManager::MemoryManager()
    : memory_limit_(0)
    , total_allocated_(0)
    , total_deallocated_(0) {
    
    InitializeDefaultAllocators();
}

MemoryManager::~MemoryManager() {
    // 析构函数会自动清理所有智能指针
}

void MemoryManager::InitializeDefaultAllocators() {
    // 设置系统分配器作为默认分配器
    default_allocator_ = std::make_unique<SystemAllocator>();
    
    // 注册一些常用的分配器
    named_allocators_["system"] = std::make_unique<SystemAllocator>();
    named_allocators_["small_pool"] = std::make_unique<FixedPoolAllocator>(64, 1000);
    named_allocators_["large_pool"] = std::make_unique<FixedPoolAllocator>(1024, 100);
    named_allocators_["stack"] = std::make_unique<StackAllocator>(1024 * 1024); // 1MB栈
}

void MemoryManager::SetDefaultAllocator(std::unique_ptr<Allocator> allocator) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    default_allocator_ = std::move(allocator);
}

void MemoryManager::RegisterAllocator(const std::string& name, std::unique_ptr<Allocator> allocator) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    named_allocators_[name] = std::move(allocator);
}

Allocator* MemoryManager::GetAllocator(const std::string& name) const {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    auto it = named_allocators_.find(name);
    return (it != named_allocators_.end()) ? it->second.get() : nullptr;
}

void* MemoryManager::Allocate(Size size, Size alignment, const std::string& allocator_name) {
    CheckMemoryLimit(size);
    
    Allocator* allocator = allocator_name.empty() ? 
        default_allocator_.get() : 
        GetAllocator(allocator_name);
    
    if (!allocator) {
        allocator = default_allocator_.get();
    }
    
    void* ptr = allocator->Allocate(size, alignment);
    
    if (ptr) {
        total_allocated_ += size;
        UpdateStats(size, 0);
        
        if (allocation_callback_) {
            allocation_callback_(ptr, size);
        }
    } else if (out_of_memory_callback_) {
        out_of_memory_callback_(size);
    }
    
    return ptr;
}

void MemoryManager::Deallocate(void* ptr, Size size, const std::string& allocator_name) {
    if (!ptr) return;
    
    Allocator* allocator = allocator_name.empty() ? 
        default_allocator_.get() : 
        GetAllocator(allocator_name);
    
    if (!allocator) {
        allocator = default_allocator_.get();
    }
    
    allocator->Deallocate(ptr, size);
    
    total_deallocated_ += size;
    UpdateStats(0, size);
    
    if (deallocation_callback_) {
        deallocation_callback_(ptr, size);
    }
}

void* MemoryManager::Reallocate(void* ptr, Size old_size, Size new_size, Size alignment, const std::string& allocator_name) {
    if (new_size > old_size) {
        CheckMemoryLimit(new_size - old_size);
    }
    
    Allocator* allocator = allocator_name.empty() ? 
        default_allocator_.get() : 
        GetAllocator(allocator_name);
    
    if (!allocator) {
        allocator = default_allocator_.get();
    }
    
    void* new_ptr = allocator->Reallocate(ptr, old_size, new_size, alignment);
    
    if (new_ptr) {
        if (new_size > old_size) {
            total_allocated_ += (new_size - old_size);
            UpdateStats(new_size - old_size, 0);
        } else {
            total_deallocated_ += (old_size - new_size);
            UpdateStats(0, old_size - new_size);
        }
        
        if (allocation_callback_) {
            allocation_callback_(new_ptr, new_size);
        }
    }
    
    return new_ptr;
}

void MemoryManager::SetGarbageCollector(std::unique_ptr<GarbageCollector> gc) {
    std::lock_guard<std::mutex> lock(manager_mutex_);
    garbage_collector_ = std::move(gc);
}

void MemoryManager::CollectGarbage() {
    if (garbage_collector_) {
        garbage_collector_->Collect();
    }
}

MemoryStats MemoryManager::GetTotalStats() const {
    MemoryStats total_stats;
    
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    // 合并所有分配器的统计信息
    if (default_allocator_) {
        auto stats = default_allocator_->GetStats();
        total_stats.total_allocated += stats.total_allocated;
        total_stats.total_freed += stats.total_freed;
        total_stats.current_usage += stats.current_usage;
        total_stats.peak_usage = std::max(total_stats.peak_usage, stats.peak_usage);
        total_stats.allocation_count += stats.allocation_count;
        total_stats.deallocation_count += stats.deallocation_count;
    }
    
    for (const auto& pair : named_allocators_) {
        auto stats = pair.second->GetStats();
        total_stats.total_allocated += stats.total_allocated;
        total_stats.total_freed += stats.total_freed;
        total_stats.current_usage += stats.current_usage;
        total_stats.peak_usage = std::max(total_stats.peak_usage, stats.peak_usage);
        total_stats.allocation_count += stats.allocation_count;
        total_stats.deallocation_count += stats.deallocation_count;
    }
    
    return total_stats;
}

std::map<std::string, MemoryStats> MemoryManager::GetAllocatorStats() const {
    std::map<std::string, MemoryStats> stats_map;
    
    std::lock_guard<std::mutex> lock(manager_mutex_);
    
    if (default_allocator_) {
        stats_map["default"] = default_allocator_->GetStats();
    }
    
    for (const auto& pair : named_allocators_) {
        stats_map[pair.first] = pair.second->GetStats();
    }
    
    return stats_map;
}

bool MemoryManager::IsMemoryLimitExceeded() const {
    Size limit = memory_limit_.load();
    if (limit == 0) return false;
    
    Size current = total_allocated_.load() - total_deallocated_.load();
    return current > limit;
}

std::string MemoryManager::GenerateMemoryReport() const {
    std::ostringstream oss;
    
    oss << "=== Memory Manager Report ===" << std::endl;
    
    // 总体统计
    auto total_stats = GetTotalStats();
    oss << "Total Statistics:" << std::endl;
    oss << "  Allocated: " << total_stats.total_allocated << " bytes" << std::endl;
    oss << "  Freed: " << total_stats.total_freed << " bytes" << std::endl;
    oss << "  Current Usage: " << total_stats.current_usage << " bytes" << std::endl;
    oss << "  Peak Usage: " << total_stats.peak_usage << " bytes" << std::endl;
    oss << "  Allocations: " << total_stats.allocation_count << std::endl;
    oss << "  Deallocations: " << total_stats.deallocation_count << std::endl;
    
    // 内存限制
    Size limit = memory_limit_.load();
    if (limit > 0) {
        oss << "  Memory Limit: " << limit << " bytes" << std::endl;
        oss << "  Limit Exceeded: " << (IsMemoryLimitExceeded() ? "YES" : "NO") << std::endl;
    }
    
    // 分配器统计
    oss << std::endl << "Allocator Statistics:" << std::endl;
    auto allocator_stats = GetAllocatorStats();
    for (const auto& pair : allocator_stats) {
        const auto& stats = pair.second;
        oss << "  " << pair.first << ":" << std::endl;
        oss << "    Allocated: " << stats.total_allocated << " bytes" << std::endl;
        oss << "    Freed: " << stats.total_freed << " bytes" << std::endl;
        oss << "    Current: " << stats.current_usage << " bytes" << std::endl;
        oss << "    Peak: " << stats.peak_usage << " bytes" << std::endl;
    }
    
    // GC统计
    if (garbage_collector_) {
        oss << std::endl << "Garbage Collector Statistics:" << std::endl;
        auto gc_stats = garbage_collector_->GetStats();
        oss << "  Collections: " << gc_stats.collections_performed << std::endl;
        oss << "  Objects: " << gc_stats.current_object_count << std::endl;
        oss << "  Memory: " << gc_stats.current_memory_usage << " bytes" << std::endl;
        oss << "  Avg Pause: " << gc_stats.average_pause_time << " seconds" << std::endl;
    }
    
    return oss.str();
}

void MemoryManager::DumpMemoryStats() const {
    std::cout << GenerateMemoryReport() << std::endl;
}

bool MemoryManager::ValidateMemoryIntegrity() const {
    // 基本的完整性检查
    Size allocated = total_allocated_.load();
    Size deallocated = total_deallocated_.load();
    
    if (deallocated > allocated) {
        std::cerr << "Memory integrity error: deallocated > allocated" << std::endl;
        return false;
    }
    
    // 检查垃圾收集器一致性
    if (garbage_collector_) {
        if (!garbage_collector_->CheckConsistency()) {
            std::cerr << "Memory integrity error: GC consistency check failed" << std::endl;
            return false;
        }
    }
    
    return true;
}

void MemoryManager::UpdateStats(Size allocated, Size deallocated) {
    // 这里可以添加更多的统计更新逻辑
}

void MemoryManager::CheckMemoryLimit(Size requested_size) {
    Size limit = memory_limit_.load();
    if (limit == 0) return;
    
    Size current = total_allocated_.load() - total_deallocated_.load();
    if (current + requested_size > limit) {
        if (out_of_memory_callback_) {
            out_of_memory_callback_(requested_size);
        }
        throw OutOfMemoryError("Memory limit exceeded");
    }
}

/* ========================================================================== */
/* 全局内存管理器 */
/* ========================================================================== */

static std::unique_ptr<MemoryManager> g_memory_manager;
static std::mutex g_memory_manager_mutex;

MemoryManager& GetGlobalMemoryManager() {
    std::lock_guard<std::mutex> lock(g_memory_manager_mutex);
    
    if (!g_memory_manager) {
        g_memory_manager = std::make_unique<MemoryManager>();
    }
    
    return *g_memory_manager;
}

void SetGlobalMemoryManager(std::unique_ptr<MemoryManager> manager) {
    std::lock_guard<std::mutex> lock(g_memory_manager_mutex);
    g_memory_manager = std::move(manager);
}

} // namespace lua_cpp