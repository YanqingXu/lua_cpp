/**
 * @file garbage_collector.cpp
 * @brief 标记-清扫垃圾收集器实现
 * @description 实现Lua兼容的标记-清扫垃圾收集器
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#include "garbage_collector.h"
#include "vm/virtual_machine.h"
#include "types/lua_table.h"
#include <algorithm>
#include <iostream>
#include <cassert>

namespace lua_cpp {

/* ========================================================================== */
/* GCObject基类实现 */
/* ========================================================================== */

GCObject::GCObject(GCObjectType type, Size size)
    : type_(type)
    , size_(size)
    , color_(GCColor::White)
    , gc_next_(nullptr)
    , gc_prev_(nullptr) {
}

void GCObject::CallFinalizer() {
    if (finalizer_) {
        try {
            finalizer_(this);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Exception in finalizer: " << e.what() << std::endl;
        }
        finalizer_ = nullptr;
    }
}

std::string GCObject::ToString() const {
    std::ostringstream oss;
    oss << "GCObject[type=";
    
    switch (type_) {
        case GCObjectType::String: oss << "String"; break;
        case GCObjectType::Table: oss << "Table"; break;
        case GCObjectType::Function: oss << "Function"; break;
        case GCObjectType::UserData: oss << "UserData"; break;
        case GCObjectType::Thread: oss << "Thread"; break;
        case GCObjectType::Proto: oss << "Proto"; break;
    }
    
    oss << ", size=" << size_ << ", color=";
    
    switch (color_) {
        case GCColor::White: oss << "White"; break;
        case GCColor::Gray: oss << "Gray"; break;
        case GCColor::Black: oss << "Black"; break;
    }
    
    oss << "]";
    return oss.str();
}

std::string GCObject::GetDebugInfo() const {
    return ToString();
}

/* ========================================================================== */
/* StringObject实现 */
/* ========================================================================== */

StringObject::StringObject(const std::string& str)
    : GCObject(GCObjectType::String, str.length() + sizeof(StringObject))
    , str_(str) {
}

void StringObject::Mark(GarbageCollector* gc) {
    // 字符串对象没有子引用，直接标记为黑色
    SetColor(GCColor::Black);
}

std::vector<GCObject*> StringObject::GetReferences() const {
    // 字符串对象没有子引用
    return {};
}

std::string StringObject::ToString() const {
    return "\"" + str_ + "\"";
}

/* ========================================================================== */
/* TableObject实现 */
/* ========================================================================== */

TableObject::TableObject(Size array_size, Size hash_size)
    : GCObject(GCObjectType::Table, sizeof(TableObject))
    , array_size_(array_size)
    , hash_size_(hash_size)
    , table_(std::make_shared<LuaTable>()) {
    
    // 估算表的内存大小
    Size estimated_size = sizeof(TableObject) + 
                         array_size * sizeof(LuaValue) +
                         hash_size * (sizeof(LuaValue) + sizeof(LuaValue));
    SetSize(estimated_size);
}

void TableObject::Set(const LuaValue& key, const LuaValue& value) {
    if (table_) {
        table_->Set(key, value);
        
        // 更新大小估算
        Size new_size = sizeof(TableObject) + 
                       table_->GetArraySize() * sizeof(LuaValue) +
                       table_->GetHashSize() * (sizeof(LuaValue) + sizeof(LuaValue));
        SetSize(new_size);
    }
}

LuaValue TableObject::Get(const LuaValue& key) const {
    if (table_) {
        return table_->Get(key);
    }
    return LuaValue();
}

Size TableObject::Size() const {
    return table_ ? table_->Size() : 0;
}

void TableObject::Mark(GarbageCollector* gc) {
    if (GetColor() != GCColor::White) {
        return; // 已经标记过
    }
    
    // 标记为灰色并加入灰色列表
    SetColor(GCColor::Gray);
    gc->AddToGrayList(this);
}

std::vector<GCObject*> TableObject::GetReferences() const {
    std::vector<GCObject*> refs;
    
    if (table_) {
        // 收集表中所有值的GC引用
        auto pairs = table_->GetAllPairs();
        for (const auto& pair : pairs) {
            // 检查键
            if (pair.first.IsGCObject()) {
                refs.push_back(pair.first.GetGCObject());
            }
            // 检查值
            if (pair.second.IsGCObject()) {
                refs.push_back(pair.second.GetGCObject());
            }
        }
    }
    
    return refs;
}

