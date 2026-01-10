#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ljre/rule_engine.h"
#include "ljre/json_adapter.h"
#include "test_helpers.h"
#include <fstream>

using namespace ljre;
using json = nlohmann::json;

// ============================================================================
// RuleEngine 构造和基础查询测试
// ============================================================================

class RuleEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建测试规则文件
        CreateTestRules();
    }

    void TearDown() override {
        // 清理测试文件
        CleanupTestData();
    }

    void CleanupTestData() {
        // 清理测试数据目录
        system("rm -rf test_data");
    }

    void CreateTestRules() {
        // 创建各种测试规则文件
        CreateRuleFile("always_pass.lua", test_helpers::rule_code::always_pass());
        CreateRuleFile("always_fail.lua", test_helpers::rule_code::always_fail());
        CreateRuleFile("age_check.lua", test_helpers::rule_code::age_check());
        CreateRuleFile("field_complete.lua", test_helpers::rule_code::field_complete());
        CreateRuleFile("throws_error.lua", test_helpers::rule_code::throws_error());
        CreateRuleFile("no_match.lua", test_helpers::rule_code::no_match_function());
        CreateRuleFile("syntax_error.lua", test_helpers::lua_code::syntax_error());
    }

    void CreateRuleFile(const std::string& filename, const std::string& content) {
        // 确保目录存在
        system("mkdir -p test_data/rules");
        std::ofstream file("test_data/rules/" + filename);
        file << content;
        file.close();  // 必须关闭文件，确保内容写入磁盘
    }

    void CreateConfigFile(const std::string& filename, const std::string& content) {
        // 确保目录存在
        system("mkdir -p test_data/configs");
        std::ofstream file("test_data/configs/" + filename);
        file << content;
        file.close();  // 必须关闭文件，确保内容写入磁盘
    }

    json CreateTestData(const std::string& username, int age, const std::string& email, const std::string& phone) {
        return {
            {"username", username},
            {"age", age},
            {"email", email},
            {"phone", phone}
        };
    }
};

TEST_F(RuleEngineTest, DefaultConstructor_CreatesValidEngine) {
    RuleEngine engine;

    EXPECT_EQ(engine.get_rule_count(), 0);
    EXPECT_FALSE(engine.has_rule("test"));
    EXPECT_TRUE(engine.get_all_rules().empty());
}

// ============================================================================
// RuleEngine 规则加载测试
// ============================================================================

TEST_F(RuleEngineTest, AddRule_ValidRule_Success) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    EXPECT_TRUE(error.empty());

    EXPECT_EQ(engine.get_rule_count(), 1);
    EXPECT_TRUE(engine.has_rule("rule1"));

    auto rules = engine.get_all_rules();
    ASSERT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].name, "rule1");
    EXPECT_EQ(rules[0].file_path, "test_data/rules/always_pass.lua");
    EXPECT_TRUE(rules[0].loaded);
}

TEST_F(RuleEngineTest, AddRule_DuplicateName_Fails) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    EXPECT_FALSE(engine.add_rule("rule1", "test_data/rules/always_fail.lua", &error));
    EXPECT_TRUE(error.find("already exists") != std::string::npos);
}

TEST_F(RuleEngineTest, AddRule_NonExistentFile_Fails) {
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.add_rule("rule1", "test_data/rules/nonexistent.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, AddRule_SyntaxError_Fails) {
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.add_rule("rule1", "test_data/rules/syntax_error.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, AddRule_NoMatchFunction_Fails) {
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.add_rule("rule1", "test_data/rules/no_match.lua", &error));
    EXPECT_TRUE(error.find("match") != std::string::npos || error.find("define") != std::string::npos);
}

TEST_F(RuleEngineTest, LoadRuleConfig_ValidConfig_LoadsAllRules) {
    // 创建配置文件
    CreateConfigFile("test_config.lua", R"(
return {
    { name = "pass_rule", file = "test_data/rules/always_pass.lua" },
    { name = "fail_rule", file = "test_data/rules/always_fail.lua" },
    { name = "age_rule", file = "test_data/rules/age_check.lua" }
}
)");

    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.load_rule_config("test_data/configs/test_config.lua", &error)) << "错误: " << error;
    EXPECT_TRUE(error.empty());

    EXPECT_EQ(engine.get_rule_count(), 3);
    EXPECT_TRUE(engine.has_rule("pass_rule"));
    EXPECT_TRUE(engine.has_rule("fail_rule"));
    EXPECT_TRUE(engine.has_rule("age_rule"));
}

