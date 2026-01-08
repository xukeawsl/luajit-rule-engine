#pragma once

#include <lua.hpp>
#include <string>

namespace ljre {

// Lua状态管理类，使用RAII管理Lua状态生命周期
class LuaState {
public:
    LuaState();
    ~LuaState();

    // 禁止拷贝
    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    // 允许移动
    LuaState(LuaState&& other) noexcept;
    LuaState& operator=(LuaState&& other) noexcept;

    // 检查状态是否有效
    bool is_valid() const noexcept { return _L != nullptr; }
    lua_State* get() const noexcept { return _L; }

    // 加载并执行Lua文件
    // 成功返回true，失败返回false
    bool load_file(const char* filename, std::string* error_msg = nullptr);

    // 加载并执行Lua缓冲区
    bool load_buffer(const char* buffer, size_t size, const char* name,
                     std::string* error_msg = nullptr);

    // 从栈顶获取错误信息
    std::string get_error_string();

private:
    lua_State* _L;
};

} // namespace ljre
