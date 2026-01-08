#include "ljre/rule_engine.h"
#include "ljre/json_adapter.h"
#include <iostream>

using namespace ljre;
using json = nlohmann::json;

int main() {
    // 创建规则引擎
    RuleEngine engine;

    // 方法1: 从配置文件加载规则
    std::string error_msg;
    if (!engine.load_rule_config("rule_config.lua", &error_msg)) {
        std::cerr << "加载规则配置失败: " << error_msg << std::endl;
        return 1;
    }

    std::cout << "成功加载 " << engine.get_rule_count() << " 条规则" << std::endl;

    // 显示所有规则
    auto rules = engine.get_all_rules();
    for (const auto& rule : rules) {
        std::cout << "  - " << rule.name << " (" << rule.file_path << ")"
                  << (rule.loaded ? " [已加载]" : " [未加载]") << std::endl;
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
    if (engine.match_rule("age_check", adapter1)) {
        std::cout << "✓ age_check 规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ age_check 规则匹配失败" << std::endl;
    }

    // 匹配所有规则
    std::vector<MatchResult> results;
    if (engine.match_all_rules(adapter1, &results, &error_msg)) {
        std::cout << "✓ 所有规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ 部分规则匹配失败" << std::endl;
    }

    std::cout << "\n详细结果:" << std::endl;
    for (const auto& result : results) {
        std::cout << "  " << (result.matched ? "✓" : "✗") << " "
                  << result.message << std::endl;
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

    if (engine.match_all_rules(adapter2, &results, &error_msg)) {
        std::cout << "✓ 所有规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ 部分规则匹配失败" << std::endl;
    }

    std::cout << "\n详细结果:" << std::endl;
    for (const auto& result : results) {
        std::cout << "  " << (result.matched ? "✓" : "✗") << " "
                  << result.message << std::endl;
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

    if (engine.match_all_rules(adapter3, &results, &error_msg)) {
        std::cout << "✓ 所有规则匹配成功" << std::endl;
    } else {
        std::cout << "✗ 部分规则匹配失败" << std::endl;
    }

    std::cout << "\n详细结果:" << std::endl;
    for (const auto& result : results) {
        std::cout << "  " << (result.matched ? "✓" : "✗") << " "
                  << result.message << std::endl;
    }

    std::cout << "\n" << std::string(50, '-') << "\n" << std::endl;

    // 测试动态添加规则
    std::cout << "测试动态添加规则:" << std::endl;
    if (engine.add_rule("age_check", "rules/age_check.lua", &error_msg)) {
        std::cout << "✓ 规则已存在，添加失败（符合预期）" << std::endl;
    } else {
        std::cout << "✗ 规则添加失败: " << error_msg << std::endl;
    }

    // 测试重新加载规则
    std::cout << "\n测试重新加载规则:" << std::endl;
    if (engine.reload_rule("age_check", &error_msg)) {
        std::cout << "✓ 规则重新加载成功" << std::endl;
    } else {
        std::cout << "✗ 规则重新加载失败: " << error_msg << std::endl;
    }

    return 0;
}
