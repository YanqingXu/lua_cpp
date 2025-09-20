#include <catch2/catch_test_macros.hpp>
#include <catch2/benchmark/catch_benchmark.hpp>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

// Mock/Forward declarations for LuaState system - will be replaced with actual implementations
namespace lua {

// Forward declarations for core types
class TValue;
class LuaString;
class LuaTable;
class LuaFunction;
class Prototype;

// Lua state status codes
enum class LuaStatus {
    OK = 0,
    YIELD = 1,
    ERRRUN = 2,
    ERRSYNTAX = 3,
    ERRMEM = 4,
    ERRERR = 5,
    ERRFILE = 6
};

// Lua type constants
enum class LuaType {
    NONE = -1,
    NIL = 0,
    BOOLEAN = 1,
    LIGHTUSERDATA = 2,
    NUMBER = 3,
    STRING = 4,
    TABLE = 5,
    FUNCTION = 6,
    USERDATA = 7,
    THREAD = 8
};

// Stack index constants
constexpr int LUA_REGISTRYINDEX = -10000;
constexpr int LUA_ENVIRONINDEX = -10001;
constexpr int LUA_GLOBALSINDEX = -10002;

// Hook event types
enum class HookEvent {
    CALL = 0,
    RET = 1,
    LINE = 2,
    COUNT = 3,
    TAILRET = 4
};

// Hook masks
constexpr int LUA_MASKCALL = (1 << static_cast<int>(HookEvent::CALL));
constexpr int LUA_MASKRET = (1 << static_cast<int>(HookEvent::RET));
constexpr int LUA_MASKLINE = (1 << static_cast<int>(HookEvent::LINE));
constexpr int LUA_MASKCOUNT = (1 << static_cast<int>(HookEvent::COUNT));

// Function pointer types
using lua_CFunction = int(*)(struct LuaState* L);
using lua_Hook = void(*)(struct LuaState* L, struct lua_Debug* ar);
using lua_Alloc = void*(*)(void* ud, void* ptr, size_t osize, size_t nsize);

// Debug information structure
struct lua_Debug {
    HookEvent event;
    const char* name;          // (n)
    const char* namewhat;      // (n) 'global', 'local', 'field', 'method'
    const char* what;          // (S) 'Lua', 'C', 'main', 'tail'
    const char* source;        // (S)
    int currentline;           // (l)
    int nups;                  // (u) number of upvalues
    int linedefined;           // (S)
    int lastlinedefined;       // (S)
    char short_src[60];        // (S)
    // Private part - implementation specific
    void* i_ci;                // active function
};

// Call information structure
struct CallInfo {
    TValue* func;              // function index in the stack
    TValue* top;               // top for this function
    const void* savedpc;       // saved program counter
    int nresults;              // expected number of results from this function
    int tailcalls;             // number of tail calls lost under this entry
    CallInfo* previous;        // dynamic call link
};

// Garbage collector state
enum class GCState {
    PAUSE,      // collection is not running
    PROPAGATE,  // marking phase
    SWEEP,      // sweeping phase
    FINALIZE    // finalizing phase
};

// Main Lua state structure
struct LuaState {
    // Stack management
    TValue* stack;             // stack base
    TValue* top;               // first free slot in the stack
    TValue* stack_last;        // last free slot in the stack
    TValue* stack_size;        // stack size
    
    // Call information
    CallInfo* ci;              // call info for current function
    CallInfo* end_ci;          // points after end of ci array
    CallInfo* base_ci;         // array of CallInfo's
    
    // Global state pointer
    struct GlobalState* l_G;
    
    // Hook management
    lua_Hook hook;
    ptrdiff_t errfunc;         // current error handling function (stack index)
    int hookmask;
    int hookcount;
    int basehookcount;
    
    // Thread/coroutine management
    LuaState* openupval;       // list of open upvalues in this stack
    struct GCObject* gclist;
    
    // Status and error information
    LuaStatus status;
    const char* errinfo;       // error message for failed operations
    
    // Environment tables
    TValue env;                // environment table
    TValue gt;                 // table of globals
    
    // Memory management
    size_t totalbytes;         // total bytes allocated
    size_t GCthreshold;        // when totalbytes > GCthreshold, run GC
    size_t GCdebt;             // bytes allocated since last GC
    GCState gcstate;           // state of garbage collector
    
    // Constructors and core operations
    LuaState();
    explicit LuaState(lua_Alloc f, void* ud);
    ~LuaState();
    
    // State management
    LuaState* newthread();
    void close();
    
