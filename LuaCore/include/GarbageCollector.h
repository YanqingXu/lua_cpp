#pragma once

#include "GCObject.h"
#include <vector>
#include <memory>
#include <unordered_set>
#include <functional>

namespace LuaCore {

// Forward declarations
class State;

/**
 * @brief Manages memory allocation and garbage collection for Lua
 * 
 * The GarbageCollector class is responsible for tracking and reclaiming memory
 * used by Lua objects. It implements a mark-and-sweep garbage collection algorithm
 * with incremental collection capabilities.
 */
class GarbageCollector {
public:
    // Constructor requires a state reference
    explicit GarbageCollector(State* state);
    
    // Destructor ensures cleanup
    ~GarbageCollector();
    
    // Register an object to be managed by the garbage collector
    void registerObject(std::shared_ptr<GCObject> object);
    
    // Unregister an object (e.g., when it's manually deleted)
    void unregisterObject(std::shared_ptr<GCObject> object);
    
    // Perform a full garbage collection cycle
    void collectGarbage();
    
    // Perform an incremental garbage collection step
    void collectGarbageIncremental();
    
    // Manual control of GC
    void setGCPause(double pause);
    void setGCStepMultiplier(double stepMultiplier);
    
    // Get current memory usage
    size_t getTotalBytes() const { return m_totalBytes; }
    
    // Settings for collection thresholds
    void setTotalBytesLimit(size_t limit) { m_totalBytesLimit = limit; }
    size_t getTotalBytesLimit() const { return m_totalBytesLimit; }
    
private:
    // Reference to the Lua state this collector belongs to
    State* m_state;
    
    // Set of all objects managed by this collector
    std::unordered_set<std::shared_ptr<GCObject>> m_objects;
    
    // Current memory usage
    size_t m_totalBytes = 0;
    
    // Threshold for triggering garbage collection
    size_t m_totalBytesLimit = 1024 * 1024; // Default: 1MB
    
    // GC settings
    double m_gcPause = 2.0;          // How long to wait after a collection (multiplier)
    double m_gcStepMultiplier = 2.0; // How much to collect in a step (multiplier)
    
    // GC phases
    enum class GCPhase {
        Idle,
        Mark,
        Sweep
    };
    GCPhase m_phase = GCPhase::Idle;
    
    // For incremental GC
    std::vector<std::shared_ptr<GCObject>> m_gray;  // Objects to be traversed
    std::unordered_set<std::shared_ptr<GCObject>> m_toSweep; // Objects to be swept
    
    // Garbage collection implementation phases
    void markRoots();    // Mark all root objects
    void markGray();     // Process gray objects
    void sweep();        // Sweep unmarked objects
    
    // Reset GC state (unmark all objects)
    void resetGCState();
};

} // namespace LuaCore
