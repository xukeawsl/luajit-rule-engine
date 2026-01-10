#pragma once

#include <string>

#include "lua.hpp"


namespace ljre {

// 数据适配器接口，用于将不同类型的数据转换为Lua table
// 用户可以根据自己的数据类型实现此接口
class DataAdapter {
public:
    virtual ~DataAdapter() = default;

    // 将数据转换为Lua table并压入栈顶
    // 成功返回true，失败返回false
    virtual bool push_to_lua(lua_State* L, std::string* error_msg = nullptr) const = 0;

    // 获取数据类型描述（用于错误提示）
    virtual const char* get_type_name() const = 0;
};

} // namespace ljre