    // Stack operations
    int gettop();
    void settop(int idx);
    void pushvalue(int idx);
    void remove(int idx);
    void insert(int idx);
    void replace(int idx);
    int checkstack(int sz);
    void xmove(LuaState* to, int n);
    
    // Access functions (stack -> C)
    int isnumber(int idx);
    int isstring(int idx);
    int iscfunction(int idx);
    int isuserdata(int idx);
    LuaType type(int idx);
    const char* typename_(LuaType tp);
    
    int equal(int idx1, int idx2);
    int rawequal(int idx1, int idx2);
    int lessthan(int idx1, int idx2);
    
    double tonumber(int idx);
    const char* tostring(int idx);
    size_t objlen(int idx);
    lua_CFunction tocfunction(int idx);
    void* touserdata(int idx);
    LuaState* tothread(int idx);
    const void* topointer(int idx);
    
    // Push functions (C -> stack)
    void pushnil();
    void pushnumber(double n);
    void pushstring(const char* s);
    void pushlstring(const char* s, size_t len);
    void pushcclosure(lua_CFunction fn, int n);
    void pushboolean(int b);
    void pushlightuserdata(void* p);
    int pushthread();
    
    // Get functions (Lua -> stack)
    void gettable(int idx);
    void getfield(int idx, const char* k);
    void rawget(int idx);
    void rawgeti(int idx, int n);
    void getmetatable(int objindex);
    void getfenv(int idx);
    
    // Set functions (stack -> Lua)
    void settable(int idx);
    void setfield(int idx, const char* k);
    void rawset(int idx);
    void rawseti(int idx, int n);
    int setmetatable(int objindex);
    int setfenv(int idx);
    
    // Load and call functions
    LuaStatus load(const char* chunk, size_t size, const char* name);
    LuaStatus call(int nargs, int nresults);
    LuaStatus pcall(int nargs, int nresults, int errfunc);
    LuaStatus cpcall(lua_CFunction func, void* ud);
    
    // Coroutine functions
    LuaStatus resume(int narg);
    int yield(int nresults);
    
    // Garbage collection
    int gc(int what, int data);
    
    // Miscellaneous functions
    int error();
    int next(int idx);
    void concat(int n);
    
    // Debug interface
    int getstack(int level, lua_Debug* ar);
    int getinfo(const char* what, lua_Debug* ar);
    const char* getlocal(const lua_Debug* ar, int n);
    const char* setlocal(const lua_Debug* ar, int n);
    const char* getupvalue(int funcindex, int n);
    const char* setupvalue(int funcindex, int n);
    
    int sethook(lua_Hook func, int mask, int count);
    lua_Hook gethook();
    int gethookmask();
    int gethookcount();
    
    // Memory management
    void* newuserdata(size_t sz);
    int getallocf(void** ud);
    void setallocf(lua_Alloc f, void* ud);
    
    // Registry operations
    void pushregistryindex();
    void getregistry();
    
private:
    // Internal stack management
    void growstack(int size);
    void reallocstack(int newsize);
    TValue* index2adr(int idx);
    int stackspace();
    
    // Internal call management
    void call_base(int nargs, int nresults, bool protected_call);
    void postcall(int wanted);
    void precall(TValue* func, int nresults);
    
    // Internal error handling
    void throw_error(const char* msg);
    void handle_error(LuaStatus status);
    
    // Internal GC integration
    void checkGC();
    void incr_top();
};

// Global state structure
struct GlobalState {
    // String table for interning
    LuaString** strt;
    int strtsize;
    
    // Garbage collection
    GCState gcstate;
    struct GCObject* rootgc;
    struct GCObject* gray;
    struct GCObject* grayagain;
    struct GCObject* weak;
    size_t totalbytes;
    size_t GCthreshold;
    
    // Memory allocation
    lua_Alloc frealloc;
    void* ud;
    
    // Main thread
    LuaState* mainthread;
    
    // Registry
    TValue l_registry;
    
    // Panic function
    lua_CFunction panic;
    
    GlobalState();
    ~GlobalState();
};

} // namespace lua

// =============================================================================
// CONTRACT TESTS FOR LUA STATE SYSTEM
// =============================================================================