TEST_F(RuleEngineTest, LoadRuleConfig_NonExistentFile_Fails) {
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.load_rule_config("test_data/configs/nonexistent.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, LoadRuleConfig_InvalidFormat_Fails) {
    // 创建无效配置文件
    CreateConfigFile("invalid_config.lua", "this is not a valid config");

    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.load_rule_config("test_data/configs/invalid_config.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, LoadRuleConfig_EmptyConfig_LoadsNoRules) {
    CreateConfigFile("empty_config.lua", "return {}\n");

    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.load_rule_config("test_data/configs/empty_config.lua", &error));
    EXPECT_EQ(engine.get_rule_count(), 0);
}

TEST_F(RuleEngineTest, LoadRuleConfig_DuplicateRuleNames_Fails) {
    CreateConfigFile("duplicate_config.lua", R"(
return {
    { name = "rule1", file = "test_data/rules/always_pass.lua" },
    { name = "rule1", file = "test_data/rules/always_fail.lua" }
}
)");

    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.load_rule_config("test_data/configs/duplicate_config.lua", &error));
    EXPECT_TRUE(error.find("already exists") != std::string::npos);
}

// ============================================================================
// RuleEngine 规则管理测试
// ============================================================================

TEST_F(RuleEngineTest, RemoveRule_ExistingRule_Success) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_EQ(engine.get_rule_count(), 1);

    EXPECT_TRUE(engine.remove_rule("rule1"));
    EXPECT_EQ(engine.get_rule_count(), 0);
    EXPECT_FALSE(engine.has_rule("rule1"));
}

TEST_F(RuleEngineTest, RemoveRule_NonExistentRule_Fails) {
    RuleEngine engine;

    EXPECT_FALSE(engine.remove_rule("nonexistent"));
}

TEST_F(RuleEngineTest, ClearRules_RemovesAllRules) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule3", "test_data/rules/age_check.lua", &error));
    ASSERT_EQ(engine.get_rule_count(), 3);

    engine.clear_rules();

    EXPECT_EQ(engine.get_rule_count(), 0);
    EXPECT_FALSE(engine.has_rule("rule1"));
    EXPECT_FALSE(engine.has_rule("rule2"));
    EXPECT_FALSE(engine.has_rule("rule3"));
}

TEST_F(RuleEngineTest, GetRuleCount_AfterAddRemove) {
    RuleEngine engine;
    std::string error;

    EXPECT_EQ(engine.get_rule_count(), 0);

    engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error);
    EXPECT_EQ(engine.get_rule_count(), 1);

    engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error);
    EXPECT_EQ(engine.get_rule_count(), 2);

    engine.remove_rule("rule1");
    EXPECT_EQ(engine.get_rule_count(), 1);

    engine.clear_rules();
    EXPECT_EQ(engine.get_rule_count(), 0);
}

// ============================================================================
// RuleEngine 规则热重载测试
// ============================================================================

TEST_F(RuleEngineTest, ReloadRule_ExistingRule_Success) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_rule", "test_data/rules/age_check.lua", &error));

    EXPECT_TRUE(engine.reload_rule("age_rule", &error));
    EXPECT_TRUE(error.empty());

    // 规则应该仍然存在
    EXPECT_TRUE(engine.has_rule("age_rule"));
}

