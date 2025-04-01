#pragma once

// Lua C++ 实现的配置选项

namespace Lua {

// 全局配置常量
constexpr int LUA_MINSTACK = 20;          // 初始栈大小
constexpr int LUA_MAXCALLS = 200;         // 调用嵌套最大层数
constexpr int LUA_MAXCSTACK = 2000;       // C函数调用最大层数
constexpr int LUA_REGISTRYINDEX = -10000; // 注册表索引
constexpr int LUA_RIDX_GLOBALS = 2;       // 全局环境在注册表中的索引
constexpr int LUA_RIDX_MAINTHREAD = 1;    // 主线程在注册表中的索引

// 垃圾回收配置
constexpr int LUA_GC_PAUSE = 200;         // GC暂停百分比
constexpr int LUA_GC_STEPMUL = 200;       // GC步进乘数

} // namespace Lua