TEST_CASE("LuaState creation and destruction", "[LuaState][lifecycle]") {
    SECTION("Default constructor creates valid state") {
        lua::LuaState L;
        
        // State should be properly initialized
        REQUIRE(L.gettop() == 0);
        REQUIRE(L.status == lua::LuaStatus::OK);
        REQUIRE(L.stack != nullptr);
        REQUIRE(L.top == L.stack);
        REQUIRE(L.ci != nullptr);
        REQUIRE(L.l_G != nullptr);
    }
    
    SECTION("Custom allocator constructor") {
        auto custom_alloc = [](void* ud, void* ptr, size_t osize, size_t nsize) -> void* {
            if (nsize == 0) {
                free(ptr);
                return nullptr;
            }
            return realloc(ptr, nsize);
        };
        
        lua::LuaState L(custom_alloc, nullptr);
        REQUIRE(L.gettop() == 0);
        REQUIRE(L.status == lua::LuaStatus::OK);
    }
    
    SECTION("Thread creation from existing state") {
        lua::LuaState L;
        lua::LuaState* thread = L.newthread();
        
        REQUIRE(thread != nullptr);
        REQUIRE(thread != &L);
        REQUIRE(thread->l_G == L.l_G);  // Should share global state
        REQUIRE(thread->gettop() == 0);
        REQUIRE(thread->status == lua::LuaStatus::OK);
    }
}

TEST_CASE("LuaState stack management", "[LuaState][stack]") {
    lua::LuaState L;
    
    SECTION("Basic stack operations") {
        REQUIRE(L.gettop() == 0);
        
        // Push values
        L.pushnil();
        REQUIRE(L.gettop() == 1);
        REQUIRE(L.type(1) == lua::LuaType::NIL);
        
        L.pushnumber(42.0);
        REQUIRE(L.gettop() == 2);
        REQUIRE(L.type(2) == lua::LuaType::NUMBER);
        REQUIRE(L.tonumber(2) == 42.0);
        
        L.pushstring("hello");
        REQUIRE(L.gettop() == 3);
        REQUIRE(L.type(3) == lua::LuaType::STRING);
        REQUIRE(std::string(L.tostring(3)) == "hello");
        
        L.pushboolean(1);
        REQUIRE(L.gettop() == 4);
        REQUIRE(L.type(4) == lua::LuaType::BOOLEAN);
    }
    
    SECTION("Stack index validation") {
        L.pushnumber(1.0);
        L.pushnumber(2.0);
        L.pushnumber(3.0);
        
        // Positive indices (from bottom)
        REQUIRE(L.tonumber(1) == 1.0);
        REQUIRE(L.tonumber(2) == 2.0);
        REQUIRE(L.tonumber(3) == 3.0);
        
        // Negative indices (from top)
        REQUIRE(L.tonumber(-1) == 3.0);
        REQUIRE(L.tonumber(-2) == 2.0);
        REQUIRE(L.tonumber(-3) == 1.0);
        
        // Invalid indices
        REQUIRE(L.type(0) == lua::LuaType::NONE);
        REQUIRE(L.type(4) == lua::LuaType::NONE);
        REQUIRE(L.type(-4) == lua::LuaType::NONE);
    }
    
    SECTION("Stack manipulation") {
        L.pushnumber(1.0);
        L.pushnumber(2.0);
        L.pushnumber(3.0);
        
        // pushvalue - duplicate value
        L.pushvalue(2);  // duplicate 2.0
        REQUIRE(L.gettop() == 4);
        REQUIRE(L.tonumber(4) == 2.0);
        
        // remove - remove value and shift
        L.remove(2);  // remove original 2.0
        REQUIRE(L.gettop() == 3);
        REQUIRE(L.tonumber(1) == 1.0);
        REQUIRE(L.tonumber(2) == 3.0);
        REQUIRE(L.tonumber(3) == 2.0);  // duplicated value
        
        // insert - move top to position
        L.pushnumber(4.0);
        L.insert(2);  // move 4.0 to position 2
        REQUIRE(L.gettop() == 4);
        REQUIRE(L.tonumber(2) == 4.0);
        
        // replace - replace value without changing stack size
        L.pushnumber(5.0);
        L.replace(3);  // replace value at position 3
        REQUIRE(L.gettop() == 4);
        REQUIRE(L.tonumber(3) == 5.0);
        
        // settop - set stack size
        L.settop(2);
        REQUIRE(L.gettop() == 2);
        REQUIRE(L.tonumber(1) == 1.0);
        REQUIRE(L.tonumber(2) == 4.0);
    }
    
    SECTION("Stack growth and limits") {
        // Check initial stack space
        int initial_space = L.checkstack(0);
        REQUIRE(initial_space > 0);
        
        // Ensure we can grow stack
        REQUIRE(L.checkstack(1000) == 1);
        
        // Push many values to test growth
        for (int i = 0; i < 1000; ++i) {
            L.pushnumber(static_cast<double>(i));
        }
        REQUIRE(L.gettop() == 1000);
        
        // Verify values are correct
        for (int i = 0; i < 1000; ++i) {
            REQUIRE(L.tonumber(i + 1) == static_cast<double>(i));
        }
    }
    
    SECTION("Stack transfer between states") {
        lua::LuaState* L2 = L.newthread();
        
        // Push values in original state
        L.pushnumber(1.0);
        L.pushstring("test");
        L.pushboolean(1);
        
        // Transfer values
        L.xmove(L2, 3);
        REQUIRE(L.gettop() == 0);
        REQUIRE(L2->gettop() == 3);
        
        // Verify transferred values
        REQUIRE(L2->tonumber(1) == 1.0);
        REQUIRE(std::string(L2->tostring(2)) == "test");
        REQUIRE(L2->type(3) == lua::LuaType::BOOLEAN);
    }
}

