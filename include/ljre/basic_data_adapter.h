#pragma once

#include "ljre/data_adapter.h"
#include <unordered_map>
#include <string>
#include <variant>

namespace ljre {

/**
 * @brief 基础数据适配器，支持动态设置字段
 *
 * 特性：
 * - 继承自 DataAdapter，自动获得唯一 ID
 * - 支持设置字符串、数字、布尔值、null
 * - 支持删除字段
 * - 命令队列机制，延迟执行
 *
 * 使用示例：
 * @code
 * auto adapter = std::make_shared<BasicDataAdapter>();
 * adapter->set("user_id", "12345");
 * adapter->set("age", 30);
 * adapter->set("score", 95.5);
 * adapter->set("active", true);
 * adapter->set_null("optional_field");
 *
 * engine->match_rule("rule1", adapter, result);
 * @endcode
 */
class BasicDataAdapter : public DataAdapter {
public:
    // 支持的值类型
    using Value = std::variant<
        std::string,   // 字符串
        double,        // 数字（浮点数）
        bool,          // 布尔值
        std::monostate // null（空值）
    >;

    /**
     * @brief 构造函数
     *
     * 自动从基类 DataAdapter 获取唯一 ID
     */
    BasicDataAdapter() = default;

    /**
     * @brief 析构函数
     */
    ~BasicDataAdapter() = default;

    // === 字段修改 API ===

    /**
     * @brief 设置字符串字段
     */
    void set(const std::string& key, const std::string& value);

    /**
     * @brief 设置字符串字段（const char* 版本）
     */
    void set(const std::string& key, const char* value);

    /**
     * @brief 设置浮点数字段
     */
    void set(const std::string& key, double value);

    /**
     * @brief 设置整数字段
     */
    void set(const std::string& key, int64_t value);

    /**
     * @brief 设置整数字段（int 版本）
     */
    void set(const std::string& key, int value);

    /**
     * @brief 设置布尔字段
     */
    void set(const std::string& key, bool value);

    /**
     * @brief 设置字段为 null
     */
    void set_null(const std::string& key);

    /**
     * @brief 删除指定字段
     */
    void remove(const std::string& key);

    /**
     * @brief 清空所有字段
     */
    void clear_fields();

    // === DataAdapter 接口实现 ===

    bool push_to_lua(lua_State* L, std::string* error_msg = nullptr) const override;

    const char* get_type_name() const override {
        return "BasicDataAdapter";
    }

    bool execute_commands(lua_State* L, std::string* error_msg = nullptr) const override;

private:
    // 字段映射表（同一 key 多次设置会自动覆盖）
    mutable std::unordered_map<std::string, Value> _fields;
};

} // namespace ljre