TEST_F(RuleEngineTest, ReloadRule_NonExistentRule_Fails) {
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.reload_rule("nonexistent", &error));
    EXPECT_TRUE(error.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, ReloadRule_ModifiedFile_UsesNewLogic) {
    // 创建一个临时规则文件
    std::ofstream rule_file("test_data/rules/reload_test.lua");
    rule_file << R"(
function match(data)
    return true, "version 1"
end
)";
    rule_file.close();

    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("test_rule", "test_data/rules/reload_test.lua", &error));

    // 测试版本 1
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);
    MatchResult result;

    ASSERT_TRUE(engine.match_rule("test_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "version 1");

    // 修改规则文件
    std::ofstream rule_file2("test_data/rules/reload_test.lua");
    rule_file2 << R"(
function match(data)
    return false, "version 2"
end
)";
    rule_file2.close();

    // 重新加载
    ASSERT_TRUE(engine.reload_rule("test_rule", &error));

    // 测试版本 2
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("test_rule", adapter, result2, &error));
    EXPECT_FALSE(result2.matched);
    EXPECT_EQ(result2.message, "version 2");
}

// ============================================================================
// RuleEngine 规则匹配测试
// ============================================================================

TEST_F(RuleEngineTest, MatchRule_PassingRule_ReturnsTrue) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass_rule", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("pass_rule", adapter, result, &error));

    EXPECT_TRUE(result.matched);
    EXPECT_FALSE(result.message.empty());
}

TEST_F(RuleEngineTest, MatchRule_FailingRule_ReturnsFalse) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("fail_rule", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("fail_rule", adapter, result, &error));

    EXPECT_FALSE(result.matched);
    EXPECT_FALSE(result.message.empty());
}

TEST_F(RuleEngineTest, MatchRule_NonExistentRule_Fails) {
    RuleEngine engine;
    std::string error;

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("nonexistent", adapter, result, &error));
    EXPECT_TRUE(error.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_AgeCheck_ValidAge_Passes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_rule", "test_data/rules/age_check.lua", &error));

    json data = CreateTestData("alice", 25, "alice@example.com", "1234567890");
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_rule", adapter, result, &error));

    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("通过") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_AgeCheck_Under18_Fails) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_rule", "test_data/rules/age_check.lua", &error));

    json data = CreateTestData("bob", 15, "bob@example.com", "1234567890");
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_rule", adapter, result, &error));

    EXPECT_FALSE(result.matched);
    EXPECT_TRUE(result.message.find("年龄不足") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_AgeCheck_MissingAge_Fails) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_rule", "test_data/rules/age_check.lua", &error));

    json data = {{"name", "charlie"}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_rule", adapter, result, &error));

    EXPECT_FALSE(result.matched);
    EXPECT_TRUE(result.message.find("缺少age字段") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_FieldComplete_AllFields_Passes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_rule", "test_data/rules/field_complete.lua", &error));

    // field_complete 规则检查的是 name, email, phone 字段
    json data = {
        {"name", "dave"},
        {"email", "dave@example.com"},
        {"phone", "9876543210"}
    };
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_rule", adapter, result, &error));

    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("通过") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_FieldComplete_MissingFields_Fails) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_rule", "test_data/rules/field_complete.lua", &error));

    json data = {{"name", "eve"}};  // 缺少 email 和 phone
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_rule", adapter, result, &error));

    EXPECT_FALSE(result.matched);
    EXPECT_TRUE(result.message.find("缺少必填字段") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_ThrowingError_Fails) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("error_rule", "test_data/rules/throws_error.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("error_rule", adapter, result, &error));
    EXPECT_FALSE(error.empty());
}

// ============================================================================
// RuleEngine 批量匹配测试
// ============================================================================

TEST_F(RuleEngineTest, MatchAllRules_AllPass_ReturnsTrue) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("pass2", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);
    EXPECT_TRUE(results.at("pass1").matched);
    EXPECT_TRUE(results.at("pass2").matched);
}

TEST_F(RuleEngineTest, MatchAllRules_SomeFail_ReturnsFalse) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);
    EXPECT_TRUE(results.at("pass").matched);
    EXPECT_FALSE(results.at("fail").matched);
}

TEST_F(RuleEngineTest, MatchAllRules_AllFail_ReturnsFalse) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("fail1", "test_data/rules/always_fail.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail2", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);
    EXPECT_FALSE(results.at("fail1").matched);
    EXPECT_FALSE(results.at("fail2").matched);
}

TEST_F(RuleEngineTest, MatchAllRules_NoRules_ReturnsTrue) {
    RuleEngine engine;

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    EXPECT_TRUE(engine.match_all_rules(adapter, results));
    EXPECT_TRUE(results.empty());
}

