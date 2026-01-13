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
    // 有一个规则通过，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(invalid_adapter1, results2, &error));

    ASSERT_EQ(results2.size(), 2);
    EXPECT_TRUE(results2.at("field").matched);   // field_complete 通过
    EXPECT_FALSE(results2.at("age").matched);    // age_check 失败（年龄不足）

    // 无效数据（缺少字段）
    json invalid_data2 = {{"name", "henry"}, {"age", 40}};
    JsonAdapter invalid_adapter2(invalid_data2);

    std::map<std::string, MatchResult> results3;
    // 有一个规则通过，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(invalid_adapter2, results3, &error));

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

// ============================================================================
// RuleEngine 边界场景和非法状态测试
// ============================================================================

TEST_F(RuleEngineTest, LoadRuleConfig_InvalidLuaFormat_Fails) {
    RuleEngine engine;
    std::string error;

    // 创建一个格式错误的配置文件
    test_helpers::TestDataFile invalid_config("invalid_config.lua", R"(
        this is not valid lua at all!!@#
    )");

    EXPECT_FALSE(engine.load_rule_config(invalid_config.path().c_str(), &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, LoadRuleConfig_ConfigReturnsNil_Fails) {
    RuleEngine engine;
    std::string error;

    // 配置文件返回 nil
    test_helpers::TestDataFile nil_config("nil_config.lua", R"(
        return nil
    )");

    EXPECT_FALSE(engine.load_rule_config(nil_config.path().c_str(), &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, LoadRuleConfig_ConfigReturnsNonTable_Fails) {
    RuleEngine engine;
    std::string error;

    // 配置文件返回非 table 值
    test_helpers::TestDataFile string_config("string_config.lua", R"(
        return "just a string"
    )");

    EXPECT_FALSE(engine.load_rule_config(string_config.path().c_str(), &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, LoadRuleConfig_MissingNameField_Fails) {
    RuleEngine engine;
    std::string error;

    // 缺少 name 字段
    test_helpers::TestDataFile no_name_config("no_name_config.lua", R"(
        return {
            { file = "test_data/rules/always_pass.lua" }
        }
    )");

    EXPECT_FALSE(engine.load_rule_config(no_name_config.path().c_str(), &error));
    EXPECT_TRUE(error.find("name") != std::string::npos || error.find("field") != std::string::npos);
}

TEST_F(RuleEngineTest, LoadRuleConfig_MissingFileField_Fails) {
    RuleEngine engine;
    std::string error;

    // 缺少 file 字段
    test_helpers::TestDataFile no_file_config("no_file_config.lua", R"(
        return {
            { name = "rule1" }
        }
    )");

    EXPECT_FALSE(engine.load_rule_config(no_file_config.path().c_str(), &error));
    EXPECT_TRUE(error.find("file") != std::string::npos || error.find("field") != std::string::npos);
}

TEST_F(RuleEngineTest, LoadRuleConfig_NonExistentRuleFile_Fails) {
    RuleEngine engine;
    std::string error;

    // 规则文件不存在
    test_helpers::TestDataFile bad_ref_config("bad_ref_config.lua", R"(
        return {
            { name = "rule1", file = "nonexistent_rule.lua" }
        }
    )");

    EXPECT_FALSE(engine.load_rule_config(bad_ref_config.path().c_str(), &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, LoadRuleConfig_EmptyConfig_Succeeds) {
    RuleEngine engine;
    std::string error;

    // 空配置
    test_helpers::TestDataFile empty_config("empty_config.lua", R"(
        return {}
    )");

    EXPECT_TRUE(engine.load_rule_config(empty_config.path().c_str(), &error));
    EXPECT_EQ(engine.get_rule_count(), 0);
}

// ============================================================================
// RuleEngine 非法状态测试
// ============================================================================

// 测试用类：通过继承访问 protected 方法来模拟非法 Lua 状态
class RuleEngineInvalidStateTest : public RuleEngine {
public:
    void invalidate_lua_state() {
        // 通过移动使 Lua 状态无效
        LuaState temp = std::move(get_lua_state());
        // temp 析构时销毁 Lua 状态，get_lua_state() 返回的引用现在指向无效状态
    }
};

TEST_F(RuleEngineTest, LoadRuleConfig_InvalidState_Fails) {
    RuleEngineInvalidStateTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试加载配置应该失败
    EXPECT_FALSE(engine.load_rule_config("test_data/configs/valid_config.lua", &error));
    EXPECT_TRUE(error.find("invalid") != std::string::npos ||
                error.find("null") != std::string::npos);
}

TEST_F(RuleEngineTest, AddRule_InvalidState_Fails) {
    RuleEngineInvalidStateTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试添加规则应该失败
    EXPECT_FALSE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    EXPECT_TRUE(error.find("invalid") != std::string::npos ||
                error.find("null") != std::string::npos);
}

TEST_F(RuleEngineTest, RemoveRule_InvalidState_Fails) {
    RuleEngineInvalidStateTest engine;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试移除规则应该失败
    EXPECT_FALSE(engine.remove_rule("rule1"));
}

TEST_F(RuleEngineTest, ReloadRule_InvalidState_Fails) {
    RuleEngineInvalidStateTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试重新加载规则应该失败
    EXPECT_FALSE(engine.reload_rule("rule1", &error));
    EXPECT_TRUE(error.find("invalid") != std::string::npos ||
                error.find("null") != std::string::npos ||
                error.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_InvalidState_Fails) {
    RuleEngineInvalidStateTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 准备测试数据
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    // 尝试匹配规则应该失败
    EXPECT_FALSE(engine.match_rule("rule1", adapter, result, &error));
    EXPECT_TRUE(error.find("invalid") != std::string::npos ||
                error.find("null") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_InvalidState_Fails) {
    RuleEngineInvalidStateTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 准备测试数据
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 尝试匹配所有规则应该失败
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));
    EXPECT_TRUE(error.find("invalid") != std::string::npos ||
                error.find("null") != std::string::npos);
}

// ============================================================================
// RuleEngine 其他边界场景和非法状态测试
// ============================================================================

TEST_F(RuleEngineTest, AddRule_EmptyFilePath_Fails) {
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.add_rule("rule1", "", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, AddRule_InvalidRuleFile_MissingMatchFunction_Fails) {
    RuleEngine engine;
    std::string error;

    // 没有匹配函数的规则文件
    test_helpers::TestDataFile no_match_func("no_match.lua", R"(
        local x = 10
        -- 没有 match 函数
    )");

    EXPECT_FALSE(engine.add_rule("rule1", no_match_func.path().c_str(), &error));
    EXPECT_TRUE(error.find("match") != std::string::npos ||
                error.find("function") != std::string::npos);
}

TEST_F(RuleEngineTest, AddRule_DuplicateRuleName_Fails) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 尝试添加同名规则
    EXPECT_FALSE(engine.add_rule("rule1", "test_data/rules/always_fail.lua", &error));
    EXPECT_TRUE(error.find("exists") != std::string::npos ||
                error.find("already") != std::string::npos);
}

TEST_F(RuleEngineTest, RemoveRule_Success) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    EXPECT_TRUE(engine.has_rule("rule1"));

    EXPECT_TRUE(engine.remove_rule("rule1"));
    EXPECT_FALSE(engine.has_rule("rule1"));
}

TEST_F(RuleEngineTest, ReloadRule_ChangesBehavior) {
    RuleEngine engine;
    std::string error;

    // 创建一个规则文件
    test_helpers::TestDataFile rule_file("dynamic_rule.lua", R"(
        function match(data)
            if data.value > 10 then
                return true, "value is greater than 10"
            else
                return false, "value is not greater than 10"
            end
        end
    )");

    ASSERT_TRUE(engine.add_rule("dynamic", rule_file.path().c_str(), &error));

    // 测试原始行为
    json data1 = {{"value", 15}};
    JsonAdapter adapter1(data1);
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("dynamic", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 修改规则文件
    test_helpers::TestDataFile new_rule_file("dynamic_rule.lua", R"(
        function match(data)
            if data.value > 20 then
                return true, "value is greater than 20"
            else
                return false, "value is not greater than 20"
            end
        end
    )");

    // 重新加载规则
    ASSERT_TRUE(engine.reload_rule("dynamic", &error));

    // 测试新行为
    json data2 = {{"value", 15}};
    JsonAdapter adapter2(data2);
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("dynamic", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched); // 现在应该失败，因为阈值改为 20
}

TEST_F(RuleEngineTest, MatchRule_RuleThrowsError_Fails) {
    RuleEngine engine;
    std::string error;

    // 创建一个会抛出错误的规则
    test_helpers::TestDataFile error_rule("error_rule.lua", R"(
        function match(data)
            error("This is an intentional error!")
        end
    )");

    ASSERT_TRUE(engine.add_rule("error_rule", error_rule.path().c_str(), &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("error_rule", adapter, result, &error));
    EXPECT_FALSE(error.empty());
}


TEST_F(RuleEngineTest, GetRuleCount_EmptyEngine_ReturnsZero) {
    RuleEngine engine;
    EXPECT_EQ(engine.get_rule_count(), 0);
}

TEST_F(RuleEngineTest, HasRule_NonExistentRule_ReturnsFalse) {
    RuleEngine engine;
    EXPECT_FALSE(engine.has_rule("nonexistent_rule"));
}

TEST_F(RuleEngineTest, ClearRules_EmptyEngine_DoesNotCrash) {
    RuleEngine engine;
    engine.clear_rules(); // 不应该崩溃
    EXPECT_EQ(engine.get_rule_count(), 0);
}

TEST_F(RuleEngineTest, ClearRules_NonEmptyEngine_ClearsAllRules) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));
    EXPECT_EQ(engine.get_rule_count(), 2);

    engine.clear_rules();
    EXPECT_EQ(engine.get_rule_count(), 0);
    EXPECT_FALSE(engine.has_rule("rule1"));
    EXPECT_FALSE(engine.has_rule("rule2"));
}

TEST_F(RuleEngineTest, GetRuleInfo_AfterOperations_IsCorrect) {
    RuleEngine engine;
    std::string error;

    // 初始状态
    auto rules = engine.get_all_rules();
    EXPECT_TRUE(rules.empty());

    // 添加规则后
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    rules = engine.get_all_rules();
    EXPECT_EQ(rules.size(), 1);
    EXPECT_EQ(rules[0].name, "rule1");

    // 移除规则后
    engine.remove_rule("rule1");
    rules = engine.get_all_rules();
    EXPECT_TRUE(rules.empty());

    // 清空规则后
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));
    engine.clear_rules();
    rules = engine.get_all_rules();
    EXPECT_TRUE(rules.empty());
}

// ============================================================================
// RuleEngine call_match_function 错误路径测试
// ============================================================================

// 用于测试的数据适配器：push_to_lua 总是失败
class FailingDataAdapter : public DataAdapter {
public:
    bool push_to_lua(lua_State*, std::string* error_msg) const override {
        if (error_msg) {
            *error_msg = "DataAdapter intentionally failed";
        }
        return false;
    }

    const char* get_type_name() const override { return "FailingDataAdapter"; }
};

// 用于测试的 RuleEngine 派生类，允许访问内部 Lua 状态
class RuleEngineInternalTest : public RuleEngine {
public:
    lua_State* get_internal_lua_state() {
        return get_lua_state().get();
    }

    // 删除 _rule_functions 表
    void delete_rule_functions_table() {
        lua_State* L = get_internal_lua_state();
        lua_pushnil(L);
        lua_setglobal(L, "_rule_functions");
    }

    // 从 _rule_functions 表中删除指定规则的函数
    void delete_rule_function(const std::string& rule_name) {
        lua_State* L = get_internal_lua_state();
        lua_getglobal(L, "_rule_functions");
        if (lua_istable(L, -1)) {
            lua_pushlstring(L, rule_name.data(), rule_name.size());
            lua_pushnil(L);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    // 将 _rule_functions 表设置为非 table 值
    void corrupt_rule_functions_table() {
        lua_State* L = get_internal_lua_state();
        lua_pushstring(L, "corrupted");
        lua_setglobal(L, "_rule_functions");
    }
};

TEST_F(RuleEngineTest, CallMatchFunction_RuleFunctionTableNotFound_ReturnsError) {
    RuleEngineInternalTest engine;
    std::string error;

    // 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 删除 _rule_functions 表
    engine.delete_rule_functions_table();

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("rule1", adapter, result, &error));
    EXPECT_EQ(error, "Rule function table not found");
}

TEST_F(RuleEngineTest, CallMatchFunction_RuleFunctionTableCorrupted_ReturnsError) {
    RuleEngineInternalTest engine;
    std::string error;

    // 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 将 _rule_functions 表设置为非 table 值
    engine.corrupt_rule_functions_table();

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("rule1", adapter, result, &error));
    EXPECT_EQ(error, "Rule function table not found");
}

TEST_F(RuleEngineTest, CallMatchFunction_MatchFunctionNotFound_ReturnsError) {
    RuleEngineInternalTest engine;
    std::string error;

    // 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 从 _rule_functions 表中删除该规则的函数
    engine.delete_rule_function("rule1");

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("rule1", adapter, result, &error));
    EXPECT_EQ(error, "Rule 'rule1' match function not found");
}

TEST_F(RuleEngineTest, CallMatchFunction_DataAdapterPushFails_ReturnsError) {
    RuleEngine engine;
    std::string error;

    // 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 使用会失败的数据适配器
    FailingDataAdapter adapter;

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("rule1", adapter, result, &error));
    EXPECT_EQ(error, "DataAdapter intentionally failed");
}

TEST_F(RuleEngineTest, CallMatchFunction_FirstReturnValueNotBoolean_ReturnsError) {
    RuleEngine engine;
    std::string error;

    // 创建一个返回非布尔值作为第一个返回值的规则
    test_helpers::TestDataFile invalid_return_rule("invalid_return.lua", R"(
        function match(data)
            -- 第一个返回值是字符串而不是布尔值
            return "invalid", "error message"
        end
    )");

    ASSERT_TRUE(engine.add_rule("bad_rule", invalid_return_rule.path().c_str(), &error));

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("bad_rule", adapter, result, &error));
    EXPECT_EQ(error, "First return value of 'match' must be boolean");
}

TEST_F(RuleEngineTest, CallMatchFunction_ReturnsNumberAsFirstValue_ReturnsError) {
    RuleEngine engine;
    std::string error;

    // 创建一个返回数字作为第一个返回值的规则
    test_helpers::TestDataFile number_return_rule("number_return.lua", R"(
        function match(data)
            -- 第一个返回值是数字
            return 42, "error message"
        end
    )");

    ASSERT_TRUE(engine.add_rule("number_rule", number_return_rule.path().c_str(), &error));

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("number_rule", adapter, result, &error));
    EXPECT_EQ(error, "First return value of 'match' must be boolean");
}

