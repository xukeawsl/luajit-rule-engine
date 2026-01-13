#include "ljre/rule_engine.h"
#include "ljre/json_adapter.h"
#include <iostream>
#include <chrono>

using namespace ljre;
using json = nlohmann::json;

// 示例 1: 普通C++函数 - 获取当前毫秒时间戳
int get_current_time_ms(lua_State* L) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

    lua_pushnumber(L, static_cast<lua_Number>(millis));
    return 1;  // 返回1个值
}

// 示例 2: 普通C++函数 - 打印日志
int log_message(lua_State* L) {
    if (lua_gettop(L) < 1) {
        lua_pushstring(L, "log: expected at least 1 argument");
        lua_error(L);
        return 0;  // 不会执行到这里
    }

    const char* message = lua_tostring(L, 1);
    std::cout << "[LUA LOG] " << message << std::endl;

    return 0;  // 不返回值
}

// 示例 3: 类成员函数
class MathHelper {
public:
    int add(lua_State* L) {
        if (lua_gettop(L) < 2) {
            lua_pushstring(L, "add: expected 2 arguments");
            lua_error(L);
            return 0;
        }

        double a = lua_tonumber(L, 1);
        double b = lua_tonumber(L, 2);
        double result = a + b;

        lua_pushnumber(L, result);
        return 1;  // 返回1个值
    }

    // 静态分发器
    static int add_dispatcher(lua_State* L) {
        auto* self = static_cast<MathHelper*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->add(L);
    }
};

