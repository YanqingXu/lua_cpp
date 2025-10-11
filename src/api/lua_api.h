#pragma once

#include "core/lua_common.h"
#include "core/lua_value.h"
#include "core/error.h"
#include <functional>
#include <memory>
#include <vector>

namespace lua_cpp {

/* ========================================================================== */
/* 前向声明 */
/* ========================================================================== */

class VirtualMachine;
class GarbageCollector;
class LuaStack;

/* ========================================================================== */
/* Lua 5.1.5 API常量 */
/* ========================================================================== */

// Lua类型常量
#define LUA_TNONE           (-1)
#define LUA_TNIL            0
#define LUA_TBOOLEAN        1
#define LUA_TLIGHTUSERDATA  2
#define LUA_TNUMBER         3
#define LUA_TSTRING         4
#define LUA_TTABLE          5
#define LUA_TFUNCTION       6
#define LUA_TUSERDATA       7
#define LUA_TTHREAD         8

// 函数调用返回值
#define LUA_OK              0
#define LUA_YIELD           1
#define LUA_ERRRUN          2
#define LUA_ERRSYNTAX       3
#define LUA_ERRMEM          4
#define LUA_ERRERR          5

// GC选项
#define LUA_GCSTOP          0
#define LUA_GCRESTART       1
#define LUA_GCCOLLECT       2
#define LUA_GCCOUNT         3
#define LUA_GCCOUNTB        4
#define LUA_GCSTEP          5
#define LUA_GCSETPAUSE      6
#define LUA_GCSETSTEPMUL    7

// 调试钩子掩码
#define LUA_MASKCALL        (1 << 0)
#define LUA_MASKRET         (1 << 1)
#define LUA_MASKLINE        (1 << 2)
#define LUA_MASKCOUNT       (1 << 3)

// 引用系统常量
#define LUA_NOREF           (-2)
#define LUA_REFNIL          (-1)

// 注册表索引
#define LUA_REGISTRYINDEX   (-10000)
#define LUA_ENVIRONINDEX    (-10001)
#define LUA_GLOBALSINDEX    (-10002)

// 最小栈大小
#define LUA_MINSTACK        20

/* ========================================================================== */
/* API错误类型 */
/* ========================================================================== */

/**
 * @brief Lua API错误
 */
class LuaAPIError : public LuaError {
public:
    explicit LuaAPIError(const std::string& message = "Lua API error")
        : LuaError(ErrorType::API, message) {}
};

/* ========================================================================== */
/* Lua状态结构 */
/* ========================================================================== */

/**
 * @brief Lua状态结构
 * 
 * 这是Lua C API的核心结构，包含了Lua虚拟机的完整状态
 * 兼容Lua 5.1.5的API接口
 */
struct lua_State {
    // 核心组件
    std::unique_ptr<VirtualMachine> vm;         // 虚拟机实例
    std::unique_ptr<GarbageCollector> gc;       // 垃圾收集器
    
    // 状态信息
    int status;                                 // 执行状态
    bool panic_function_set;                    // 是否设置了panic函数
    
    // 钩子函数
    lua_Hook hook;                              // 调试钩子
    int hook_mask;                              // 钩子掩码
    int hook_count;                             // 钩子计数
    
    // 错误处理
    int error_function_index;                   // 错误处理函数索引
    
    // 构造函数
    lua_State();
    ~lua_State();
    