TEST_F(RuleEngineTest, CallMatchFunction_ReturnsNilAsFirstValue_ReturnsError) {
    RuleEngine engine;
    std::string error;

    // 创建一个返回 nil 作为第一个返回值的规则
    test_helpers::TestDataFile nil_return_rule("nil_return.lua", R"(
        function match(data)
            -- 第一个返回值是 nil
            return nil, "error message"
        end
    )");

    ASSERT_TRUE(engine.add_rule("nil_rule", nil_return_rule.path().c_str(), &error));

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("nil_rule", adapter, result, &error));
    EXPECT_EQ(error, "First return value of 'match' must be boolean");
}

TEST_F(RuleEngineTest, CallMatchFunction_ReturnsTableAsFirstValue_ReturnsError) {
    RuleEngine engine;
    std::string error;

    // 创建一个返回 table 作为第一个返回值的规则
    test_helpers::TestDataFile table_return_rule("table_return.lua", R"(
        function match(data)
            -- 第一个返回值是 table
            return {result = true}, "error message"
        end
    )");

    ASSERT_TRUE(engine.add_rule("table_rule", table_return_rule.path().c_str(), &error));

    // 尝试匹配规则
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("table_rule", adapter, result, &error));
    EXPECT_EQ(error, "First return value of 'match' must be boolean");
}

