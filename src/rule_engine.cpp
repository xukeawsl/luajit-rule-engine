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
    LuaStackGuard guard(L);  // 自动管理栈平衡

    // 检查栈顶是否是table
    if (!lua_istable(L, -1)) {
        if (error_msg) {
            *error_msg = "Config file must return a table";
        }
        return false;
    }

    // 遍历table中的每个规则配置
    lua_pushnil(L); // 第一个key
    // lua_next 会从栈上弹出一个 key（键）， 然后把索引指定的表中 key-value（健值）对压入堆栈
    while (lua_next(L, -2) != 0) {
        // 现在栈上: -1 => value, -2 => key
        if (lua_istable(L, -1)) {
            // 获取规则名称
            lua_getfield(L, -1, "name");
            if (!lua_isstring(L, -1)) {
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
                if (error_msg) {
                    *error_msg = "Rule file must be a string";
                }
                return false;
            }
            std::string file_path = lua_tostring(L, -1);
            lua_pop(L, 1);

            // 添加规则
            if (!add_rule(rule_name, file_path, error_msg)) {
                return false;
            }
        }

        lua_pop(L, 1); // 弹出value，保留key用于下一次迭代
    }

    return true;
    // 栈守卫析构时自动清理栈
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
    LuaStackGuard guard(L);  // 自动管理栈平衡

    lua_getglobal(L, "match");  // 获取全局 match 函数
    if (!lua_isfunction(L, -1)) {
        if (error_msg) {
            *error_msg = "Rule file must define a 'match' function";
        }
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
    lua_pushlstring(L, rule_name.data(), rule_name.size());  // key
    lua_pushvalue(L, -3);  // value (match 函数)
    lua_rawset(L, -3);  // _rule_functions[rule_name] = match

    // 添加到规则列表
    Rule rule;
    rule.name = rule_name;
    rule.file_path = file_path;
    _rules[rule_name] = rule;

    return true;
    // 栈守卫析构时自动清理栈
}

bool RuleEngine::remove_rule(const std::string& rule_name) {
    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        return false;
    }

    // 从 Lua 的 _rule_functions 表中删除规则函数
    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    lua_getglobal(L, "_rule_functions");
    if (lua_istable(L, -1)) {
        lua_pushlstring(L, rule_name.data(), rule_name.size());
        lua_pushnil(L);
        lua_rawset(L, -3);  // _rule_functions[rule_name] = nil
    }

    _rules.erase(it);
    return true;
}

bool RuleEngine::reload_rule(const std::string& rule_name, std::string* error_msg) {
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' not found";
        }
        return false;
    }

    // 保存文件路径
    std::string file_path = it->second.file_path;

    // 移除旧规则（会清理 _rule_functions 表）
    remove_rule(rule_name);

    // 重新添加规则（这会重新加载并更新 _rule_functions 表）
    return add_rule(rule_name, file_path, error_msg);
}

bool RuleEngine::match_rule(const std::string& rule_name, const DataAdapter& data_adapter,
                            MatchResult& result, std::string* error_msg) {
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' not found";
        }
        return false;
    }

    return call_match_function(rule_name, data_adapter, result, error_msg);
}

