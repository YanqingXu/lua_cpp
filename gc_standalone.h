/**
 * @file gc_standalone.h
 * @brief 独立的垃圾收集器实现（用于验证核心算法）
 * @description 不依赖复杂VM系统的GC核心实现
 * @author Lua C++ Project
 * @date 2025-09-21
 */

#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include <string>
#include <chrono>
#include <atomic>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace lua_cpp {

// 简化的Size类型
using Size = std::size_t;

/**
 * @brief GC对象颜色（三色标记算法）
 */
enum class GCColor {
    White,          // 白色：未标记（可回收）
    Gray,           // 灰色：已标记但子对象未扫描
    Black           // 黑色：已标记且子对象已扫描
};

/**
 * @brief GC状态
 */
enum class GCState {
    Pause,          // 暂停状态
    Propagate,      // 传播状态
    Sweep,          // 清扫状态
    Finalize        // 终结状态
};

/**
 * @brief GC统计信息
 */
struct GCStats {
    Size collections_performed = 0;    // 执行的回收次数
    Size total_freed_bytes = 0;        // 总释放字节数
    Size total_freed_objects = 0;      // 总释放对象数
    Size current_object_count = 0;     // 当前对象数量
    Size current_memory_usage = 0;     // 当前内存使用量
    Size max_memory_used = 0;          // 最大内存使用量
    double average_pause_time = 0.0;   // 平均暂停时间（秒）
    double total_pause_time = 0.0;     // 总暂停时间（秒）
};

/**
 * @brief 前向声明
 */
class StandaloneGC;

/**
 * @brief GC对象基类（简化版）
 */
class GCObject {
public:
    explicit GCObject(Size size = 0)
        : size_(size)
        , color_(GCColor::White)
        , marked_(false) {}
    
    virtual ~GCObject() = default;
    
    // 禁用拷贝
    GCObject(const GCObject&) = delete;
    GCObject& operator=(const GCObject&) = delete;
    
    // 颜色管理
    GCColor GetColor() const { return color_; }
    void SetColor(GCColor color) { color_ = color; }
    bool IsMarked() const { return color_ != GCColor::White; }
    
    // 大小管理
    Size GetSize() const { return size_; }
    void SetSize(Size size) { size_ = size; }
    
    // 标记接口（纯虚函数）
    virtual void Mark(StandaloneGC* gc) = 0;
    
    // 获取引用的子对象
    virtual std::vector<GCObject*> GetReferences() const = 0;
    
    // 清理资源
    virtual void Cleanup() {}
    
    // 调试信息
    virtual std::string ToString() const {
        std::ostringstream oss;
        oss << "GCObject[size=" << size_ << ", color=";
        switch (color_) {
            case GCColor::White: oss << "White"; break;
            case GCColor::Gray: oss << "Gray"; break;
            case GCColor::Black: oss << "Black"; break;
        }
        oss << "]";
        return oss.str();
    }
    
private:
    Size size_;                 // 对象大小
    GCColor color_;             // 标记颜色
    bool marked_;               // 标记状态
};

/**
 * @brief 测试用的字符串对象
 */
class TestStringObject : public GCObject {
public:
    explicit TestStringObject(const std::string& value)
        : GCObject(value.size() + sizeof(TestStringObject))
        , value_(value) {}
    
    const std::string& GetValue() const { return value_; }
    
    void Mark(StandaloneGC* gc) override {
        if (GetColor() != GCColor::White) return;
        SetColor(GCColor::Black);
    }
    
    std::vector<GCObject*> GetReferences() const override {
        return {}; // 字符串没有子对象
    }
    
    std::string ToString() const override {
        return "StringObject[\"" + value_ + "\"]";
    }
    
private:
    std::string value_;
};

/**
 * @brief 测试用的容器对象（有子对象引用）
 */
class TestContainerObject : public GCObject {
public:
    explicit TestContainerObject(const std::string& name = "container")
        : GCObject(sizeof(TestContainerObject))
        , name_(name) {}
    