TEST_F(RuleEngineTest, CallMatchFunction_OnlyOneReturnValue_WorksCorrectly) {
    RuleEngine engine;
    std::string error;

    // 创建一个只返回一个值的规则（第二个返回值可选）
    test_helpers::TestDataFile single_return_rule("single_return.lua", R"(
        function match(data)
            -- 只返回布尔值
            return true
        end
    )");

    ASSERT_TRUE(engine.add_rule("single_rule", single_return_rule.path().c_str(), &error));

    // 尝试匹配规则（应该成功）
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    EXPECT_TRUE(engine.match_rule("single_rule", adapter, result, &error));
    EXPECT_TRUE(error.empty());
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.empty());  // 没有第二个返回值，message 应该为空
}

// ============================================================================
// RuleEngine JIT 控制测试
// ============================================================================

TEST_F(RuleEngineTest, EnableJit_JitIsEnabled) {
    RuleEngine engine;
    std::string error;

    // 创建一个简单规则
    CreateRuleFile("simple_check.lua", R"(
function match(data)
    return data["value"] > 10, "value check"
end
)");

    ASSERT_TRUE(engine.add_rule("simple_check", "test_data/rules/simple_check.lua", &error));

    // 启用 JIT
    engine.enable_jit();

    // 执行规则匹配
    json data = {{"value", 15}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("simple_check", adapter, result, &error));

    EXPECT_TRUE(result.matched);
    EXPECT_STREQ(result.message.c_str(), "value check");
}

