#pragma once

#include "ljre/data_adapter.h"
#include <nlohmann/json.hpp>
#include <string>

namespace ljre {

// nlohmann::json 适配器
class JsonAdapter : public DataAdapter {
public:
    explicit JsonAdapter(const nlohmann::json& data) : data_(data) {}

    bool push_to_lua(lua_State* L, std::string* error_msg) const override;
    const char* get_type_name() const override { return "nlohmann::json"; }

private:
    const nlohmann::json& data_;

    // 递归将json值转换为Lua值并压栈
    bool push_json_value(lua_State* L, const nlohmann::json& j, std::string* error_msg) const;
};

} // namespace ljre