TEST_CASE("LuaState type system and conversions", "[LuaState][types]") {
    lua::LuaState L;
    
    SECTION("Type identification") {
        L.pushnil();
        L.pushnumber(42.0);
        L.pushstring("hello");
        L.pushboolean(1);
        L.pushboolean(0);
        
        REQUIRE(L.type(1) == lua::LuaType::NIL);
        REQUIRE(L.type(2) == lua::LuaType::NUMBER);
        REQUIRE(L.type(3) == lua::LuaType::STRING);
        REQUIRE(L.type(4) == lua::LuaType::BOOLEAN);
        REQUIRE(L.type(5) == lua::LuaType::BOOLEAN);
        
        // Type name function
        REQUIRE(std::string(L.typename_(lua::LuaType::NIL)) == "nil");
        REQUIRE(std::string(L.typename_(lua::LuaType::NUMBER)) == "number");
        REQUIRE(std::string(L.typename_(lua::LuaType::STRING)) == "string");
        REQUIRE(std::string(L.typename_(lua::LuaType::BOOLEAN)) == "boolean");
    }
    
    SECTION("Type checking predicates") {
        L.pushnumber(42.0);
        L.pushstring("123");
        L.pushstring("hello");
        
        // isnumber - numbers and numeric strings
        REQUIRE(L.isnumber(1) == 1);
        REQUIRE(L.isnumber(2) == 1);  // numeric string
        REQUIRE(L.isnumber(3) == 0);  // non-numeric string
        
        // isstring - strings and numbers
        REQUIRE(L.isstring(1) == 1);  // numbers are convertible
        REQUIRE(L.isstring(2) == 1);
        REQUIRE(L.isstring(3) == 1);
    }
    
    SECTION("Value conversion") {
        L.pushnumber(42.5);
        L.pushstring("123.25");
        L.pushstring("hello");
        L.pushboolean(1);
        L.pushnil();
        
        // tonumber conversions
        REQUIRE(L.tonumber(1) == 42.5);
        REQUIRE(L.tonumber(2) == 123.25);
        REQUIRE(L.tonumber(3) == 0.0);  // non-numeric string -> 0
        REQUIRE(L.tonumber(4) == 1.0);  // true -> 1
        REQUIRE(L.tonumber(5) == 0.0);  // nil -> 0
        
        // tostring conversions
        REQUIRE(std::string(L.tostring(1)) == "42.5");
        REQUIRE(std::string(L.tostring(2)) == "123.25");
        REQUIRE(std::string(L.tostring(3)) == "hello");
        REQUIRE(std::string(L.tostring(4)) == "true");
        REQUIRE(L.tostring(5) == nullptr);  // nil -> nullptr
    }
    
    SECTION("Value comparison") {
        L.pushnumber(42.0);
        L.pushnumber(42.0);
        L.pushnumber(24.0);
        L.pushstring("42");
        L.pushstring("42");
        
        // equal - uses metamethods and coercion
        REQUIRE(L.equal(1, 2) == 1);  // same numbers
        REQUIRE(L.equal(1, 3) == 0);  // different numbers
        REQUIRE(L.equal(1, 4) == 1);  // number == numeric string
        REQUIRE(L.equal(4, 5) == 1);  // same strings
        
        // rawequal - raw comparison without metamethods
        REQUIRE(L.rawequal(1, 2) == 1);  // same numbers
        REQUIRE(L.rawequal(1, 4) == 0);  // different types
        REQUIRE(L.rawequal(4, 5) == 1);  // same strings
        
        // lessthan - ordering comparison
        REQUIRE(L.lessthan(3, 1) == 1);  // 24 < 42
        REQUIRE(L.lessthan(1, 3) == 0);  // 42 < 24
    }
}

