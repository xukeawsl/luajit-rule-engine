#pragma once

#include <string>
#include <unordered_map>
#include <map>
#include <set>
#include <memory>
#include <vector>

#include "ljre/lua_state.h"
#include "ljre/data_adapter.h"


namespace ljre {

// 缓存清理策略
enum class CacheCleanupPolicy {
    Never,      // 从不自动清理（仅在 weak_ptr 过期或引擎析构时清理）
    Aggressive  // 积极清理（每次 match_rule 调用都检查）- 默认策略
};

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
    bool match_rule(const std::string& rule_name,
                    std::shared_ptr<DataAdapter> data_adapter,
                    MatchResult& result,
                    std::string* error_msg = nullptr) const;

    // 检查多个规则是否匹配
    // results 存储 {规则名: 匹配结果} 的映射，按规则名字母顺序排序
    // 返回值: true 表示至少有一个规则匹配成功，false 表示所有规则都失败
    // 注意：即使某个规则调用失败，也会将其结果添加到 results 中
    bool match_rule(const std::vector<std::string>& rule_names,
                    std::shared_ptr<DataAdapter> data_adapter,
                    std::map<std::string, MatchResult>& results,
                    std::string* error_msg = nullptr) const;

    // 检查所有规则是否匹配
    bool match_all_rules(std::shared_ptr<DataAdapter> data_adapter,
                         std::map<std::string, MatchResult>& results,
                         std::string* error_msg = nullptr) const;

    // 获取所有规则信息
    std::vector<RuleInfo> get_all_rules() const;

    // 检查规则是否存在
    bool has_rule(const std::string& rule_name) const;

    // 获取规则数量
    size_t get_rule_count() const { return _rules.size(); }

    // 设置缓存清理策略
    void set_cache_cleanup_policy(CacheCleanupPolicy policy) {
        _cleanup_policy = policy;
    }

    // 获取当前缓存清理策略
    CacheCleanupPolicy get_cache_cleanup_policy() const {
        return _cleanup_policy;
    }

    // 清空所有规则
    void clear_rules();

    // JIT 控制方法
    // 启用 JIT 编译（默认已启用）
    bool enable_jit();

    // 禁用 JIT 编译，切换到解释模式
    bool disable_jit();

    // 刷新 JIT 编译器缓存，清除已编译的代码
    bool flush_jit();

    // 注册 C++ 函数到 Lua 的 ljre 命名空间
    // 注册后可在 Lua 中通过 ljre.function_name() 调用
    // 函数签名必须是 int (*)(lua_State* L)，返回值为返回值个数
    bool register_function(const std::string& function_name,
                           int (*function_ptr)(lua_State* L),
                           std::string* error_msg = nullptr);

    // 注册类成员函数到 Lua 的 ljre 命名空间
    // 使用 lua_pushcclosure 将 instance 指针作为 upvalue[1] 传递
    // 模板参数：类类型，成员函数指针类型
    //
    // 注意：此实现需要一个静态分发器函数，使用方式如下：
    // class MyClass {
    // public:
    //     int my_method(lua_State* L) { /* ... */ }
    //
    //     // 静态分发器
    //     static int my_method_dispatcher(lua_State* L) {
    //         auto* self = static_cast<MyClass*>(lua_touserdata(L, lua_upvalueindex(1)));
    //         return self->my_method(L);
    //     }
    // };
    //
    // 使用：
    // MyClass obj;
    // engine.register_function("my_method", &MyClass::my_method_dispatcher, &obj);
    template <typename Class>
    bool register_function(const std::string& function_name,
                           int (*dispatcher)(lua_State* L),
                           Class* instance,
                           std::string* error_msg = nullptr) {
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

        // 设置 ljre[function_name] = closure
        lua_pushstring(L, function_name.c_str());
        lua_pushlightuserdata(L, instance);  // 压入 upvalue
        lua_pushcclosure(L, dispatcher, 1);  // 1 个 upvalue，通过 lua_upvalueindex(1) 访问
        lua_rawset(L, -3);  // ljre[function_name] = closure

        // 记录到元数据（用于 clone）
        RegisteredFunctionInfo info;
        info.type = RegisteredFunctionInfo::MEMBER;
        info.name = function_name;
        info.function_ptr = reinterpret_cast<void*>(dispatcher);
        info.instance_ptr = static_cast<void*>(instance);
        _metadata.registered_functions[function_name] = info;

        return true;
    }

    // 注销函数
    bool unregister_function(const std::string& function_name);

    // 清空所有已注册的函数
    void clear_registered_functions();

    // 检查函数是否已注册
    bool has_function(const std::string& function_name);

    // 获取所有已注册的函数名
    std::vector<std::string> get_registered_functions();

    // === Lua 公共函数管理 ===

