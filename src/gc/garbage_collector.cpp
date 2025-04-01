#include "garbage_collector.hpp"
#include "object/string.hpp"
#include "object/table.hpp"
#include "object/function.hpp"
#include "object/userdata.hpp"
#include "vm/state.hpp"

namespace Lua {
namespace GC {

//===========================
// StringPool 实现
//===========================

StringPool::StringPool() {
    // 初始化哈希映射表
}

Ptr<Object::String> StringPool::find(const Str& value) {
    // 计算哈希值
    u32 hash = computeHash(value);
    
    // 查找对应哈希的字符串列表
    auto it = m_strings.find(hash);
    if (it != m_strings.end()) {
        // 在相同哈希值的字符串中查找完全匹配的
        for (const auto& str : it->second) {
            if (str->value() == value) {
                return str;
            }
        }
    }
    
    return nullptr;
}

void StringPool::add(const Ptr<Object::String>& str) {
    u32 hash = computeHash(str->value());
    m_strings[hash].push_back(str);
}

void StringPool::remove(const Ptr<Object::String>& str) {
    u32 hash = computeHash(str->value());
    
    auto it = m_strings.find(hash);
    if (it != m_strings.end()) {
        auto& strings = it->second;
        // 查找并移除匹配的字符串
        for (auto strIt = strings.begin(); strIt != strings.end(); ++strIt) {
            if (*strIt == str) {
                strings.erase(strIt);
                // 如果列表为空，移除整个哈希项
                if (strings.empty()) {
                    m_strings.erase(it);
                }
                return;
            }
        }
    }
}

void StringPool::clear() {
    m_strings.clear();
}

u32 StringPool::computeHash(const Str& value) {
    // 简单的字符串哈希函数
    u32 hash = 5381;
    for (char c : value) {
        hash = ((hash << 5) + hash) + static_cast<u32>(c); // hash * 33 + c
    }
    return hash;
}

//===========================
// GarbageCollector 实现
//===========================

GarbageCollector::GarbageCollector()
    : m_state(nullptr), m_totalMemory(0), m_gcThreshold(GC_INITIAL_THRESHOLD), 
      m_pause(false), m_gcState(GCState::Idle), m_currentIndex(0) {
    // 初始化字符串池
    m_stringPool = std::make_shared<StringPool>();
}

GarbageCollector::~GarbageCollector() {
    // 清理所有对象
    collectGarbage();
    
    // 清空字符串池
    m_stringPool->clear();
    
    // 释放所有对象
    m_objects.clear();
}

Ptr<Object::String> GarbageCollector::createString(const Str& value) {
    // 首先在字符串池中查找
    auto pooled = m_stringPool->find(value);
    if (pooled) {
        return pooled;
    }
    
    // 创建新字符串对象
    auto str = std::make_shared<Object::String>(value);
    
    // 添加到字符串池和对象列表
    m_stringPool->add(str);
    m_objects.push_back(str);
    
    // 更新内存使用量
    m_totalMemory += sizeof(Object::String) + value.length();
    
    // 检查是否需要触发GC
    maybeGC();
    
    return str;
}

Ptr<Object::Table> GarbageCollector::createTable(i32 narray, i32 nrec) {
    // 创建新表对象
    auto table = std::make_shared<Object::Table>(narray, nrec);
    
    // 添加到对象列表
    m_objects.push_back(table);
    
    // 更新内存使用量
    m_totalMemory += sizeof(Object::Table) + narray * sizeof(Object::Value);
    
    // 检查是否需要触发GC
    maybeGC();
    
    return table;
}

Ptr<Object::Function> GarbageCollector::createFunction(Ptr<VM::FunctionProto> proto) {
    // 创建新函数对象
    auto func = std::make_shared<Object::Function>(proto);
    
    // 添加到对象列表
    m_objects.push_back(func);
    
    // 更新内存使用量
    m_totalMemory += sizeof(Object::Function);
    
    // 检查是否需要触发GC
    maybeGC();
    
    return func;
}

Ptr<Object::UserData> GarbageCollector::createUserData(usize size) {
    // 创建新用户数据对象
    auto userData = std::make_shared<Object::UserData>(size);
    
    // 添加到对象列表
    m_objects.push_back(userData);
    
    // 更新内存使用量
    m_totalMemory += sizeof(Object::UserData) + size;
    
    // 检查是否需要触发GC
    maybeGC();
    
    return userData;
}

void GarbageCollector::collectGarbage() {
    if (m_pause) {
        return;
    }
    
    // 标记阶段
    markRoots();
    mark();
    
    // 清理阶段
    sweep();
    
    // 调整GC阈值
    m_gcThreshold = m_totalMemory * GC_THRESHOLD_FACTOR;
    if (m_gcThreshold < GC_INITIAL_THRESHOLD) {
        m_gcThreshold = GC_INITIAL_THRESHOLD;
    }
}

void GarbageCollector::collectGarbageIncremental() {
    if (m_pause) {
        return;
    }
    
    // 每次增量GC处理的最大对象数量
    constexpr size_t INCREMENTAL_WORK_UNITS = 10;
    
    // 增量GC的实现
    switch (m_gcState) {
        case GCState::Idle:
            // 开始新的GC周期
            markRoots();
            m_gcState = GCState::Mark;
            m_currentIndex = 0;
            break;
            
        case GCState::Mark:
            // 执行部分标记工作
            if (m_toMark.empty()) {
                // 标记阶段完成，进入清除阶段
                m_gcState = GCState::Sweep;
                m_currentIndex = 0;
            } else {
                // 执行一批标记工作
                size_t workDone = 0;
                while (!m_toMark.empty() && workDone < INCREMENTAL_WORK_UNITS) {
                    // 取出一个对象进行标记
                    auto obj = m_toMark.back();
                    m_toMark.pop_back();
                    
                    // 如果对象已经被标记，跳过
                    if (obj->isMarked()) {
                        continue;
                    }
                    
                    // 标记对象
                    obj->mark();
                    
                    // 根据对象类型，将其引用的对象加入标记队列
                    switch (obj->type()) {
                        case GCObject::Type::Table: {
                            auto table = std::static_pointer_cast<Object::Table>(obj);
                            // 标记表中的所有键和值
                            for (const auto& entry : table->getEntries()) {
                                if (entry.key.isGCObject()) {
                                    m_toMark.push_back(entry.key.asGCObject());
                                }
                                if (entry.value.isGCObject()) {
                                    m_toMark.push_back(entry.value.asGCObject());
                                }
                            }
                            break;
                        }
                        case GCObject::Type::Function: {
                            auto func = std::static_pointer_cast<Object::Function>(obj);
                            // 标记函数的upvalues和原型
                            // ...
                            break;
                        }
                        // 添加其他需要处理引用的类型
                        default:
                            break;
                    }
                    
                    ++workDone;
                }
            }
            break;
            
        case GCState::Sweep:
            // 执行部分清理工作
            {
                size_t workDone = 0;
                size_t end = std::min(m_currentIndex + INCREMENTAL_WORK_UNITS, m_objects.size());
                
                while (m_currentIndex < end) {
                    if (m_currentIndex < m_objects.size()) {
                        auto& obj = m_objects[m_currentIndex];
                        
                        if (!obj->isMarked()) {
                            // 对象未被标记，需要被回收
                            
                            // 如果是字符串，从字符串池中移除
                            if (obj->type() == GCObject::Type::String) {
                                auto str = std::static_pointer_cast<Object::String>(obj);
                                m_stringPool->remove(str);
                            }
                            
                            // 更新内存使用量
                            m_totalMemory -= getObjectSize(obj);
                            
                            // 交换删除以保持性能
                            if (m_currentIndex != m_objects.size() - 1) {
                                std::swap(m_objects[m_currentIndex], m_objects.back());
                            }
                            m_objects.pop_back();
                            
                            // 不增加索引，因为我们交换了元素
                        } else {
                            // 清除标记位
                            obj->unmark();
                            ++m_currentIndex;
                        }
                    } else {
                        break;
                    }
                    
                    ++workDone;
                }
                
                // 检查是否完成所有对象
                if (m_currentIndex >= m_objects.size()) {
                    // 清理阶段完成，调整GC阈值
                    m_gcThreshold = m_totalMemory * GC_THRESHOLD_FACTOR;
                    if (m_gcThreshold < GC_INITIAL_THRESHOLD) {
                        m_gcThreshold = GC_INITIAL_THRESHOLD;
                    }
                    
                    // 重置GC状态
                    m_gcState = GCState::Idle;
                }
            }
            break;
    }
}

void GarbageCollector::pauseGC() {
    m_pause = true;
}

void GarbageCollector::resumeGC() {
    m_pause = false;
}

void GarbageCollector::fullGC() {
    bool wasPaused = m_pause;
    m_pause = false;
    collectGarbage();
    m_pause = wasPaused;
}

void GarbageCollector::markRoots() {
    if (!m_state) {
        return;
    }
    
    // 重置所有对象的标记
    for (auto& obj : m_objects) {
        obj->unmark();
    }
    
    // TODO: 标记全局表和注册表
    
    // TODO: 标记堆栈中的所有值
    
    // TODO: 标记其他根对象
}

void GarbageCollector::mark() {
    // 实现标记阶段
    // 从根对象开始，标记所有可达对象
    // TODO: 完整实现标记阶段
}

void GarbageCollector::sweep() {
    // 清理未标记的对象
    auto it = m_objects.begin();
    while (it != m_objects.end()) {
        if (!(*it)->isMarked()) {
            // 移除未标记的对象
            auto obj = *it;
            
            // 如果是字符串，从字符串池中移除
            if (obj->type() == GCObject::Type::String) {
                auto str = std::static_pointer_cast<Object::String>(obj);
                m_stringPool->remove(str);
            }
            
            // 更新内存使用量
            m_totalMemory -= getObjectSize(obj);
            
            // 从对象列表中移除
            it = m_objects.erase(it);
        } else {
            // 重置标记位
            (*it)->unmark();
            ++it;
        }
    }
}

void GarbageCollector::maybeGC() {
    if (!m_pause && m_totalMemory >= m_gcThreshold) {
        collectGarbage();
    }
}

usize GarbageCollector::getObjectSize(const Ptr<GCObject>& obj) const {
    usize size = 0;
    
    switch (obj->type()) {
        case GCObject::Type::String: {
            auto str = std::static_pointer_cast<Object::String>(obj);
            size = sizeof(Object::String) + str->value().length();
            break;
        }
        case GCObject::Type::Table: {
            auto table = std::static_pointer_cast<Object::Table>(obj);
            // 表的大小估计
            size = sizeof(Object::Table);
            // TODO: 更精确地计算表大小
            break;
        }
        case GCObject::Type::Function: {
            size = sizeof(Object::Function);
            // TODO: 更精确地计算函数大小
            break;
        }
        case GCObject::Type::UserData: {
            auto userData = std::static_pointer_cast<Object::UserData>(obj);
            size = sizeof(Object::UserData) + userData->getSize();
            break;
        }
        default:
            size = sizeof(GCObject);
            break;
    }
    
    return size;
}

} // namespace GC
} // namespace Lua
