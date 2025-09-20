/**
 * @file lua-api.h
 * @brief Lua 5.1.5 完全兼容的 C API 接口契约
 * @date 2025-09-20
 */

#ifndef LUA_API_H
#define LUA_API_H

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/* 基础类型定义 */
/* ========================================================================== */

typedef struct lua_State lua_State;
typedef int (*lua_CFunction) (lua_State *L);
typedef const char * (*lua_Reader) (lua_State *L, void *ud, size_t *sz);
typedef int (*lua_Writer) (lua_State *L, const void* p, size_t sz, void* ud);
typedef void * (*lua_Alloc) (void *ud, void *ptr, size_t osize, size_t nsize);

/* Lua基础类型 */
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

/* 最小Lua栈大小 */
#define LUA_MINSTACK        20

/* ========================================================================== */
/* 状态管理 API */
/* ========================================================================== */

/**
 * @brief 创建新的Lua状态机
 * @return 新的lua_State指针，失败返回NULL
 */
lua_State *luaL_newstate(void);

/**
 * @brief 创建新的Lua状态机（带自定义分配器）
 * @param f 内存分配函数
 * @param ud 用户数据
 * @return 新的lua_State指针，失败返回NULL
 */
lua_State *lua_newstate(lua_Alloc f, void *ud);

/**
 * @brief 关闭Lua状态机并释放所有资源
 * @param L Lua状态机
 */
void lua_close(lua_State *L);

/**
 * @brief 创建新线程
 * @param L Lua状态机
 * @return 新线程的lua_State指针
 */
lua_State *lua_newthread(lua_State *L);

/* ========================================================================== */
/* 栈操作 API */
/* ========================================================================== */

/**
 * @brief 获取栈顶索引
 * @param L Lua状态机
 * @return 栈顶元素的索引
 */
int lua_gettop(lua_State *L);

/**
 * @brief 设置栈顶位置
 * @param L Lua状态机
 * @param idx 新的栈顶索引
 */
void lua_settop(lua_State *L, int idx);

/**
 * @brief 将指定索引的值复制到栈顶
 * @param L Lua状态机
 * @param idx 要复制的值的索引
 */
void lua_pushvalue(lua_State *L, int idx);

/**
 * @brief 移除指定索引的值
 * @param L Lua状态机
 * @param idx 要移除的值的索引
 */
void lua_remove(lua_State *L, int idx);

/**
 * @brief 将栈顶值插入到指定位置
 * @param L Lua状态机
 * @param idx 插入位置
 */
void lua_insert(lua_State *L, int idx);

/**
 * @brief 将栈顶值替换指定位置的值
 * @param L Lua状态机
 * @param idx 要替换的位置
 */
void lua_replace(lua_State *L, int idx);

/**
 * @brief 检查栈空间是否足够
 * @param L Lua状态机
 * @param sz 需要的栈空间大小
 * @return 1表示成功，0表示失败
 */
int lua_checkstack(lua_State *L, int sz);

/* ========================================================================== */
/* 类型检查和转换 API */
/* ========================================================================== */

/**
 * @brief 获取指定索引值的类型
 * @param L Lua状态机
 * @param idx 值的索引
 * @return 类型常量（LUA_T*）
 */
int lua_type(lua_State *L, int idx);

/**
 * @brief 获取类型名称
 * @param L Lua状态机
 * @param tp 类型常量
 * @return 类型名称字符串
 */
const char *lua_typename(lua_State *L, int tp);

/**
 * @brief 比较两个值是否相等
 * @param L Lua状态机
 * @param idx1 第一个值的索引
 * @param idx2 第二个值的索引
 * @return 1表示相等，0表示不等
 */
int lua_equal(lua_State *L, int idx1, int idx2);

/**
 * @brief 比较两个值的大小关系
 * @param L Lua状态机
 * @param idx1 第一个值的索引
 * @param idx2 第二个值的索引
 * @return 1表示idx1 < idx2，0表示其他情况
 */
int lua_lessthan(lua_State *L, int idx1, int idx2);

/* ========================================================================== */
/* 值获取 API */
/* ========================================================================== */

/**
 * @brief 将值转换为数字
 * @param L Lua状态机
 * @param idx 值的索引
 * @return 数字值
 */
lua_Number lua_tonumber(lua_State *L, int idx);

/**
 * @brief 将值转换为整数
 * @param L Lua状态机
 * @param idx 值的索引
 * @return 整数值
 */
lua_Integer lua_tointeger(lua_State *L, int idx);

/**
 * @brief 将值转换为布尔值
 * @param L Lua状态机
 * @param idx 值的索引
 * @return 布尔值
 */
int lua_toboolean(lua_State *L, int idx);

/**
 * @brief 将值转换为字符串
 * @param L Lua状态机
 * @param idx 值的索引
 * @param len 输出字符串长度（可为NULL）
 * @return 字符串指针
 */
const char *lua_tolstring(lua_State *L, int idx, size_t *len);

/**
 * @brief 获取对象长度
 * @param L Lua状态机
 * @param idx 对象索引
 * @return 对象长度
 */
size_t lua_objlen(lua_State *L, int idx);

/**
 * @brief 获取C函数指针
 * @param L Lua状态机
 * @param idx 函数索引
 * @return C函数指针
 */
lua_CFunction lua_tocfunction(lua_State *L, int idx);

/* ========================================================================== */
/* 值压栈 API */
/* ========================================================================== */

/**
 * @brief 压入nil值
 * @param L Lua状态机
 */
void lua_pushnil(lua_State *L);

/**
 * @brief 压入数字
 * @param L Lua状态机
 * @param n 数字值
 */
