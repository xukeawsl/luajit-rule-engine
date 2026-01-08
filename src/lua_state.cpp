#include "ljre/lua_state.h"

namespace ljre {

LuaState::LuaState() : _L(luaL_newstate()) {
    if (_L) {
        // 启用JIT编译
        luaJIT_setmode(_L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON);
        // 打开标准库
        luaL_openlibs(_L);
    }
}

LuaState::~LuaState() {
    if (_L) {
        lua_close(_L);
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
            *error_msg = "Lua state is null";
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
            *error_msg = "Lua state is null";
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
        return "Lua state is null";
    }

    if (lua_isstring(_L, -1)) {
        std::string error = lua_tostring(_L, -1);
        lua_pop(_L, 1);
        return error;
    }
    return "Unknown error";
}

} // namespace ljre