/* ========================================================================== */
/* FunctionObject实现 */
/* ========================================================================== */

FunctionObject::FunctionObject(const Proto* proto)
    : GCObject(GCObjectType::Function, sizeof(FunctionObject))
    , proto_(proto) {
    
    // 估算函数对象大小
    Size estimated_size = sizeof(FunctionObject);
    if (proto_) {
        estimated_size += proto_->instructions.size() * sizeof(Instruction);
        estimated_size += proto_->constants.size() * sizeof(LuaValue);
    }
    SetSize(estimated_size);
}

void FunctionObject::Mark(GarbageCollector* gc) {
    if (GetColor() != GCColor::White) {
        return;
    }
    
    SetColor(GCColor::Gray);
    gc->AddToGrayList(this);
}

std::vector<GCObject*> FunctionObject::GetReferences() const {
    std::vector<GCObject*> refs;
    
    if (proto_) {
        // 标记常量表中的GC对象
        for (const auto& constant : proto_->constants) {
            if (constant.IsGCObject()) {
                refs.push_back(constant.GetGCObject());
            }
        }
        
        // 标记嵌套函数
        for (const auto& nested : proto_->nested_functions) {
            if (nested) {
                // 这里需要将Proto*转换为GCObject*
                // 实际实现中Proto也应该是GCObject的子类
            }
        }
    }
    
    return refs;
}

/* ========================================================================== */
/* GarbageCollector主类实现 */
/* ========================================================================== */

GarbageCollector::GarbageCollector(VirtualMachine* vm)
    : vm_(vm)
    , state_(GCState::Pause)
    , total_bytes_(0)
    , gc_threshold_(config_.initial_threshold)
    , last_collection_time_(std::chrono::steady_clock::now())
    , collection_count_(0)
    , object_count_(0)
    , all_objects_(nullptr)
    , gray_list_(nullptr)
    , gray_count_(0)
    , sweep_current_(nullptr)
    , pause_start_time_(std::chrono::steady_clock::now()) {
    
    // 初始化统计信息
    stats_.collections_performed = 0;
    stats_.total_freed_bytes = 0;
    stats_.total_freed_objects = 0;
    stats_.max_memory_used = 0;
    stats_.average_pause_time = 0.0;
}

GarbageCollector::~GarbageCollector() {
    // 清理所有对象
    FreeAllObjects();
}

/* ========================================================================== */
/* 对象生命周期管理 */
/* ========================================================================== */

void GarbageCollector::RegisterObject(GCObject* obj) {
    if (!obj) return;
    
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    // 加入全局对象链表
    obj->gc_next_ = all_objects_;
    obj->gc_prev_ = nullptr;
    
    if (all_objects_) {
        all_objects_->gc_prev_ = obj;
    }
    all_objects_ = obj;
    
    // 更新统计
    total_bytes_ += obj->GetSize();
    object_count_++;
    
    if (total_bytes_ > stats_.max_memory_used) {
        stats_.max_memory_used = total_bytes_;
    }
    
    // 检查是否需要触发GC
    if (config_.enable_auto_gc && ShouldTriggerGC()) {
        TriggerGC();
    }
}

void GarbageCollector::UnregisterObject(GCObject* obj) {
    if (!obj) return;
    
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    // 从链表中移除
    if (obj->gc_prev_) {
        obj->gc_prev_->gc_next_ = obj->gc_next_;
    } else {
        all_objects_ = obj->gc_next_;
    }
    
    if (obj->gc_next_) {
        obj->gc_next_->gc_prev_ = obj->gc_prev_;
    }
    
    // 从灰色列表中移除（如果在其中）
    RemoveFromGrayList(obj);
    
    // 更新统计
    total_bytes_ -= obj->GetSize();
    object_count_--;
    
    obj->gc_next_ = nullptr;
    obj->gc_prev_ = nullptr;
}

/* ========================================================================== */
/* 垃圾收集控制 */
/* ========================================================================== */

void GarbageCollector::Collect() {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    auto start_time = std::chrono::steady_clock::now();
    Size start_bytes = total_bytes_;
    Size start_objects = object_count_;
    
    try {
        if (config_.enable_incremental) {
            PerformIncrementalCollection();
        } else {
            PerformFullCollection();
        }
        
        // 更新统计信息
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time).count();
        
        stats_.collections_performed++;
        stats_.total_freed_bytes += start_bytes - total_bytes_;
        stats_.total_freed_objects += start_objects - object_count_;
        stats_.average_pause_time = 
            (stats_.average_pause_time * (stats_.collections_performed - 1) + duration) / 
            stats_.collections_performed;
        
        collection_count_++;
        last_collection_time_ = end_time;
        
        // 调整GC阈值
        AdjustThreshold();
        
    } catch (const std::exception& e) {
        std::cerr << "Warning: Exception during garbage collection: " << e.what() << std::endl;
    }
}

