#include "ljre/lua_state.h"


namespace ljre {

LuaState::LuaState() : _L(luaL_newstate()) {
    if (_L) {
        // 启用JIT编译（默认启用）
        luaJIT_setmode(_L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);

        // 只打开规则引擎必要的库
        // base: 基础函数 (print, assert, tonumber, tostring, pcall 等)
        // table: 表操作 (insert, remove, sort 等)
        // string: 字符串操作
        // math: 数学函数
        // 不打开: io (文件操作), os (操作系统操作), debug (调试),
        //         package (模块加载), bit, ffi, jit
        luaopen_base(_L);
        luaopen_table(_L);
        luaopen_string(_L);
        luaopen_math(_L);
    }
}

LuaState::~LuaState() {
    if (_L) {
        lua_close(_L);
        _L = nullptr;
    }
}

LuaState::LuaState(LuaState&& other) noexcept : _L(other._L) {
    other._L = nullptr;
}

LuaState& LuaState::operator=(LuaState&& other) noexcept {
    if (this != &other) {
        if (_L) {
            lua_close(_L);
        }
        _L = other._L;
        other._L = nullptr;
    }
    return *this;
}

bool LuaState::load_file(const char* filename, std::string* error_msg) {
    if (!_L) {
        if (error_msg) {
            *error_msg = "Failed to load file: Lua state is null";
        }
        return false;
    }

    if (luaL_dofile(_L, filename) != LUA_OK) {
        if (error_msg) {
            *error_msg = get_error_string();
        }
        return false;
    }
    return true;
}

bool LuaState::load_buffer(const char* buffer, size_t size, const char* name,
                           std::string* error_msg) {
    if (!_L) {
        if (error_msg) {
            *error_msg = "Failed to load buffer: Lua state is null";
        }
        return false;
    }

    if (luaL_loadbuffer(_L, buffer, size, name) != LUA_OK ||
        lua_pcall(_L, 0, 0, 0) != LUA_OK) {
        if (error_msg) {
            *error_msg = get_error_string();
        }
        return false;
    }
    return true;
}

std::string LuaState::get_error_string() {
    if (!_L) {
        return "Failed to get error string: Lua state is null";
    }

    if (lua_isstring(_L, -1)) {
        const char* error_msg = lua_tostring(_L, -1);
        std::string error;
        if (error_msg) {
            error = error_msg;
        } else {
            error = "Empty error message";
        }
        lua_pop(_L, 1);
        return error;
    }

    return "Failed to get error string: Unknown error";
}

bool LuaState::enable_jit() {
    if (!_L) {
        return false;
    }
    // 启用 JIT 编译引擎
    return luaJIT_setmode(_L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON) != 0;
}

bool LuaState::disable_jit() {
    if (!_L) {
        return false;
    }
    // 禁用 JIT 编译引擎，切换到解释模式
    return luaJIT_setmode(_L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF) != 0;
}

bool LuaState::flush_jit() {
    if (!_L) {
        return false;
    }
    // 刷新 JIT 编译器缓存，清除所有已编译的代码
    // 之后会重新开始编译热代码
    return luaJIT_setmode(_L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_FLUSH) != 0;
}

bool LuaState::is_jit_enabled() const {
    if (!_L) {
        return false;
    }

    // 使用栈守卫确保栈平衡
    LuaStackGuard guard(_L);

    // 通过检查 jit 模块的状态来判断 JIT 是否启用
    lua_getglobal(_L, "jit");
    if (!lua_istable(_L, -1)) {
        return false;
    }

    lua_getfield(_L, -1, "status");
    if (!lua_isfunction(_L, -1)) {
        return false;
    }

    // 调用 jit.status() 函数
    if (lua_pcall(_L, 0, 1, 0) != LUA_OK) {
        return false;
    }

    // jit.status() 返回 true 表示 JIT 已启用，false 或带参数表示禁用或部分禁用
    return lua_toboolean(_L, -1) != 0;
    // 栈守卫析构时自动恢复栈
}

} // namespace ljre