TEST_CASE("LuaState table operations", "[LuaState][tables]") {
    lua::LuaState L;
    
    SECTION("Table creation and access") {
        // Create a table (in real implementation, would use lua_newtable)
        // For now, simulate with userdata or use load/call
        
        // This section would test:
        // - Table creation
        // - Field access (gettable, getfield)
        // - Field assignment (settable, setfield)
        // - Raw access (rawget, rawset, rawgeti, rawseti)
        // - Length operator (objlen)
        // - Table traversal (next)
        
        // Mock implementation for contract testing
        REQUIRE(true);  // Placeholder
    }
    
    SECTION("Metatable operations") {
        // This section would test:
        // - Metatable assignment (setmetatable)
        // - Metatable retrieval (getmetatable)
        // - Metamethod invocation through operators
        
        REQUIRE(true);  // Placeholder
    }
    
    SECTION("Environment operations") {
        // This section would test:
        // - Environment table access (getfenv)
        // - Environment table assignment (setfenv)
        // - Global table access
        
        REQUIRE(true);  // Placeholder
    }
}

TEST_CASE("LuaState function calls and error handling", "[LuaState][calls]") {
    lua::LuaState L;
    
    SECTION("C function registration and calls") {
        auto test_func = [](lua::LuaState* L) -> int {
            double a = L->tonumber(1);
            double b = L->tonumber(2);
            L->pushnumber(a + b);
            return 1;  // number of return values
        };
        
        L.pushcclosure(test_func, 0);
        L.pushnumber(3.0);
        L.pushnumber(4.0);
        
        REQUIRE(L.call(2, 1) == lua::LuaStatus::OK);
        REQUIRE(L.gettop() == 1);
        REQUIRE(L.tonumber(1) == 7.0);
    }
    
    SECTION("Protected calls and error handling") {
        auto error_func = [](lua::LuaState* L) -> int {
            return L->error();  // Raise an error
        };
        
        L.pushcclosure(error_func, 0);
        
        // Protected call should catch the error
        lua::LuaStatus status = L.pcall(0, 0, 0);
        REQUIRE(status == lua::LuaStatus::ERRRUN);
        REQUIRE(L.gettop() == 1);  // Error message on stack
        REQUIRE(L.type(1) == lua::LuaType::STRING);
    }
    
    SECTION("Lua code loading and execution") {
        const char* code = "return 2 + 3";
        
        lua::LuaStatus load_status = L.load(code, strlen(code), "test");
        REQUIRE(load_status == lua::LuaStatus::OK);
        REQUIRE(L.gettop() == 1);
        REQUIRE(L.type(1) == lua::LuaType::FUNCTION);
        
        lua::LuaStatus call_status = L.call(0, 1);
        REQUIRE(call_status == lua::LuaStatus::OK);
        REQUIRE(L.gettop() == 1);
        REQUIRE(L.tonumber(1) == 5.0);
    }
    
    SECTION("Error function handling") {
        auto error_handler = [](lua::LuaState* L) -> int {
            L.pushstring("Error handled!");
            return 1;
        };
        
        auto failing_func = [](lua::LuaState* L) -> int {
            L.pushstring("Original error");
            return L.error();
        };
        
        L.pushcclosure(error_handler, 0);
        int errfunc_idx = L.gettop();
        
        L.pushcclosure(failing_func, 0);
        
        lua::LuaStatus status = L.pcall(0, 0, errfunc_idx);
        REQUIRE(status == lua::LuaStatus::ERRRUN);
        REQUIRE(std::string(L.tostring(-1)) == "Error handled!");
    }
}

TEST_CASE("LuaState coroutine support", "[LuaState][coroutines]") {
    lua::LuaState L;
    
    SECTION("Thread creation and status") {
        lua::LuaState* thread = L.newthread();
        
        REQUIRE(thread->status == lua::LuaStatus::OK);
        REQUIRE(L.pushthread() == 1);  // Main thread
        REQUIRE(thread->pushthread() == 0);  // Non-main thread
    }
    
    SECTION("Coroutine yield and resume") {
        lua::LuaState* co = L.newthread();
        
        // Mock coroutine function that yields
        auto yielding_func = [](lua::LuaState* L) -> int {
            L->pushnumber(1.0);
            return L->yield(1);
        };
        
        co->pushcclosure(yielding_func, 0);
        
        // Resume coroutine
        lua::LuaStatus status = co->resume(0);
        REQUIRE(status == lua::LuaStatus::YIELD);
        REQUIRE(co->gettop() == 1);
        REQUIRE(co->tonumber(1) == 1.0);
        
        // Resume again (should complete)
        status = co->resume(0);
        REQUIRE(status == lua::LuaStatus::OK);
    }
    
    SECTION("Coroutine error handling") {
        lua::LuaState* co = L.newthread();
        
        auto error_func = [](lua::LuaState* L) -> int {
            L->pushstring("Coroutine error");
            return L->error();
        };
        
        co->pushcclosure(error_func, 0);
        
        lua::LuaStatus status = co->resume(0);
        REQUIRE(status == lua::LuaStatus::ERRRUN);
        REQUIRE(co->gettop() == 1);
        REQUIRE(std::string(co->tostring(1)) == "Coroutine error");
    }
}