bool RuleEngine::match_rule(const std::vector<std::string>& rule_names,
                             const DataAdapter& data_adapter,
                             std::map<std::string, MatchResult>& results,
                             std::string* error_msg) {
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    results.clear();

    // 如果规则列表为空, 直接返回 true
    if (rule_names.empty()) {
        return true;
    }

    // 规则去重
    std::set<std::string> unique_rules(rule_names.begin(), rule_names.end());

    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);  // 自动管理栈平衡

    // 将数据压入栈顶
    if (!data_adapter.push_to_lua(L, error_msg)) {
        return false;
    }

    // 数据表现在在栈顶（-1 位置），记住它的位置
    int data_table_index = lua_gettop(L);

    bool any_matched = false;
    for (const auto& rule_name : unique_rules) {
        if (has_rule(rule_name) == false) {
            // 规则不存在，记录错误并继续
            MatchResult result;
            result.matched = false;
            result.message = "Rule '" + rule_name + "' not found";
            results[rule_name] = result;
            continue;
        }

        // 获取规则函数
        lua_getglobal(L, "_rule_functions");
        if (!lua_istable(L, -1)) {
            // 如果函数表不存在，记录错误并继续
            MatchResult result;
            result.matched = false;
            result.message = "Rule function table not found";
            results[rule_name] = result;
            lua_pop(L, 1);  // 清理栈
            continue;
        }

        lua_pushlstring(L, rule_name.data(), rule_name.size());
        lua_rawget(L, -2);  // 获取 _rule_functions[rule_name]
        lua_remove(L, -2);  // 移除 _rule_functions 表

        if (!lua_isfunction(L, -1)) {
            // 如果函数不存在，记录错误并继续
            MatchResult result;
            result.matched = false;
            result.message = "Rule '" + rule_name + "' match function not found";
            results[rule_name] = result;
            lua_pop(L, 1);  // 清理栈
            continue;
        }

        // 复制数据表作为参数（不重新转换）
        lua_pushvalue(L, data_table_index);

        // 调用 match 函数：1 个参数，2 个返回值
        if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
            // 调用失败，记录错误并继续处理下一个规则
            MatchResult result;
            result.matched = false;
            result.message = "Failed to call match: " + _lua_state.get_error_string();
            results[rule_name] = result;
            lua_pop(L, 2);  // 清理返回值
            continue;
        }

        // 获取返回值
        MatchResult result;
        if (!lua_isboolean(L, -2)) {
            result.matched = false;
            result.message = "First return value is not boolean";
        } else {
            result.matched = lua_toboolean(L, -2);

            // lua_isstring 会对数字返回 true（因为数字可以转换为字符串）
            // 所以需要使用 lua_type 来精确检查类型
            if (lua_type(L, -1) == LUA_TSTRING) {
                result.message = lua_tostring(L, -1);
            }
            // 如果不是字符串，message 保持为空字符串
        }

        lua_pop(L, 2);  // 清理返回值
        results[rule_name] = result;

        if (result.matched) {
            any_matched = true;
        }
    }

    return any_matched;
}

bool RuleEngine::match_all_rules(const DataAdapter& data_adapter,
                                 std::map<std::string, MatchResult>& results,
                                 std::string* error_msg) {
    std::vector<std::string> rule_names;
    rule_names.reserve(_rules.size());

    for (const auto& [rule_name, _] : _rules) {
        rule_names.push_back(rule_name);
    }

    return match_rule(rule_names, data_adapter, results, error_msg);
}

