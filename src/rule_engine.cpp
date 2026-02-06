#include "ljre/rule_engine.h"


namespace ljre {

RuleEngine::RuleEngine()
    : _cleanup_policy(CacheCleanupPolicy::Aggressive) {
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

    // 记录到元数据
    _metadata.rule_files[rule_name] = file_path;

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

bool RuleEngine::match_rule(const std::string& rule_name,
                            std::shared_ptr<DataAdapter> data_adapter,
                            MatchResult& result, std::string* error_msg) const {
    if (!_lua_state.is_valid()) {
        result.matched = false;
        result.message = "Lua state is invalid";
        if (error_msg) {
            *error_msg = result.message;
        }
        return false;
    }

    auto it = _rules.find(rule_name);
    if (it == _rules.end()) {
        result.matched = false;
        result.message = "Rule '" + rule_name + "' not found";
        if (error_msg) {
            *error_msg = result.message;
        }
        return false;
    }

    return call_match_function(rule_name, data_adapter, result, error_msg);
}

bool RuleEngine::match_rule(const std::vector<std::string>& rule_names,
                             std::shared_ptr<DataAdapter> data_adapter,
                             std::map<std::string, MatchResult>& results,
                             std::string* error_msg) const {
    results.clear();

    // 检查 Lua 状态是否有效
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    // 如果规则列表为空, 直接返回 true
    if (rule_names.empty()) {
        return true;
    }

    // 规则去重
    std::set<std::string> unique_rules(rule_names.begin(), rule_names.end());

    // 循环调用单次匹配（自动利用缓存）
    bool any_matched = false;
    for (const auto& rule_name : unique_rules) {
        MatchResult result;
        if (match_rule(rule_name, data_adapter, result, error_msg)) {
            // 调用成功
            if (result.matched) {
                any_matched = true;
            }
            results[rule_name] = result;
        } else {
            // 调用失败
            results[rule_name] = result;
        }
    }

    return any_matched;
}

bool RuleEngine::match_all_rules(std::shared_ptr<DataAdapter> data_adapter,
                                 std::map<std::string, MatchResult>& results,
                                 std::string* error_msg) const {
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
                                     std::shared_ptr<DataAdapter> data_adapter,
                                     MatchResult& result,
                                     std::string* error_msg) const {
    lua_State* L = _lua_state.get();
    LuaStackGuard guard(L);  // 自动管理栈平衡

    // === 1. 根据策略清理缓存 ===
    if (_cleanup_policy == CacheCleanupPolicy::Aggressive) {
        cleanup_expired_cache();  // 积极清理：每次调用都检查
    }
    // CacheCleanupPolicy::Never 不执行自动清理

    // === 2. 获取适配器 ID 并检查缓存 ===
    uint64_t adapter_id = data_adapter->get_id();
    auto it = _adapter_cache.find(adapter_id);

    if (it != _adapter_cache.end()) {
        // 命中缓存：直接获取 table
        // 注：由于 adapter ID 是唯一且单调递增的，如果找到缓存，
        //     一定是当前 shared_ptr 创建的，weak_ptr 肯定有效
        lua_rawgeti(L, LUA_REGISTRYINDEX, it->second.registry_ref);
    }

    // === 3. 未命中：转换数据并缓存 ===
    if (it == _adapter_cache.end()) {
        // 转换数据
        if (!data_adapter->push_to_lua(L, error_msg)) {
            result.matched = false;
            if (error_msg) {
                result.message = *error_msg;
            }
            return false;
        }

        // 存入 Registry
        int ref = luaL_ref(L, LUA_REGISTRYINDEX);

        // 保存缓存（保存 weak_ptr）
        _adapter_cache[adapter_id] = {ref, data_adapter};

        // 重新获取 table 到栈顶
        lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    }

    // === 4. 执行字段修改命令（由引擎侧调用）===
    if (!data_adapter->execute_commands(L, error_msg)) {
        result.matched = false;
        if (error_msg) {
            result.message = *error_msg;
        }
        return false;
    }

    // === 5. 调用规则匹配函数 ===
    // 从规则函数表中获取对应规则的match函数
    lua_getglobal(L, "_rule_functions");
    if (!lua_istable(L, -1)) {
        result.matched = false;
        result.message = "Rule function table not found";
        if (error_msg) {
            *error_msg = result.message;
        }
        return false;
    }

    lua_pushlstring(L, rule_name.data(), rule_name.size());
    lua_rawget(L, -2);  // 获取 _rule_functions[rule_name]
    lua_remove(L, -2);  // 移除 _rule_functions 表

    if (!lua_isfunction(L, -1)) {
        result.matched = false;
        result.message = "Rule '" + rule_name + "' match function not found";
        if (error_msg) {
            *error_msg = result.message;
        }
        return false;
    }

    // 交换栈顶：现在 -1 是函数，-2 是数据 table
    lua_insert(L, -2);

    // 调用match函数，1个参数（数据 table），2个返回值
    if (lua_pcall(L, 1, 2, 0) != LUA_OK) {
        // Lua 调用失败（包括 C++ 异常），标记为匹配失败
        result.matched = false;
        result.message = "Failed to call match: " + _lua_state.get_error_string();
        if (error_msg) {
            *error_msg = result.message;
        }
        return false;
    }

    // 获取第一个返回值：是否匹配成功
    if (!lua_isboolean(L, -2)) {
        result.matched = false;
        result.message = "First return value of 'match' must be boolean";
        if (error_msg) {
            *error_msg = result.message;
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

    // 记录到元数据
    RegisteredFunctionInfo info;
    info.type = RegisteredFunctionInfo::NORMAL;
    info.name = function_name;
    info.function_ptr = reinterpret_cast<void*>(function_ptr);
    info.instance_ptr = nullptr;
    _metadata.registered_functions[function_name] = info;

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

    // 从元数据中删除
    _metadata.registered_functions.erase(function_name);

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

    // 清理元数据
    _metadata.registered_functions.clear();
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

bool RuleEngine::add_lua_file(const std::string& file_path, std::string* error_msg) {
    if (!_lua_state.is_valid()) {
        if (error_msg) {
            *error_msg = "Lua state is invalid";
        }
        return false;
    }

    // 加载并执行 Lua 文件
    // 文件内容会自行决定如何组织函数（设置什么全局变量）
    // 引擎只负责加载执行，不做任何限制
    if (!_lua_state.load_file(file_path.c_str(), error_msg)) {
        return false;
    }

    // 记录到元数据
    _metadata.loaded_lua_files.push_back(file_path);

    // 文件执行后，定义的全局变量会自动可用
    // 引擎不需要做任何额外处理

    return true;
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

// ============================================================================
// Clone 方法实现
// ============================================================================

std::unique_ptr<RuleEngine> RuleEngine::clone(CloneOptions options,
                                               std::string* error_msg) const {
    auto new_engine = std::make_unique<RuleEngine>();

    if (!new_engine->_lua_state.is_valid()) {
        return nullptr;
    }

    if (options == NONE) {
        return new_engine;
    }

    // 1. 克隆 Lua 公共文件
    if (options & LUA_FILES) {
        for (const auto& file : _metadata.loaded_lua_files) {
            if (!new_engine->add_lua_file(file, error_msg)) {
                return nullptr;
            }
        }
    }

    // 2. 克隆 C++ 普通函数
    if (options & CPP_FUNCTIONS) {
        for (const auto& [name, info] : _metadata.registered_functions) {
            if (info.type == RegisteredFunctionInfo::NORMAL) {
                auto func = reinterpret_cast<int (*)(lua_State*)>(info.function_ptr);
                if (!new_engine->register_function(name, func, error_msg)) {
                    return nullptr;
                }
            }
        }
    }

    // 3. 克隆 C++ 成员函数
    if (options & CPP_MEMBER_FUNCTIONS) {
        for (const auto& [name, info] : _metadata.registered_functions) {
            if (info.type == RegisteredFunctionInfo::MEMBER) {
                auto func = reinterpret_cast<int (*)(lua_State*)>(info.function_ptr);
                if (!new_engine->register_function(name, func, info.instance_ptr, error_msg)) {
                    return nullptr;
                }
            }
        }
    }

    // 4. 克隆规则文件
    if (options & RULES) {
        for (const auto& [rule_name, file_path] : _metadata.rule_files) {
            if (!new_engine->add_rule(rule_name, file_path, error_msg)) {
                return nullptr;
            }
        }
    }

    return new_engine;
}

void RuleEngine::cleanup_expired_cache() const {
    auto it = _adapter_cache.begin();
    while (it != _adapter_cache.end()) {
        // 检查 weak_ptr 是否过期
        if (it->second.weak_ptr.expired()) {
            // 过期了，清理缓存
            luaL_unref(_lua_state.get(), LUA_REGISTRYINDEX, it->second.registry_ref);
            it = _adapter_cache.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace ljre
