#include "LuaCore/UserData.h"

namespace LuaCore {

UserData::UserData(std::shared_ptr<void> data, std::type_index type, std::shared_ptr<Table> metatable)
    : m_data(data), m_typeInfo(type), m_metatable(metatable) {
}

void UserData::mark() {
    // Skip if already marked
    if (isMarked()) {
        return;
    }
    
    // Mark this object
    GCObject::mark();
    
    // Mark metatable if present
    if (m_metatable) {
        m_metatable->mark();
    }
}

} // namespace LuaCore