void lua_pushnumber(lua_State *L, lua_Number n);

/**
 * @brief 压入整数
 * @param L Lua状态机
 * @param n 整数值
 */
void lua_pushinteger(lua_State *L, lua_Integer n);

/**
 * @brief 压入字符串
 * @param L Lua状态机
 * @param s 字符串指针
 * @param l 字符串长度
 */
void lua_pushlstring(lua_State *L, const char *s, size_t l);

/**
 * @brief 压入C字符串
 * @param L Lua状态机
 * @param s C字符串指针
 */
void lua_pushstring(lua_State *L, const char *s);

/**
 * @brief 压入格式化字符串
 * @param L Lua状态机
 * @param fmt 格式字符串
 * @param ... 参数
 * @return 生成的字符串指针
 */
const char *lua_pushfstring(lua_State *L, const char *fmt, ...);

/**
 * @brief 压入C函数
 * @param L Lua状态机
 * @param f C函数指针
 */
void lua_pushcfunction(lua_State *L, lua_CFunction f);

/**
 * @brief 压入布尔值
 * @param L Lua状态机
 * @param b 布尔值
 */
void lua_pushboolean(lua_State *L, int b);

/* ========================================================================== */
/* 表操作 API */
/* ========================================================================== */

/**
 * @brief 创建新表
 * @param L Lua状态机
 * @param narr 数组部分预分配大小
 * @param nrec 哈希部分预分配大小
 */
void lua_createtable(lua_State *L, int narr, int nrec);

/**
 * @brief 获取表字段
 * @param L Lua状态机
 * @param idx 表的索引
 */
void lua_gettable(lua_State *L, int idx);

/**
 * @brief 获取表字段（通过字符串键）
 * @param L Lua状态机
 * @param idx 表的索引
 * @param k 字符串键
 */
void lua_getfield(lua_State *L, int idx, const char *k);

/**
 * @brief 设置表字段
 * @param L Lua状态机
 * @param idx 表的索引
 */
void lua_settable(lua_State *L, int idx);

/**
 * @brief 设置表字段（通过字符串键）
 * @param L Lua状态机
 * @param idx 表的索引
 * @param k 字符串键
 */
void lua_setfield(lua_State *L, int idx, const char *k);

/**
 * @brief 获取元表
 * @param L Lua状态机
 * @param objindex 对象索引
 * @return 1表示有元表，0表示没有
 */
int lua_getmetatable(lua_State *L, int objindex);

/**
 * @brief 设置元表
 * @param L Lua状态机
 * @param objindex 对象索引
 * @return 1表示成功，0表示失败
 */
int lua_setmetatable(lua_State *L, int objindex);

/* ========================================================================== */
/* 函数调用 API */
/* ========================================================================== */

/**
 * @brief 调用函数
 * @param L Lua状态机
 * @param nargs 参数个数
 * @param nresults 期望的返回值个数
 */
void lua_call(lua_State *L, int nargs, int nresults);

/**
 * @brief 安全调用函数
 * @param L Lua状态机
 * @param nargs 参数个数
 * @param nresults 期望的返回值个数
 * @param errfunc 错误处理函数索引
 * @return 0表示成功，其他表示错误码
 */
int lua_pcall(lua_State *L, int nargs, int nresults, int errfunc);

/* ========================================================================== */
/* 垃圾回收 API */
/* ========================================================================== */

#define LUA_GCSTOP       0
#define LUA_GCRESTART    1
#define LUA_GCCOLLECT    2
#define LUA_GCCOUNT      3
#define LUA_GCCOUNTB     4
#define LUA_GCSTEP       5
#define LUA_GCSETPAUSE   6
#define LUA_GCSETSTEPMUL 7

/**
 * @brief 垃圾回收控制
 * @param L Lua状态机
 * @param what 操作类型
 * @param data 操作参数
 * @return 操作结果
 */
int lua_gc(lua_State *L, int what, int data);

/* ========================================================================== */
/* 错误处理 API */
/* ========================================================================== */

/**
 * @brief 抛出错误
 * @param L Lua状态机
 * @return 不返回
 */
int lua_error(lua_State *L);

/* ========================================================================== */
/* 全局变量 API */
/* ========================================================================== */

/**
 * @brief 获取全局变量
 * @param L Lua状态机
 * @param name 变量名
 */
void lua_getglobal(lua_State *L, const char *name);

/**
 * @brief 设置全局变量
 * @param L Lua状态机
 * @param name 变量名
 */
void lua_setglobal(lua_State *L, const char *name);

/* ========================================================================== */
/* 辅助宏定义 */
/* ========================================================================== */

#define lua_pop(L,n)        lua_settop(L, -(n)-1)
#define lua_newtable(L)     lua_createtable(L, 0, 0)
#define lua_tostring(L,i)   lua_tolstring(L, (i), NULL)
#define lua_isfunction(L,n) (lua_type(L, (n)) == LUA_TFUNCTION)
#define lua_istable(L,n)    (lua_type(L, (n)) == LUA_TTABLE)
#define lua_islightuserdata(L,n) (lua_type(L, (n)) == LUA_TLIGHTUSERDATA)
#define lua_isnil(L,n)      (lua_type(L, (n)) == LUA_TNIL)
#define lua_isboolean(L,n)  (lua_type(L, (n)) == LUA_TBOOLEAN)
#define lua_isthread(L,n)   (lua_type(L, (n)) == LUA_TTHREAD)
#define lua_isnone(L,n)     (lua_type(L, (n)) == LUA_TNONE)
#define lua_isnoneornil(L, n) (lua_type(L, (n)) <= 0)

#ifdef __cplusplus
}
#endif

#endif /* LUA_API_H */