    // 禁用拷贝和移动
    lua_State(const lua_State&) = delete;
    lua_State& operator=(const lua_State&) = delete;
    lua_State(lua_State&&) = delete;
    lua_State& operator=(lua_State&&) = delete;
};

/* ========================================================================== */
/* C函数类型 */
/* ========================================================================== */

/**
 * @brief C函数类型
 */
using lua_CFunction = int(*)(lua_State* L);

/**
 * @brief 内存分配函数类型
 */
using lua_Alloc = void*(*)(void* ud, void* ptr, size_t osize, size_t nsize);

/**
 * @brief panic函数类型
 */
using lua_PFunction = int(*)(lua_State* L);

/**
 * @brief 调试钩子函数类型
 */
using lua_Hook = void(*)(lua_State* L, lua_Debug* ar);

/**
 * @brief 数字类型（Lua中的number）
 */
using lua_Number = double;

/**
 * @brief 整数类型（Lua中的integer）
 */
using lua_Integer = long long;

/* ========================================================================== */
/* 调试信息结构 */
/* ========================================================================== */

/**
 * @brief 调试信息结构
 */
struct lua_Debug {
    int event;                          // 事件类型
    const char* name;                   // 函数名
    const char* namewhat;               // 名称类型
    const char* what;                   // 函数类型
    const char* source;                 // 源文件
    int currentline;                    // 当前行号
    int nups;                          // upvalue数量
    int linedefined;                   // 函数定义行号
    int lastlinedefined;               // 函数结束行号
    char short_src[60];                // 短源文件名
    