TEST_CASE("LuaState debug interface", "[LuaState][debug]") {
    lua::LuaState L;
    
    SECTION("Hook function registration") {
        bool hook_called = false;
        auto hook_func = [](lua::LuaState* L, lua::lua_Debug* ar) {
            // Hook implementation would set external flag
        };
        
        // Set hook for all events
        int mask = lua::LUA_MASKCALL | lua::LUA_MASKRET | lua::LUA_MASKLINE;
        REQUIRE(L.sethook(hook_func, mask, 0) == 1);
        
        // Verify hook is set
        REQUIRE(L.gethook() == hook_func);
        REQUIRE(L.gethookmask() == mask);
    }
    
    SECTION("Stack inspection") {
        auto test_func = [](lua::LuaState* L) -> int {
            lua::lua_Debug ar;
            
            // Get information about current function
            if (L->getstack(0, &ar)) {
                L->getinfo("nSl", &ar);
                
                // Verify debug information
                REQUIRE(ar.what != nullptr);
                REQUIRE(ar.currentline >= 0);
            }
            
            return 0;
        };
        
        L.pushcclosure(test_func, 0);
        L.call(0, 0);
    }
    
    SECTION("Local variable access") {
        // This would test getlocal/setlocal functions
        // Requires more complex setup with actual Lua functions
        REQUIRE(true);  // Placeholder
    }
    
    SECTION("Upvalue access") {
        // Create closure with upvalues
        L.pushnumber(42.0);
        
        auto closure_func = [](lua::LuaState* L) -> int {
            // Access upvalue
            const char* name = L->getupvalue(1, 1);
            REQUIRE(name != nullptr);
            REQUIRE(L->type(-1) == lua::LuaType::NUMBER);
            REQUIRE(L->tonumber(-1) == 42.0);
            
            // Modify upvalue
            L->pushnumber(24.0);
            L->setupvalue(1, 1);
            
            return 0;
        };
        
        L.pushcclosure(closure_func, 1);  // 1 upvalue
        L.pushvalue(-1);  // duplicate for call
        L.call(0, 0);
        
        // Verify upvalue was modified
        const char* name = L.getupvalue(-1, 1);
        REQUIRE(L.tonumber(-1) == 24.0);
    }
}

TEST_CASE("LuaState garbage collection integration", "[LuaState][gc]") {
    lua::LuaState L;
    
    SECTION("Manual GC control") {
        // Get initial memory usage
        int initial_mem = L.gc(0, 0);  // LUA_GCCOUNT
        
        // Allocate some objects
        for (int i = 0; i < 100; ++i) {
            L.pushstring("test string for GC");
        }
        L.settop(0);  // Remove references
        
        // Force garbage collection
        L.gc(2, 0);  // LUA_GCCOLLECT
        
        // Memory should be freed
        int final_mem = L.gc(0, 0);
        // Note: exact comparison depends on GC implementation
        REQUIRE(final_mem >= 0);
    }
    
    SECTION("GC step control") {
        // Enable incremental GC
        L.gc(4, 1);  // LUA_GCSETPAUSE
        L.gc(5, 100);  // LUA_GCSETSTEPMUL
        
        // Perform incremental GC steps
        for (int i = 0; i < 10; ++i) {
            int result = L.gc(3, 1);  // LUA_GCSTEP
            if (result == 1) break;  // Collection cycle completed
        }
        
        REQUIRE(true);  // Test that GC operations don't crash
    }
    
    SECTION("Finalizer handling") {
        // This would test proper finalizer invocation
        // Requires userdata with __gc metamethod
        REQUIRE(true);  // Placeholder
    }
}

