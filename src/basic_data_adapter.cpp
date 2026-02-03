#include "ljre/basic_data_adapter.h"

namespace ljre {

// === 字段修改 API 实现 ===

void BasicDataAdapter::set(const std::string& key, const std::string& value) {
    _fields[key] = value;
}

void BasicDataAdapter::set(const std::string& key, const char* value) {
    _fields[key] = std::string(value);
}

void BasicDataAdapter::set(const std::string& key, double value) {
    _fields[key] = value;
}

void BasicDataAdapter::set(const std::string& key, int64_t value) {
    _fields[key] = static_cast<double>(value);
}

void BasicDataAdapter::set(const std::string& key, int value) {
    _fields[key] = static_cast<double>(value);
}

void BasicDataAdapter::set(const std::string& key, bool value) {
    _fields[key] = value;
}

void BasicDataAdapter::set_null(const std::string& key) {
    _fields[key] = std::monostate{};
}

void BasicDataAdapter::remove(const std::string& key) {
    _fields.erase(key);
}

void BasicDataAdapter::clear_fields() {
    _fields.clear();
}

// === DataAdapter 接口实现 ===

bool BasicDataAdapter::push_to_lua(lua_State* L, std::string* error_msg) const {
    (void)error_msg;  // 未使用参数
    lua_createtable(L, 0, 0);  // 创建空 table
    return true;
}

bool BasicDataAdapter::execute_commands(lua_State* L, std::string* error_msg) const {
    // 栈顶应该是 table
    if (!lua_istable(L, -1)) {
        if (error_msg) {
            *error_msg = "Stack top is not a table";
        }
        return false;
    }

    // Lambda: 将 Value 压入栈
    auto push_value = [&](const Value& v) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;

            if constexpr (std::is_same_v<T, std::string>) {
                lua_pushlstring(L, arg.data(), arg.size());
            } else if constexpr (std::is_same_v<T, double>) {
                lua_pushnumber(L, arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                lua_pushboolean(L, arg);
            } else if constexpr (std::is_same_v<T, std::monostate>) {
                lua_pushnil(L);
            }
        }, v);
    };

    // 遍历所有字段并设置
    for (const auto& [key, value] : _fields) {
        if (key.empty()) {
            if (error_msg) {
                *error_msg = "Cannot use empty key";
            }
            return false;
        }

        // 压入 key
        lua_pushlstring(L, key.data(), key.size());

        // 压入 value
        push_value(value);

        // 设置到 table
        lua_rawset(L, -3);  // table[key] = value
    }

    return true;
}

} // namespace ljre