TEST_F(RuleEngineTest, DisableJit_JitIsDisabled) {
    RuleEngine engine;
    std::string error;

    // 创建一个计算密集型规则
    CreateRuleFile("compute_intensive.lua", R"(
function match(data)
    local sum = 0
    for i = 1, 100 do
        sum = sum + i
    end
    return sum == 5050, "computation complete"
end
)");

    ASSERT_TRUE(engine.add_rule("compute", "test_data/rules/compute_intensive.lua", &error));

    // 禁用 JIT
    engine.disable_jit();

    // 执行规则匹配（应该仍然工作，只是没有 JIT 优化）
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("compute", adapter, result, &error));

    EXPECT_TRUE(result.matched);
    EXPECT_STREQ(result.message.c_str(), "computation complete");
}

TEST_F(RuleEngineTest, ToggleJit_MultipleTimes_WorksCorrectly) {
    RuleEngine engine;
    std::string error;

    // 创建一个简单规则
    CreateRuleFile("toggle_test.lua", R"(
function match(data)
    return data["status"] == "active", "status check"
end
)");

    ASSERT_TRUE(engine.add_rule("toggle_test", "test_data/rules/toggle_test.lua", &error));

    json data = {{"status", "active"}};
    JsonAdapter adapter(data);

    // 测试启用 JIT
    engine.enable_jit();
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("toggle_test", adapter, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 测试禁用 JIT
    engine.disable_jit();
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("toggle_test", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);

    // 再次启用 JIT
    engine.enable_jit();
    MatchResult result3;
    ASSERT_TRUE(engine.match_rule("toggle_test", adapter, result3, &error));
    EXPECT_TRUE(result3.matched);

    // 再次禁用 JIT
    engine.disable_jit();
    MatchResult result4;
    ASSERT_TRUE(engine.match_rule("toggle_test", adapter, result4, &error));
    EXPECT_TRUE(result4.matched);
}

TEST_F(RuleEngineTest, JitStatus_AffectsPerformance) {
    RuleEngine engine;
    std::string error;

    // 创建一个循环密集型规则，用于观察 JIT 性能差异
    CreateRuleFile("loop_test.lua", R"(
function match(data)
    local count = 0
    for i = 1, 1000 do
        count = count + 1
    end
    return count == 1000, "loop completed"
end
)");

    ASSERT_TRUE(engine.add_rule("loop_test", "test_data/rules/loop_test.lua", &error));

    json data = {{"test", "data"}};
    JsonAdapter adapter(data);

    // 使用 JIT 执行
    engine.enable_jit();
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("loop_test", adapter, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 不使用 JIT 执行
    engine.disable_jit();
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("loop_test", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);

    // 两种方式都应该得到相同的结果
    EXPECT_EQ(result1.matched, result2.matched);
    EXPECT_STREQ(result1.message.c_str(), result2.message.c_str());
}

TEST_F(RuleEngineTest, EnableJit_MultipleRules_AllBenefit) {
    RuleEngine engine;
    std::string error;

    // 创建多个规则
    CreateRuleFile("rule1.lua", R"(
function match(data)
    return data["value1"] > 0, "rule1 check"
end
)");

    CreateRuleFile("rule2.lua", R"(
function match(data)
    return data["value2"] > 0, "rule2 check"
end
)");

    CreateRuleFile("rule3.lua", R"(
function match(data)
    return data["value3"] > 0, "rule3 check"
end
)");

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/rule1.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/rule2.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule3", "test_data/rules/rule3.lua", &error));

    // 启用 JIT
    engine.enable_jit();

    // 批量匹配所有规则
    json data = {
        {"value1", 10},
        {"value2", 20},
        {"value3", 30}
    };
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    ASSERT_TRUE(engine.match_all_rules(adapter, results, &error));

    // 验证所有规则都通过
    EXPECT_EQ(results.size(), 3);
    EXPECT_TRUE(results.at("rule1").matched);
    EXPECT_TRUE(results.at("rule2").matched);
    EXPECT_TRUE(results.at("rule3").matched);
}

TEST_F(RuleEngineTest, DisableJit_MultipleRules_AllWork) {
    RuleEngine engine;
    std::string error;

    // 创建多个规则
    CreateRuleFile("rule1.lua", R"(
function match(data)
    return data["value1"] > 0, "rule1 check"
end
)");

    CreateRuleFile("rule2.lua", R"(
function match(data)
    return data["value2"] < 100, "rule2 check"
end
)");

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/rule1.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/rule2.lua", &error));

    // 禁用 JIT
    engine.disable_jit();

    // 批量匹配所有规则
    json data = {
        {"value1", 10},
        {"value2", 50}
    };
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    ASSERT_TRUE(engine.match_all_rules(adapter, results, &error));

    // 验证所有规则都通过（即使没有 JIT）
    EXPECT_EQ(results.size(), 2);
    EXPECT_TRUE(results.at("rule1").matched);
    EXPECT_TRUE(results.at("rule2").matched);
}

// ============================================================================
// RuleEngine flush_jit 测试
// ============================================================================

TEST_F(RuleEngineTest, FlushJit_ClearsJitCache) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("simple_check.lua", R"(
function match(data)
    return data["value"] > 10, "value check"
end
)");

    ASSERT_TRUE(engine.add_rule("simple_check", "test_data/rules/simple_check.lua", &error));

    // 先执行一次规则，让 JIT 编译
    json data = {{"value", 15}};
    JsonAdapter adapter(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("simple_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);

    // 刷新 JIT 缓存
    EXPECT_TRUE(engine.flush_jit());

    // 刷新后仍然可以正常执行规则
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("simple_check", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);
}

TEST_F(RuleEngineTest, FlushJit_MultipleCalls_AllSucceed) {
    RuleEngine engine;

    // 多次调用 flush_jit 应该都能成功
    EXPECT_TRUE(engine.flush_jit());
    EXPECT_TRUE(engine.flush_jit());
    EXPECT_TRUE(engine.flush_jit());
}

// ============================================================================
// RuleEngine JIT 控制 - 无效 Lua 状态测试
// ============================================================================

// 测试辅助类：用于模拟无效 Lua 状态的 RuleEngine
class RuleEngineWithInvalidState : public RuleEngine {
public:
    RuleEngineWithInvalidState() : RuleEngine() {
        // 获取 LuaState 并将其置为无效状态
        // 注意：这里我们通过移动语义"窃取"内部的 LuaState
        // 然后让原来的 RuleEngine 持有空状态
        _stolen_state = std::move(get_lua_state());
    }

    // 测试无效状态下的 JIT 控制方法
    bool test_enable_jit_with_invalid_state() {
        return enable_jit();
    }

    bool test_disable_jit_with_invalid_state() {
        return disable_jit();
    }

    bool test_flush_jit_with_invalid_state() {
        return flush_jit();
    }

private:
    LuaState _stolen_state;
};

TEST_F(RuleEngineTest, EnableJit_InvalidState_ReturnsFalse) {
    RuleEngineWithInvalidState engine;

    // 当 Lua 状态无效时，enable_jit 应该返回 false
    EXPECT_FALSE(engine.test_enable_jit_with_invalid_state());
}

TEST_F(RuleEngineTest, DisableJit_InvalidState_ReturnsFalse) {
    RuleEngineWithInvalidState engine;

    // 当 Lua 状态无效时，disable_jit 应该返回 false
    EXPECT_FALSE(engine.test_disable_jit_with_invalid_state());
}

TEST_F(RuleEngineTest, FlushJit_InvalidState_ReturnsFalse) {
    RuleEngineWithInvalidState engine;

    // 当 Lua 状态无效时，flush_jit 应该返回 false
    EXPECT_FALSE(engine.test_flush_jit_with_invalid_state());
}

// ============================================================================
// RuleEngine match_all_rules 修改后的行为测试
// ============================================================================

TEST_F(RuleEngineTest, MatchAllRules_EmptyRules_ReturnsTrue) {
    RuleEngine engine;

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    std::string error;

    // 空规则集合应该返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));
    EXPECT_TRUE(results.empty());
    EXPECT_TRUE(error.empty());
}

TEST_F(RuleEngineTest, MatchAllRules_AnyOnePass_ReturnsTrue) {
    RuleEngine engine;
    std::string error;

    // 添加一个通过规则和一个失败规则
    ASSERT_TRUE(engine.add_rule("pass_rule", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail_rule", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 只要有一个规则匹配成功就应该返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);
    EXPECT_TRUE(results.at("pass_rule").matched);
    EXPECT_FALSE(results.at("fail_rule").matched);
}

TEST_F(RuleEngineTest, MatchAllRules_AllPass_ReturnsTrue) {
    RuleEngine engine;
    std::string error;

    // 添加多个通过规则
    ASSERT_TRUE(engine.add_rule("pass1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("pass2", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("pass3", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 所有规则都通过，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 3);
    EXPECT_TRUE(results.at("pass1").matched);
    EXPECT_TRUE(results.at("pass2").matched);
    EXPECT_TRUE(results.at("pass3").matched);
}

TEST_F(RuleEngineTest, MatchAllRules_AllFail_ReturnsFalse) {
    RuleEngine engine;
    std::string error;

    // 添加多个失败规则
    ASSERT_TRUE(engine.add_rule("fail1", "test_data/rules/always_fail.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail2", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 所有规则都失败，应该返回 false
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);
    EXPECT_FALSE(results.at("fail1").matched);
    EXPECT_FALSE(results.at("fail2").matched);
}

TEST_F(RuleEngineTest, MatchAllRules_CallFailure_SetsMatchedFalse) {
    RuleEngine engine;
    std::string error;

    // 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));

    // 添加一个会抛出错误的规则
    test_helpers::TestDataFile error_rule("error_rule.lua", R"(
        function match(data)
            error("Intentional error in match")
        end
    )");
    ASSERT_TRUE(engine.add_rule("error_rule", error_rule.path().c_str(), &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 有一个规则匹配成功，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);

    // 第一个规则应该正常匹配
    EXPECT_TRUE(results.at("pass").matched);
    EXPECT_FALSE(results.at("pass").message.empty());

    // 第二个规则调用失败，应该设置为 matched = false 并包含错误信息
    EXPECT_FALSE(results.at("error_rule").matched);
    EXPECT_TRUE(results.at("error_rule").message.find("Failed to call match") != std::string::npos);
    EXPECT_TRUE(results.at("error_rule").message.find("Intentional error") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_AllCallFailure_ReturnsFalse) {
    RuleEngine engine;
    std::string error;

    // 添加两个都会抛出错误的规则
    test_helpers::TestDataFile error_rule1("error_rule1.lua", R"(
        function match(data)
            error("Error in rule 1")
        end
    )");

    test_helpers::TestDataFile error_rule2("error_rule2.lua", R"(
        function match(data)
            error("Error in rule 2")
        end
    )");

    ASSERT_TRUE(engine.add_rule("error1", error_rule1.path().c_str(), &error));
    ASSERT_TRUE(engine.add_rule("error2", error_rule2.path().c_str(), &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 所有规则都调用失败，应该返回 false
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 2);

    // 验证两个规则都被设置为失败状态
    EXPECT_FALSE(results.at("error1").matched);
    EXPECT_TRUE(results.at("error1").message.find("Failed to call match") != std::string::npos);
    EXPECT_TRUE(results.at("error1").message.find("Error in rule 1") != std::string::npos);

    EXPECT_FALSE(results.at("error2").matched);
    EXPECT_TRUE(results.at("error2").message.find("Failed to call match") != std::string::npos);
    EXPECT_TRUE(results.at("error2").message.find("Error in rule 2") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_ComplexMixedScenario) {
    RuleEngine engine;
    std::string error;

    // 添加混合类型的规则：通过的、失败的、调用错误的
    ASSERT_TRUE(engine.add_rule("pass1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail1", "test_data/rules/always_fail.lua", &error));

    test_helpers::TestDataFile error_rule("error_rule.lua", R"(
        function match(data)
            error("Runtime error")
        end
    )");
    ASSERT_TRUE(engine.add_rule("error_rule", error_rule.path().c_str(), &error));

    ASSERT_TRUE(engine.add_rule("pass2", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail2", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    std::map<std::string, MatchResult> results;
    // 有规则匹配成功，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    ASSERT_EQ(results.size(), 5);

    // 验证通过规则
    EXPECT_TRUE(results.at("pass1").matched);
    EXPECT_TRUE(results.at("pass2").matched);

    // 验证失败规则
    EXPECT_FALSE(results.at("fail1").matched);
    EXPECT_FALSE(results.at("fail2").matched);

    // 验证错误规则
    EXPECT_FALSE(results.at("error_rule").matched);
    EXPECT_TRUE(results.at("error_rule").message.find("Failed to call match") != std::string::npos);
}