int main() {
    // 创建规则引擎
    RuleEngine engine;

    std::cout << "=== 函数注册测试 ===" << std::endl;

    // 注册普通C++函数
    std::string error_msg;
    if (!engine.register_function("get_time_ms", get_current_time_ms, &error_msg)) {
        std::cerr << "Failed to register get_time_ms: " << error_msg << std::endl;
        return 1;
    }

    if (!engine.register_function("log", log_message, &error_msg)) {
        std::cerr << "Failed to register log: " << error_msg << std::endl;
        return 1;
    }

    // 注册类成员函数
    MathHelper math_helper;
    if (!engine.register_function("add", &MathHelper::add_dispatcher, &math_helper, &error_msg)) {
        std::cerr << "Failed to register add: " << error_msg << std::endl;
        return 1;
    }

    std::cout << "已注册函数: ";
    auto functions = engine.get_registered_functions();
    for (const auto& func : functions) {
        std::cout << func << " ";
    }
    std::cout << std::endl;

    std::cout << "\n=== 规则加载测试 ===" << std::endl;

    // 方法1: 从配置文件加载规则
    if (!engine.load_rule_config("rule_config.lua", &error_msg)) {
        std::cerr << "加载规则配置失败: " << error_msg << std::endl;
        return 1;
    }

    std::cout << "成功加载 " << engine.get_rule_count() << " 条规则" << std::endl;

    // 显示所有规则
    auto rules = engine.get_all_rules();
    for (const auto& rule : rules) {
        std::cout << "  - " << rule.name << " (" << rule.file_path << ")" << std::endl;
    }

    std::cout << std::endl;

    // 测试数据1: 有效的用户数据
    json valid_user = {
        {"username", "zhang_san"},
        {"email", "zhangsan@example.com"},
        {"age", 25},
        {"phone", "13800138000"}
    };

    std::cout << "测试数据1 (有效用户):" << std::endl;
    std::cout << valid_user.dump(2) << std::endl;

    JsonAdapter adapter1(valid_user);

    // 匹配单个规则
    MatchResult result;
    if (engine.match_rule("age_check", adapter1, result)) {
        std::cout << "✓ age_check 规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ age_check 规则匹配失败" << std::endl;
    }

    // 匹配所有规则（只要有一个规则通过就返回 true）
    std::map<std::string, MatchResult> results;
    if (engine.match_all_rules(adapter1, results)) {
        std::cout << "✓ 至少一个规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ 所有规则匹配失败" << std::endl;
    }

    std::cout << "\n详细结果:" << std::endl;
    for (const auto& pair : results) {
        std::cout << "  [" << pair.first << "] "
                  << (pair.second.matched ? "✓" : "✗") << " "
                  << pair.second.message << std::endl;
    }

    std::cout << "\n" << std::string(50, '-') << "\n" << std::endl;

    // 测试数据2: 无效的用户数据（年龄不足）
    json invalid_user1 = {
        {"username", "li_si"},
        {"email", "lisi@example.com"},
        {"age", 15},
        {"phone", "13900139000"}
    };

    std::cout << "测试数据2 (年龄不足):" << std::endl;
    std::cout << invalid_user1.dump(2) << std::endl;

    JsonAdapter adapter2(invalid_user1);
    std::map<std::string, MatchResult> results2;

    if (engine.match_all_rules(adapter2, results2)) {
        std::cout << "✓ 至少一个规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ 所有规则匹配失败" << std::endl;
    }

    std::cout << "\n详细结果:" << std::endl;
    for (const auto& pair : results2) {
        std::cout << "  [" << pair.first << "] "
                  << (pair.second.matched ? "✓" : "✗") << " "
                  << pair.second.message << std::endl;
    }

    std::cout << "\n" << std::string(50, '-') << "\n" << std::endl;

    // 测试数据3: 无效的用户数据（缺少字段）
    json invalid_user2 = {
        {"username", "wang_wu"},
        {"age", 30}
        // 缺少 email 和 phone
    };

    std::cout << "测试数据3 (缺少字段):" << std::endl;
    std::cout << invalid_user2.dump(2) << std::endl;

    JsonAdapter adapter3(invalid_user2);
    std::map<std::string, MatchResult> results3;

    if (engine.match_all_rules(adapter3, results3)) {
        std::cout << "✓ 至少一个规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ 所有规则匹配失败" << std::endl;
    }

    std::cout << "\n详细结果:" << std::endl;
    for (const auto& pair : results3) {
        std::cout << "  [" << pair.first << "] "
                  << (pair.second.matched ? "✓" : "✗") << " "
                  << pair.second.message << std::endl;
    }

    std::cout << "\n" << std::string(50, '-') << "\n" << std::endl;

    // 测试动态添加规则
    std::cout << "测试动态添加规则:" << std::endl;
    if (!engine.add_rule("age_check", "rules/age_check.lua", &error_msg)) {
        std::cout << "✓ 规则已存在，添加失败（符合预期）: " << error_msg << std::endl;
    } else {
        std::cout << "✗ 规则不应该添加成功" << std::endl;
    }

    // 测试重新加载规则
    std::cout << "\n测试重新加载规则:" << std::endl;
    if (engine.reload_rule("age_check", &error_msg)) {
        std::cout << "✓ 规则重新加载成功" << std::endl;
    } else {
        std::cout << "✗ 规则重新加载失败: " << error_msg << std::endl;
    }

    std::cout << "\n" << std::string(50, '-') << "\n" << std::endl;

    // 测试函数注册功能
    std::cout << "=== 函数注册测试 ===" << std::endl;

    // 添加测试规则
    if (!engine.add_rule("function_test", "rules/function_test.lua", &error_msg)) {
        std::cerr << "添加 function_test 规则失败: " << error_msg << std::endl;
        return 1;
    }

    // 测试数据: 使用注册函数
    json test_data = {
        {"value1", 60},
        {"value2", 50}
    };

    std::cout << "测试函数注册功能:" << std::endl;
    std::cout << test_data.dump(2) << std::endl;

    JsonAdapter adapter4(test_data);
    MatchResult result4;

    if (engine.match_rule("function_test", adapter4, result4, &error_msg)) {
        std::cout << "✓ function_test 规则执行成功" << std::endl;
        std::cout << "结果: " << (result4.matched ? "匹配成功" : "匹配失败") << std::endl;
        std::cout << "消息: " << result4.message << std::endl;
    } else {
        std::cout << "✗ function_test 规则执行失败: " << error_msg << std::endl;
    }

    // 测试函数管理功能
    std::cout << "\n--- 函数管理测试 ---" << std::endl;

    std::cout << "has_function('get_time_ms'): "
              << (engine.has_function("get_time_ms") ? "true" : "false") << std::endl;

    std::cout << "has_function('nonexistent'): "
              << (engine.has_function("nonexistent") ? "true" : "false") << std::endl;

    std::cout << "unregister_function('log'): "
              << (engine.unregister_function("log") ? "成功" : "失败") << std::endl;

    functions = engine.get_registered_functions();
    std::cout << "注销 log 后剩余函数: ";
    for (const auto& func : functions) {
        std::cout << func << " ";
    }
    std::cout << std::endl;

    return 0;
}