void GarbageCollector::PerformFullCollection() {
    // 1. 标记阶段
    MarkPhase();
    
    // 2. 清除阶段
    SweepPhase();
    
    // 3. 终结阶段
    FinalizePhase();
}

void GarbageCollector::PerformIncrementalCollection() {
    Size steps_performed = 0;
    const Size max_steps_per_cycle = 100;
    
    while (steps_performed < max_steps_per_cycle) {
        switch (state_) {
            case GCState::Pause:
                if (ShouldStartCollection()) {
                    StartMarkPhase();
                    state_ = GCState::Propagate;
                } else {
                    return; // 不需要收集
                }
                break;
                
            case GCState::Propagate:
                if (PerformMarkStep()) {
                    state_ = GCState::AtomicMark;
                }
                break;
                
            case GCState::AtomicMark:
                PerformAtomicMark();
                state_ = GCState::Sweep;
                break;
                
            case GCState::Sweep:
                if (PerformSweepStep()) {
                    state_ = GCState::Finalize;
                }
                break;
                
            case GCState::Finalize:
                PerformFinalize();
                state_ = GCState::Pause;
                return; // 完成一轮收集
        }
        
        steps_performed++;
    }
}

/* ========================================================================== */
/* 标记阶段实现 */
/* ========================================================================== */

void GarbageCollector::MarkPhase() {
    // 1. 将所有对象标记为白色
    ResetColors();
    
    // 2. 标记根对象
    MarkRoots();
    
    // 3. 传播标记
    PropagateMarks();
}

void GarbageCollector::ResetColors() {
    GCObject* obj = all_objects_;
    while (obj) {
        obj->SetColor(GCColor::White);
        obj = obj->gc_next_;
    }
    
    // 清空灰色列表
    gray_list_ = nullptr;
    gray_count_ = 0;
}

void GarbageCollector::MarkRoots() {
    if (!vm_) return;
    
    // 标记虚拟机栈
    MarkVMStack();
    
    // 标记全局变量
    MarkGlobals();
    
    // 标记调用栈
    MarkCallStack();
    
    // 标记注册表
    MarkRegistry();
}

void GarbageCollector::MarkVMStack() {
    if (!vm_) return;
    
    Size stack_top = vm_->GetStackTop();
    
    for (Size i = 0; i < stack_top; i++) {
        const LuaValue& value = vm_->GetStack(i);
        if (value.IsGCObject()) {
            MarkObject(value.GetGCObject());
        }
    }
}

void GarbageCollector::MarkGlobals() {
    // 标记全局变量表
    // 这里需要VM提供访问全局变量的接口
    // 暂时跳过，等VM接口完善后再实现
}

void GarbageCollector::MarkCallStack() {
    if (!vm_) return;
    
    // 通过VM的栈跟踪来标记调用栈中的对象
    // 这里需要VM提供更详细的调用栈访问接口
    // 暂时跳过
}

void GarbageCollector::MarkRegistry() {
    // 标记注册表中的对象
    // 注册表通常是一个特殊的表对象
    // 暂时跳过，等VM接口完善后再实现
}

void GarbageCollector::MarkObject(GCObject* obj) {
    if (!obj || obj->GetColor() != GCColor::White) {
        return;
    }
    
    // 调用对象的标记方法
    obj->Mark(this);
}

void GarbageCollector::PropagateMarks() {
    while (gray_list_) {
        GCObject* obj = PopFromGrayList();
        PropagateMarkFrom(obj);
    }
}

void GarbageCollector::PropagateMarkFrom(GCObject* obj) {
    if (!obj || obj->GetColor() != GCColor::Gray) {
        return;
    }
    
    // 获取所有引用的对象
    auto refs = obj->GetReferences();
    
    // 标记所有引用的对象
    for (GCObject* ref : refs) {
        if (ref && ref->GetColor() == GCColor::White) {
            ref->Mark(this);
        }
    }
    
    // 将对象标记为黑色
    obj->SetColor(GCColor::Black);
}