std::vector<RuleInfo> RuleEngine::get_all_rules() const {
    std::vector<RuleInfo> infos;
    infos.reserve(_rules.size());

    for (const auto& pair : _rules) {
        RuleInfo info;
        info.name = pair.second.name;
        info.file_path = pair.second.file_path;
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

bool RuleEngine::enable_jit() {
    if (!_lua_state.is_valid()) {
        return false;
    }
    return _lua_state.enable_jit();
}

bool RuleEngine::disable_jit() {
    if (!_lua_state.is_valid()) {
        return false;
    }
    return _lua_state.disable_jit();
}

bool RuleEngine::flush_jit() {
    if (!_lua_state.is_valid()) {
        return false;
    }
    return _lua_state.flush_jit();
}

bool RuleEngine::load_rule_file(const std::string& file_path, std::string* error_msg) {
    return _lua_state.load_file(file_path.c_str(), error_msg);
}

bool RuleEngine::call_match_function(const std::string& rule_name,
                                     const DataAdapter& data_adapter,
                                     MatchResult& result,
                                     std::string* error_msg) {
    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);  // 自动管理栈平衡

    // 从规则函数表中获取对应规则的match函数
    lua_getglobal(L, "_rule_functions");
    if (!lua_istable(L, -1)) {
        if (error_msg) {
            *error_msg = "Rule function table not found";
        }
        return false;
    }

    lua_pushlstring(L, rule_name.data(), rule_name.size());
    lua_rawget(L, -2);  // 获取 _rule_functions[rule_name]
    lua_remove(L, -2);  // 移除 _rule_functions 表

    if (!lua_isfunction(L, -1)) {
        if (error_msg) {
            *error_msg = "Rule '" + rule_name + "' match function not found";
        }
        return false;
    }

    // 将数据压入栈顶
    if (!data_adapter.push_to_lua(L, error_msg)) {
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
        return false;
    }
    bool matched = lua_toboolean(L, -2);

    // 获取第二个返回值：错误信息（可选）
    std::string message;
    if (lua_type(L, -1) == LUA_TSTRING) {
        message = lua_tostring(L, -1);
    }

    result.matched = matched;
    result.message = message;

    return true;
    // 栈守卫析构时自动清理栈
}

bool RuleEngine::register_function(const std::string& function_name,
                                    int (*function_ptr)(lua_State* L),
                                    std::string* error_msg) {
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    // 确保 ljre 表存在
    ensure_ljre_table();

    // 获取 ljre 表到栈顶
    lua_getglobal(L, "ljre");

    // 设置 ljre[function_name] = function
    lua_pushstring(L, function_name.c_str());
    lua_pushcfunction(L, function_ptr);
    lua_rawset(L, -3);

    return true;
}

bool RuleEngine::unregister_function(const std::string& function_name) {
    if (!_lua_state.is_valid()) {
        return false;
    }

    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    // 确保 ljre 表存在
    ensure_ljre_table();

    // 获取 ljre 表到栈顶
    lua_getglobal(L, "ljre");

    // 检查函数是否存在
    lua_pushstring(L, function_name.c_str());
    lua_rawget(L, -2);

    if (!lua_isfunction(L, -1)) {
        return false;  // 函数不存在
    }

    lua_pop(L, 1);  // 弹出函数值

    // 设置 ljre[function_name] = nil
    lua_pushstring(L, function_name.c_str());
    lua_pushnil(L);
    lua_rawset(L, -3);

    return true;
}

void RuleEngine::clear_registered_functions() {
    if (!_lua_state.is_valid()) {
        return;
    }

    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    // 删除整个 ljre 表
    lua_pushnil(L);
    lua_setglobal(L, "ljre");
}

bool RuleEngine::has_function(const std::string& function_name) {
    if (!_lua_state.is_valid()) {
        return false;
    }

    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    // 确保 ljre 表存在
    ensure_ljre_table();

    // 获取 ljre 表到栈顶
    lua_getglobal(L, "ljre");

    // 检查 function_name 是否存在
    lua_pushstring(L, function_name.c_str());
    lua_rawget(L, -2);

    bool exists = lua_isfunction(L, -1);

    return exists;
}

std::vector<std::string> RuleEngine::get_registered_functions() {
    std::vector<std::string> functions;

    if (!_lua_state.is_valid()) {
        return functions;
    }

    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    // 确保 ljre 表存在
    ensure_ljre_table();

    // 获取 ljre 表到栈顶，遍历表
    lua_getglobal(L, "ljre");
    lua_pushnil(L);  // 第一个 key
    while (lua_next(L, -2) != 0) {
        // -1 => value, -2 => key
        if (lua_isfunction(L, -1) && lua_isstring(L, -2)) {
            functions.push_back(lua_tostring(L, -2));
        }
        lua_pop(L, 1);  // 弹出 value，保留 key
    }

    return functions;
}

void RuleEngine::ensure_ljre_table() {
    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);

    // 检查 ljre 表是否存在
    lua_getglobal(L, "ljre");
    if (lua_istable(L, -1)) {
        return;  // 表已存在，栈守卫自动清理
    }

    // 表不存在，创建一个
    lua_pop(L, 1);  // 弹出非 table 值（可能是 nil）
    lua_createtable(L, 0, 0);  // 创建新 table
    lua_setglobal(L, "ljre");
}

} // namespace ljre