TEST_F(RuleEngineTest, MatchAllRules_ComplexScenario) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age", "test_data/rules/age_check.lua", &error));
    ASSERT_TRUE(engine.add_rule("field", "test_data/rules/field_complete.lua", &error));

    // 有效数据 - field_complete 规则需要 name, email, phone 字段
    json valid_data = {
        {"age", 35},
        {"name", "frank"},
        {"email", "frank@example.com"},
        {"phone", "5555555555"}
    };
    JsonAdapter valid_adapter(valid_data);

    std::map<std::string, MatchResult> results;
    EXPECT_TRUE(engine.match_all_rules(valid_adapter, results, &error));

    ASSERT_EQ(results.size(), 2);
    EXPECT_TRUE(results.at("age").matched);
    EXPECT_TRUE(results.at("field").matched);

    // 无效数据（年龄不足）
    json invalid_data1 = {
        {"age", 16},
        {"name", "grace"},
        {"email", "grace@example.com"},
        {"phone", "5555555555"}
    };
    JsonAdapter invalid_adapter1(invalid_data1);

    std::map<std::string, MatchResult> results2;
    EXPECT_FALSE(engine.match_all_rules(invalid_adapter1, results2, &error));

    ASSERT_EQ(results2.size(), 2);
    EXPECT_TRUE(results2.at("field").matched);   // field_complete 通过
    EXPECT_FALSE(results2.at("age").matched);    // age_check 失败（年龄不足）

    // 无效数据（缺少字段）
    json invalid_data2 = {{"name", "henry"}, {"age", 40}};
    JsonAdapter invalid_adapter2(invalid_data2);

    std::map<std::string, MatchResult> results3;
    EXPECT_FALSE(engine.match_all_rules(invalid_adapter2, results3, &error));

    ASSERT_EQ(results3.size(), 2);
    EXPECT_TRUE(results3.at("age").matched);      // age_check 通过
    EXPECT_FALSE(results3.at("field").matched);   // field_complete 失败（缺少字段）
}

// ============================================================================
// RuleEngine 边界条件和压力测试
// ============================================================================

TEST_F(RuleEngineTest, MultipleEngines_WorkIndependently) {
    RuleEngine engine1;
    RuleEngine engine2;
    std::string error;

    ASSERT_TRUE(engine1.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine2.add_rule("rule2", "test_data/rules/always_fail.lua", &error));

    EXPECT_EQ(engine1.get_rule_count(), 1);
    EXPECT_EQ(engine2.get_rule_count(), 1);

    EXPECT_TRUE(engine1.has_rule("rule1"));
    EXPECT_FALSE(engine2.has_rule("rule1"));

    EXPECT_TRUE(engine2.has_rule("rule2"));
    EXPECT_FALSE(engine1.has_rule("rule2"));
}

TEST_F(RuleEngineTest, LargeNumberOfRules_HandlesCorrectly) {
    RuleEngine engine;
    std::string error;

    // 创建多个规则文件
    for (int i = 0; i < 20; ++i) {
        std::string filename = "test_data/rules/rule_" + std::to_string(i) + ".lua";
        std::ofstream file(filename);
        file << test_helpers::rule_code::always_pass();
        file.close();  // 必须关闭文件，否则内容可能还未写入磁盘

        std::string rule_name = "rule_" + std::to_string(i);
        ASSERT_TRUE(engine.add_rule(rule_name, filename, &error));
    }

    EXPECT_EQ(engine.get_rule_count(), 20);

    auto all_rules = engine.get_all_rules();
    EXPECT_EQ(all_rules.size(), 20);
}