/* ========================================================================== */
/* 清除阶段实现 */
/* ========================================================================== */

void GarbageCollector::SweepPhase() {
    GCObject* obj = all_objects_;
    GCObject* next;
    
    Size freed_bytes = 0;
    Size freed_objects = 0;
    
    while (obj) {
        next = obj->gc_next_;
        
        if (obj->GetColor() == GCColor::White) {
            // 未标记的对象，需要回收
            freed_bytes += obj->GetSize();
            freed_objects++;
            
            // 调用清理函数
            obj->Cleanup();
            
            // 从链表中移除
            UnregisterObject(obj);
            
            // 加入终结列表（如果有终结器）
            if (obj->HasFinalizer()) {
                finalization_list_.push_back(obj);
            } else {
                delete obj;
            }
        }
        
        obj = next;
    }
    
    // 更新统计
    stats_.total_freed_bytes += freed_bytes;
    stats_.total_freed_objects += freed_objects;
}

/* ========================================================================== */
/* 终结阶段实现 */
/* ========================================================================== */

void GarbageCollector::FinalizePhase() {
    for (GCObject* obj : finalization_list_) {
        if (obj) {
            obj->CallFinalizer();
            delete obj;
        }
    }
    finalization_list_.clear();
}

/* ========================================================================== */
/* 灰色列表管理 */
/* ========================================================================== */

void GarbageCollector::AddToGrayList(GCObject* obj) {
    if (!obj || obj->GetColor() != GCColor::Gray) {
        return;
    }
    
    // 简单的链表实现
    obj->gc_next_ = gray_list_;
    gray_list_ = obj;
    gray_count_++;
}

GCObject* GarbageCollector::PopFromGrayList() {
    if (!gray_list_) {
        return nullptr;
    }
    
    GCObject* obj = gray_list_;
    gray_list_ = obj->gc_next_;
    gray_count_--;
    
    obj->gc_next_ = nullptr;
    return obj;
}

void GarbageCollector::RemoveFromGrayList(GCObject* obj) {
    if (!obj || !gray_list_) {
        return;
    }
    
    if (gray_list_ == obj) {
        gray_list_ = obj->gc_next_;
        gray_count_--;
        obj->gc_next_ = nullptr;
        return;
    }
    
    GCObject* current = gray_list_;
    while (current && current->gc_next_) {
        if (current->gc_next_ == obj) {
            current->gc_next_ = obj->gc_next_;
            gray_count_--;
            obj->gc_next_ = nullptr;
            return;
        }
        current = current->gc_next_;
    }
}

/* ========================================================================== */
/* GC触发条件和阈值管理 */
/* ========================================================================== */

bool GarbageCollector::ShouldTriggerGC() const {
    if (!config_.enable_auto_gc) {
        return false;
    }
    
    return total_bytes_ >= gc_threshold_;
}

bool GarbageCollector::ShouldStartCollection() const {
    return ShouldTriggerGC();
}

void GarbageCollector::TriggerGC() {
    if (state_ == GCState::Pause) {
        Collect();
    }
}

void GarbageCollector::AdjustThreshold() {
    // 基于当前内存使用调整阈值
    Size base_threshold = total_bytes_ * config_.pause_multiplier / 100;
    gc_threshold_ = std::max(base_threshold, config_.initial_threshold);
}

/* ========================================================================== */
/* 增量GC步骤实现 */
/* ========================================================================== */

void GarbageCollector::StartMarkPhase() {
    ResetColors();
    MarkRoots();
    pause_start_time_ = std::chrono::steady_clock::now();
}

bool GarbageCollector::PerformMarkStep() {
    const Size steps_per_call = 10;
    Size steps = 0;
    
    while (gray_list_ && steps < steps_per_call) {
        GCObject* obj = PopFromGrayList();
        PropagateMarkFrom(obj);
        steps++;
    }
    
    return gray_list_ == nullptr; // 返回true表示标记完成
}

void GarbageCollector::PerformAtomicMark() {
    // 原子标记阶段：确保所有根对象都被标记
    MarkRoots();
    
    // 完成剩余的标记传播
    PropagateMarks();
}

