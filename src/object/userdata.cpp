#include "userdata.hpp"
#include "table.hpp"

namespace Lua {
namespace Object {

UserData::UserData(std::shared_ptr<void> data, std::type_index type, std::shared_ptr<Table> metatable)
    : GC::GCObject(GC::GCObjectType::UserData),
      m_data(data), m_typeInfo(type), m_metatable(metatable) {
}

void UserData::mark(GC::GCManager* gc) {
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

} // namespace Object
} // namespace LuaCpp
