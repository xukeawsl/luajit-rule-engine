#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <vector>

#include "ljre/lua_state.h"
#include "ljre/data_adapter.h"


namespace ljre {

// 规则匹配结果
struct MatchResult {
    bool matched;           // 是否匹配成功
    std::string message;    // 错误信息或提示信息
};

// 规则信息
struct RuleInfo {
    std::string name;       // 规则名称
    std::string file_path;  // 规则文件路径
};

// 规则引擎类
class RuleEngine {
public:
    RuleEngine();
    ~RuleEngine() = default;

    // 禁止拷贝和移动（管理Lua状态）
    RuleEngine(const RuleEngine&) = delete;
    RuleEngine& operator=(const RuleEngine&) = delete;
    RuleEngine(RuleEngine&&) = delete;
    RuleEngine& operator=(RuleEngine&&) = delete;

    // 从配置文件加载规则列表
    // 配置文件格式为Lua table，例如:
    // return {
    //   { name = "rule1", file = "/path/to/rule1.lua" },
    //   { name = "rule2", file = "/path/to/rule2.lua" }
    // }
    bool load_rule_config(const char* config_file, std::string* error_msg = nullptr);

    // 添加单个规则
    bool add_rule(const std::string& rule_name, const std::string& file_path,
                  std::string* error_msg = nullptr);

    // 移除规则
    bool remove_rule(const std::string& rule_name);

    // 重新加载规则（热更新）
    bool reload_rule(const std::string& rule_name, std::string* error_msg = nullptr);

    // 检查指定规则是否匹配
    // data_adapter 用于将业务数据转换为Lua table
    bool match_rule(const std::string& rule_name, const DataAdapter& data_adapter,
                    MatchResult& result, std::string* error_msg = nullptr);

    // 检查所有规则是否匹配
    // results 存储 {规则名: 匹配结果} 的映射，按规则名字母顺序排序
    bool match_all_rules(const DataAdapter& data_adapter,
                         std::map<std::string, MatchResult>& results,
                         std::string* error_msg = nullptr);

    // 获取所有规则信息
    std::vector<RuleInfo> get_all_rules() const;

    // 检查规则是否存在
    bool has_rule(const std::string& rule_name) const;

    // 获取规则数量
    size_t get_rule_count() const { return _rules.size(); }

    // 清空所有规则
    void clear_rules();

protected:
    // 用于测试：允许派生类访问内部状态
    // 测试类可以继承 RuleEngine 并访问这些成员
    LuaState& get_lua_state() { return _lua_state; }

private:
    struct Rule {
        std::string name;
        std::string file_path;
    };

    LuaState _lua_state;
    std::unordered_map<std::string, Rule> _rules;

    // 内部方法：加载规则文件
    bool load_rule_file(const std::string& file_path, std::string* error_msg);

    // 内部方法：调用规则匹配函数
    bool call_match_function(const std::string& rule_name,
                             const DataAdapter& data_adapter,
                             MatchResult& result,
                             std::string* error_msg);
};

} // namespace ljre