TEST_CASE("LuaState memory management", "[LuaState][memory]") {
    lua::LuaState L;
    
    SECTION("Userdata allocation") {
        size_t size = 1024;
        void* ud = L.newuserdata(size);
        
        REQUIRE(ud != nullptr);
        REQUIRE(L.gettop() == 1);
        REQUIRE(L.type(1) == lua::LuaType::USERDATA);
        REQUIRE(L.touserdata(1) == ud);
        REQUIRE(L.objlen(1) == size);
        
        // Test that memory is accessible
        memset(ud, 0x42, size);
        REQUIRE(static_cast<unsigned char*>(ud)[0] == 0x42);
        REQUIRE(static_cast<unsigned char*>(ud)[size-1] == 0x42);
    }
    
    SECTION("Custom allocator integration") {
        size_t allocated = 0;
        size_t deallocated = 0;
        
        auto tracking_alloc = [&](void* ud, void* ptr, size_t osize, size_t nsize) -> void* {
            if (nsize == 0) {
                deallocated += osize;
                free(ptr);
                return nullptr;
            } else if (ptr == nullptr) {
                allocated += nsize;
                return malloc(nsize);
            } else {
                deallocated += osize;
                allocated += nsize;
                return realloc(ptr, nsize);
            }
        };
        
        lua::LuaState L2(tracking_alloc, nullptr);
        
        // Allocate some memory
        L2.newuserdata(100);
        L2.newuserdata(200);
        
        REQUIRE(allocated > 0);
        
        // Force cleanup
        L2.settop(0);
        L2.gc(2, 0);  // LUA_GCCOLLECT
        
        // Some memory should be deallocated
        REQUIRE(deallocated > 0);
    }
    
    SECTION("Memory limits and errors") {
        // Test behavior when memory allocation fails
        // This requires careful setup to simulate OOM conditions
        REQUIRE(true);  // Placeholder
    }
}

TEST_CASE("LuaState registry and references", "[LuaState][registry]") {
    lua::LuaState L;
    
    SECTION("Registry access") {
        // Push value to registry
        L.pushstring("registry_value");
        L.pushstring("registry_key");
        L.pushvalue(-2);
        L.settable(lua::LUA_REGISTRYINDEX);
        
        // Retrieve value from registry
        L.pushstring("registry_key");
        L.gettable(lua::LUA_REGISTRYINDEX);
        
        REQUIRE(L.type(-1) == lua::LuaType::STRING);
        REQUIRE(std::string(L.tostring(-1)) == "registry_value");
    }
    
    SECTION("Global table access") {
        // Access global table through registry
        L.getfield(lua::LUA_REGISTRYINDEX, "_G");
        REQUIRE(L.type(-1) == lua::LuaType::TABLE);
        
        // Set global variable
        L.pushstring("global_value");
        L.setfield(-2, "global_key");
        
        // Get global variable
        L.getfield(-1, "global_key");
        REQUIRE(std::string(L.tostring(-1)) == "global_value");
    }
    
    SECTION("Reference management") {
        // This would test luaL_ref/luaL_unref functionality
        // for managing references to Lua objects from C
        REQUIRE(true);  // Placeholder
    }
}

// =============================================================================
// PERFORMANCE CONTRACT TESTS
// =============================================================================

TEST_CASE("LuaState performance contracts", "[LuaState][performance]") {
    SECTION("Stack operations performance") {
        lua::LuaState L;
        
        BENCHMARK("Stack push/pop operations") {
            for (int i = 0; i < 1000; ++i) {
                L.pushnumber(static_cast<double>(i));
            }
            L.settop(0);
        };
        
        BENCHMARK("Stack access by index") {
            // Prepare stack
            for (int i = 0; i < 1000; ++i) {
                L.pushnumber(static_cast<double>(i));
            }
            
            return [&] {
                for (int i = 1; i <= 1000; ++i) {
                    volatile double val = L.tonumber(i);
                    (void)val;
                }
            };
        };
    }
    
    SECTION("Function call performance") {
        lua::LuaState L;
        
        auto simple_func = [](lua::LuaState* L) -> int {
            double a = L->tonumber(1);
            double b = L->tonumber(2);
            L->pushnumber(a + b);
            return 1;
        };
        
        L.pushcclosure(simple_func, 0);
        
        BENCHMARK("C function calls") {
            L.pushvalue(1);  // function
            L.pushnumber(1.0);
            L.pushnumber(2.0);
            L.call(2, 1);
            L.pop(1);  // remove result
        };
    }
    
    SECTION("Memory allocation performance") {
        lua::LuaState L;
        
        BENCHMARK("Userdata allocation") {
            void* ud = L.newuserdata(1024);
            (void)ud;
            L.pop(1);
        };
        
        BENCHMARK("String creation") {
            L.pushstring("performance test string");
            L.pop(1);
        };
    }
}

// =============================================================================
// COMPATIBILITY AND EDGE CASE TESTS
// =============================================================================