    void AddChild(GCObject* child) {
        if (child) {
            children_.push_back(child);
            SetSize(GetSize() + sizeof(GCObject*));
        }
    }
    
    void RemoveChild(GCObject* child) {
        auto it = std::find(children_.begin(), children_.end(), child);
        if (it != children_.end()) {
            children_.erase(it);
            SetSize(GetSize() - sizeof(GCObject*));
        }
    }
    
    const std::vector<GCObject*>& GetChildren() const { return children_; }
    const std::string& GetName() const { return name_; }
    
    void Mark(StandaloneGC* gc) override {
        if (GetColor() != GCColor::White) return;
        SetColor(GCColor::Gray); // 先标记为灰色，子对象扫描后变为黑色
    }
    
    std::vector<GCObject*> GetReferences() const override {
        return children_;
    }
    
    std::string ToString() const override {
        std::ostringstream oss;
        oss << "ContainerObject[name=\"" << name_ << "\", children=" << children_.size() << "]";
        return oss.str();
    }
    
private:
    std::string name_;
    std::vector<GCObject*> children_;
};

/**
 * @brief 独立的垃圾收集器
 */
class StandaloneGC {
public:
    explicit StandaloneGC(Size threshold = 1024)
        : gc_threshold_(threshold)
        , current_memory_(0)
        , state_(GCState::Pause)
        , incremental_step_size_(10) {}
    
    ~StandaloneGC() {
        // 清理所有对象
        for (auto* obj : all_objects_) {
            obj->Cleanup();
            delete obj;
        }
    }
    
    // 禁用拷贝
    StandaloneGC(const StandaloneGC&) = delete;
    StandaloneGC& operator=(const StandaloneGC&) = delete;
    
    /**
     * @brief 创建并注册对象
     */
    template<typename T, typename... Args>
    T* CreateObject(Args&&... args) {
        T* obj = new T(std::forward<Args>(args)...);
        RegisterObject(obj);
        return obj;
    }
    
    /**
     * @brief 注册对象到GC
     */
    void RegisterObject(GCObject* obj) {
        if (obj) {
            all_objects_.insert(obj);
            current_memory_ += obj->GetSize();
            stats_.current_object_count++;
            stats_.current_memory_usage += obj->GetSize();
            
            if (stats_.current_memory_usage > stats_.max_memory_used) {
                stats_.max_memory_used = stats_.current_memory_usage;
            }
            
            // 检查是否需要触发GC
            if (current_memory_ > gc_threshold_) {
                Collect();
            }
        }
    }
    
    /**
     * @brief 添加根对象
     */
    void AddRoot(GCObject* obj) {
        if (obj) {
            root_objects_.insert(obj);
        }
    }
    
    /**
     * @brief 移除根对象
     */
    void RemoveRoot(GCObject* obj) {
        root_objects_.erase(obj);
    }
    
    /**
     * @brief 执行完整垃圾收集
     */
    void Collect() {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "[GC] Starting collection, objects=" << all_objects_.size() 
                  << ", memory=" << current_memory_ << " bytes" << std::endl;
        
        // 1. 重置所有对象颜色为白色
        ResetColors();
        
        // 2. 标记阶段：从根对象开始标记
        MarkPhase();
        
        // 3. 清扫阶段：回收未标记的对象
        SweepPhase();
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(end_time - start_time).count();
        
        stats_.collections_performed++;
        stats_.total_pause_time += duration;
        stats_.average_pause_time = stats_.total_pause_time / stats_.collections_performed;
        
        std::cout << "[GC] Collection completed in " << duration << "s, "
                  << "objects=" << all_objects_.size() 
                  << ", memory=" << current_memory_ << " bytes" << std::endl;
    }
    
    /**
     * @brief 执行增量垃圾收集
     */
    void PerformIncrementalStep() {
        switch (state_) {
            case GCState::Pause:
                ResetColors();
                state_ = GCState::Propagate;
                break;
                
            case GCState::Propagate:
                if (PerformMarkStep()) {
                    state_ = GCState::Sweep;
                }
                break;
                
            case GCState::Sweep:
                if (PerformSweepStep()) {
                    state_ = GCState::Finalize;
                }
                break;
                
            case GCState::Finalize:
                FinalizeSweep();
                stats_.collections_performed++;
                state_ = GCState::Pause;
                break;
        }
    }
    