bool GarbageCollector::PerformSweepStep() {
    const Size objects_per_step = 50;
    Size processed = 0;
    
    if (!sweep_current_) {
        sweep_current_ = all_objects_;
    }
    
    while (sweep_current_ && processed < objects_per_step) {
        GCObject* next = sweep_current_->gc_next_;
        
        if (sweep_current_->GetColor() == GCColor::White) {
            // 回收对象
            sweep_current_->Cleanup();
            
            if (sweep_current_->HasFinalizer()) {
                finalization_list_.push_back(sweep_current_);
            } else {
                UnregisterObject(sweep_current_);
                delete sweep_current_;
            }
        }
        
        sweep_current_ = next;
        processed++;
    }
    
    if (!sweep_current_) {
        // 清除阶段完成
        return true;
    }
    
    return false;
}

void GarbageCollector::PerformFinalize() {
    FinalizePhase();
    AdjustThreshold();
}

/* ========================================================================== */
/* 内存管理辅助 */
/* ========================================================================== */

void GarbageCollector::FreeAllObjects() {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    // 首先清理终结列表
    for (GCObject* obj : finalization_list_) {
        if (obj) {
            delete obj;
        }
    }
    finalization_list_.clear();
    
    // 然后清理所有对象
    GCObject* obj = all_objects_;
    while (obj) {
        GCObject* next = obj->gc_next_;
        obj->Cleanup();
        delete obj;
        obj = next;
    }
    
    all_objects_ = nullptr;
    gray_list_ = nullptr;
    sweep_current_ = nullptr;
    total_bytes_ = 0;
    object_count_ = 0;
    gray_count_ = 0;
}

/* ========================================================================== */
/* 配置和状态查询 */
/* ========================================================================== */

void GarbageCollector::SetConfig(const GCConfig& config) {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    config_ = config;
    
    // 重新调整阈值
    AdjustThreshold();
}

GCConfig GarbageCollector::GetConfig() const {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    return config_;
}

GCStats GarbageCollector::GetStats() const {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    GCStats stats = stats_;
    stats.current_memory_usage = total_bytes_;
    stats.current_object_count = object_count_;
    stats.gc_threshold = gc_threshold_;
    
    return stats;
}

/* ========================================================================== */
/* 调试和诊断 */
/* ========================================================================== */

void GarbageCollector::DumpStats() const {
    auto stats = GetStats();
    
    std::cout << "=== Garbage Collector Statistics ===" << std::endl;
    std::cout << "Current memory usage: " << stats.current_memory_usage << " bytes" << std::endl;
    std::cout << "Current object count: " << stats.current_object_count << std::endl;
    std::cout << "GC threshold: " << stats.gc_threshold << " bytes" << std::endl;
    std::cout << "Collections performed: " << stats.collections_performed << std::endl;
    std::cout << "Total freed bytes: " << stats.total_freed_bytes << std::endl;
    std::cout << "Total freed objects: " << stats.total_freed_objects << std::endl;
    std::cout << "Max memory used: " << stats.max_memory_used << " bytes" << std::endl;
    std::cout << "Average pause time: " << stats.average_pause_time << " seconds" << std::endl;
}

void GarbageCollector::DumpObjects() const {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    std::cout << "=== GC Object Dump ===" << std::endl;
    std::cout << "Total objects: " << object_count_ << std::endl;
    
    GCObject* obj = all_objects_;
    Size index = 0;
    
    while (obj && index < 100) { // 限制输出数量
        std::cout << "[" << index << "] " << obj->GetDebugInfo() << std::endl;
        obj = obj->gc_next_;
        index++;
    }
    
    if (obj) {
        std::cout << "... and " << (object_count_ - index) << " more objects" << std::endl;
    }
}

bool GarbageCollector::CheckConsistency() const {
    std::lock_guard<std::mutex> lock(gc_mutex_);
    
    // 检查对象链表的一致性
    Size counted_objects = 0;
    Size counted_bytes = 0;
    
    GCObject* obj = all_objects_;
    GCObject* prev = nullptr;
    
    while (obj) {
        // 检查双向链表指针
        if (obj->gc_prev_ != prev) {
            std::cerr << "GC consistency error: Invalid prev pointer" << std::endl;
            return false;
        }
        
        counted_objects++;
        counted_bytes += obj->GetSize();
        
        prev = obj;
        obj = obj->gc_next_;
    }
    
    if (counted_objects != object_count_) {
        std::cerr << "GC consistency error: Object count mismatch" << std::endl;
        return false;
    }
    
    if (counted_bytes != total_bytes_) {
        std::cerr << "GC consistency error: Byte count mismatch" << std::endl;
        return false;
    }
    
    return true;
}

} // namespace lua_cpp