TEST_CASE("LuaState Lua 5.1.5 compatibility", "[LuaState][compatibility]") {
    lua::LuaState L;
    
    SECTION("Stack index behavior matches Lua 5.1") {
        // Test exact Lua 5.1 stack indexing behavior
        L.pushnumber(1.0);
        L.pushnumber(2.0);
        
        REQUIRE(L.tonumber(1) == 1.0);
        REQUIRE(L.tonumber(2) == 2.0);
        REQUIRE(L.tonumber(-1) == 2.0);
        REQUIRE(L.tonumber(-2) == 1.0);
        
        // Invalid indices return LUA_TNONE
        REQUIRE(L.type(0) == lua::LuaType::NONE);
        REQUIRE(L.type(3) == lua::LuaType::NONE);
    }
    
    SECTION("Error handling matches Lua 5.1") {
        // Test exact error propagation behavior
        auto error_func = [](lua::LuaState* L) -> int {
            L->pushstring("test error");
            return L->error();
        };
        
        L.pushcclosure(error_func, 0);
        lua::LuaStatus status = L.pcall(0, 0, 0);
        
        REQUIRE(status == lua::LuaStatus::ERRRUN);
        REQUIRE(L.type(-1) == lua::LuaType::STRING);
    }
    
    SECTION("Type coercion matches Lua 5.1") {
        // Test exact type coercion rules
        L.pushnumber(42.0);
        L.pushstring("42");
        L.pushstring("42.0");
        L.pushstring("hello");
        
        REQUIRE(L.equal(1, 2) == 1);  // number == numeric string
        REQUIRE(L.equal(1, 3) == 1);  // number == float string
        REQUIRE(L.equal(1, 4) == 0);  // number != non-numeric string
        
        REQUIRE(L.rawequal(1, 2) == 0);  // different types in raw comparison
    }
}

TEST_CASE("LuaState error conditions and recovery", "[LuaState][errors]") {
    lua::LuaState L;
    
    SECTION("Stack overflow protection") {
        // Test stack overflow detection and handling
        bool overflow_detected = false;
        
        try {
            for (int i = 0; i < 100000; ++i) {
                if (!L.checkstack(1)) {
                    overflow_detected = true;
                    break;
                }
                L.pushnumber(static_cast<double>(i));
            }
        } catch (...) {
            overflow_detected = true;
        }
        
        REQUIRE(overflow_detected);
    }
    
    SECTION("Invalid operations error handling") {
        // Test error handling for invalid operations
        L.pushnumber(42.0);
        
        // Invalid table access should handle gracefully
        REQUIRE_NOTHROW(L.getfield(1, "key"));  // number is not a table
        REQUIRE(L.type(-1) == lua::LuaType::NIL);
    }
    
    SECTION("Memory exhaustion handling") {
        // Test behavior under memory pressure
        // This requires careful setup to simulate OOM
        REQUIRE(true);  // Placeholder
    }
    
    SECTION("Recursive call detection") {
        // Test protection against infinite recursion
        auto recursive_func = [](lua::LuaState* L) -> int {
            L->pushvalue(1);  // push self
            return L->call(0, 0);  // recursive call
        };
        
        L.pushcclosure(recursive_func, 0);
        lua::LuaStatus status = L.pcall(0, 0, 0);
        
        // Should detect stack overflow or C stack overflow
        REQUIRE(status != lua::LuaStatus::OK);
    }
}

TEST_CASE("LuaState thread safety considerations", "[LuaState][threading]") {
    SECTION("Thread-local state isolation") {
        // Each Lua state should be isolated
        lua::LuaState L1;
        lua::LuaState L2;
        
        L1.pushnumber(42.0);
        L2.pushnumber(24.0);
        
        REQUIRE(L1.tonumber(1) == 42.0);
        REQUIRE(L2.tonumber(1) == 24.0);
        REQUIRE(L1.gettop() == 1);
        REQUIRE(L2.gettop() == 1);
    }
    
    SECTION("Global state sharing in threads") {
        lua::LuaState L;
        lua::LuaState* thread = L.newthread();
        
        // Should share global state but have separate stacks
        REQUIRE(thread->l_G == L.l_G);
        
        L.pushnumber(1.0);
        thread->pushnumber(2.0);
        
        REQUIRE(L.gettop() == 2);  // main + thread
        REQUIRE(thread->gettop() == 1);
    }
    
    SECTION("Concurrent access safety") {
        // Test that states are not thread-safe by default
        // This is a documentation test - Lua states require external synchronization
        REQUIRE(true);  // Placeholder - actual test would require threading
    }
}