    /**
     * @brief 获取统计信息
     */
    const GCStats& GetStats() const { return stats_; }
    
    /**
     * @brief 检查一致性
     */
    bool CheckConsistency() const {
        for (auto* obj : all_objects_) {
            if (!obj) {
                std::cerr << "[GC] Null object in collection!" << std::endl;
                return false;
            }
            
            // 检查引用一致性
            auto refs = obj->GetReferences();
            for (auto* ref : refs) {
                if (ref && all_objects_.find(ref) == all_objects_.end()) {
                    std::cerr << "[GC] Object references non-managed object!" << std::endl;
                    return false;
                }
            }
        }
        return true;
    }
    
    /**
     * @brief 设置GC阈值
     */
    void SetThreshold(Size threshold) { gc_threshold_ = threshold; }
    
    /**
     * @brief 获取当前内存使用量
     */
    Size GetCurrentMemory() const { return current_memory_; }
    
    /**
     * @brief 获取对象数量
     */
    Size GetObjectCount() const { return all_objects_.size(); }
    
    /**
     * @brief 打印调试信息
     */
    void PrintDebugInfo() const {
        std::cout << "\n=== GC Debug Info ===" << std::endl;
        std::cout << "Objects: " << all_objects_.size() << std::endl;
        std::cout << "Memory: " << current_memory_ << " bytes" << std::endl;
        std::cout << "Roots: " << root_objects_.size() << std::endl;
        std::cout << "State: ";
        switch (state_) {
            case GCState::Pause: std::cout << "Pause"; break;
            case GCState::Propagate: std::cout << "Propagate"; break;
            case GCState::Sweep: std::cout << "Sweep"; break;
            case GCState::Finalize: std::cout << "Finalize"; break;
        }
        std::cout << std::endl;
        
        std::cout << "\nObjects by color:" << std::endl;
        Size white_count = 0, gray_count = 0, black_count = 0;
        for (auto* obj : all_objects_) {
            switch (obj->GetColor()) {
                case GCColor::White: white_count++; break;
                case GCColor::Gray: gray_count++; break;
                case GCColor::Black: black_count++; break;
            }
        }
        std::cout << "  White: " << white_count << std::endl;
        std::cout << "  Gray: " << gray_count << std::endl;
        std::cout << "  Black: " << black_count << std::endl;
        
        std::cout << "\nGC Statistics:" << std::endl;
        std::cout << "  Collections: " << stats_.collections_performed << std::endl;
        std::cout << "  Freed objects: " << stats_.total_freed_objects << std::endl;
        std::cout << "  Freed bytes: " << stats_.total_freed_bytes << std::endl;
        std::cout << "  Average pause: " << stats_.average_pause_time << "s" << std::endl;
        std::cout << "=====================\n" << std::endl;
    }
    
private:
    /**
     * @brief 重置所有对象颜色为白色
     */
    void ResetColors() {
        for (auto* obj : all_objects_) {
            obj->SetColor(GCColor::White);
        }
    }
    
    /**
     * @brief 标记阶段
     */
    void MarkPhase() {
        // 标记所有根对象
        for (auto* root : root_objects_) {
            MarkObject(root);
        }
        
        // 传播标记
        PropagateMarks();
    }
    
    /**
     * @brief 标记单个对象
     */
    void MarkObject(GCObject* obj) {
        if (!obj || obj->GetColor() != GCColor::White) {
            return;
        }
        
        obj->Mark(this);
        gray_queue_.push_back(obj);
    }
    
    /**
     * @brief 传播标记到所有灰色对象
     */
    void PropagateMarks() {
        while (!gray_queue_.empty()) {
            PropagateMarkFrom(gray_queue_.back());
            gray_queue_.pop_back();
        }
    }
    