    // 内部使用
    void* i_ci;                        // 调用信息指针
};

/**
 * @brief 库函数注册结构
 */
struct luaL_Reg {
    const char* name;                   // 函数名
    lua_CFunction func;                 // 函数指针
};

/* ========================================================================== */
/* 状态操作函数 */
/* ========================================================================== */

/**
 * @brief 创建新的Lua状态
 * @param f 内存分配函数
 * @param ud 分配函数的用户数据
 * @return 新的Lua状态指针
 */
lua_State* lua_newstate(lua_Alloc f, void* ud);

/**
 * @brief 关闭Lua状态
 * @param L Lua状态指针
 */
void lua_close(lua_State* L);

/**
 * @brief 创建新线程
 * @param L Lua状态指针
 * @return 新线程指针
 */
lua_State* lua_newthread(lua_State* L);

/**
 * @brief 设置panic函数
 * @param L Lua状态指针
 * @param panicf panic函数
 * @return 之前的panic函数
 */
lua_PFunction lua_atpanic(lua_State* L, lua_PFunction panicf);

/* ========================================================================== */
/* 基础栈操作 */
/* ========================================================================== */

/**
 * @brief 获取栈顶索引
 * @param L Lua状态指针
 * @return 栈顶索引
 */
int lua_gettop(lua_State* L);

/**
 * @brief 设置栈顶索引
 * @param L Lua状态指针
 * @param idx 新的栈顶索引
 */
void lua_settop(lua_State* L, int idx);

/**
 * @brief 将指定索引的值推入栈顶
 * @param L Lua状态指针
 * @param idx 索引
 */
void lua_pushvalue(lua_State* L, int idx);

/**
 * @brief 移除指定索引的值
 * @param L Lua状态指针
 * @param idx 索引
 */
void lua_remove(lua_State* L, int idx);

/**
 * @brief 在指定位置插入栈顶值
 * @param L Lua状态指针
 * @param idx 插入位置
 */
void lua_insert(lua_State* L, int idx);

/**
 * @brief 用栈顶值替换指定位置的值
 * @param L Lua状态指针
 * @param idx 位置索引
 */
void lua_replace(lua_State* L, int idx);

/**
 * @brief 检查栈空间
 * @param L Lua状态指针
 * @param extra 需要的额外空间
 * @return 成功返回1，失败返回0
 */
int lua_checkstack(lua_State* L, int extra);

/**
 * @brief 在状态间移动值
 * @param from 源状态
 * @param to 目标状态
 * @param n 移动的值数量
 */
void lua_xmove(lua_State* from, lua_State* to, int n);

/* ========================================================================== */
/* 访问函数（栈->C） */
/* ========================================================================== */

/**
 * @brief 检查值是否为数字
 * @param L Lua状态指针
 * @param idx 索引
 * @return 是数字返回1，否则返回0
 */
int lua_isnumber(lua_State* L, int idx);

/**
 * @brief 检查值是否为字符串
 * @param L Lua状态指针
 * @param idx 索引
 * @return 是字符串返回1，否则返回0
 */
int lua_isstring(lua_State* L, int idx);

/**
 * @brief 检查值是否为C函数
 * @param L Lua状态指针
 * @param idx 索引
 * @return 是C函数返回1，否则返回0
 */
int lua_iscfunction(lua_State* L, int idx);

/**
 * @brief 检查值是否为用户数据
 * @param L Lua状态指针
 * @param idx 索引
 * @return 是用户数据返回1，否则返回0
 */
int lua_isuserdata(lua_State* L, int idx);

/**
 * @brief 获取值的类型
 * @param L Lua状态指针
 * @param idx 索引
 * @return 类型常量
 */
int lua_type(lua_State* L, int idx);

/**
 * @brief 获取类型名称
 * @param L Lua状态指针
 * @param tp 类型常量
 * @return 类型名称字符串
 */
const char* lua_typename(lua_State* L, int tp);

/**
 * @brief 比较两个值是否相等
 * @param L Lua状态指针
 * @param idx1 第一个值的索引
 * @param idx2 第二个值的索引
 * @return 相等返回1，否则返回0
 */
int lua_equal(lua_State* L, int idx1, int idx2);

/**
 * @brief 原始比较两个值是否相等
 * @param L Lua状态指针
 * @param idx1 第一个值的索引
 * @param idx2 第二个值的索引
 * @return 相等返回1，否则返回0
 */
int lua_rawequal(lua_State* L, int idx1, int idx2);

/**
 * @brief 比较两个值的大小
 * @param L Lua状态指针
 * @param idx1 第一个值的索引
 * @param idx2 第二个值的索引
 * @return idx1 < idx2 返回1，否则返回0
 */
int lua_lessthan(lua_State* L, int idx1, int idx2);

/**
 * @brief 转换为数字
 * @param L Lua状态指针
 * @param idx 索引
 * @return 数字值
 */
lua_Number lua_tonumber(lua_State* L, int idx);

/**
 * @brief 转换为整数
 * @param L Lua状态指针
 * @param idx 索引
 * @return 整数值
 */
lua_Integer lua_tointeger(lua_State* L, int idx);

/**
 * @brief 转换为布尔值
 * @param L Lua状态指针
 * @param idx 索引
 * @return 布尔值
 */
int lua_toboolean(lua_State* L, int idx);

/**
 * @brief 转换为字符串
 * @param L Lua状态指针
 * @param idx 索引
 * @param len 字符串长度（输出参数）
 * @return 字符串指针
 */
const char* lua_tolstring(lua_State* L, int idx, size_t* len);

/**
 * @brief 获取对象大小
 * @param L Lua状态指针
 * @param idx 索引
 * @return 对象大小
 */
size_t lua_objlen(lua_State* L, int idx);

/**
 * @brief 转换为C函数
 * @param L Lua状态指针
 * @param idx 索引
 * @return C函数指针
 */
lua_CFunction lua_tocfunction(lua_State* L, int idx);

/**
 * @brief 转换为用户数据
 * @param L Lua状态指针
 * @param idx 索引
 * @return 用户数据指针
 */
void* lua_touserdata(lua_State* L, int idx);

/**
 * @brief 转换为线程
 * @param L Lua状态指针
 * @param idx 索引
 * @return 线程指针
 */
lua_State* lua_tothread(lua_State* L, int idx);

/**
 * @brief 转换为指针
 * @param L Lua状态指针
 * @param idx 索引
 * @return 指针值
 */
const void* lua_topointer(lua_State* L, int idx);

/* ========================================================================== */
/* 推送函数（C->栈） */
/* ========================================================================== */

/**
 * @brief 推入nil值
 * @param L Lua状态指针
 */
void lua_pushnil(lua_State* L);

/**
 * @brief 推入数字值
 * @param L Lua状态指针
 * @param n 数字值
 */
void lua_pushnumber(lua_State* L, lua_Number n);

/**
 * @brief 推入整数值
 * @param L Lua状态指针
 * @param n 整数值
 */
void lua_pushinteger(lua_State* L, lua_Integer n);

/**
 * @brief 推入字符串
 * @param L Lua状态指针
 * @param s 字符串
 * @param len 字符串长度
 */
void lua_pushlstring(lua_State* L, const char* s, size_t len);

/**
 * @brief 推入C字符串
 * @param L Lua状态指针
 * @param s C字符串
 */
void lua_pushstring(lua_State* L, const char* s);

/**
 * @brief 推入格式化字符串
 * @param L Lua状态指针
 * @param fmt 格式字符串
 * @param ... 参数
 * @return 字符串指针
 */
const char* lua_pushfstring(lua_State* L, const char* fmt, ...);

/**
 * @brief 推入C函数
 * @param L Lua状态指针
 * @param f C函数指针
 */
void lua_pushcfunction(lua_State* L, lua_CFunction f);

/**
 * @brief 推入带upvalue的C函数
 * @param L Lua状态指针
 * @param f C函数指针
 * @param n upvalue数量
 */
void lua_pushcclosure(lua_State* L, lua_CFunction f, int n);

/**
 * @brief 推入布尔值
 * @param L Lua状态指针
 * @param b 布尔值
 */
void lua_pushboolean(lua_State* L, int b);

/**
 * @brief 推入轻量用户数据
 * @param L Lua状态指针
 * @param p 指针
 */
void lua_pushlightuserdata(lua_State* L, void* p);

/**
 * @brief 推入当前线程
 * @param L Lua状态指针
 * @return 成功返回1
 */
int lua_pushthread(lua_State* L);

/* ========================================================================== */
/* 获取函数（Lua->栈） */
/* ========================================================================== */

/**
 * @brief 获取表中的值
 * @param L Lua状态指针
 * @param idx 表的索引
 */
void lua_gettable(lua_State* L, int idx);

/**
 * @brief 获取表中指定字段的值
 * @param L Lua状态指针
 * @param idx 表的索引
 * @param k 字段名
 */
void lua_getfield(lua_State* L, int idx, const char* k);

/**
 * @brief 原始获取表中的值
 * @param L Lua状态指针
 * @param idx 表的索引
 */
void lua_rawget(lua_State* L, int idx);

/**
 * @brief 原始获取表中指定整数索引的值
 * @param L Lua状态指针
 * @param idx 表的索引
 * @param n 整数索引
 */
void lua_rawgeti(lua_State* L, int idx, int n);

/**
 * @brief 创建新表
 * @param L Lua状态指针
 * @param narr 数组部分大小
 * @param nrec 哈希部分大小
 */
void lua_createtable(lua_State* L, int narr, int nrec);

/**
 * @brief 创建新用户数据
 * @param L Lua状态指针
 * @param sz 用户数据大小
 * @return 用户数据指针
 */
void* lua_newuserdata(lua_State* L, size_t sz);

/**
 * @brief 获取元表
 * @param L Lua状态指针
 * @param objindex 对象索引
 * @return 有元表返回1，否则返回0
 */
int lua_getmetatable(lua_State* L, int objindex);

/**
 * @brief 获取环境表
 * @param L Lua状态指针
 * @param idx 索引
 */
void lua_getfenv(lua_State* L, int idx);

/* ========================================================================== */
/* 设置函数（栈->Lua） */
/* ========================================================================== */

/**
 * @brief 设置表中的值
 * @param L Lua状态指针
 * @param idx 表的索引
 */
void lua_settable(lua_State* L, int idx);

/**
 * @brief 设置表中指定字段的值
 * @param L Lua状态指针
 * @param idx 表的索引
 * @param k 字段名
 */
void lua_setfield(lua_State* L, int idx, const char* k);

/**
 * @brief 原始设置表中的值
 * @param L Lua状态指针
 * @param idx 表的索引
 */
void lua_rawset(lua_State* L, int idx);

/**
 * @brief 原始设置表中指定整数索引的值
 * @param L Lua状态指针
 * @param idx 表的索引
 * @param n 整数索引
 */
void lua_rawseti(lua_State* L, int idx, int n);

/**
 * @brief 设置元表
 * @param L Lua状态指针
 * @param objindex 对象索引
 * @return 成功返回1
 */
int lua_setmetatable(lua_State* L, int objindex);

/**
 * @brief 设置环境表
 * @param L Lua状态指针
 * @param idx 索引
 * @return 成功返回1
 */
int lua_setfenv(lua_State* L, int idx);

/* ========================================================================== */
/* 调用函数 */
/* ========================================================================== */

/**
 * @brief 调用函数
 * @param L Lua状态指针
 * @param nargs 参数数量
 * @param nresults 结果数量
 */
void lua_call(lua_State* L, int nargs, int nresults);

/**
 * @brief 保护调用函数
 * @param L Lua状态指针
 * @param nargs 参数数量
 * @param nresults 结果数量
 * @param errfunc 错误处理函数索引
 * @return 调用结果状态
 */
int lua_pcall(lua_State* L, int nargs, int nresults, int errfunc);

/**
 * @brief 保护调用C函数
 * @param L Lua状态指针
 * @param func C函数指针
 * @param ud 用户数据
 * @return 调用结果状态
 */
int lua_cpcall(lua_State* L, lua_CFunction func, void* ud);

/**
 * @brief 加载Lua代码
 * @param L Lua状态指针
 * @param reader 读取函数
 * @param dt 读取器数据
 * @param chunkname 代码块名称
 * @return 加载结果状态
 */
int lua_load(lua_State* L, lua_Reader reader, void* dt, const char* chunkname);

/**
 * @brief 转储函数
 * @param L Lua状态指针
 * @param writer 写入函数
 * @param data 写入器数据
 * @return 转储结果状态
 */
int lua_dump(lua_State* L, lua_Writer writer, void* data);

/* ========================================================================== */
/* 协程函数 */
/* ========================================================================== */

/**
 * @brief 恢复协程
 * @param L 协程状态指针
 * @param from 调用者状态指针
 * @param narg 参数数量
 * @return 恢复结果状态
 */
int lua_resume(lua_State* L, lua_State* from, int narg);

/**
 * @brief yield协程
 * @param L Lua状态指针
 * @param nresults 结果数量
 * @return yield结果
 */
int lua_yield(lua_State* L, int nresults);

/**
 * @brief 获取协程状态
 * @param L Lua状态指针
 * @return 状态值
 */
int lua_status(lua_State* L);

/* ========================================================================== */
/* 垃圾收集函数 */
/* ========================================================================== */

/**
 * @brief 垃圾收集控制
 * @param L Lua状态指针
 * @param what 操作类型
 * @param data 数据参数
 * @return 操作结果
 */
int lua_gc(lua_State* L, int what, int data);

/* ========================================================================== */
/* 杂项函数 */
/* ========================================================================== */

/**
 * @brief 抛出错误
 * @param L Lua状态指针
 * @return 不返回
 */
int lua_error(lua_State* L);

/**
 * @brief 遍历表
 * @param L Lua状态指针
 * @param idx 表的索引
 * @return 有下一个元素返回1，否则返回0
 */
int lua_next(lua_State* L, int idx);

/**
 * @brief 连接字符串
 * @param L Lua状态指针
 * @param n 字符串数量
 */
void lua_concat(lua_State* L, int n);

/**
 * @brief 获取分配器
 * @param L Lua状态指针
 * @param ud 用户数据（输出参数）
 * @return 分配器函数
 */
lua_Alloc lua_getallocf(lua_State* L, void** ud);

/**
 * @brief 设置分配器
 * @param L Lua状态指针
 * @param f 分配器函数
 * @param ud 用户数据
 */
void lua_setallocf(lua_State* L, lua_Alloc f, void* ud);

/* ========================================================================== */
/* 一些有用的宏 */
/* ========================================================================== */

#define lua_pop(L,n)        lua_settop(L, -(n)-1)
#define lua_newtable(L)     lua_createtable(L, 0, 0)
#define lua_register(L,n,f) (lua_pushcfunction(L, (f)), lua_setglobal(L, (n)))
#define lua_pushcfunction(L,f)  lua_pushcclosure(L, (f), 0)
#define lua_strlen(L,i)     lua_objlen(L, (i))
#define lua_isfunction(L,n) (lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L,n)    (lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L,n)    (lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)      (lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L,n)  (lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L,n)   (lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L,n)     (lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n)   (lua_type(L, (n)) <= 0)
#define lua_pushliteral(L, s)   lua_pushlstring(L, "" s, (sizeof(s)/sizeof(char))-1)
#define lua_setglobal(L,s)  lua_setfield(L, LUA_GLOBALSINDEX, (s))
#define lua_getglobal(L,s)  lua_getfield(L, LUA_GLOBALSINDEX, (s))
#define lua_tostring(L,i)   lua_tolstring(L, (i), NULL)

/* ========================================================================== */
/* 辅助库函数原型 */
/* ========================================================================== */

// 读取器和写入器类型
using lua_Reader = const char*(*)(lua_State* L, void* ud, size_t* sz);
using lua_Writer = int(*)(lua_State* L, const void* p, size_t sz, void* ud);

/* ========================================================================== */
/* 调试API */
/* ========================================================================== */

/**
 * @brief 获取调试信息
 * @param L Lua状态指针
 * @param what 信息类型
 * @param ar 调试结构
 * @return 成功返回1
 */
int lua_getinfo(lua_State* L, const char* what, lua_Debug* ar);

/**
 * @brief 获取局部变量
 * @param L Lua状态指针
 * @param ar 调试结构
 * @param n 变量索引
 * @return 变量名
 */
const char* lua_getlocal(lua_State* L, const lua_Debug* ar, int n);

/**
 * @brief 设置局部变量
 * @param L Lua状态指针
 * @param ar 调试结构
 * @param n 变量索引
 * @return 变量名
 */
const char* lua_setlocal(lua_State* L, const lua_Debug* ar, int n);

/**
 * @brief 获取upvalue
 * @param L Lua状态指针
 * @param funcindex 函数索引
 * @param n upvalue索引
 * @return upvalue名
 */
const char* lua_getupvalue(lua_State* L, int funcindex, int n);

/**
 * @brief 设置upvalue
 * @param L Lua状态指针
 * @param funcindex 函数索引
 * @param n upvalue索引
 * @return upvalue名
 */
const char* lua_setupvalue(lua_State* L, int funcindex, int n);

/**
 * @brief 设置调试钩子
 * @param L Lua状态指针
 * @param func 钩子函数
 * @param mask 掩码
 * @param count 计数
 * @return 之前的掩码
 */
int lua_sethook(lua_State* L, lua_Hook func, int mask, int count);

/**
 * @brief 获取调试钩子
 * @param L Lua状态指针
 * @return 钩子函数
 */
lua_Hook lua_gethook(lua_State* L);

/**
 * @brief 获取钩子掩码
 * @param L Lua状态指针
 * @return 掩码
 */
int lua_gethookmask(lua_State* L);

/**
 * @brief 获取钩子计数
 * @param L Lua状态指针
 * @return 计数
 */
int lua_gethookcount(lua_State* L);

/**
 * @brief 获取调用栈信息
 * @param L Lua状态指针
 * @param level 栈层级
 * @param ar 调试结构
 * @return 成功返回1
 */
int lua_getstack(lua_State* L, int level, lua_Debug* ar);

} // namespace lua_cpp

/* ========================================================================== */
/* 辅助库API声明 */
/* ========================================================================== */

extern "C" {

// 包含辅助库函数声明
#include "api/luaaux.h"

} // extern "C"