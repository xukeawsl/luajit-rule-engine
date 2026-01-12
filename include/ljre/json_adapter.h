#pragma once

#include "ljre/data_adapter.h"

#include <string>

#include "nlohmann/json.hpp"


namespace ljre {

// nlohmann::json 适配器
class JsonAdapter : public DataAdapter {
public:
    // 默认最大嵌套深度，足够处理大多数场景，同时避免栈溢出
    static constexpr size_t MAX_NESTING_DEPTH = 8192;

    explicit JsonAdapter(const nlohmann::json& data,
                        size_t max_nesting_depth = MAX_NESTING_DEPTH)
        : _data(data)
        , _max_nesting_depth(max_nesting_depth)
    {}

    bool push_to_lua(lua_State* L, std::string* error_msg) const override;

    const char* get_type_name() const override { return "nlohmann::json"; }

    // 设置最大嵌套深度
    void set_max_nesting_depth(size_t depth) {
        _max_nesting_depth = std::max(1UL, std::min(depth, MAX_NESTING_DEPTH));
    }

    // 获取当前最大嵌套深度
    size_t get_max_nesting_depth() const {
        return _max_nesting_depth;
    }

private:
    // 递归将json值转换为Lua值并压栈
    // current_depth: 当前递归深度，用于检测是否超过最大嵌套深度
    bool push_json_value(lua_State* L, const nlohmann::json& j,
                        std::string* error_msg, size_t current_depth) const;

    const nlohmann::json& _data;
    size_t _max_nesting_depth;
};

} // namespace ljre
