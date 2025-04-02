#include "userdata.hpp"
#include "table.hpp"

namespace Lua {
UserData::UserData(Ptr<void> data, std::type_index type, Ptr<Table> metatable)
    : GC::GCObject(GC::GCObjectType::UserData),
      m_data(data), m_typeInfo(type), m_metatable(metatable) {
}

void UserData::mark(GarbageCollector* gc) {
    // Skip if already marked
    if (isMarked()) {
        return;
    }
    
    // Mark this object
    GCObject::mark(gc);
    
    // Mark metatable if present
    if (m_metatable) {
        m_metatable->mark(gc);
    }
}

} // namespace Lua