    /**
     * @brief 从指定对象传播标记
     */
    void PropagateMarkFrom(GCObject* obj) {
        if (!obj) return;
        
        // 标记所有引用的对象
        auto refs = obj->GetReferences();
        for (auto* ref : refs) {
            if (ref && ref->GetColor() == GCColor::White) {
                ref->Mark(this);
                gray_queue_.push_back(ref);
            }
        }
        
        // 当前对象变为黑色
        obj->SetColor(GCColor::Black);
    }
    
    /**
     * @brief 清扫阶段
     */
    void SweepPhase() {
        auto it = all_objects_.begin();
        while (it != all_objects_.end()) {
            auto* obj = *it;
            if (obj->GetColor() == GCColor::White) {
                // 未标记的对象，需要回收
                Size obj_size = obj->GetSize();
                
                // 更新统计信息
                stats_.total_freed_objects++;
                stats_.total_freed_bytes += obj_size;
                stats_.current_object_count--;
                stats_.current_memory_usage -= obj_size;
                
                // 更新内存计数（只减一次）
                if (current_memory_ >= obj_size) {
                    current_memory_ -= obj_size;
                } else {
                    current_memory_ = 0; // 防止下溢
                }
                
                obj->Cleanup();
                delete obj;
                it = all_objects_.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    /**
     * @brief 执行一步标记操作（增量GC）
     */
    bool PerformMarkStep() {
        Size processed = 0;
        
        // 如果这是第一步，标记根对象
        if (gray_queue_.empty()) {
            for (auto* root : root_objects_) {
                MarkObject(root);
            }
        }
        
        // 处理一定数量的灰色对象
        while (!gray_queue_.empty() && processed < incremental_step_size_) {
            PropagateMarkFrom(gray_queue_.back());
            gray_queue_.pop_back();
            processed++;
        }
        
        return gray_queue_.empty(); // 如果队列为空，标记阶段完成
    }
    
    /**
     * @brief 执行一步清扫操作（增量GC）
     */
    bool PerformSweepStep() {
        Size processed = 0;
        
        if (sweep_current_ == all_objects_.end()) {
            sweep_current_ = all_objects_.begin();
        }
        
        while (sweep_current_ != all_objects_.end() && processed < incremental_step_size_) {
            auto it = sweep_current_;
            ++sweep_current_;
            
            auto* obj = *it;
            if (obj->GetColor() == GCColor::White) {
                // 标记为待删除
                objects_to_delete_.push_back(obj);
                all_objects_.erase(it);
                
                // 重置迭代器（因为容器被修改）
                if (sweep_current_ == it) {
                    sweep_current_ = all_objects_.begin();
                }
            }
            processed++;
        }
        
        return sweep_current_ == all_objects_.end();
    }
    
    /**
     * @brief 完成清扫，实际删除对象
     */
    void FinalizeSweep() {
        for (auto* obj : objects_to_delete_) {
            Size obj_size = obj->GetSize();
            
            // 更新统计信息
            stats_.total_freed_objects++;
            stats_.total_freed_bytes += obj_size;
            stats_.current_object_count--;
            stats_.current_memory_usage -= obj_size;
            
            // 更新内存计数（只减一次）
            if (current_memory_ >= obj_size) {
                current_memory_ -= obj_size;
            } else {
                current_memory_ = 0; // 防止下溢
            }
            
            obj->Cleanup();
            delete obj;
        }
        objects_to_delete_.clear();
        sweep_current_ = all_objects_.begin();
    }
    
private:
    std::unordered_set<GCObject*> all_objects_;     // 所有管理的对象
    std::unordered_set<GCObject*> root_objects_;    // 根对象集合
    std::vector<GCObject*> gray_queue_;             // 灰色对象队列
    std::vector<GCObject*> objects_to_delete_;      // 待删除对象列表
    
    Size gc_threshold_;                             // GC触发阈值
    Size current_memory_;                           // 当前内存使用量
    GCState state_;                                 // GC状态
    Size incremental_step_size_;                    // 增量GC步长
    
    std::unordered_set<GCObject*>::iterator sweep_current_; // 清扫迭代器
    
    mutable GCStats stats_;                         // 统计信息
};

} // namespace lua_cpp