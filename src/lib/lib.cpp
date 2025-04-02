#include "lib.hpp"

namespace Lua {
namespace Lib {

void openLibs(State* state) {
    if (!state) return;
    
    // 打开基础库
    openBaseLib(state);
    
    // 打开数学库
    openMathLib(state);
    
    // 打开字符串库
    openStringLib(state);
    
    // TODO: 在未来添加更多标准库
    // 例如：表库、IO库、OS库、协程库、包管理库、调试库等
}

}} // namespace Lua::Lib
