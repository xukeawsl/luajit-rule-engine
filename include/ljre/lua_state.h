#pragma once

#include <string>

#include "lua.hpp"


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

    // JIT 控制方法
    // 启用 JIT 编译（启用所有功能的 JIT）
    bool enable_jit();
    // 禁用 JIT 编译（切换到解释模式）
    bool disable_jit();
    // 刷新 JIT 编译器缓存（清除已编译的代码）
    bool flush_jit();
    // 检查 JIT 是否启用
    bool is_jit_enabled() const;

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

// 栈守卫类 - RAII 方式管理 Lua 栈平衡
// 在构造时记录栈位置，析构时自动恢复到该位置
class LuaStackGuard {
public:
    explicit LuaStackGuard(lua_State* L) : _L(L), _top(lua_gettop(_L)) {}
    ~LuaStackGuard() {
        if (_L) {
            lua_settop(_L, _top);
        }
    }

    // 禁止拷贝和移动
    LuaStackGuard(const LuaStackGuard&) = delete;
    LuaStackGuard& operator=(const LuaStackGuard&) = delete;
    LuaStackGuard(LuaStackGuard&&) = delete;
    LuaStackGuard& operator=(LuaStackGuard&&) = delete;

    // 获取记录的栈位置
    int get_top() const noexcept { return _top; }

    // 手动释放守卫（不再自动恢复栈）
    void release() noexcept { _L = nullptr; }

private:
    lua_State* _L;
    int _top;
};

} // namespace ljre
