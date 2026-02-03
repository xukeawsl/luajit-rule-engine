#pragma once

#include <string>
#include <atomic>
#include <cstdint>

#include "lua.hpp"


namespace ljre {

// 数据适配器接口，用于将不同类型的数据转换为Lua table
// 用户可以根据自己的数据类型实现此接口
class DataAdapter {
protected:
    /**
     * @brief 构造函数（自动分配唯一 ID）
     *
     * 每个适配器实例在构造时都会从静态原子变量中获取唯一 ID
     */
    DataAdapter() : _id(_next_id.fetch_add(1, std::memory_order_relaxed)) {}

public:
    virtual ~DataAdapter() = default;

    // 将数据转换为Lua table并压入栈顶
    // 成功返回true，失败返回false
    virtual bool push_to_lua(lua_State* L, std::string* error_msg = nullptr) const = 0;

    // 获取数据类型描述（用于错误提示）
    virtual const char* get_type_name() const = 0;

    // 获取适配器唯一标识ID（用于引擎缓存）
    uint64_t get_id() const { return _id; }

    // 执行待处理的命令（用于设置字段等操作）
    // 成功返回true，失败返回false
    virtual bool execute_commands(lua_State* /*L*/, std::string* /*error_msg*/) const {
        return true;
    }

protected:
    // 静态原子变量，用于生成唯一 ID
    static std::atomic<uint64_t> _next_id;

    // 实例唯一 ID
    uint64_t _id;
};

} // namespace ljre
