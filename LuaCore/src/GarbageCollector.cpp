#include "LuaCore/GarbageCollector.h"
#include "LuaCore/State.h"
#include <algorithm>

namespace LuaCore {

GarbageCollector::GarbageCollector(State* state)
    : m_state(state) {
}

GarbageCollector::~GarbageCollector() {
    // Force a full collection to clean up any remaining objects
    collectGarbage();
    
    // Clear all object references
    m_objects.clear();
}

void GarbageCollector::registerObject(std::shared_ptr<GCObject> object) {
    if (!object) {
        return;
    }
    
    // Add to the set of managed objects
    m_objects.insert(object);
    
    // Update memory tracking
    m_totalBytes += object->getMemorySize();
    
    // Check if we need to start garbage collection
    if (m_totalBytes > m_totalBytesLimit && m_phase == GCPhase::Idle) {
        collectGarbageIncremental();
    }
}

void GarbageCollector::unregisterObject(std::shared_ptr<GCObject> object) {
    if (!object || m_objects.find(object) == m_objects.end()) {
        return;
    }
    
    // Update memory tracking
    m_totalBytes -= object->getMemorySize();
    
    // Remove from the managed objects set
    m_objects.erase(object);
}

void GarbageCollector::collectGarbage() {
    // Reset all GC state
    resetGCState();
    
    // Mark all reachable objects
    markRoots();
    
    // Process all gray objects until none are left
    while (!m_gray.empty()) {
        markGray();
    }
    
    // Sweep all unmarked objects
    sweep();
    
    // Update GC thresholds
    m_totalBytesLimit = static_cast<size_t>(m_totalBytes * m_gcPause);
    
    // Reset GC phase
    m_phase = GCPhase::Idle;
}

void GarbageCollector::collectGarbageIncremental() {
    // If we're idle, start a new collection cycle
    if (m_phase == GCPhase::Idle) {
        resetGCState();
        markRoots();
        m_phase = GCPhase::Mark;
        return;
    }
    
    // If we're in the mark phase, process some gray objects
    if (m_phase == GCPhase::Mark) {
        // Calculate how many objects to process in this step
        size_t workAmount = std::max(
            static_cast<size_t>(1),
            static_cast<size_t>(m_totalBytes / (1024 * m_gcStepMultiplier))
        );
        
        // Process gray objects up to the work amount
        for (size_t i = 0; i < workAmount && !m_gray.empty(); ++i) {
            markGray();
        }
        
        // If no more gray objects, move to sweep phase
        if (m_gray.empty()) {
            // Prepare for sweep phase
            for (const auto& obj : m_objects) {
                if (!obj->isMarked()) {
                    m_toSweep.insert(obj);
                }
            }
            m_phase = GCPhase::Sweep;
        }
        
        return;
    }
    
    // If we're in the sweep phase, sweep some objects
    if (m_phase == GCPhase::Sweep) {
        // Calculate how many objects to sweep in this step
        size_t workAmount = std::max(
            static_cast<size_t>(1),
            static_cast<size_t>(m_totalBytes / (1024 * m_gcStepMultiplier))
        );
        
        // Sweep objects up to the work amount
        for (size_t i = 0; i < workAmount && !m_toSweep.empty(); ++i) {
            auto it = m_toSweep.begin();
            auto obj = *it;
            m_toSweep.erase(it);
            
            // Remove from the managed objects set
            m_totalBytes -= obj->getMemorySize();
            m_objects.erase(obj);
        }
        
        // If no more objects to sweep, end the collection cycle
        if (m_toSweep.empty()) {
            // Update GC thresholds
            m_totalBytesLimit = static_cast<size_t>(m_totalBytes * m_gcPause);
            
            // Reset GC phase
            m_phase = GCPhase::Idle;
        }
        
        return;
    }
}

void GarbageCollector::setGCPause(double pause) {
    m_gcPause = pause;
}

void GarbageCollector::setGCStepMultiplier(double stepMultiplier) {
    m_gcStepMultiplier = stepMultiplier;
}

void GarbageCollector::markRoots() {
    // Mark all global objects from the state
    if (auto globals = m_state->getGlobals()) {
        globals->mark();
        m_gray.push_back(globals);
    }
    
    // Mark registry table
    if (auto registry = m_state->getRegistry()) {
        registry->mark();
        m_gray.push_back(registry);
    }
    
    // Mark all values on the stack
    for (int i = 0; i < m_state->getTop(); ++i) {
        const Value& value = m_state->get(i + 1);
        
        // For table values
        if (value.isTable() && value.asTable()) {
            value.asTable()->mark();
            m_gray.push_back(value.asTable());
        }
        
        // For function values
        if (value.isFunction() && value.asFunction()) {
            value.asFunction()->mark();
            m_gray.push_back(value.asFunction());
        }
        
        // For userdata values
        if (value.isUserData() && value.asUserData()) {
            value.asUserData()->mark();
            m_gray.push_back(value.asUserData());
        }
    }
}

void GarbageCollector::markGray() {
    if (m_gray.empty()) {
        return;
    }
    
    // Get the next gray object
    auto obj = m_gray.back();
    m_gray.pop_back();
    
    // Mark its references
    // This will add more gray objects to the list if needed
    obj->mark();
}

void GarbageCollector::sweep() {
    // Identify unmarked objects
    std::vector<std::shared_ptr<GCObject>> toRemove;
    
    for (const auto& obj : m_objects) {
        if (!obj->isMarked()) {
            toRemove.push_back(obj);
        }
    }
    
    // Remove unmarked objects
    for (const auto& obj : toRemove) {
        // Update memory tracking
        m_totalBytes -= obj->getMemorySize();
        
        // Remove from the managed objects set
        m_objects.erase(obj);
    }
}

void GarbageCollector::resetGCState() {
    // Clear gray list
    m_gray.clear();
    
    // Clear sweep set
    m_toSweep.clear();
    
    // Unmark all objects
    for (const auto& obj : m_objects) {
        obj->unmark();
    }
}

} // namespace LuaCore
