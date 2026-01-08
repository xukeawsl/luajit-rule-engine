#include "ljre/rule_engine.h"

namespace ljre {

RuleEngine::RuleEngine() {
    // Lua状态会在构造函数中自动初始化
}

bool RuleEngine::load_rule_config(const char* config_file, std::string* error_msg) {
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    // 加载配置文件
    if (!_lua_state.load_file(config_file, error_msg)) {
        return false;
    }

    // 配置文件应该返回一个table，现在在栈顶
    lua_State* L = _lua_state.get();

    // 检查栈顶是否是table
    if (!lua_istable(L, -1)) {
        if (error_msg) {
            *error_msg = "Config file must return a table";
        }
        return false;
    }

    // 遍历table中的每个规则配置
    lua_pushnil(L); // 第一个key
    while (lua_next(L, -2) != 0) {
        // 现在栈上: -1 => value, -2 => key
        if (lua_istable(L, -1)) {
            // 获取规则名称
            lua_getfield(L, -1, "name");
            if (!lua_isstring(L, -1)) {
                lua_pop(L, 2); // 弹出value和key
                if (error_msg) {
                    *error_msg = "Rule name must be a string";
                }
                return false;
            }
            std::string rule_name = lua_tostring(L, -1);
            lua_pop(L, 1);

            // 获取规则文件路径
            lua_getfield(L, -1, "file");
            if (!lua_isstring(L, -1)) {
                lua_pop(L, 2); // 弹出value和key
                if (error_msg) {
                    *error_msg = "Rule file must be a string";
                }
                return false;
            }
            std::string file_path = lua_tostring(L, -1);
            lua_pop(L, 1);

            // 添加规则
            if (!add_rule(rule_name, file_path, error_msg)) {
                lua_pop(L, 2); // 弹出value和key
                return false;
            }
        }

        lua_pop(L, 1); // 弹出value，保留key用于下一次迭代
    }

    return true;
}

bool RuleEngine::add_rule(const std::string& rule_name, const std::string& file_path,
                          std::string* error_msg) {
    // 检查规则是否已存在
    if (_rules.find(rule_name) != _rules.end()) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' already exists";
        }
        return false;
    }

    // 加载规则文件
    if (!load_rule_file(file_path, error_msg)) {
        return false;
    }

    // 将 match 函数保存到规则表中
    lua_State* L = _lua_state.get();
    lua_getglobal(L, "match");  // 获取全局 match 函数
    if (!lua_isfunction(L, -1)) {
        if (error_msg) {
            *error_msg = "Rule file must define a 'match' function";
        }
        lua_pop(L, 1);
        return false;
    }

    // 创建规则函数表（如果不存在）
    lua_getglobal(L, "_rule_functions");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);  // 弹出非 table 值
        lua_createtable(L, 0, 0);  // 创建新 table
        lua_setglobal(L, "_rule_functions");
        lua_getglobal(L, "_rule_functions");  // 重新获取
    }

    // 将 match 函数存入表中
    lua_pushstring(L, rule_name.c_str());  // key
    lua_pushvalue(L, -3);  // value (match 函数)
    lua_rawset(L, -3);  // _rule_functions[rule_name] = match
    lua_pop(L, 2);  // 弹出 _rule_functions 表和 match 函数

    // 添加到规则列表
    Rule rule;
    rule.name = rule_name;
    rule.file_path = file_path;
    rule.loaded = true;
    _rules[rule_name] = rule;

    return true;
}

bool RuleEngine::remove_rule(const std::string& rule_name) {
    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        return false;
    }

    _rules.erase(it);
    return true;
}

bool RuleEngine::reload_rule(const std::string& rule_name, std::string* error_msg) {
    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' not found";
        }
        return false;
    }

    // 重新加载规则文件
    if (!load_rule_file(it->second.file_path, error_msg)) {
        it->second.loaded = false;
        return false;
    }

    it->second.loaded = true;
    return true;
}

bool RuleEngine::match_rule(const std::string& rule_name, const DataAdapter& data_adapter,
                            MatchResult* result, std::string* error_msg) {
    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' not found";
        }
        return false;
    }

    if (!it->second.loaded) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' is not loaded";
        }
        return false;
    }

    return call_match_function(rule_name, data_adapter, result, error_msg);
}

bool RuleEngine::match_all_rules(const DataAdapter& data_adapter,
                                 std::vector<MatchResult>* results,
                                 std::string* error_msg) {
    if (results) {
        results->clear();
        results->reserve(_rules.size());
    }

    bool all_matched = true;
    for (const auto& pair : _rules) {
        MatchResult result;
        if (!match_rule(pair.first, data_adapter, &result, error_msg)) {
            if (results) {
                results->push_back(result);
            }
            all_matched = false;
        } else if (results) {
            results->push_back(result);
        }

        if (!result.matched) {
            all_matched = false;
        }
    }

    return all_matched;
}

std::vector<RuleInfo> RuleEngine::get_all_rules() const {
    std::vector<RuleInfo> infos;
    infos.reserve(_rules.size());

    for (const auto& pair : _rules) {
        RuleInfo info;
        info.name = pair.second.name;
        info.file_path = pair.second.file_path;
        info.loaded = pair.second.loaded;
        infos.push_back(info);
    }

    return infos;
}

bool RuleEngine::has_rule(const std::string& rule_name) const {
    return _rules.find(rule_name) != _rules.end();
}

void RuleEngine::clear_rules() {
    _rules.clear();
}

bool RuleEngine::load_rule_file(const std::string& file_path, std::string* error_msg) {
    return _lua_state.load_file(file_path.c_str(), error_msg);
}

bool RuleEngine::call_match_function(const std::string& rule_name,
                                     const DataAdapter& data_adapter,
                                     MatchResult* result,
                                     std::string* error_msg) {
    lua_State* L = _lua_state.get();

    // 从规则函数表中获取对应规则的match函数
    lua_getglobal(L, "_rule_functions");
    if (!lua_istable(L, -1)) {
        if (error_msg) {
            *error_msg = "Rule function table not found";
        }
        lua_pop(L, 1);
        return false;
    }

    lua_pushstring(L, rule_name.c_str());
    lua_rawget(L, -2);  // 获取 _rule_functions[rule_name]
    lua_remove(L, -2);  // 移除 _rule_functions 表

    if (!lua_isfunction(L, -1)) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' match function not found";
        }
        lua_pop(L, 1);
        return false;
    }

    // 将数据压入栈顶
    if (!data_adapter.push_to_lua(L, error_msg)) {
        lua_pop(L, 1); // 弹出函数
        return false;
    }

    // 调用match函数，1个参数，2个返回值
    if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
        if (error_msg) {
            *error_msg = _lua_state.get_error_string();
        }
        return false;
    }

    // 获取第一个返回值：是否匹配成功
    if (!lua_isboolean(L, -2)) {
        if (error_msg) {
            *error_msg = "First return value of 'match' must be boolean";
        }
        lua_pop(L, 2);
        return false;
    }
    bool matched = lua_toboolean(L, -2);

    // 获取第二个返回值：错误信息（可选）
    std::string message;
    if (lua_isstring(L, -1)) {
        message = lua_tostring(L, -1);
    }
    lua_pop(L, 2); // 弹出两个返回值

    if (result) {
        result->matched = matched;
        result->message = message;
    }

    return true;
}

} // namespace ljre