    /**
     * 加载 Lua 公共函数文件
     *
     * 此方法会执行指定的 Lua 文件，用户可以在文件中自由定义全局变量和函数。
     * 引擎只负责加载和执行文件，不限制命名空间的选择。
     *
     * @param file_path Lua 文件路径
     * @param error_msg 错误信息输出（可选）
     * @return 成功返回 true，失败返回 false
     *
     * 使用示例：
     *   engine.add_lua_file("utils.lua");
     *   engine.add_lua_file("validators.lua");
     *
     * Lua 文件格式示例（utils.lua）：
     *   utils = {}
     *
     *   function utils.is_adult(data)
     *       return data.age and data.age >= 18
     *   end
     *
     *   function utils.calculate_score(data)
     *       local score = 0
     *       if data.vip then score = score + 10 end
     *       return score
     *   end
     *
     * 规则中使用：
     *   function match(data)
     *       if not utils.is_adult(data) then
     *           return false, "Not an adult"
     *       end
     *       local score = utils.calculate_score(data)
     *       return true, "Score: " .. score
     *   end
     *
     * 注意：
     *   - 用户可以在 Lua 文件中自由选择命名空间（utils, validators, common 等）
     *   - 可以定义多个全局变量
     *   - 引擎只负责加载执行，不做任何限制
     */
    bool add_lua_file(const std::string& file_path, std::string* error_msg = nullptr);

    // === Clone 方法 ===

    // 克隆选项位标志
    enum CloneOption : uint32_t {
        NONE = 0,                      // 不克隆任何内容，返回空引擎
        LUA_FILES = 1 << 0,           // 克隆 Lua 公共文件
        CPP_FUNCTIONS = 1 << 1,       // 克隆 C++ 普通函数
        CPP_MEMBER_FUNCTIONS = 1 << 2,// 克隆 C++ 成员函数
        RULES = 1 << 3,               // 克隆规则文件
        ALL = LUA_FILES | CPP_FUNCTIONS | CPP_MEMBER_FUNCTIONS | RULES
    };

    using CloneOptions = uint32_t;

    // 克隆当前引擎，创建一个独立的副本
    // options: 位标志组合，指定要克隆哪些内容
    // error_msg: 错误信息输出（可选）
    // 返回: 成功返回新引擎指针，失败返回 nullptr
    std::unique_ptr<RuleEngine> clone(CloneOptions options = ALL,
                                       std::string* error_msg = nullptr) const;

    // 便捷克隆方法
    std::unique_ptr<RuleEngine> clone_lua_files(std::string* error_msg = nullptr) const {
        return clone(LUA_FILES, error_msg);
    }

    std::unique_ptr<RuleEngine> clone_cpp_functions(std::string* error_msg = nullptr) const {
        return clone(CPP_FUNCTIONS | CPP_MEMBER_FUNCTIONS, error_msg);
    }

    std::unique_ptr<RuleEngine> clone_rules(std::string* error_msg = nullptr) const {
        return clone(RULES, error_msg);
    }

    std::unique_ptr<RuleEngine> clone_safe(std::string* error_msg = nullptr) const {
        return clone(ALL, error_msg);
    }

protected:
    // 用于测试：允许派生类访问内部状态
    // 测试类可以继承 RuleEngine 并访问这些成员
    LuaState& get_lua_state() { return _lua_state; }

    // 缓存条目
    struct AdapterCacheEntry {
        int registry_ref;                    // Lua Registry 引用
        std::weak_ptr<DataAdapter> weak_ptr; // 用于检查是否过期
    };

    // 缓存清理策略
    CacheCleanupPolicy _cleanup_policy;

    // 适配器缓存（用 ID 作为 key）
    mutable std::unordered_map<uint64_t, AdapterCacheEntry> _adapter_cache;

    // 清理过期的缓存
    void cleanup_expired_cache() const;

private:
    struct Rule {
        std::string name;
        std::string file_path;
    };

    // 注册的函数信息（用于 clone）
    struct RegisteredFunctionInfo {
        enum FunctionType {
            NORMAL,       // 普通函数：int (*)(lua_State*)
            MEMBER        // 成员函数：int (Class::*dispatcher)(lua_State*)
        };

        FunctionType type;
        std::string name;
        void* function_ptr;   // 函数指针
        void* instance_ptr;   // 实例指针（仅 MEMBER 类型）
    };

    // 引擎元数据（用于 clone）
    struct EngineMetadata {
        std::vector<std::string> loaded_lua_files;
        std::map<std::string, RegisteredFunctionInfo> registered_functions;
        std::map<std::string, std::string> rule_files;

        void clear() {
            loaded_lua_files.clear();
            registered_functions.clear();
            rule_files.clear();
        }
    };

    LuaState _lua_state;
    std::unordered_map<std::string, Rule> _rules;
    mutable EngineMetadata _metadata;

    // 内部方法：加载规则文件
    bool load_rule_file(const std::string& file_path, std::string* error_msg);

    // 内部方法：调用规则匹配函数
    bool call_match_function(const std::string& rule_name,
                             std::shared_ptr<DataAdapter> data_adapter,
                             MatchResult& result,
                             std::string* error_msg) const;

    // 内部方法：确保 ljre 全局表存在（保持栈平衡）
    void ensure_ljre_table();
};

} // namespace ljre