TEST_F(RuleEngineTest, SpecialCharactersInRuleName_HandlesCorrectly) {
    RuleEngine engine;
    std::string error;

    // 创建规则文件
    std::ofstream file("test_data/rules/special_rule.lua");
    file << test_helpers::rule_code::always_pass();
    file.close();  // 必须关闭文件，否则内容可能还未写入磁盘

    // 使用包含特殊字符的规则名
    std::string rule_name = "rule_with_特殊字符_123";
    ASSERT_TRUE(engine.add_rule(rule_name, "test_data/rules/special_rule.lua", &error));

    EXPECT_TRUE(engine.has_rule(rule_name));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule(rule_name, adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, GetRuleInfo_ReturnsCorrectInfo) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("test_rule", "test_data/rules/age_check.lua", &error));

    auto rules = engine.get_all_rules();

    ASSERT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].name, "test_rule");
    EXPECT_EQ(rules[0].file_path, "test_data/rules/age_check.lua");
    EXPECT_TRUE(rules[0].loaded);
}

TEST_F(RuleEngineTest, AddAndRemoveMultipleRules_MaintainsCorrectState) {
    RuleEngine engine;
    std::string error;

    // 添加多个规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule3", "test_data/rules/age_check.lua", &error));

    EXPECT_EQ(engine.get_rule_count(), 3);

    // 移除中间的规则
    ASSERT_TRUE(engine.remove_rule("rule2"));

    EXPECT_EQ(engine.get_rule_count(), 2);
    EXPECT_TRUE(engine.has_rule("rule1"));
    EXPECT_FALSE(engine.has_rule("rule2"));
    EXPECT_TRUE(engine.has_rule("rule3"));

    // 获取规则列表
    auto rules = engine.get_all_rules();
    EXPECT_EQ(rules.size(), 2);

    // 验证规则名称（顺序可能不确定）
    std::vector<std::string> rule_names;
    for (const auto& rule : rules) {
        rule_names.push_back(rule.name);
    }
    EXPECT_TRUE(rule_names.end() != std::find(rule_names.begin(), rule_names.end(), "rule1"));
    EXPECT_TRUE(rule_names.end() != std::find(rule_names.begin(), rule_names.end(), "rule3"));
}

// ============================================================================
// RuleEngine 错误处理测试
// ============================================================================

TEST_F(RuleEngineTest, MatchRule_WithoutErrorMsg_DoesNotCrash) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    // 不传递 error_msg，不应该崩溃
    EXPECT_TRUE(engine.match_rule("pass", adapter, result));
}

TEST_F(RuleEngineTest, AddRule_WithoutErrorMsg_DoesNotCrash) {
    RuleEngine engine;

    // 不传递 error_msg，不应该崩溃
    EXPECT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua"));
}

TEST_F(RuleEngineTest, LoadRuleConfig_WithoutErrorMsg_DoesNotCrash) {
    // 创建配置文件
    CreateConfigFile("no_error_test.lua", R"(
return {
    { name = "pass", file = "test_data/rules/always_pass.lua" }
}
)");

    RuleEngine engine;

    // 不传递 error_msg，不应该崩溃
    EXPECT_TRUE(engine.load_rule_config("test_data/configs/no_error_test.lua"));
}

TEST_F(RuleEngineTest, ReloadRule_WithoutErrorMsg_DoesNotCrash) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));

    // 不传递 error_msg，不应该崩溃
    EXPECT_TRUE(engine.reload_rule("pass"));
}

TEST_F(RuleEngineTest, MatchAllRules_WithoutErrorMsg_DoesNotCrash) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 不传递 error_msg，不应该崩溃
    EXPECT_TRUE(engine.match_all_rules(adapter, results));
}

// ============================================================================
// RuleEngine 消息内容测试
// ============================================================================

TEST_F(RuleEngineTest, MatchRule_MessageContent_IsCorrect) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age", "test_data/rules/age_check.lua", &error));

    // 测试通过时的消息
    json valid_data = {{"age", 25}};
    JsonAdapter valid_adapter(valid_data);

    MatchResult pass_result;
    ASSERT_TRUE(engine.match_rule("age", valid_adapter, pass_result, &error));
    EXPECT_FALSE(pass_result.message.empty());

    // 测试失败时的消息
    json invalid_data = {{"age", 15}};
    JsonAdapter invalid_adapter(invalid_data);

    MatchResult fail_result;
    ASSERT_TRUE(engine.match_rule("age", invalid_adapter, fail_result, &error));
    EXPECT_FALSE(fail_result.message.empty());
    EXPECT_TRUE(fail_result.message.find("15") != std::string::npos);
}
