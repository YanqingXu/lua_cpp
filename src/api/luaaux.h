#pragma once

#include "lua_api.h"
#include <string>
#include <vector>

extern "C" {

/* ========================================================================== */
/* Lua 5.1.5 辅助库常量 */
/* ========================================================================== */

#define LUA_ERRFILE     (LUA_ERRERR+1)

#define LUA_LOADED_TABLE    "_LOADED"
#define LUA_PRELOAD_TABLE   "_PRELOAD"

/* ========================================================================== */
/* 辅助库错误处理 */
/* ========================================================================== */

/**
 * @brief 抛出参数错误
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param extramsg 额外消息
 * @return 不返回
 */
int luaL_argerror(lua_State* L, int narg, const char* extramsg);

/**
 * @brief 抛出类型错误
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param tname 期望的类型名
 * @return 不返回
 */
int luaL_typerror(lua_State* L, int narg, const char* tname);

/**
 * @brief 参数错误，期望特定值
 * @param L Lua状态指针
 * @param cond 条件
 * @param narg 参数编号
 * @param msg 错误消息
 */
void luaL_argcheck(lua_State* L, int cond, int narg, const char* msg);

/**
 * @brief 检查字符串参数
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param l 字符串长度（输出）
 * @return 字符串指针
 */
const char* luaL_checklstring(lua_State* L, int narg, size_t* l);

/**
 * @brief 可选字符串参数
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param def 默认值
 * @param l 字符串长度（输出）
 * @return 字符串指针
 */
const char* luaL_optlstring(lua_State* L, int narg, const char* def, size_t* l);

/**
 * @brief 检查数字参数
 * @param L Lua状态指针
 * @param narg 参数编号
 * @return 数字值
 */
lua_Number luaL_checknumber(lua_State* L, int narg);

/**
 * @brief 可选数字参数
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param def 默认值
 * @return 数字值
 */
lua_Number luaL_optnumber(lua_State* L, int narg, lua_Number def);

/**
 * @brief 检查整数参数
 * @param L Lua状态指针
 * @param narg 参数编号
 * @return 整数值
 */
lua_Integer luaL_checkinteger(lua_State* L, int narg);

/**
 * @brief 可选整数参数
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param def 默认值
 * @return 整数值
 */
lua_Integer luaL_optinteger(lua_State* L, int narg, lua_Integer def);

/**
 * @brief 检查栈空间
 * @param L Lua状态指针
 * @param sz 需要的空间
 * @param msg 错误消息
 */
void luaL_checkstack(lua_State* L, int sz, const char* msg);

/**
 * @brief 检查类型
 * @param L Lua状态指针
 * @param narg 参数编号
 * @param t 期望的类型
 */
void luaL_checktype(lua_State* L, int narg, int t);

/**
 * @brief 检查任何值（非nil）
 * @param L Lua状态指针
 * @param narg 参数编号
 */
void luaL_checkany(lua_State* L, int narg);

/**
 * @brief 创建新的元表
 * @param L Lua状态指针
 * @param tname 类型名
 * @return 新创建返回1，已存在返回0
 */
int luaL_newmetatable(lua_State* L, const char* tname);

/**
 * @brief 检查用户数据
 * @param L Lua状态指针
 * @param ud 参数编号
 * @param tname 类型名
 * @return 用户数据指针
 */
void* luaL_checkudata(lua_State* L, int ud, const char* tname);

/**
 * @brief 抛出错误（格式化消息）
 * @param L Lua状态指针
 * @param fmt 格式字符串
 * @param ... 参数
 * @return 不返回
 */
int luaL_error(lua_State* L, const char* fmt, ...);

/**
 * @brief 在全局表中查找字段
 * @param L Lua状态指针
 * @param fname 字段名
 * @param szhint 大小提示
 * @return 字段类型
 */
int luaL_getmetafield(lua_State* L, int obj, const char* e);

/**
 * @brief 调用元方法
 * @param L Lua状态指针
 * @param obj 对象索引
 * @param e 事件名
 * @return 有元方法返回1，否则返回0
 */
int luaL_callmeta(lua_State* L, int obj, const char* e);

/* ========================================================================== */
/* 缓冲区操作 */
/* ========================================================================== */

/**
 * @brief Lua缓冲区结构
 */
typedef struct luaL_Buffer {
    char* p;            // 当前位置
    int lvl;            // 嵌套级别
    lua_State* L;       // Lua状态
    char buffer[LUAL_BUFFERSIZE];  // 缓冲区
} luaL_Buffer;

#define LUAL_BUFFERSIZE     BUFSIZ

/**
 * @brief 初始化缓冲区
 * @param L Lua状态指针
 * @param B 缓冲区指针
 */
void luaL_buffinit(lua_State* L, luaL_Buffer* B);

/**
 * @brief 准备缓冲区空间
 * @param B 缓冲区指针
 * @param sz 需要的空间大小
 * @return 缓冲区指针
 */
char* luaL_prepbuffer(luaL_Buffer* B);

/**
 * @brief 添加字符到缓冲区
 * @param B 缓冲区指针
 * @param c 字符
 */
void luaL_addchar(luaL_Buffer* B, char c);

/**
 * @brief 添加大小信息到缓冲区
 * @param B 缓冲区指针
 * @param sz 大小
 */
void luaL_addsize(luaL_Buffer* B, size_t sz);

/**
 * @brief 添加字符串到缓冲区
 * @param B 缓冲区指针
 * @param s 字符串
 */
void luaL_addstring(luaL_Buffer* B, const char* s);

/**
 * @brief 添加指定长度字符串到缓冲区
 * @param B 缓冲区指针
 * @param s 字符串
 * @param l 长度
 */
void luaL_addlstring(luaL_Buffer* B, const char* s, size_t l);

/**
 * @brief 添加栈顶值到缓冲区
 * @param B 缓冲区指针
 */
void luaL_addvalue(luaL_Buffer* B);

/**
 * @brief 完成缓冲区操作并推入结果
 * @param B 缓冲区指针
 */
void luaL_pushresult(luaL_Buffer* B);

/* ========================================================================== */
/* 文件操作 */
/* ========================================================================== */

/**
 * @brief 加载文件
 * @param L Lua状态指针
 * @param filename 文件名
 * @return 加载结果状态
 */
int luaL_loadfile(lua_State* L, const char* filename);

/**
 * @brief 加载缓冲区
 * @param L Lua状态指针
 * @param buff 缓冲区
 * @param sz 大小
 * @param name 名称
 * @return 加载结果状态
 */
int luaL_loadbuffer(lua_State* L, const char* buff, size_t sz, const char* name);

/**
 * @brief 加载字符串
 * @param L Lua状态指针
 * @param s 字符串
 * @return 加载结果状态
 */
int luaL_loadstring(lua_State* L, const char* s);

/**
 * @brief 执行文件
 * @param L Lua状态指针
 * @param filename 文件名
 * @return 执行结果状态
 */
int luaL_dofile(lua_State* L, const char* filename);

/**
 * @brief 执行字符串
 * @param L Lua状态指针
 * @param str 字符串
 * @return 执行结果状态
 */
int luaL_dostring(lua_State* L, const char* str);

/* ========================================================================== */
/* 库注册 */
/* ========================================================================== */

/**
 * @brief 注册库函数
 * @param L Lua状态指针
 * @param libname 库名
 * @param l 函数数组
 */
void luaL_register(lua_State* L, const char* libname, const luaL_Reg* l);

/**
 * @brief 创建新库
 * @param L Lua状态指针
 * @param l 函数数组
 */
void luaL_newlib(lua_State* L, const luaL_Reg* l);

/**
 * @brief 打开标准库
 * @param L Lua状态指针
 */
void luaL_openlibs(lua_State* L);

/* ========================================================================== */
/* 引用系统 */
/* ========================================================================== */

/**
 * @brief 创建引用
 * @param L Lua状态指针
 * @param t 表索引
 * @return 引用ID
 */
int luaL_ref(lua_State* L, int t);

/**
 * @brief 释放引用
 * @param L Lua状态指针
 * @param t 表索引
 * @param ref 引用ID
 */
void luaL_unref(lua_State* L, int t, int ref);

/* ========================================================================== */
/* 模块系统 */
/* ========================================================================== */

/**
 * @brief 获取模块
 * @param L Lua状态指针
 * @param modname 模块名
 * @param openf 打开函数
 */
void luaL_getmodule(lua_State* L, const char* modname, int openf);

/**
 * @brief 设置函数
 * @param L Lua状态指针
 * @param libname 库名
 * @param l 函数数组
 */
void luaL_setfuncs(lua_State* L, const luaL_Reg* l, int nup);

/**
 * @brief require函数
 * @param L Lua状态指针
 * @param modname 模块名
 */
void luaL_requiref(lua_State* L, const char* modname, lua_CFunction openf, int glb);

/* ========================================================================== */
/* 字符串操作 */
/* ========================================================================== */

/**
 * @brief 转换为字符串（通过__tostring元方法）
 * @param L Lua状态指针
 * @param idx 索引
 * @return 字符串指针
 */
const char* luaL_tolstring(lua_State* L, int idx, size_t* len);

/**
 * @brief 类型名称
 * @param L Lua状态指针
 * @param idx 索引
 * @return 类型名称
 */
const char* luaL_typename(lua_State* L, int idx);

/* ========================================================================== */
/* 兼容性支持 */
/* ========================================================================== */

/**
 * @brief 查找表项
 * @param L Lua状态指针
 * @param idx 表索引
 * @param fname 字段名路径（如"a.b.c"）
 * @return 字段类型
 */
int luaL_getsubtable(lua_State* L, int idx, const char* fname);

/**
 * @brief 追加栈轨迹
 * @param L Lua状态指针
 * @param L1 目标状态
 * @param msg 错误消息
 */
void luaL_traceback(lua_State* L, lua_State* L1, const char* msg, int level);

/**
 * @brief 长度操作（兼容性）
 * @param L Lua状态指针
 * @param idx 索引
 * @return 长度
 */
int luaL_len(lua_State* L, int idx);

/**
 * @brief 使用元方法进行设置
 * @param L Lua状态指针
 * @param idx 表索引
 * @param k 键名
 */
void luaL_setmetatable(lua_State* L, const char* tname);

/**
 * @brief 测试用户数据类型
 * @param L Lua状态指针
 * @param ud 参数编号
 * @param tname 类型名
 * @return 匹配返回用户数据指针，否则返回NULL
 */
void* luaL_testudata(lua_State* L, int ud, const char* tname);

/* ========================================================================== */
/* 有用的宏 */
/* ========================================================================== */

#define luaL_argcheck(L, cond, numarg, extramsg) \
    ((void)((cond) || luaL_argerror(L, (numarg), (extramsg))))

#define luaL_checkstring(L,n)   (luaL_checklstring(L, (n), NULL))
#define luaL_optstring(L,n,d)   (luaL_optlstring(L, (n), (d), NULL))
#define luaL_checkint(L,n)      ((int)luaL_checkinteger(L, (n)))
#define luaL_optint(L,n,d)      ((int)luaL_optinteger(L, (n), (d)))
#define luaL_checklong(L,n)     ((long)luaL_checkinteger(L, (n)))
#define luaL_optlong(L,n,d)     ((long)luaL_optinteger(L, (n), (d)))

#define luaL_typename(L,i)      lua_typename(L, lua_type(L,(i)))

#define luaL_dofile(L, fn) \
    (luaL_loadfile(L, fn) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_dostring(L, s) \
    (luaL_loadstring(L, s) || lua_pcall(L, 0, LUA_MULTRET, 0))

#define luaL_getmetatable(L,n)  (lua_getfield(L, LUA_REGISTRYINDEX, (n)))

#define luaL_opt(L,f,n,d)       (lua_isnoneornil(L,(n)) ? (d) : f(L,(n)))

/* ========================================================================== */
/* 初始化函数 */
/* ========================================================================== */

/**
 * @brief 创建标准Lua状态
 * @return 新的Lua状态指针
 */
lua_State* luaL_newstate(void);

/**
 * @brief 检查Lua版本
 * @param L Lua状态指针
 * @param ver 版本号
 */
void luaL_checkversion_(lua_State* L, lua_Number ver);

#define luaL_checkversion(L)    luaL_checkversion_(L, LUA_VERSION_NUM)

/* ========================================================================== */
/* 文件句柄支持 */
/* ========================================================================== */

/**
 * @brief 文件结果类型
 */
typedef struct luaL_Stream {
    FILE* f;        // 文件指针
    lua_CFunction closef;  // 关闭函数
} luaL_Stream;

} // extern "C"