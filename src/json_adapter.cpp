#include "ljre/json_adapter.h"
#include <stdexcept>

namespace ljre {

bool JsonAdapter::push_to_lua(lua_State* L, std::string* error_msg) const {
    if (!L) {
        if (error_msg) {
            *error_msg = "Lua state is null";
        }
        return false;
    }

    return push_json_value(L, data_, error_msg);
}

bool JsonAdapter::push_json_value(lua_State* L, const nlohmann::json& j,
                                   std::string* error_msg) const {
    try {
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

            case nlohmann::json::value_t::string:
                lua_pushstring(L, j.get<std::string>().c_str());
                break;

            case nlohmann::json::value_t::array: {
                lua_createtable(L, j.size(), 0);

                // Lua数组从1开始索引
                for (size_t i = 0; i < j.size(); ++i) {
                    if (!push_json_value(L, j[i], error_msg)) {
                        lua_pop(L, 1); // 弹出table
                        return false;
                    }
                    lua_rawseti(L, -2, static_cast<int>(i + 1));
                }
                break;
            }

            case nlohmann::json::value_t::object: {
                lua_createtable(L, 0, static_cast<int>(j.size()));

                for (auto it = j.begin(); it != j.end(); ++it) {
                    // 先压入key
                    lua_pushstring(L, it.key().c_str());

                    // 再压入value
                    if (!push_json_value(L, it.value(), error_msg)) {
                        lua_pop(L, 2); // 弹出key和table
                        return false;
                    }

                    // 设置table[key] = value
                    lua_rawset(L, -3);
                }
                break;
            }

            case nlohmann::json::value_t::discarded:
            default:
                if (error_msg) {
                    *error_msg = "Unsupported JSON type";
                }
                return false;
        }

        return true;

    } catch (const std::exception& e) {
        if (error_msg) {
            *error_msg = std::string("JSON conversion error: ") + e.what();
        }
        return false;
    }
}

} // namespace ljre
