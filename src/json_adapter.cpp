#include "ljre/json_adapter.h"


namespace ljre {

bool JsonAdapter::push_to_lua(lua_State* L, std::string* error_msg) const {
    if (!L) {
        if (error_msg) {
            *error_msg = "Lua state is null";
        }
        return false;
    }

    // 转换 JSON 数据（不执行命令，命令由引擎侧调用 execute_commands）
    return push_json_value(L, _data, error_msg, 0);
}

bool JsonAdapter::push_json_value(lua_State* L, const nlohmann::json& j,
                                   std::string* error_msg, size_t current_depth) const {
    switch (j.type()) {
        case nlohmann::json::value_t::null:
            lua_pushnil(L);
            break;

        case nlohmann::json::value_t::boolean:
            lua_pushboolean(L, j.get<bool>());
            break;

        case nlohmann::json::value_t::number_integer:
        case nlohmann::json::value_t::number_unsigned:
            lua_pushinteger(L, j.get<lua_Integer>());
            break;

        case nlohmann::json::value_t::number_float:
            lua_pushnumber(L, j.get<lua_Number>());
            break;

        case nlohmann::json::value_t::string: {
            const std::string& str = j.get_ref<const std::string&>();
            lua_pushlstring(L, str.data(), str.size());
            break;
        }

        case nlohmann::json::value_t::array: {
            lua_createtable(L, j.size(), 0);

            // 检查下一层是否会超过最大深度
            if (current_depth + 1 >= _max_nesting_depth) {
                // 子元素会被截断为 nil，但数组本身已创建
                // 数组元素全部设为 nil
                for (size_t i = 0; i < j.size(); ++i) {
                    lua_pushnil(L);
                    lua_rawseti(L, -2, static_cast<int>(i + 1));
                }
            } else {
                // Lua数组从1开始索引
                for (size_t i = 0; i < j.size(); ++i) {
                    if (!push_json_value(L, j[i], error_msg, current_depth + 1)) {
                        lua_pop(L, 1); // 弹出table
                        return false;
                    }
                    lua_rawseti(L, -2, static_cast<int>(i + 1));
                }
            }
            break;
        }

        case nlohmann::json::value_t::object: {
            lua_createtable(L, 0, static_cast<int>(j.size()));

            // 检查下一层是否会超过最大深度
            if (current_depth + 1 >= _max_nesting_depth) {
                // 子元素会被截断为 nil，但对象本身已创建
                // 所有字段值设为 nil
                for (auto it = j.begin(); it != j.end(); ++it) {
                    const auto& key = it.key();
                    lua_pushlstring(L, key.data(), key.size());
                    lua_pushnil(L);
                    lua_rawset(L, -3);
                }
            } else {
                // 正常处理
                for (auto it = j.begin(); it != j.end(); ++it) {
                    // 先压入key（使用 pushlstring 支持包含空字符的 key）
                    const auto& key = it.key();
                    lua_pushlstring(L, key.data(), key.size());

                    // 再压入value（深度 +1）
                    if (!push_json_value(L, it.value(), error_msg, current_depth + 1)) {
                        lua_pop(L, 2); // 弹出key和table
                        return false;
                    }

                    // 设置table[key] = value
                    lua_rawset(L, -3);
                }
            }
            break;
        }

        default:
            if (error_msg) {
                *error_msg = "Unsupported JSON value type: " + std::string(j.type_name());
            }
            return false;
    }

    return true;
}

} // namespace ljre
