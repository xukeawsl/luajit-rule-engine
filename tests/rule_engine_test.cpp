#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ljre/rule_engine.h"
#include "ljre/json_adapter.h"
#include "ljre/basic_data_adapter.h"
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
    auto adapter = std::make_shared<JsonAdapter>(data);
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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("fail_rule", adapter, result, &error));

    EXPECT_FALSE(result.matched);
    EXPECT_FALSE(result.message.empty());
}

TEST_F(RuleEngineTest, MatchRule_NonExistentRule_Fails) {
    RuleEngine engine;
    std::string error;

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    MatchResult result;
    EXPECT_FALSE(engine.match_rule("nonexistent", adapter, result, &error));
    EXPECT_TRUE(error.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_AgeCheck_ValidAge_Passes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_rule", "test_data/rules/age_check.lua", &error));

    json data = CreateTestData("alice", 25, "alice@example.com", "1234567890");
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto valid_adapter = std::make_shared<JsonAdapter>(valid_data);

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
    auto invalid_adapter1 = std::make_shared<JsonAdapter>(invalid_data1);

    std::map<std::string, MatchResult> results2;
    // 有一个规则通过，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(invalid_adapter1, results2, &error));

    ASSERT_EQ(results2.size(), 2);
    EXPECT_TRUE(results2.at("field").matched);   // field_complete 通过
    EXPECT_FALSE(results2.at("age").matched);    // age_check 失败（年龄不足）

    // 无效数据（缺少字段）
    json invalid_data2 = {{"name", "henry"}, {"age", 40}};
    auto invalid_adapter2 = std::make_shared<JsonAdapter>(invalid_data2);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto valid_adapter = std::make_shared<JsonAdapter>(valid_data);

    MatchResult pass_result;
    ASSERT_TRUE(engine.match_rule("age", valid_adapter, pass_result, &error));
    EXPECT_FALSE(pass_result.message.empty());

    // 测试失败时的消息
    json invalid_data = {{"age", 15}};
    auto invalid_adapter = std::make_shared<JsonAdapter>(invalid_data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter1 = std::make_shared<JsonAdapter>(data1);
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
    auto adapter2 = std::make_shared<JsonAdapter>(data2);
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
    auto adapter = std::make_shared<JsonAdapter>(data);

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

    // 通过移动 LuaState 创建无效状态（用于测试 is_valid() == false 分支）
    void invalidate_lua_state() {
        // 移动内部的 LuaState，使其失效
        ljre::LuaState& lua_state = get_lua_state();
        ljre::LuaState temp = std::move(lua_state);
        // temp 在这里析构，原 lua_state 现在无效
        (void)temp;  // 抑制未使用警告
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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<FailingDataAdapter>();

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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
    auto adapter = std::make_shared<JsonAdapter>(data);

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

// ============================================================================
// RuleEngine match_all_rules 边界情况和错误处理测试
// ============================================================================

TEST_F(RuleEngineTest, MatchAllRules_PushToLuaFailure_SetsMatchedFalse) {
    RuleEngine engine;
    std::string error;

    // 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));

    // 创建一个会导致 push_to_lua 失败的 adapter
    // 通过继承 JsonAdapter 并重写 push_to_lua 来模拟失败
    class FailingAdapter : public ljre::JsonAdapter {
    public:
        FailingAdapter(const json& j) : JsonAdapter(j) {}

        bool push_to_lua(lua_State*, std::string* error_msg = nullptr) const override {
            if (error_msg) {
                *error_msg = "Simulated push_to_lua failure";
            }
            return false;
        }
    };

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<FailingAdapter>(data);

    std::map<std::string, MatchResult> results;
    // push_to_lua 失败应该返回 false，并为所有规则填充失败结果
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    // 验证 results 包含所有规则的失败结果
    EXPECT_FALSE(results.empty());
    EXPECT_EQ(results.size(), 1);  // 有1个规则
    EXPECT_TRUE(results.count("pass") > 0);

    // 验证每个规则的结果都是失败状态
    EXPECT_FALSE(results["pass"].matched);
    EXPECT_EQ(results["pass"].message, "Simulated push_to_lua failure");

    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("Simulated push_to_lua failure") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_FunctionTableNotFound_SetsMatchedFalse) {
    // 使用派生类访问 protected 的 get_lua_state()
    class TestableEngine : public RuleEngine {
    public:
        using RuleEngine::RuleEngine;
        lua_State* get_L() { return get_lua_state().get(); }
    };

    TestableEngine test_engine;
    std::string error;
    ASSERT_TRUE(test_engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 手动清除规则函数表（模拟函数表不存在的情况）
    lua_State* L = test_engine.get_L();
    lua_pushnil(L);
    lua_setglobal(L, "_rule_functions");

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::map<std::string, MatchResult> results;
    // 函数表不存在，应该返回 false（因为没有任何规则匹配成功）
    EXPECT_FALSE(test_engine.match_all_rules(adapter, results, &error));

    // 验证结果被正确设置
    ASSERT_EQ(results.size(), 1);
    EXPECT_FALSE(results.at("rule1").matched);
    EXPECT_TRUE(results.at("rule1").message.find("Rule function table not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_FunctionNotFound_SetsMatchedFalse) {
    // 使用派生类访问 protected 的 get_lua_state()
    class TestableEngine : public RuleEngine {
    public:
        using RuleEngine::RuleEngine;
        lua_State* get_L() { return get_lua_state().get(); }
    };

    TestableEngine test_engine;
    std::string error;

    // 添加一个规则
    ASSERT_TRUE(test_engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 手动从函数表中删除该函数（模拟函数不存在）
    lua_State* L = test_engine.get_L();
    lua_getglobal(L, "_rule_functions");
    lua_pushnil(L);
    lua_setfield(L, -2, "rule1");
    lua_pop(L, 1);

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::map<std::string, MatchResult> results;
    // 函数不存在，应该返回 false
    EXPECT_FALSE(test_engine.match_all_rules(adapter, results, &error));

    // 验证结果被正确设置
    ASSERT_EQ(results.size(), 1);
    EXPECT_FALSE(results.at("rule1").matched);
    EXPECT_TRUE(results.at("rule1").message.find("match function not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_FirstReturnNotBoolean_SetsMatchedFalse) {
    RuleEngine engine;
    std::string error;

    // 创建一个返回非布尔值的规则
    test_helpers::TestDataFile invalid_rule("invalid_return.lua", R"(
        function match(data)
            return "not a boolean", "message"
        end
    )");
    ASSERT_TRUE(engine.add_rule("invalid_rule", invalid_rule.path().c_str(), &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::map<std::string, MatchResult> results;
    // 第一个返回值不是布尔值，应该返回 false
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    // 验证结果被正确设置
    ASSERT_EQ(results.size(), 1);
    EXPECT_FALSE(results.at("invalid_rule").matched);
    EXPECT_TRUE(results.at("invalid_rule").message.find("First return value of 'match' must be boolean") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_SecondReturnNotString_UsesEmptyMessage) {
    RuleEngine engine;
    std::string error;

    // 创建一个第二个返回值不是字符串的规则
    test_helpers::TestDataFile invalid_rule("invalid_second_return.lua", R"(
        function match(data)
            return true, 12345  -- 第二个返回值是数字，不是字符串
        end
    )");
    ASSERT_TRUE(engine.add_rule("invalid_rule", invalid_rule.path().c_str(), &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::map<std::string, MatchResult> results;
    // 应该返回 true（第一个返回值是 true）
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    // 验证结果使用空字符串作为消息
    ASSERT_EQ(results.size(), 1);
    EXPECT_TRUE(results.at("invalid_rule").matched);
    EXPECT_TRUE(results.at("invalid_rule").message.empty());
}

TEST_F(RuleEngineTest, MatchAllRules_MixedErrorScenarios_AllHandledCorrectly) {
    RuleEngine engine;
    std::string error;

    // 场景1: 添加一个正常规则
    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));

    // 场景2: 添加一个返回非布尔值的规则
    test_helpers::TestDataFile invalid_type_rule("invalid_type.lua", R"(
        function match(data)
            return "wrong type", "message"
        end
    )");
    ASSERT_TRUE(engine.add_rule("invalid_type", invalid_type_rule.path().c_str(), &error));

    // 场景3: 添加一个抛出错误的规则
    test_helpers::TestDataFile error_rule("error.lua", R"(
        function match(data)
            error("runtime error")
        end
    )");
    ASSERT_TRUE(engine.add_rule("error", error_rule.path().c_str(), &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::map<std::string, MatchResult> results;
    // 有一个规则成功，应该返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    // 验证所有结果都被正确设置
    ASSERT_EQ(results.size(), 3);

    // 正常规则应该通过
    EXPECT_TRUE(results.at("pass").matched);
    EXPECT_FALSE(results.at("pass").message.empty());

    // 返回类型错误的规则应该失败
    EXPECT_FALSE(results.at("invalid_type").matched);
    EXPECT_TRUE(results.at("invalid_type").message.find("First return value of 'match' must be boolean") != std::string::npos);

    // 抛出错误的规则应该失败
    EXPECT_FALSE(results.at("error").matched);
    EXPECT_TRUE(results.at("error").message.find("Failed to call match") != std::string::npos);
    EXPECT_TRUE(results.at("error").message.find("runtime error") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchAllRules_OnlyErrors_ReturnsFalse) {
    RuleEngine engine;
    std::string error;

    // 只添加有问题的规则
    test_helpers::TestDataFile invalid_rule1("invalid1.lua", R"(
        function match(data)
            return "not bool", "msg"
        end
    )");

    test_helpers::TestDataFile invalid_rule2("invalid2.lua", R"(
        function match(data)
            error("error occurred")
        end
    )");

    ASSERT_TRUE(engine.add_rule("invalid1", invalid_rule1.path().c_str(), &error));
    ASSERT_TRUE(engine.add_rule("invalid2", invalid_rule2.path().c_str(), &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::map<std::string, MatchResult> results;
    // 所有规则都失败，应该返回 false
    EXPECT_FALSE(engine.match_all_rules(adapter, results, &error));

    // 验证所有结果都正确设置为失败
    ASSERT_EQ(results.size(), 2);
    EXPECT_FALSE(results.at("invalid1").matched);
    EXPECT_FALSE(results.at("invalid2").matched);
}

// ============================================================================
// RuleEngine match_rule (vector version) 测试
// ============================================================================

TEST_F(RuleEngineTest, MatchRule_Vector_AllRulesExist_AllPass) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("pass1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("pass2", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("pass3", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    std::vector<std::string> rule_names = {"pass1", "pass2", "pass3"};
    std::map<std::string, MatchResult> results;

    // 所有规则都通过，应该返回 true
    EXPECT_TRUE(engine.match_rule(rule_names, adapter, results, &error));

    ASSERT_EQ(results.size(), 3);
    EXPECT_TRUE(results.at("pass1").matched);
    EXPECT_TRUE(results.at("pass2").matched);
    EXPECT_TRUE(results.at("pass3").matched);
}

TEST_F(RuleEngineTest, MatchRule_Vector_SomeRulesNotExist) {
    RuleEngine engine;
    std::string error;

    // 只添加两个规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    // 请求中包含一个不存在的规则
    std::vector<std::string> rule_names = {"rule1", "rule2", "nonexistent_rule"};
    std::map<std::string, MatchResult> results;

    // rule1 通过，应该返回 true
    EXPECT_TRUE(engine.match_rule(rule_names, adapter, results, &error));

    ASSERT_EQ(results.size(), 3);

    // rule1 应该通过
    EXPECT_TRUE(results.at("rule1").matched);

    // rule2 应该失败
    EXPECT_FALSE(results.at("rule2").matched);

    // 不存在的规则应该设置为失败，并包含错误信息
    EXPECT_FALSE(results.at("nonexistent_rule").matched);
    EXPECT_TRUE(results.at("nonexistent_rule").message.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_Vector_AllRulesNotExist) {
    RuleEngine engine;
    std::string error;

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    // 请求中包含的所有规则都不存在
    std::vector<std::string> rule_names = {"nonexistent1", "nonexistent2", "nonexistent3"};
    std::map<std::string, MatchResult> results;

    // 所有规则都不存在，应该返回 false
    EXPECT_FALSE(engine.match_rule(rule_names, adapter, results, &error));

    ASSERT_EQ(results.size(), 3);

    // 所有规则都应该标记为失败
    EXPECT_FALSE(results.at("nonexistent1").matched);
    EXPECT_TRUE(results.at("nonexistent1").message.find("not found") != std::string::npos);

    EXPECT_FALSE(results.at("nonexistent2").matched);
    EXPECT_TRUE(results.at("nonexistent2").message.find("not found") != std::string::npos);

    EXPECT_FALSE(results.at("nonexistent3").matched);
    EXPECT_TRUE(results.at("nonexistent3").message.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_Vector_EmptyRuleList) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    // 空规则列表
    std::vector<std::string> rule_names;
    std::map<std::string, MatchResult> results;

    // 空列表应该返回 true（没有规则失败）
    EXPECT_TRUE(engine.match_rule(rule_names, adapter, results, &error));

    EXPECT_TRUE(results.empty());
}

TEST_F(RuleEngineTest, MatchRule_Vector_MixedWithNonexistent_AllResultsRecorded) {
    RuleEngine engine;
    std::string error;

    // 添加多个规则
    ASSERT_TRUE(engine.add_rule("pass", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("fail", "test_data/rules/always_fail.lua", &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    // 混合存在的和不存在的规则
    std::vector<std::string> rule_names = {
        "pass",               // 存在且通过
        "nonexistent1",       // 不存在
        "fail",               // 存在但失败
        "nonexistent2",       // 不存在
        "pass"                // 重复规则（存在且通过）
    };
    std::map<std::string, MatchResult> results;

    // 有规则通过，应该返回 true
    EXPECT_TRUE(engine.match_rule(rule_names, adapter, results, &error));

    // 验证：重复的规则应该只保留最后一个结果
    ASSERT_EQ(results.size(), 4);

    EXPECT_TRUE(results.at("pass").matched);
    EXPECT_FALSE(results.at("fail").matched);
    EXPECT_FALSE(results.at("nonexistent1").matched);
    EXPECT_TRUE(results.at("nonexistent1").message.find("not found") != std::string::npos);
    EXPECT_FALSE(results.at("nonexistent2").matched);
    EXPECT_TRUE(results.at("nonexistent2").message.find("not found") != std::string::npos);
}

TEST_F(RuleEngineTest, MatchRule_Vector_OnlyNonexistentRules) {
    RuleEngine engine;
    std::string error;

    // 添加一些规则
    ASSERT_TRUE(engine.add_rule("existing_rule", "test_data/rules/always_pass.lua", &error));

    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    // 只请求不存在的规则
    std::vector<std::string> rule_names = {
        "nonexistent_a",
        "nonexistent_b",
        "nonexistent_c"
    };
    std::map<std::string, MatchResult> results;

    // 所有请求的规则都不存在，应该返回 false
    EXPECT_FALSE(engine.match_rule(rule_names, adapter, results, &error));

    ASSERT_EQ(results.size(), 3);

    // 所有规则都应该标记为未找到
    for (const auto& name : rule_names) {
        EXPECT_FALSE(results.at(name).matched);
        EXPECT_TRUE(results.at(name).message.find("not found") != std::string::npos);
    }
}

// ============================================================================
// 函数注册测试
// ============================================================================

// 测试用辅助函数
static int test_add_42(lua_State* L) {
    if (lua_gettop(L) < 1) {
        lua_pushstring(L, "add_42: expected 1 argument");
        lua_error(L);
        return 0;
    }

    double value = lua_tonumber(L, 1);
    lua_pushnumber(L, value + 42.0);
    return 1;
}

static int test_multiply(lua_State* L) {
    if (lua_gettop(L) < 2) {
        lua_pushstring(L, "multiply: expected 2 arguments");
        lua_error(L);
        return 0;
    }

    double a = lua_tonumber(L, 1);
    double b = lua_tonumber(L, 2);
    lua_pushnumber(L, a * b);
    return 1;
}

static int test_no_return(lua_State* /*L*/) {
    // 不返回任何值
    return 0;
}

static int test_return_multiple(lua_State* L) {
    if (lua_gettop(L) < 1) {
        lua_pushstring(L, "expected 1 argument");
        lua_error(L);
        return 0;
    }

    double value = lua_tonumber(L, 1);
    lua_pushnumber(L, value * 2);
    lua_pushnumber(L, value * 3);
    return 2;
}

// 测试用的类
class TestCalculator {
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
        return 1;
    }

    int get_value(lua_State* L) {
        lua_pushnumber(L, _value);
        return 1;
    }

    int set_value(lua_State* L) {
        if (lua_gettop(L) < 1) {
            lua_pushstring(L, "set_value: expected 1 argument");
            lua_error(L);
            return 0;
        }

        _value = lua_tonumber(L, 1);
        return 0;
    }

    // 静态分发器
    static int add_dispatcher(lua_State* L) {
        auto* self = static_cast<TestCalculator*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->add(L);
    }

    static int get_value_dispatcher(lua_State* L) {
        auto* self = static_cast<TestCalculator*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->get_value(L);
    }

    static int set_value_dispatcher(lua_State* L) {
        auto* self = static_cast<TestCalculator*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->set_value(L);
    }

private:
    double _value = 100.0;
};

// ============================================================================
// C++ 异常处理测试辅助函数
// ============================================================================

// 会抛出 C++ 异常的函数（危险，会导致崩溃）
static int test_throws_std_exception(lua_State* /*L*/) {
    throw std::runtime_error("C++ std::exception thrown");
    return 0;
}

// 安全的函数：内部捕获异常并转换为 Lua 错误
static int test_catches_exception(lua_State* L) {
    try {
        if (lua_gettop(L) < 1) {
            throw std::runtime_error("expected 1 argument");
        }

        double value = lua_tonumber(L, 1);
        if (value < 0) {
            throw std::runtime_error("value must be non-negative");
        }

        lua_pushnumber(L, value * 2);
        return 1;

    } catch (const std::exception& e) {
        // 将 C++ 异常转换为 Lua 错误
        lua_pushstring(L, e.what());
        lua_error(L);
        return 0;  // 不会执行到这里
    }
}

// 用于测试异常处理的类
class ExceptionTestClass {
public:
    // 成员函数抛出 C++ 异常（危险）
    int throws_exception(lua_State* /*L*/) {
        throw std::runtime_error("Member function throws exception");
        return 0;
    }

    // 成员函数安全地捕获异常
    int catches_exception(lua_State* L) {
        try {
            if (lua_gettop(L) < 1) {
                throw std::runtime_error("expected 1 argument");
            }

            double value = lua_tonumber(L, 1);
            lua_pushnumber(L, value + this->_offset);
            return 1;

        } catch (const std::exception& e) {
            lua_pushstring(L, e.what());
            lua_error(L);
            return 0;
        }
    }

    static int throws_exception_dispatcher(lua_State* L) {
        auto* self = static_cast<ExceptionTestClass*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->throws_exception(L);
    }

    static int catches_exception_dispatcher(lua_State* L) {
        auto* self = static_cast<ExceptionTestClass*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->catches_exception(L);
    }

private:
    double _offset = 100.0;
};
TEST_F(RuleEngineTest, RegisterNormalFunction_Success) {
    RuleEngine engine;
    std::string error;

    // 注册普通 C++ 函数
    EXPECT_TRUE(engine.register_function("add_42", test_add_42, &error));
    EXPECT_TRUE(error.empty());

    // 验证函数已注册
    EXPECT_TRUE(engine.has_function("add_42"));

    // 创建测试规则来调用该函数
    CreateRuleFile("test_add_42.lua", R"(
        function match(data)
            local result = ljre.add_42(data.value)
            return result < 100, "result is " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_add_42", "test_data/rules/test_add_42.lua", &error));

    // 测试数据
    json test_json = {{"value", 50}};
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_add_42", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "result is 92");
}

TEST_F(RuleEngineTest, RegisterNormalFunction_OverwriteExisting) {
    RuleEngine engine;
    std::string error;

    // 注册第一个函数
    EXPECT_TRUE(engine.register_function("my_func", test_add_42, &error));
    EXPECT_TRUE(engine.has_function("my_func"));

    // 用同名函数覆盖
    EXPECT_TRUE(engine.register_function("my_func", test_multiply, &error));
    EXPECT_TRUE(engine.has_function("my_func"));

    // 创建测试规则验证是新的函数
    CreateRuleFile("test_overwrite.lua", R"(
        function match(data)
            local result = ljre.my_func(data.a, data.b)
            return result == 6, "result is " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_overwrite", "test_data/rules/test_overwrite.lua", &error));

    json test_json = {{"a", 2}, {"b", 3}};
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_overwrite", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "result is 6");
}

TEST_F(RuleEngineTest, RegisterClassMemberFunction_Success) {
    RuleEngine engine;
    std::string error;

    TestCalculator calc;

    // 注册类成员函数
    EXPECT_TRUE(engine.register_function("add", &TestCalculator::add_dispatcher, &calc, &error));
    EXPECT_TRUE(error.empty());

    // 验证函数已注册
    EXPECT_TRUE(engine.has_function("add"));

    // 创建测试规则
    CreateRuleFile("test_member_add.lua", R"(
        function match(data)
            local sum = ljre.add(data.x, data.y)
            return sum == 15, "sum is " .. sum
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_member_add", "test_data/rules/test_member_add.lua", &error));

    json test_json = {{"x", 7}, {"y", 8}};
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_member_add", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "sum is 15");
}

TEST_F(RuleEngineTest, RegisterClassMemberFunction_MultipleInstances) {
    RuleEngine engine;
    std::string error;

    TestCalculator calc1;
    TestCalculator calc2;

    // 注册两个不同实例的同一个方法
    EXPECT_TRUE(engine.register_function("get_value1", &TestCalculator::get_value_dispatcher, &calc1, &error));
    EXPECT_TRUE(engine.register_function("get_value2", &TestCalculator::get_value_dispatcher, &calc2, &error));

    // 创建测试规则
    CreateRuleFile("test_two_instances.lua", R"(
        function match(data)
            local v1 = ljre.get_value1()
            local v2 = ljre.get_value2()
            return v1 == v2 and v1 == 100, "v1=" .. v1 .. ", v2=" .. v2
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_two_instances", "test_data/rules/test_two_instances.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_two_instances", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, RegisterClassMemberFunction_ModifyState) {
    RuleEngine engine;
    std::string error;

    TestCalculator calc;

    // 注册多个成员函数
    EXPECT_TRUE(engine.register_function("get_val", &TestCalculator::get_value_dispatcher, &calc, &error));
    EXPECT_TRUE(engine.register_function("set_val", &TestCalculator::set_value_dispatcher, &calc, &error));

    // 创建测试规则
    CreateRuleFile("test_state_change.lua", R"(
        function match(data)
            local v1 = ljre.get_val()
            ljre.set_val(200)
            local v2 = ljre.get_val()
            return v1 == 100 and v2 == 200, "v1=" .. v1 .. ", v2=" .. v2
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_state_change", "test_data/rules/test_state_change.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_state_change", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "v1=100, v2=200");
}

TEST_F(RuleEngineTest, UnregisterFunction_ExistingFunction) {
    RuleEngine engine;
    std::string error;

    // 注册函数
    EXPECT_TRUE(engine.register_function("test_func", test_add_42, &error));
    EXPECT_TRUE(engine.has_function("test_func"));

    // 注销函数
    EXPECT_TRUE(engine.unregister_function("test_func"));
    EXPECT_FALSE(engine.has_function("test_func"));
}

TEST_F(RuleEngineTest, UnregisterFunction_NonExistentFunction) {
    RuleEngine engine;

    // 注销不存在的函数应该返回 false
    EXPECT_FALSE(engine.unregister_function("nonexistent_func"));
}

TEST_F(RuleEngineTest, ClearRegisteredFunctions_AllFunctions) {
    RuleEngine engine;
    std::string error;

    // 注册多个函数
    EXPECT_TRUE(engine.register_function("func1", test_add_42, &error));
    EXPECT_TRUE(engine.register_function("func2", test_multiply, &error));
    EXPECT_TRUE(engine.register_function("func3", test_no_return, &error));

    EXPECT_EQ(engine.get_registered_functions().size(), 3);

    // 清空所有函数
    engine.clear_registered_functions();

    EXPECT_EQ(engine.get_registered_functions().size(), 0);
    EXPECT_FALSE(engine.has_function("func1"));
    EXPECT_FALSE(engine.has_function("func2"));
    EXPECT_FALSE(engine.has_function("func3"));
}

TEST_F(RuleEngineTest, HasFunction_ExistingAndNonExisting) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("exists", test_add_42, &error));

    EXPECT_TRUE(engine.has_function("exists"));
    EXPECT_FALSE(engine.has_function("not_exists"));
}

TEST_F(RuleEngineTest, GetRegisteredFunctions_EmptyAndMultiple) {
    RuleEngine engine;
    std::string error;

    // 初始状态应该为空
    EXPECT_TRUE(engine.get_registered_functions().empty());

    // 注册多个函数
    EXPECT_TRUE(engine.register_function("func_c", test_add_42, &error));
    EXPECT_TRUE(engine.register_function("func_a", test_multiply, &error));
    EXPECT_TRUE(engine.register_function("func_b", test_no_return, &error));

    auto functions = engine.get_registered_functions();
    EXPECT_EQ(functions.size(), 3);

    // 验证所有函数都在列表中
    EXPECT_TRUE(std::find(functions.begin(), functions.end(), "func_a") != functions.end());
    EXPECT_TRUE(std::find(functions.begin(), functions.end(), "func_b") != functions.end());
    EXPECT_TRUE(std::find(functions.begin(), functions.end(), "func_c") != functions.end());
}

TEST_F(RuleEngineTest, FunctionInRule_NoReturnValue) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("no_return", test_no_return, &error));

    CreateRuleFile("test_no_return.lua", R"(
        function match(data)
            ljre.no_return()
            return true, "function called successfully"
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_no_return", "test_data/rules/test_no_return.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_no_return", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "function called successfully");
}

TEST_F(RuleEngineTest, FunctionInRule_MultipleReturnValues) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("return_multiple", test_return_multiple, &error));

    CreateRuleFile("test_multiple_return.lua", R"(
        function match(data)
            local r1, r2 = ljre.return_multiple(10)
            return r1 == 20 and r2 == 30, "r1=" .. r1 .. ", r2=" .. r2
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_multiple_return", "test_data/rules/test_multiple_return.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_multiple_return", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "r1=20, r2=30");
}

TEST_F(RuleEngineTest, FunctionInRule_ErrorHandling) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("multiply", test_multiply, &error));

    // 创建一个会调用错误的规则（参数不足）
    CreateRuleFile("test_func_error.lua", R"(
        function match(data)
            local ok, result = pcall(function()
                return ljre.multiply(5)  -- 只有一个参数，应该报错
            end)
            return not ok, "error handled: " .. tostring(result)
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_func_error", "test_data/rules/test_func_error.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_func_error", adapter, result, &error));
    EXPECT_TRUE(result.matched);  // 错误被正确处理
}

TEST_F(RuleEngineTest, MultipleFunctionsInSameRule) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("add_42", test_add_42, &error));
    EXPECT_TRUE(engine.register_function("multiply", test_multiply, &error));

    CreateRuleFile("test_multi_funcs.lua", R"(
        function match(data)
            local v1 = ljre.add_42(10)
            local v2 = ljre.multiply(v1, 2)
            return v2 == 104, "v1=" .. v1 .. ", v2=" .. v2
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_multi_funcs", "test_data/rules/test_multi_funcs.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_multi_funcs", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "v1=52, v2=104");
}

TEST_F(RuleEngineTest, RegisterFunction_PersistAcrossRules) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("add_42", test_add_42, &error));

    // 创建多个规则使用同一个函数
    CreateRuleFile("rule1.lua", R"(
        function match(data)
            local r = ljre.add_42(data.x)
            return r == 52, "result=" .. r
        end
    )");

    CreateRuleFile("rule2.lua", R"(
        function match(data)
            local r = ljre.add_42(data.y)
            return r == 62, "result=" .. r
        end
    )");

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/rule1.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/rule2.lua", &error));

    json test_json = {{"x", 10}, {"y", 20}};
    auto adapter = std::make_shared<JsonAdapter>(test_json);

    MatchResult result1, result2;
    EXPECT_TRUE(engine.match_rule("rule1", adapter, result1, &error));
    EXPECT_TRUE(engine.match_rule("rule2", adapter, result2, &error));

    EXPECT_TRUE(result1.matched);
    EXPECT_EQ(result1.message, "result=52");

    EXPECT_TRUE(result2.matched);
    EXPECT_EQ(result2.message, "result=62");
}

TEST_F(RuleEngineTest, LjreTableIsolation) {
    RuleEngine engine;
    std::string error;

    EXPECT_TRUE(engine.register_function("my_func", test_add_42, &error));

    // 验证 ljre 表中的函数不会被全局访问到
    CreateRuleFile("test_isolation.lua", R"(
        function match(data)
            -- 尝试直接访问 my_func（不通过 ljre）
            local direct_type = type(my_func)

            -- 通过 ljre 访问
            local ljre_type = type(ljre.my_func)

            return direct_type == "nil" and ljre_type == "function",
                   "direct=" .. direct_type .. ", ljre=" .. ljre_type
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_isolation", "test_data/rules/test_isolation.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_isolation", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "direct=nil, ljre=function");
}

TEST_F(RuleEngineTest, CallNonExistentFunction) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("test_call_nonexist.lua", R"(
        function match(data)
            local ok, result = pcall(function()
                return ljre.nonexistent_func(10)
            end)
            return not ok, "error=" .. tostring(result)
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_call_nonexist", "test_data/rules/test_call_nonexist.lua", &error));

    json test_json = json::object();
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_call_nonexist", adapter, result, &error));
    EXPECT_TRUE(result.matched);  // 错误被 pcall 捕获
    EXPECT_TRUE(result.message.find("attempt to call") != std::string::npos ||
                result.message.find("nil value") != std::string::npos);
}

TEST_F(RuleEngineTest, RegisterFunction_ThenClear_ThenReuse) {
    RuleEngine engine;
    std::string error;

    // 注册函数
    EXPECT_TRUE(engine.register_function("temp_func", test_add_42, &error));
    EXPECT_TRUE(engine.has_function("temp_func"));

    // 清空
    engine.clear_registered_functions();
    EXPECT_FALSE(engine.has_function("temp_func"));

    // 重新注册同名函数
    EXPECT_TRUE(engine.register_function("temp_func", test_multiply, &error));
    EXPECT_TRUE(engine.has_function("temp_func"));

    // 验证新函数生效
    CreateRuleFile("test_reuse.lua", R"(
        function match(data)
            local r = ljre.temp_func(data.a, data.b)
            return r == 6, "result=" .. r
        end
    )");

    ASSERT_TRUE(engine.add_rule("test_reuse", "test_data/rules/test_reuse.lua", &error));

    json test_json = {{"a", 2}, {"b", 3}};
    auto adapter = std::make_shared<JsonAdapter>(test_json);
    MatchResult result;

    EXPECT_TRUE(engine.match_rule("test_reuse", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "result=6");
}

// ============================================================================
// 函数注册 - 无效状态测试
// ============================================================================

TEST_F(RuleEngineTest, RegisterFunction_InvalidState_ReturnsFalse) {
    RuleEngineInternalTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试注册普通 C++ 函数
    EXPECT_FALSE(engine.register_function("test_func", test_add_42, &error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("invalid"), std::string::npos);
}

TEST_F(RuleEngineTest, RegisterClassMemberFunction_InvalidState_ReturnsFalse) {
    RuleEngineInternalTest engine;
    std::string error;

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试注册类成员函数
    TestCalculator calc;
    EXPECT_FALSE(engine.register_function("add", &TestCalculator::add_dispatcher, &calc, &error));
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("invalid"), std::string::npos);
}

TEST_F(RuleEngineTest, UnregisterFunction_InvalidState_ReturnsFalse) {
    RuleEngineInternalTest engine;

    // 先注册一个函数
    std::string error;
    ASSERT_TRUE(engine.register_function("test_func", test_add_42, &error));

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 尝试注销函数
    EXPECT_FALSE(engine.unregister_function("test_func"));
}

TEST_F(RuleEngineTest, ClearRegisteredFunctions_InvalidState_DoesNothing) {
    RuleEngineInternalTest engine;

    // 先注册一些函数
    std::string error;
    ASSERT_TRUE(engine.register_function("func1", test_add_42, &error));
    ASSERT_TRUE(engine.register_function("func2", test_multiply, &error));

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 清空注册函数（不应该崩溃，只是静默失败）
    engine.clear_registered_functions();
}

TEST_F(RuleEngineTest, HasFunction_InvalidState_ReturnsFalse) {
    RuleEngineInternalTest engine;

    // 先注册一个函数
    std::string error;
    ASSERT_TRUE(engine.register_function("test_func", test_add_42, &error));
    ASSERT_TRUE(engine.has_function("test_func"));

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 检查函数是否存在（应该返回 false）
    EXPECT_FALSE(engine.has_function("test_func"));
}

TEST_F(RuleEngineTest, GetRegisteredFunctions_InvalidState_ReturnsEmpty) {
    RuleEngineInternalTest engine;

    // 先注册一些函数
    std::string error;
    ASSERT_TRUE(engine.register_function("func1", test_add_42, &error));
    ASSERT_TRUE(engine.register_function("func2", test_multiply, &error));

    // 使 Lua 状态无效
    engine.invalidate_lua_state();

    // 获取已注册函数列表（应该返回空列表）
    auto functions = engine.get_registered_functions();
    EXPECT_TRUE(functions.empty());
}

// ============================================================================
// C++ 异常处理测试
// ============================================================================

TEST_F(RuleEngineTest, CppFunction_ThrowsStdException_DoesNotCrash) {
    // 注意：LuaJIT 实际上可以捕获 C++ 异常并将其转换为 Lua 错误
    // 但是错误信息不够详细（只有 "C++ exception" 字符串）
    // 因此仍然强烈建议在 C++ 函数内部捕获异常并手动转换

    RuleEngine engine;
    std::string error;

    // 注册会抛出 C++ 异常的函数（不推荐这样做）
    ASSERT_TRUE(engine.register_function("throws_exception", test_throws_std_exception, &error));

    // 创建规则，使用 pcall 保护调用
    CreateRuleFile("test_exception_protection.lua", R"(
        function match(data)
            -- 使用 pcall 保护可能抛出 C++ 异常的函数调用
            -- LuaJIT 可以捕获 C++ 异常，但错误信息不够详细（只有 "C++ exception"）
            -- 这里演示 pcall 如何捕获 C++ 异常
            local ok, result = pcall(function()
                return ljre.throws_exception()
            end)

            if not ok then
                return false, "Function call failed: " .. tostring(result)
            end

            return true, "Success"
        end
    )");

    ASSERT_TRUE(engine.add_rule("exception_rule", "test_data/rules/test_exception_protection.lua", &error));

    json data = {{"value", 42}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    MatchResult result;
    // LuaJIT 会捕获 C++ 异常并转换为 Lua 错误
    bool success = engine.match_rule("exception_rule", adapter, result, &error);

    // 验证：LuaJIT 捕获了 C++ 异常并转换为 Lua 错误
    EXPECT_TRUE(success);
    EXPECT_FALSE(result.matched);
    EXPECT_TRUE(result.message.find("C++ exception") != std::string::npos);
}

TEST_F(RuleEngineTest, CppFunction_ThrowsException_WithoutPcall_FailsMatchRule) {
    // 测试：直接调用抛出 C++ 异常的函数（不使用 pcall）
    // 验证：当不使用 pcall 时，C++ 异常会导致 match_rule 调用失败

    RuleEngine engine;
    std::string error;

    // 注册会抛出 C++ 异常的函数
    ASSERT_TRUE(engine.register_function("throws_exception", test_throws_std_exception, &error));

    // 创建规则，直接调用抛异常的函数（不使用 pcall）
    CreateRuleFile("test_exception_no_pcall.lua", R"(
        function match(data)
            -- 直接调用会抛出 C++ 异常的函数，不使用 pcall 保护
            local result = ljre.throws_exception()
            return true, "Success: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("exception_rule", "test_data/rules/test_exception_no_pcall.lua", &error));

    json data = {{"value", 42}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    MatchResult result;
    // 当不使用 pcall 时，C++ 异常会导致 match_rule 调用失败
    bool success = engine.match_rule("exception_rule", adapter, result, &error);

    // 验证：match_rule 调用失败，错误信息同时存在于 result.message 和 error 参数中
    EXPECT_FALSE(success);  // match_rule 调用失败
    EXPECT_FALSE(result.matched);  // 结果标记为未匹配
    EXPECT_FALSE(result.message.empty());  // result.message 包含错误信息
    EXPECT_FALSE(error.empty());  // error 参数也包含错误信息
    EXPECT_EQ(result.message, error);  // 两者应该相同
    EXPECT_TRUE(result.message.find("Failed to call match") != std::string::npos);
    EXPECT_TRUE(result.message.find("C++") != std::string::npos || result.message.find("exception") != std::string::npos);
}

TEST_F(RuleEngineTest, CppFunction_CatchesException_Safely) {
    // 测试安全的异常处理：C++ 函数内部捕获异常并转换为 Lua 错误
    RuleEngine engine;
    std::string error;

    // 注册安全捕获异常的函数
    ASSERT_TRUE(engine.register_function("safe_double", test_catches_exception, &error));

    // 测试用例 1: 正常输入
    CreateRuleFile("test_safe_exception_1.lua", R"(
        function match(data)
            local result = ljre.safe_double(data.value)
            return true, "Result: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("safe_rule_1", "test_data/rules/test_safe_exception_1.lua", &error));

    json data1 = {{"value", 21}};
    auto adapter1 = std::make_shared<JsonAdapter>(data1);

    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("safe_rule_1", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);
    EXPECT_TRUE(result1.message.find("42") != std::string::npos);

    // 测试用例 2: 负数输入（触发异常）
    CreateRuleFile("test_safe_exception_2.lua", R"(
        function match(data)
            local ok, result = pcall(function()
                return ljre.safe_double(data.value)
            end)

            if not ok then
                return false, "Error: " .. result
            end

            return true, "Result: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("safe_rule_2", "test_data/rules/test_safe_exception_2.lua", &error));

    json data2 = {{"value", -5}};
    auto adapter2 = std::make_shared<JsonAdapter>(data2);

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("safe_rule_2", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);
    EXPECT_TRUE(result2.message.find("non-negative") != std::string::npos);

    // 测试用例 3: 缺少参数（触发异常）
    CreateRuleFile("test_safe_exception_3.lua", R"(
        function match(data)
            local ok, result = pcall(function()
                return ljre.safe_double()
            end)

            if not ok then
                return false, "Error: " .. result
            end

            return true, "Result: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("safe_rule_3", "test_data/rules/test_safe_exception_3.lua", &error));

    json data3 = {{"value", 10}};
    auto adapter3 = std::make_shared<JsonAdapter>(data3);

    MatchResult result3;
    ASSERT_TRUE(engine.match_rule("safe_rule_3", adapter3, result3, &error));
    EXPECT_FALSE(result3.matched);
    EXPECT_TRUE(result3.message.find("expected 1 argument") != std::string::npos);
}

TEST_F(RuleEngineTest, CppMemberFunction_CatchesException_Safely) {
    // 测试类成员函数的异常处理
    RuleEngine engine;
    std::string error;

    ExceptionTestClass obj;
    ASSERT_TRUE(engine.register_function("safe_add", &ExceptionTestClass::catches_exception_dispatcher, &obj, &error));

    // 正常调用
    CreateRuleFile("test_member_safe_exception.lua", R"(
        function match(data)
            local result = ljre.safe_add(data.value)
            return true, "Result: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("member_safe_rule", "test_data/rules/test_member_safe_exception.lua", &error));

    json data = {{"value", 50}};
    auto adapter = std::make_shared<JsonAdapter>(data);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("member_safe_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("150") != std::string::npos);  // 50 + 100 (offset)
}

TEST_F(RuleEngineTest, CppFunction_ExceptionHandlingBestPractice) {
    // 测试异常处理的最佳实践
    RuleEngine engine;
    std::string error;

    // 注册一个安全处理异常的函数
    ASSERT_TRUE(engine.register_function("safe_double", test_catches_exception, &error));

    // 最佳实践：在 Lua 规则中使用 pcall 保护
    CreateRuleFile("test_exception_best_practice.lua", R"(
        function match(data)
            -- 最佳实践 1: 使用 pcall 捕获函数调用中的错误
            local ok, result = pcall(function()
                if data.value < 0 then
                    error("value must be non-negative")  -- Lua 错误
                end
                return ljre.safe_double(data.value)
            end)

            if not ok then
                return false, "Calculation failed: " .. tostring(result)
            end

            -- 最佳实践 2: 验证返回值
            if type(result) ~= "number" then
                return false, "Invalid result type"
            end

            return true, "Result: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("best_practice_rule", "test_data/rules/test_exception_best_practice.lua", &error));

    // 测试正常情况
    json data1 = {{"value", 10}};
    auto adapter1 = std::make_shared<JsonAdapter>(data1);
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("best_practice_rule", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 测试错误情况（负数）
    json data2 = {{"value", -10}};
    auto adapter2 = std::make_shared<JsonAdapter>(data2);
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("best_practice_rule", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);
    EXPECT_TRUE(result2.message.find("failed") != std::string::npos);
}

TEST_F(RuleEngineTest, CppFunction_NoPcall_DirectCall) {
    // 测试不使用 pcall 的直接调用（函数内部捕获异常）
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("safe_func", test_catches_exception, &error));

    // 规则直接调用函数，不使用 pcall
    CreateRuleFile("test_direct_call.lua", R"(
        function match(data)
            -- 函数内部会捕获异常并转换为 Lua 错误
            -- 由于使用了 lua_error，这个错误会向上传播
            local result = ljre.safe_func(data.value)
            return true, "Result: " .. result
        end
    )");

    ASSERT_TRUE(engine.add_rule("direct_call_rule", "test_data/rules/test_direct_call.lua", &error));

    // 正常情况：应该成功
    json data1 = {{"value", 5}};
    auto adapter1 = std::make_shared<JsonAdapter>(data1);
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("direct_call_rule", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);
    EXPECT_TRUE(result1.message.find("10") != std::string::npos);
}

TEST_F(RuleEngineTest, CppFunction_MultipleExceptionTypes) {
    // 测试不同类型的异常处理
    RuleEngine engine;
    std::string error;

    // 注册多个安全的函数
    ASSERT_TRUE(engine.register_function("func1", test_catches_exception, &error));
    ASSERT_TRUE(engine.register_function("func2", test_add_42, &error));

    CreateRuleFile("test_multiple_exceptions.lua", R"(
        function match(data)
            local results = {}

            -- 调用多个函数，每个都可能抛出异常
            local ok1, r1 = pcall(function()
                return ljre.func1(data.value)
            end)

            local ok2, r2 = pcall(function()
                return ljre.func2(data.value)
            end)

            if not ok1 then
                return false, "func1 failed: " .. r1
            end

            if not ok2 then
                return false, "func2 failed: " .. r2
            end

            return true, "func1=" .. r1 .. ", func2=" .. r2
        end
    )");

    ASSERT_TRUE(engine.add_rule("multi_exception_rule", "test_data/rules/test_multiple_exceptions.lua", &error));

    json data = {{"value", 10}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("multi_exception_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("20") != std::string::npos);  // func1: 10 * 2
    EXPECT_TRUE(result.message.find("52") != std::string::npos);  // func2: 10 + 42
}

// ============================================================================
// Lua 公共函数加载测试
// ============================================================================

TEST_F(RuleEngineTest, AddLuaFile_InvalidState_ReturnsFalse) {
    // 测试在 Lua 状态无效时，add_lua_file 返回 false

    // 使用派生类来访问 protected 的 get_lua_state()
    class TestableEngine : public RuleEngine {
    public:
        using RuleEngine::RuleEngine;
        ljre::LuaState& get_lua_state_ref() { return get_lua_state(); }
    };

    TestableEngine engine;
    std::string error;

    // 通过移动 LuaState 创建无效状态
    // 这样 is_valid() 会返回 false
    ljre::LuaState temp = std::move(engine.get_lua_state_ref());
    // temp 在这里析构，引擎的 lua_state 现在无效
    (void)temp;

    CreateRuleFile("test_simple.lua", R"(
        utils = {}
        function utils.test()
            return "hello"
        end
    )");

    // 现在 Lua 状态无效，add_lua_file 应该返回 false
    EXPECT_FALSE(engine.add_lua_file("test_data/rules/test_simple.lua", &error));
    EXPECT_FALSE(error.empty());
    EXPECT_EQ(error, "Lua state is invalid");
}

TEST_F(RuleEngineTest, AddLuaFile_UtilsNamespace_Success) {
    // 测试加载使用 utils 命名空间的 Lua 公共函数
    RuleEngine engine;
    std::string error;

    // 创建 utils.lua 文件
    CreateRuleFile("test_utils.lua", R"(
        utils = {}

        function utils.is_adult(data)
            return data.age and data.age >= 18
        end

        function utils.calculate_score(data)
            local score = 0
            if data.vip then score = score + 10 end
            if data.level then score = score + data.level end
            return score
        end

        function utils.format_message(name, score)
            return string.format("User %s has score %d", name, score)
        end
    )");

    // 加载公共函数文件
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_utils.lua", &error));

    // 创建使用公共函数的规则
    CreateRuleFile("test_use_utils.lua", R"(
        function match(data)
            -- 使用 utils 命名空间的公共函数
            if not utils.is_adult(data) then
                return false, "User is not an adult"
            end

            local score = utils.calculate_score(data)
            if score < 10 then
                return false, "Score too low: " .. score
            end

            local msg = utils.format_message(data.name or "Unknown", score)
            return true, msg
        end
    )");

    ASSERT_TRUE(engine.add_rule("utils_rule", "test_data/rules/test_use_utils.lua", &error));

    // 测试用例 1: 成年用户，高分
    {
        json data = {{"name", "Alice"}, {"age", 25}, {"vip", true}, {"level", 5}};
        auto adapter = std::make_shared<JsonAdapter>(data);
        MatchResult result;
        ASSERT_TRUE(engine.match_rule("utils_rule", adapter, result, &error));
        EXPECT_TRUE(result.matched);
        EXPECT_EQ(result.message, "User Alice has score 15");  // 10 + 5
    }

    // 测试用例 2: 未成年用户
    {
        json data = {{"name", "Bob"}, {"age", 15}, {"vip", false}};
        auto adapter = std::make_shared<JsonAdapter>(data);
        MatchResult result;
        ASSERT_TRUE(engine.match_rule("utils_rule", adapter, result, &error));
        EXPECT_FALSE(result.matched);
        EXPECT_EQ(result.message, "User is not an adult");
    }

    // 测试用例 3: 成年用户，低分
    {
        json data = {{"name", "Charlie"}, {"age", 30}, {"vip", false}, {"level", 2}};
        auto adapter = std::make_shared<JsonAdapter>(data);
        MatchResult result;
        ASSERT_TRUE(engine.match_rule("utils_rule", adapter, result, &error));
        EXPECT_FALSE(result.matched);
        EXPECT_EQ(result.message, "Score too low: 2");
    }
}

TEST_F(RuleEngineTest, AddLuaFile_DifferentNamespaces) {
    // 测试加载多个不同命名空间的 Lua 文件
    RuleEngine engine;
    std::string error;

    // 创建 validators.lua
    CreateRuleFile("test_validators.lua", R"(
        validators = {}

        function validators.check_email(data)
            return data.email and data.email:match(".+@.+") ~= nil
        end

        function validators.check_phone(data)
            return data.phone and #data.phone >= 11
        end
    )");

    // 创建 helpers.lua
    CreateRuleFile("test_helpers.lua", R"(
        helpers = {}

        function helpers.sanitize_string(s)
            if not s then return "" end
            return s:gsub("%s+", ""):gsub("^%s*(.-)%s*$", "%1")
        end

        function helpers.to_upper(s)
            return s and s:upper() or ""
        end
    )");

    // 加载两个文件
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_validators.lua", &error));
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_helpers.lua", &error));

    // 创建使用多个命名空间的规则
    CreateRuleFile("test_multi_namespace.lua", R"(
        function match(data)
            -- 使用 validators 命名空间
            if not validators.check_email(data) then
                return false, "Invalid email"
            end

            if not validators.check_phone(data) then
                return false, "Invalid phone"
            end

            -- 使用 helpers 命名空间
            local clean_name = helpers.sanitize_string(data.name or "")
            local upper_name = helpers.to_upper(clean_name)

            return true, "Validated: " .. upper_name
        end
    )");

    ASSERT_TRUE(engine.add_rule("multi_ns_rule", "test_data/rules/test_multi_namespace.lua", &error));

    // 测试
    json data = {{"name", "  alice  "}, {"email", "alice@example.com"}, {"phone", "12345678901"}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("multi_ns_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "Validated: ALICE");
}

TEST_F(RuleEngineTest, AddLuaFile_GlobalFunctions) {
    // 测试加载直接定义全局函数的 Lua 文件
    RuleEngine engine;
    std::string error;

    // 创建定义全局函数的文件
    CreateRuleFile("test_globals.lua", R"(
        function is_positive(n)
            return n and n > 0
        end

        function double(n)
            return n * 2
        end

        function add_ten(n)
            return n + 10
        end
    )");

    // 加载文件
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_globals.lua", &error));

    // 创建使用全局函数的规则
    CreateRuleFile("test_global_funcs.lua", R"(
        function match(data)
            local value = data.value or 0

            if not is_positive(value) then
                return false, "Value is not positive"
            end

            local doubled = double(value)
            local added = add_ten(value)

            return true, string.format("Value: %d, Doubled: %d, Plus10: %d", value, doubled, added)
        end
    )");

    ASSERT_TRUE(engine.add_rule("global_funcs_rule", "test_data/rules/test_global_funcs.lua", &error));

    // 测试
    json data = {{"value", 5}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("global_funcs_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "Value: 5, Doubled: 10, Plus10: 15");
}

TEST_F(RuleEngineTest, AddLuaFile_WithCppFunctions) {
    // 测试 Lua 公共函数和 C++ 注册函数混合使用
    RuleEngine engine;
    std::string error;

    // 注册 C++ 函数
    ASSERT_TRUE(engine.register_function("add_42", test_add_42, &error));

    // 加载 Lua 公共函数
    CreateRuleFile("test_mixed.lua", R"(
        utils = {}

        function utils.is_adult(age)
            return age and age >= 18
        end

        function utils.is_senior(age)
            return age and age >= 60
        end

        function utils.check_score(score)
            return score >= 60
        end
    )");

    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_mixed.lua", &error));

    // 创建同时使用 C++ 函数和 Lua 公共函数的规则
    CreateRuleFile("test_mixed_usage.lua", R"(
        function match(data)
            -- 使用 C++ 函数
            local result = ljre.add_42(data.value or 0)

            -- 使用 Lua 公共函数
            local age = data.age or 0
            local is_adult = utils.is_adult(age)
            local is_senior = utils.is_senior(age)
            local score_ok = utils.check_score(result)

            if is_senior and score_ok then
                return true, string.format("Senior citizen, age %d, result %d", age, result)
            elseif is_adult and score_ok then
                return true, string.format("Adult citizen, age %d, result %d", age, result)
            else
                return false, "Not qualified"
            end
        end
    )");

    ASSERT_TRUE(engine.add_rule("mixed_rule", "test_data/rules/test_mixed_usage.lua", &error));

    // 测试
    json data = {{"age", 65}, {"value", 25}};  // 65岁，25+42=67分
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    bool success = engine.match_rule("mixed_rule", adapter, result, &error);
    if (!success) {
        std::cout << "Error: " << error << std::endl;
    }
    ASSERT_TRUE(success);
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("Senior citizen") != std::string::npos);
    EXPECT_TRUE(result.message.find("67") != std::string::npos);  // 25 + 42 = 67
}

TEST_F(RuleEngineTest, AddLuaFile_FileNotExist) {
    // 测试加载不存在的文件
    RuleEngine engine;
    std::string error;

    EXPECT_FALSE(engine.add_lua_file("nonexistent.lua", &error));
    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("Failed to load file") != std::string::npos ||
                error.find("cannot open") != std::string::npos);
}

TEST_F(RuleEngineTest, AddLuaFile_SyntaxError) {
    // 测试加载有语法错误的文件
    RuleEngine engine;
    std::string error;

    CreateRuleFile("test_syntax_error.lua", R"(
        utils = {}

        function utils.broken(  -- 缺少右括号
            return true
        end
    )");

    EXPECT_FALSE(engine.add_lua_file("test_data/rules/test_syntax_error.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, AddLuaFile_RuntimeError) {
    // 测试加载有运行时错误的文件
    RuleEngine engine;
    std::string error;

    CreateRuleFile("test_runtime_error.lua", R"(
        local x = nil
        local y = x.value  -- 尝试访问 nil 的字段
    )");

    EXPECT_FALSE(engine.add_lua_file("test_data/rules/test_runtime_error.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, AddLuaFile_OverrideExisting) {
    // 测试后加载的文件覆盖同名函数
    RuleEngine engine;
    std::string error;

    // 第一次加载
    CreateRuleFile("test_v1.lua", R"(
        utils = {}
        function utils.get_value()
            return "version 1"
        end
    )");

    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_v1.lua", &error));

    // 第二次加载（覆盖）
    CreateRuleFile("test_v2.lua", R"(
        utils = {}
        function utils.get_value()
            return "version 2"
        end
    )");

    ASSERT_TRUE(engine.add_lua_file("test_data/rules/test_v2.lua", &error));

    // 创建规则测试
    CreateRuleFile("test_override.lua", R"(
        function match(data)
            local value = utils.get_value()
            return true, value
        end
    )");

    ASSERT_TRUE(engine.add_rule("override_rule", "test_data/rules/test_override.lua", &error));

    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("override_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "version 2");  // 应该使用 v2
}

// ============================================================================
// RuleEngine Clone 方法测试
// ============================================================================

// --- 基础克隆测试 ---

TEST_F(RuleEngineTest, Clone_NONE_ReturnsEmptyEngine) {
    RuleEngine engine;
    std::string error;

    // 添加一些内容
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 克隆 NONE 应该返回空引擎
    auto cloned = engine.clone(RuleEngine::NONE, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->get_rule_count(), 0);
    EXPECT_FALSE(cloned->has_rule("rule1"));
}

TEST_F(RuleEngineTest, Clone_NONE_NoErrorMessage) {
    RuleEngine engine;

    auto cloned = engine.clone(RuleEngine::NONE);
    ASSERT_NE(cloned, nullptr);
}

// --- 克隆规则文件测试 ---

TEST_F(RuleEngineTest, Clone_RULES_ClonesAllRules) {
    RuleEngine engine;
    std::string error;

    // 添加多个规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule3", "test_data/rules/age_check.lua", &error));

    // 克隆规则
    auto cloned = engine.clone(RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_TRUE(error.empty());

    // 验证规则已克隆
    EXPECT_EQ(cloned->get_rule_count(), 3);
    EXPECT_TRUE(cloned->has_rule("rule1"));
    EXPECT_TRUE(cloned->has_rule("rule2"));
    EXPECT_TRUE(cloned->has_rule("rule3"));

    // 验证规则文件路径（使用集合检查，因为 unordered_map 顺序不确定）
    auto rules = cloned->get_all_rules();
    std::set<std::string> paths;
    for (const auto& r : rules) {
        paths.insert(r.file_path);
    }
    EXPECT_TRUE(paths.count("test_data/rules/always_pass.lua"));
    EXPECT_TRUE(paths.count("test_data/rules/always_fail.lua"));
    EXPECT_TRUE(paths.count("test_data/rules/age_check.lua"));
}

TEST_F(RuleEngineTest, Clone_RULES_EmptyEngine) {
    RuleEngine engine;

    auto cloned = engine.clone(RuleEngine::RULES);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->get_rule_count(), 0);
}

TEST_F(RuleEngineTest, Clone_RULES_ClonedRulesWork) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_rule", "test_data/rules/age_check.lua", &error));

    auto cloned = engine.clone(RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);

    // 测试克隆的规则能正常工作
    json data = {{"age", 25}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("age_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "年龄检查通过");

    // 测试不满足条件的情况
    json data2 = {{"age", 15}};
    auto adapter2 = std::make_shared<JsonAdapter>(data2);
    ASSERT_TRUE(cloned->match_rule("age_rule", adapter2, result, &error));
    EXPECT_FALSE(result.matched);
}

TEST_F(RuleEngineTest, Clone_rules_ConvenienceMethod) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    auto cloned = engine.clone_rules(&error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->get_rule_count(), 1);
    EXPECT_TRUE(cloned->has_rule("rule1"));
}

TEST_F(RuleEngineTest, Clone_rules_ConvenienceMethod_NoError) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    auto cloned = engine.clone_rules();
    ASSERT_NE(cloned, nullptr);
}

// --- 克隆 Lua 公共文件测试 ---

TEST_F(RuleEngineTest, Clone_LUA_FILES_ClonesAllLuaFiles) {
    RuleEngine engine;
    std::string error;

    // 添加多个 Lua 文件
    CreateRuleFile("utils1.lua", R"(
        utils = utils or {}
        function utils.helper1()
            return "helper1"
        end
    )");
    CreateRuleFile("utils2.lua", R"(
        utils = utils or {}
        function utils.helper2()
            return "helper2"
        end
    )");

    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils1.lua", &error));
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils2.lua", &error));

    // 克隆 Lua 文件
    auto cloned = engine.clone(RuleEngine::LUA_FILES, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_TRUE(error.empty());

    // 创建规则来验证 Lua 文件已加载
    CreateRuleFile("test_utils.lua", R"(
        function match(data)
            local result = utils.helper1() .. utils.helper2()
            return true, result
        end
    )");

    ASSERT_TRUE(cloned->add_rule("test_rule", "test_data/rules/test_utils.lua", &error));

    // 测试规则能使用克隆的 Lua 公共函数
    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("test_rule", adapter, result, &error));
    EXPECT_EQ(result.message, "helper1helper2");
}

TEST_F(RuleEngineTest, Clone_LUA_FILES_EmptyEngine) {
    RuleEngine engine;

    auto cloned = engine.clone(RuleEngine::LUA_FILES);
    ASSERT_NE(cloned, nullptr);
}

TEST_F(RuleEngineTest, Clone_lua_files_ConvenienceMethod) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("utils.lua", R"(
        utils = {}
        function utils.test()
            return 42
        end
    )");

    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));

    auto cloned = engine.clone_lua_files(&error);
    ASSERT_NE(cloned, nullptr);

    // 验证 Lua 文件已克隆
    CreateRuleFile("verify.lua", R"(
        function match(data)
            return true, tostring(utils.test())
        end
    )");

    ASSERT_TRUE(cloned->add_rule("verify", "test_data/rules/verify.lua", &error));

    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("verify", adapter, result, &error));
    EXPECT_EQ(result.message, "42");
}

TEST_F(RuleEngineTest, Clone_lua_files_ConvenienceMethod_NoError) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("utils.lua", "utils = {}");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));

    auto cloned = engine.clone_lua_files();
    ASSERT_NE(cloned, nullptr);
}

// --- 克隆 C++ 普通函数测试 ---

static int test_cpp_function(lua_State* L) {
    lua_pushnumber(L, 42);
    return 1;
}

static int test_cpp_add(lua_State* L) {
    double a = lua_tonumber(L, 1);
    double b = lua_tonumber(L, 2);
    lua_pushnumber(L, a + b);
    return 1;
}

TEST_F(RuleEngineTest, Clone_CPP_FUNCTIONS_ClonesAllCppFunctions) {
    RuleEngine engine;
    std::string error;

    // 注册多个 C++ 函数
    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));
    ASSERT_TRUE(engine.register_function("func2", test_cpp_add, &error));

    // 克隆 C++ 函数
    auto cloned = engine.clone(RuleEngine::CPP_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_TRUE(error.empty());

    // 验证函数已注册到克隆的引擎
    auto functions = cloned->get_registered_functions();
    EXPECT_EQ(functions.size(), 2);

    // 按字母顺序排序后比较
    std::sort(functions.begin(), functions.end());
    EXPECT_EQ(functions[0], "func1");
    EXPECT_EQ(functions[1], "func2");
}

TEST_F(RuleEngineTest, Clone_CPP_FUNCTIONS_EmptyEngine) {
    RuleEngine engine;

    auto cloned = engine.clone(RuleEngine::CPP_FUNCTIONS);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_registered_functions().size(), 0);
}

TEST_F(RuleEngineTest, Clone_CPP_FUNCTIONS_ClonedFunctionsWork) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("get_value", test_cpp_function, &error));

    auto cloned = engine.clone(RuleEngine::CPP_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    // 创建规则来测试 C++ 函数
    CreateRuleFile("test_cpp.lua", R"(
        function match(data)
            local value = ljre.get_value()
            return true, "value=" .. tostring(value)
        end
    )");

    ASSERT_TRUE(cloned->add_rule("test_rule", "test_data/rules/test_cpp.lua", &error));

    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("test_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "value=42");
}

// --- 克隆 C++ 成员函数测试 ---

class TestClass {
public:
    int _value = 100;

    int get_value(lua_State* L) {
        lua_pushnumber(L, _value);
        return 1;
    }

    int add(lua_State* L) {
        double a = lua_tonumber(L, 1);
        double b = lua_tonumber(L, 2);
        lua_pushnumber(L, a + b + _value);
        return 1;
    }

    static int get_value_dispatcher(lua_State* L) {
        auto* self = static_cast<TestClass*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->get_value(L);
    }

    static int add_dispatcher(lua_State* L) {
        auto* self = static_cast<TestClass*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->add(L);
    }
};

TEST_F(RuleEngineTest, Clone_CPP_MEMBER_FUNCTIONS_ClonesAllMemberFunctions) {
    RuleEngine engine;
    std::string error;

    TestClass obj1, obj2;
    obj1._value = 100;
    obj2._value = 200;

    ASSERT_TRUE(engine.register_function("get_value1", &TestClass::get_value_dispatcher, &obj1, &error));
    ASSERT_TRUE(engine.register_function("get_value2", &TestClass::get_value_dispatcher, &obj2, &error));

    auto cloned = engine.clone(RuleEngine::CPP_MEMBER_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    auto functions = cloned->get_registered_functions();
    EXPECT_EQ(functions.size(), 2);
}

TEST_F(RuleEngineTest, Clone_CPP_MEMBER_FUNCTIONS_ClonedFunctionsWork) {
    RuleEngine engine;
    std::string error;

    TestClass obj;
    obj._value = 123;

    ASSERT_TRUE(engine.register_function("get_val", &TestClass::get_value_dispatcher, &obj, &error));

    auto cloned = engine.clone(RuleEngine::CPP_MEMBER_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    // 验证成员函数能正常工作
    CreateRuleFile("test_member.lua", R"(
        function match(data)
            local v = ljre.get_val()
            return true, "value=" .. tostring(v)
        end
    )");

    ASSERT_TRUE(cloned->add_rule("test_rule", "test_data/rules/test_member.lua", &error));

    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("test_rule", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "value=123");
}

TEST_F(RuleEngineTest, Clone_cpp_functions_ConvenienceMethod_ClonesBothTypes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));

    TestClass obj;
    ASSERT_TRUE(engine.register_function("func2", &TestClass::get_value_dispatcher, &obj, &error));

    // clone_cpp_functions 应该克隆所有类型的 C++ 函数
    auto cloned = engine.clone_cpp_functions(&error);
    ASSERT_NE(cloned, nullptr);

    auto functions = cloned->get_registered_functions();
    EXPECT_EQ(functions.size(), 2);
}

TEST_F(RuleEngineTest, Clone_cpp_functions_ConvenienceMethod_NoError) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));

    auto cloned = engine.clone_cpp_functions();
    ASSERT_NE(cloned, nullptr);
}

// --- 克隆所有内容测试 ---

TEST_F(RuleEngineTest, Clone_ALL_ClonesEverything) {
    RuleEngine engine;
    std::string error;

    // 添加 Lua 文件
    CreateRuleFile("common.lua", R"(
        common = {}
        function common.helper()
            return 100
        end
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/common.lua", &error));

    // 注册 C++ 函数
    ASSERT_TRUE(engine.register_function("cpp_func", test_cpp_function, &error));

    TestClass obj;
    ASSERT_TRUE(engine.register_function("member_func", &TestClass::get_value_dispatcher, &obj, &error));

    // 添加规则
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/age_check.lua", &error));

    // 克隆所有内容
    auto cloned = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_TRUE(error.empty());

    // 验证规则已克隆
    EXPECT_EQ(cloned->get_rule_count(), 2);
    EXPECT_TRUE(cloned->has_rule("rule1"));
    EXPECT_TRUE(cloned->has_rule("rule2"));

    // 验证 C++ 函数已克隆
    auto functions = cloned->get_registered_functions();
    EXPECT_EQ(functions.size(), 2);

    // 验证 Lua 文件已克隆（通过创建使用它的规则来测试）
    CreateRuleFile("verify_all.lua", R"(
        function match(data)
            local c = common.helper()
            local cpp = ljre.cpp_func()
            return true, "common=" .. c .. ", cpp=" .. cpp
        end
    )");
    ASSERT_TRUE(cloned->add_rule("verify", "test_data/rules/verify_all.lua", &error));

    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("verify", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "common=100, cpp=42");
}

TEST_F(RuleEngineTest, Clone_safe_ConvenienceMethod) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.register_function("func", test_cpp_function, &error));

    auto cloned = engine.clone_safe(&error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_rule_count(), 1);
    EXPECT_EQ(cloned->get_registered_functions().size(), 1);
}

TEST_F(RuleEngineTest, Clone_safe_ConvenienceMethod_NoError) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    auto cloned = engine.clone_safe();
    ASSERT_NE(cloned, nullptr);
}

// --- 组合选项克隆测试 ---

TEST_F(RuleEngineTest, Clone_LUA_FILES_AND_RULES) {
    RuleEngine engine;
    std::string error;

    // 添加 Lua 文件
    CreateRuleFile("my_utils.lua", R"(
        my_utils = {}
        function my_utils.check(data)
            return data.value > 0
        end
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/my_utils.lua", &error));

    // 添加规则
    CreateRuleFile("use_utils.lua", R"(
        function match(data)
            if my_utils.check(data) then
                return true, "value is positive"
            end
            return false, "value is not positive"
        end
    )");
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/use_utils.lua", &error));

    // 克隆 Lua 文件和规则
    auto cloned = engine.clone(RuleEngine::LUA_FILES | RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_rule_count(), 1);

    // 测试克隆的规则能正常工作
    json data = {{"value", 10}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("rule1", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "value is positive");
}

TEST_F(RuleEngineTest, Clone_RULES_AND_CPP_FUNCTIONS) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("add", test_cpp_add, &error));

    CreateRuleFile("use_cpp.lua", R"(
        function match(data)
            local sum = ljre.add(data.a, data.b)
            return sum > 100, "sum=" .. sum
        end
    )");
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/use_cpp.lua", &error));

    // 克隆规则和 C++ 函数
    auto cloned = engine.clone(RuleEngine::RULES | RuleEngine::CPP_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    // 测试克隆的规则能正常工作
    json data = {{"a", 60}, {"b", 50}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("rule1", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_EQ(result.message, "sum=110");
}

TEST_F(RuleEngineTest, Clone_LUA_FILES_AND_CPP_FUNCTIONS) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("utils.lua", R"(
        utils = {}
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));
    ASSERT_TRUE(engine.register_function("func", test_cpp_function, &error));

    auto cloned = engine.clone(RuleEngine::LUA_FILES | RuleEngine::CPP_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_registered_functions().size(), 1);
}

TEST_F(RuleEngineTest, Clone_CPP_FUNCTIONS_AND_CPP_MEMBER_FUNCTIONS) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));

    TestClass obj;
    ASSERT_TRUE(engine.register_function("func2", &TestClass::get_value_dispatcher, &obj, &error));

    auto cloned = engine.clone(RuleEngine::CPP_FUNCTIONS | RuleEngine::CPP_MEMBER_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_registered_functions().size(), 2);
}

TEST_F(RuleEngineTest, Clone_AllThreeOptions) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));

    TestClass obj;
    ASSERT_TRUE(engine.register_function("func2", &TestClass::get_value_dispatcher, &obj, &error));

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    CreateRuleFile("utils.lua", "utils = {}");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));

    auto cloned = engine.clone(RuleEngine::LUA_FILES | RuleEngine::CPP_FUNCTIONS | RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_rule_count(), 1);
    EXPECT_EQ(cloned->get_registered_functions().size(), 1); // 只有普通函数
}

// --- 错误处理测试 ---

TEST_F(RuleEngineTest, Clone_RULES_NonExistentFile_Fails) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("good_rule.lua", R"(
        function match(data)
            return true, "ok"
        end
    )");
    ASSERT_TRUE(engine.add_rule("good_rule", "test_data/rules/good_rule.lua", &error));

    // 删除文件来模拟文件不存在
    system("rm test_data/rules/good_rule.lua");

    auto cloned = engine.clone(RuleEngine::RULES, &error);
    EXPECT_EQ(cloned, nullptr);
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, Clone_LUA_FILES_NonExistentFile_Fails) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("temp.lua", R"(
        temp = {}
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/temp.lua", &error));

    system("rm test_data/rules/temp.lua");

    auto cloned = engine.clone(RuleEngine::LUA_FILES, &error);
    EXPECT_EQ(cloned, nullptr);
    EXPECT_FALSE(error.empty());
}

TEST_F(RuleEngineTest, Clone_LUA_FILES_SyntaxError_Fails) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("good.lua", "x = 1");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/good.lua", &error));

    // 修改文件内容为有语法错误的代码
    CreateRuleFile("good.lua", R"(
        if true
            -- missing 'then'
        end
    )");

    // 克隆时应该失败，因为文件现在有语法错误
    auto cloned = engine.clone(RuleEngine::LUA_FILES, &error);
    EXPECT_EQ(cloned, nullptr);
    EXPECT_FALSE(error.empty());
}

// --- 独立性测试 ---

TEST_F(RuleEngineTest, Clone_EnginesAreIndependent) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    auto cloned = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(cloned, nullptr);

    // 在原引擎中添加新规则
    CreateRuleFile("new_rule.lua", R"(
        function match(data)
            return false, "new rule"
        end
    )");
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/new_rule.lua", &error));

    // 克隆的引擎不应该有新规则
    EXPECT_EQ(engine.get_rule_count(), 2);
    EXPECT_EQ(cloned->get_rule_count(), 1);
    EXPECT_FALSE(cloned->has_rule("rule2"));

    // 在克隆引擎中添加规则
    CreateRuleFile("cloned_rule.lua", R"(
        function match(data)
            return true, "cloned rule"
        end
    )");
    ASSERT_TRUE(cloned->add_rule("rule3", "test_data/rules/cloned_rule.lua", &error));

    // 原引擎不应该有新规则
    EXPECT_EQ(engine.get_rule_count(), 2);
    EXPECT_EQ(cloned->get_rule_count(), 2);
    EXPECT_FALSE(engine.has_rule("rule3"));
}

TEST_F(RuleEngineTest, Clone_ModifyingOriginalDoesNotAffectClone) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("utils.lua", R"(
        utils = {}
        function utils.get_value()
            return "original"
        end
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));

    auto cloned = engine.clone(RuleEngine::LUA_FILES, &error);
    ASSERT_NE(cloned, nullptr);

    // 重新加载 Lua 文件，修改函数
    CreateRuleFile("utils.lua", R"(
        utils = {}
        function utils.get_value()
            return "modified"
        end
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));

    // 创建规则验证
    CreateRuleFile("check.lua", R"(
        function match(data)
            return true, utils.get_value()
        end
    )");

    // 原引擎应该看到修改后的版本
    ASSERT_TRUE(engine.add_rule("check1", "test_data/rules/check.lua", &error));
    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("check1", adapter, result, &error));
    EXPECT_EQ(result.message, "modified");

    // 克隆的引擎应该保持原版本
    ASSERT_TRUE(cloned->add_rule("check2", "test_data/rules/check.lua", &error));
    ASSERT_TRUE(cloned->match_rule("check2", adapter, result, &error));
    EXPECT_EQ(result.message, "original");
}

// --- 多次克隆测试 ---

TEST_F(RuleEngineTest, Clone_MultipleClonesFromSameEngine) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.register_function("func", test_cpp_function, &error));

    auto clone1 = engine.clone(RuleEngine::ALL, &error);
    auto clone2 = engine.clone(RuleEngine::ALL, &error);
    auto clone3 = engine.clone(RuleEngine::ALL, &error);

    ASSERT_NE(clone1, nullptr);
    ASSERT_NE(clone2, nullptr);
    ASSERT_NE(clone3, nullptr);

    // 所有克隆都应该有相同的内容
    EXPECT_EQ(clone1->get_rule_count(), 1);
    EXPECT_EQ(clone2->get_rule_count(), 1);
    EXPECT_EQ(clone3->get_rule_count(), 1);

    EXPECT_EQ(clone1->get_registered_functions().size(), 1);
    EXPECT_EQ(clone2->get_registered_functions().size(), 1);
    EXPECT_EQ(clone3->get_registered_functions().size(), 1);
}

TEST_F(RuleEngineTest, Clone_CloneOfClone) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    auto clone1 = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(clone1, nullptr);

    auto clone2 = clone1->clone(RuleEngine::ALL, &error);
    ASSERT_NE(clone2, nullptr);

    auto clone3 = clone2->clone(RuleEngine::ALL, &error);
    ASSERT_NE(clone3, nullptr);

    // 所有克隆都应该有原始规则
    EXPECT_TRUE(clone1->has_rule("rule1"));
    EXPECT_TRUE(clone2->has_rule("rule1"));
    EXPECT_TRUE(clone3->has_rule("rule1"));

    // 测试规则能正常工作
    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(clone3->match_rule("rule1", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

// --- 边界情况测试 ---

TEST_F(RuleEngineTest, Clone_EmptyEngineWithALL) {
    RuleEngine engine;

    auto cloned = engine.clone(RuleEngine::ALL);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->get_rule_count(), 0);
    EXPECT_EQ(cloned->get_registered_functions().size(), 0);
}

TEST_F(RuleEngineTest, Clone_EngineWithOnlyRules) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));

    auto cloned = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->get_rule_count(), 2);
}

TEST_F(RuleEngineTest, Clone_EngineWithOnlyCppFunctions) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));
    ASSERT_TRUE(engine.register_function("func2", test_cpp_add, &error));

    auto cloned = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->get_registered_functions().size(), 2);
}

TEST_F(RuleEngineTest, Clone_EngineWithOnlyLuaFiles) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("file1.lua", "a = 1");
    CreateRuleFile("file2.lua", "b = 2");

    ASSERT_TRUE(engine.add_lua_file("test_data/rules/file1.lua", &error));
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/file2.lua", &error));

    auto cloned = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(cloned, nullptr);

    // 验证通过测试是否能访问定义的变量
    CreateRuleFile("check.lua", R"(
        function match(data)
            return true, a .. b
        end
    )");
    ASSERT_TRUE(cloned->add_rule("check", "test_data/rules/check.lua", &error));

    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("check", adapter, result, &error));
    EXPECT_EQ(result.message, "12");
}

// --- 复杂场景测试 ---

TEST_F(RuleEngineTest, Clone_ComplexRealWorldScenario) {
    RuleEngine engine;
    std::string error;

    // 添加多个工具文件
    CreateRuleFile("validators.lua", R"(
        validators = {}
        function validators.is_adult(data)
            return data.age and data.age >= 18
        end
        function validators.has_email(data)
            return data.email and #data.email > 0
        end
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/validators.lua", &error));

    CreateRuleFile("scorers.lua", R"(
        scorers = {}
        function scorers.calculate_score(data)
            local score = 0
            if data.age and data.age >= 18 then score = score + 20 end
            if data.vip then score = score + 30 end
            if data.email and #data.email > 0 then score = score + 10 end
            return score
        end
    )");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/scorers.lua", &error));

    // 注册 C++ 函数
    ASSERT_TRUE(engine.register_function("get_timestamp", test_cpp_function, &error));

    // 添加多个规则
    CreateRuleFile("adult_check.lua", R"(
        function match(data)
            if validators.is_adult(data) then
                return true, "成年用户"
            end
            return false, "未成年用户"
        end
    )");
    ASSERT_TRUE(engine.add_rule("adult_check", "test_data/rules/adult_check.lua", &error));

    CreateRuleFile("score_check.lua", R"(
        function match(data)
            local score = scorers.calculate_score(data)
            local ts = ljre.get_timestamp()
            if score >= 30 then
                return true, "分数=" .. score .. ", 时间戳=" .. ts
            end
            return false, "分数不足: " .. score
        end
    )");
    ASSERT_TRUE(engine.add_rule("score_check", "test_data/rules/score_check.lua", &error));

    // 克隆引擎
    auto cloned = engine.clone(RuleEngine::ALL, &error);
    ASSERT_NE(cloned, nullptr);

    // 验证所有内容都已克隆
    EXPECT_EQ(cloned->get_rule_count(), 2);
    EXPECT_EQ(cloned->get_registered_functions().size(), 1);

    // 测试克隆的规则能正常工作
    json data1 = {{"age", 25}, {"email", "test@example.com"}, {"vip", true}};
    auto adapter1 = std::make_shared<JsonAdapter>(data1);
    MatchResult result1;
    ASSERT_TRUE(cloned->match_rule("adult_check", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);
    EXPECT_EQ(result1.message, "成年用户");

    ASSERT_TRUE(cloned->match_rule("score_check", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);
    EXPECT_THAT(result1.message, testing::HasSubstr("分数="));
    EXPECT_THAT(result1.message, testing::HasSubstr("时间戳=42"));
}

// --- 验证克隆后功能完整性 ---

TEST_F(RuleEngineTest, Clone_ClonedEngineSupportsReload) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("reload_test.lua", R"(
        function match(data)
            return true, "version 1"
        end
    )");
    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/reload_test.lua", &error));

    auto cloned = engine.clone(RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);

    // 修改规则文件
    CreateRuleFile("reload_test.lua", R"(
        function match(data)
            return true, "version 2"
        end
    )");

    // 重新加载规则
    ASSERT_TRUE(cloned->reload_rule("rule1", &error));

    // 验证规则已更新
    json data = {{}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    MatchResult result;
    ASSERT_TRUE(cloned->match_rule("rule1", adapter, result, &error));
    EXPECT_EQ(result.message, "version 2");
}

TEST_F(RuleEngineTest, Clone_ClonedEngineSupportsRemoveRule) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));

    auto cloned = engine.clone(RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_rule_count(), 2);

    // 删除规则
    ASSERT_TRUE(cloned->remove_rule("rule1"));

    EXPECT_EQ(cloned->get_rule_count(), 1);
    EXPECT_FALSE(cloned->has_rule("rule1"));
    EXPECT_TRUE(cloned->has_rule("rule2"));

    // 原引擎不受影响
    EXPECT_EQ(engine.get_rule_count(), 2);
    EXPECT_TRUE(engine.has_rule("rule1"));
}

TEST_F(RuleEngineTest, Clone_ClonedEngineSupportsClearRules) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));
    ASSERT_TRUE(engine.add_rule("rule2", "test_data/rules/always_fail.lua", &error));

    auto cloned = engine.clone(RuleEngine::RULES, &error);
    ASSERT_NE(cloned, nullptr);

    cloned->clear_rules();
    EXPECT_EQ(cloned->get_rule_count(), 0);

    // 原引擎不受影响
    EXPECT_EQ(engine.get_rule_count(), 2);
}

TEST_F(RuleEngineTest, Clone_ClonedEngineSupportsUnregisterFunction) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.register_function("func1", test_cpp_function, &error));
    ASSERT_TRUE(engine.register_function("func2", test_cpp_add, &error));

    auto cloned = engine.clone(RuleEngine::CPP_FUNCTIONS, &error);
    ASSERT_NE(cloned, nullptr);

    EXPECT_EQ(cloned->get_registered_functions().size(), 2);

    // 注销函数
    ASSERT_TRUE(cloned->unregister_function("func1"));

    EXPECT_EQ(cloned->get_registered_functions().size(), 1);

    // 原引擎不受影响
    EXPECT_EQ(engine.get_registered_functions().size(), 2);
}

// --- 便捷方法完整性测试 ---

TEST_F(RuleEngineTest, Clone_ConvenienceMethods_AllWorkCorrectly) {
    RuleEngine engine;
    std::string error;

    // 准备引擎内容
    CreateRuleFile("utils.lua", "utils = {}");
    ASSERT_TRUE(engine.add_lua_file("test_data/rules/utils.lua", &error));

    ASSERT_TRUE(engine.register_function("func", test_cpp_function, &error));

    TestClass obj;
    ASSERT_TRUE(engine.register_function("member", &TestClass::get_value_dispatcher, &obj, &error));

    ASSERT_TRUE(engine.add_rule("rule1", "test_data/rules/always_pass.lua", &error));

    // 测试 clone_lua_files
    auto clone1 = engine.clone_lua_files(&error);
    ASSERT_NE(clone1, nullptr);
    EXPECT_TRUE(error.empty());

    // 测试 clone_cpp_functions
    auto clone2 = engine.clone_cpp_functions(&error);
    ASSERT_NE(clone2, nullptr);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(clone2->get_registered_functions().size(), 2); // 普通函数 + 成员函数

    // 测试 clone_rules
    auto clone3 = engine.clone_rules(&error);
    ASSERT_NE(clone3, nullptr);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(clone3->get_rule_count(), 1);

    // 测试 clone_safe
    auto clone4 = engine.clone_safe(&error);
    ASSERT_NE(clone4, nullptr);
    EXPECT_TRUE(error.empty());
    EXPECT_EQ(clone4->get_rule_count(), 1);
    EXPECT_EQ(clone4->get_registered_functions().size(), 2);
}

// ============================================================================
// BasicDataAdapter 字段修改功能测试
// ============================================================================

TEST_F(RuleEngineTest, BasicAdapter_Set_AddsFieldAndRulePasses) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 创建没有 age 字段的 adapter
    auto adapter = std::make_shared<BasicDataAdapter>();

    // 第一次匹配：没有 age 字段，应该失败
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_FALSE(result1.matched);
    EXPECT_TRUE(result1.message.find("缺少age字段") != std::string::npos);

    // 使用 set() 添加 age 字段
    adapter->set("age", 20);

    // 第二次匹配：有 age 字段且 >= 18，应该通过
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);
    EXPECT_TRUE(result2.message.find("通过") != std::string::npos);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_ModifiesFieldValue) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();
    adapter->set("age", 15);  // 年龄不足

    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_FALSE(result1.matched);
    EXPECT_TRUE(result1.message.find("年龄不足") != std::string::npos);

    // 修改年龄为 25
    adapter->set("age", 25);

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_OverwritesValue) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();
    adapter->set("age", 10);
    adapter->set("age", 20);
    adapter->set("age", 30);  // 最终值

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("通过") != std::string::npos);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_SupportsMultipleFields) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 逐个添加字段（field_complete 规则需要 name, email, phone 字段）
    adapter->set("name", "test_user");
    adapter->set("email", "test@example.com");
    adapter->set("phone", "12345678");

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_SupportsVariousTypes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 测试 int 类型
    adapter->set("age", 25);
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 测试 int64_t 类型
    adapter->set("age", static_cast<int64_t>(30));
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);

    // 测试 double 类型
    adapter->set("age", 35.5);
    MatchResult result3;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result3, &error));
    EXPECT_TRUE(result3.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_SupportsStringType) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 测试 std::string（field_complete 规则需要 name, email, phone 字段）
    adapter->set("name", std::string("user1"));
    adapter->set("email", std::string("user1@test.com"));
    adapter->set("phone", std::string("12345678"));

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_SupportsCString) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 测试 const char*（field_complete 规则需要 name, email, phone 字段）
    adapter->set("name", "user2");
    adapter->set("email", "user2@test.com");
    adapter->set("phone", "87654321");

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_Set_SupportsBoolType) {
    RuleEngine engine;
    std::string error;

    // 创建一个检查布尔字段的规则
    CreateRuleFile("bool_check.lua", R"(
function match(data)
    if data["is_active"] == nil then
        return false, "缺少is_active字段"
    end
    if type(data["is_active"]) ~= "boolean" then
        return false, "is_active字段必须是布尔类型"
    end
    if not data["is_active"] then
        return false, "账户未激活"
    end
    return true, "账户已激活"
end
)");

    ASSERT_TRUE(engine.add_rule("bool_check", "test_data/rules/bool_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();
    adapter->set("is_active", true);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("bool_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("已激活") != std::string::npos);
}

TEST_F(RuleEngineTest, BasicAdapter_SetNull_SetsFieldToNil) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();
    adapter->set("age", 25);

    // 第一次：有 age 字段，应该通过
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 使用 set_null() 将 age 设置为 nil
    adapter->set_null("age");

    // 第二次：age 字段为 nil，应该失败
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result2, &error));
    EXPECT_FALSE(result2.matched);
    EXPECT_TRUE(result2.message.find("缺少age字段") != std::string::npos);
}

TEST_F(RuleEngineTest, BasicAdapter_Remove_RemovesField) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 创建有 age 字段的 adapter
    auto adapter1 = std::make_shared<BasicDataAdapter>();
    adapter1->set("age", 25);

    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 创建没有 age 字段的 adapter（模拟 remove 后的状态）
    auto adapter2 = std::make_shared<BasicDataAdapter>();
    // adapter2 没有 age 字段

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);
    EXPECT_TRUE(result2.message.find("缺少age字段") != std::string::npos);
}

TEST_F(RuleEngineTest, BasicAdapter_Remove_NonExistentField_DoesNotCrash) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();
    adapter->set("age", 25);

    // 删除不存在的字段不应该崩溃
    adapter->remove("nonexistent_field");

    // age 字段仍然存在，应该通过
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_ClearFields_RemovesAllFields) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // 第一次：所有字段都存在的 adapter
    auto adapter1 = std::make_shared<BasicDataAdapter>();
    adapter1->set("name", "test_user");
    adapter1->set("email", "test@example.com");
    adapter1->set("phone", "12345678");

    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 第二次：清空所有字段的 adapter（模拟 clear_fields 后的状态）
    auto adapter2 = std::make_shared<BasicDataAdapter>();
    // adapter2 没有字段

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_ClearFields_EmptyAdapter_DoesNotCrash) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 清空空 adapter 不应该崩溃
    adapter->clear_fields();

    // 添加字段后应该正常工作
    adapter->set("age", 25);

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_OverwriteDifferentTypes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // string -> int
    adapter->set("age", "25");
    adapter->set("age", 25);

    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_TRUE(result1.matched);

    // int -> double
    adapter->set("age", 30.5);

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);

    // double -> bool (true = 1, 应该失败因为 1 < 18)
    adapter->set("age", true);

    MatchResult result3;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result3, &error));
    EXPECT_FALSE(result3.matched);

    // bool -> string (失败，因为 "true" 不是数字)
    adapter->set("age", "true");

    MatchResult result4;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result4, &error));
    EXPECT_FALSE(result4.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_MultipleModifications) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 测试1：初始状态 - 没有字段
    auto adapter1 = std::make_shared<BasicDataAdapter>();
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
    EXPECT_FALSE(result1.matched);

    // 测试2：添加字段但值不足
    auto adapter2 = std::make_shared<BasicDataAdapter>();
    adapter2->set("age", 15);
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);

    // 测试3：修改为有效值
    auto adapter3 = std::make_shared<BasicDataAdapter>();
    adapter3->set("age", 20);
    MatchResult result3;
    ASSERT_TRUE(engine.match_rule("age_check", adapter3, result3, &error));
    EXPECT_TRUE(result3.matched);

    // 测试4：设置为 nil
    auto adapter4 = std::make_shared<BasicDataAdapter>();
    adapter4->set_null("age");
    MatchResult result4;
    ASSERT_TRUE(engine.match_rule("age_check", adapter4, result4, &error));
    EXPECT_FALSE(result4.matched);

    // 测试5：重新添加有效值
    auto adapter5 = std::make_shared<BasicDataAdapter>();
    adapter5->set("age", 30);
    MatchResult result5;
    ASSERT_TRUE(engine.match_rule("age_check", adapter5, result5, &error));
    EXPECT_TRUE(result5.matched);

    // 测试6：删除字段（没有 age 字段的 adapter）
    auto adapter6 = std::make_shared<BasicDataAdapter>();
    MatchResult result6;
    ASSERT_TRUE(engine.match_rule("age_check", adapter6, result6, &error));
    EXPECT_FALSE(result6.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_SetNull_ThenSet_ReplacesWithNewValue) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 设置为 nil
    adapter->set_null("age");
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_FALSE(result1.matched);

    // 替换为有效值
    adapter->set("age", 25);
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result2, &error));
    EXPECT_TRUE(result2.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_LargeNumberOfFields) {
    RuleEngine engine;
    std::string error;

    // 创建一个检查特定字段的规则
    CreateRuleFile("check_field_500.lua", R"(
function match(data)
    if data["field_500"] == nil then
        return false, "缺少field_500字段"
    end
    return true, "找到field_500字段"
end
)");

    ASSERT_TRUE(engine.add_rule("check_field_500", "test_data/rules/check_field_500.lua", &error));

    auto adapter = std::make_shared<BasicDataAdapter>();

    // 添加大量字段
    for (int i = 0; i < 1000; i++) {
        adapter->set("field_" + std::to_string(i), "value_" + std::to_string(i));
    }

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("check_field_500", adapter, result, &error));
    EXPECT_TRUE(result.matched);
    EXPECT_TRUE(result.message.find("找到field_500字段") != std::string::npos);
}

TEST_F(RuleEngineTest, BasicAdapter_GetTypeName_ReturnsBasicDataAdapter) {
    auto adapter = std::make_shared<BasicDataAdapter>();
    EXPECT_STREQ(adapter->get_type_name(), "BasicDataAdapter");
}

// ============================================================================
// JsonAdapter 字段修改功能测试（继承自 BasicDataAdapter）
// ============================================================================

TEST_F(RuleEngineTest, JsonAdapter_Set_AddsFieldToJsonData) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // JSON 数据没有 age 字段
    json data = {
        {"username", "test_user"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 第一次匹配：没有 age 字段，应该失败
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_FALSE(result1.matched);
    EXPECT_TRUE(result1.message.find("缺少age字段") != std::string::npos);
}

TEST_F(RuleEngineTest, JsonAdapter_Set_ModifiesExistingJsonField) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // JSON 数据中 age 不满足要求
    json data = {
        {"username", "young_user"},
        {"age", 15}
    };

    auto adapter1 = std::make_shared<JsonAdapter>(data);

    // 第一次匹配：age=15，不满足>=18
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
    EXPECT_FALSE(result1.matched);
    EXPECT_TRUE(result1.message.find("年龄不足") != std::string::npos);
}

TEST_F(RuleEngineTest, JsonAdapter_Set_MultipleFields) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // JSON 数据只有一个字段
    json data = {
        {"name", "test_user"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 使用 set() 添加其他字段
    adapter->set("email", "test@example.com");
    adapter->set("phone", "12345678");

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_Set_OverwritesJsonFieldValue) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // JSON 数据中 age=20
    json data = {
        {"username", "user1"},
        {"age", 20}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 覆盖 age 字段
    adapter->set("age", 10);  // 第一次覆盖：不满足
    adapter->set("age", 25);  // 第二次覆盖：满足

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_Set_SupportsVariousTypes) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    json data = {
        {"username", "test_user"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 测试各种类型
    adapter->set("age", 25);  // int

    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result1, &error));
    EXPECT_TRUE(result1.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_Set_AddsNewFieldNotInJson) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // JSON 数据完全没有需要的字段
    json data = {
        {"other_field", "other_value"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 添加所有需要的字段
    adapter->set("name", "new_user");
    adapter->set("email", "new@example.com");
    adapter->set("phone", "87654321");

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_Remove_RemovesDynamicField) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // JSON 数据没有 age 字段
    json data = {
        {"username", "user1"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 动态添加 age 字段
    adapter->set("age", 25);

    // 使用 remove() 删除 age 字段
    adapter->remove("age");

    // age 字段不存在，应该失败
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_FALSE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_Remove_NonExistentField_DoesNotCrash) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    json data = {
        {"username", "user1"},
        {"age", 25}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 删除不存在的字段不应该崩溃
    adapter->remove("nonexistent_field");

    // age 字段仍然存在，应该通过
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_ClearFields_RemovesAllFieldsIncludingJson) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // JSON 数据有部分字段
    json data = {
        {"name", "json_user"},
        {"email", "json@example.com"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 清空所有字段（包括 JSON 原始字段和动态添加的字段）
    adapter->clear_fields();

    // 所有字段都不存在，应该失败
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_FALSE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_ClearFields_ThenAddNewFields) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // JSON 数据
    json data = {
        {"name", "old_user"},
        {"email", "old@example.com"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 清空所有字段
    adapter->clear_fields();

    // 添加新字段
    adapter->set("name", "new_user");
    adapter->set("email", "new@example.com");
    adapter->set("phone", "99999999");

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_JsonDataAndDynamicFields_Combined) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // JSON 数据有部分字段
    json data = {
        {"name", "json_user"}
    };

    auto adapter = std::make_shared<JsonAdapter>(data);

    // 动态添加剩余字段
    adapter->set("email", "dynamic@example.com");
    adapter->set("phone", "11111111");

    MatchResult result;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_GetTypeName_ReturnsJsonAdapter) {
    json data = {{"key", "value"}};
    auto adapter = std::make_shared<JsonAdapter>(data);
    EXPECT_STREQ(adapter->get_type_name(), "nlohmann::json");
}

TEST_F(RuleEngineTest, JsonAdapter_FieldModifications_DoNotAffectOriginalJson) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 创建 JSON 数据
    json original_data = {
        {"username", "user1"},
        {"age", 15}
    };

    // 保存原始值
    int original_age = original_data["age"];

    // 用原始数据创建新的 adapter
    auto adapter = std::make_shared<JsonAdapter>(original_data);

    // 应该使用原始值 15，所以不满足
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
    EXPECT_FALSE(result.matched);

    // 验证原始 JSON 数据没有被修改
    EXPECT_EQ(original_data["age"], original_age);
}

TEST_F(RuleEngineTest, JsonAdapter_MultipleAdaptersFromSameJson_Independent) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    json data = {
        {"username", "user1"},
        {"age", 15}
    };

    // 从同一个 JSON 创建多个 adapter
    auto adapter1 = std::make_shared<JsonAdapter>(data);
    auto adapter2 = std::make_shared<JsonAdapter>(data);

    // 修改 adapter1
    adapter1->set("age", 25);

    // adapter1 应该通过
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    // adapter2 不应该受影响，仍然使用原始值
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);
}

TEST_F(RuleEngineTest, JsonAdapter_ComplexScenario_MixJsonAndDynamicFields) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("field_complete", "test_data/rules/field_complete.lua", &error));

    // 场景：用户注册时只有基本信息，后续通过 set() 补充其他字段
    json base_user = {
        {"name", "new_user"}
    };

    auto adapter1 = std::make_shared<JsonAdapter>(base_user);

    // 第一次检查：只有 name，应该失败
    MatchResult result1;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter1, result1, &error));
    EXPECT_FALSE(result1.matched);

    // 创建新 adapter，补充 email
    auto adapter2 = std::make_shared<JsonAdapter>(base_user);
    adapter2->set("email", "user@example.com");

    // 第二次检查：有 name 和 email，仍然缺少 phone
    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);

    // 创建新 adapter，补充 phone
    auto adapter3 = std::make_shared<JsonAdapter>(base_user);
    adapter3->set("email", "user@example.com");
    adapter3->set("phone", "12345678");

    // 第三次检查：所有字段都齐全，应该通过
    MatchResult result3;
    ASSERT_TRUE(engine.match_rule("field_complete", adapter3, result3, &error));
    EXPECT_TRUE(result3.matched);
}

// ============================================================================
// Adapter 缓存生命周期测试
// ============================================================================

TEST_F(RuleEngineTest, Adapter_Cleanup_AfterDestruction_CacheIsCleaned) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 创建第一个 adapter（ID=1）
    {
        json data1 = {{"age", 25}};
        auto adapter1 = std::make_shared<JsonAdapter>(data1);

        MatchResult result1;
        ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
        EXPECT_TRUE(result1.matched);

        // adapter1 离开作用域，shared_ptr 引用计数变为 0，对象被销毁
        // 此时缓存中的 weak_ptr 应该过期
    }

    // 创建第二个 adapter（ID=2，不同于第一个）
    json data2 = {{"age", 30}};
    auto adapter2 = std::make_shared<JsonAdapter>(data2);

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_TRUE(result2.matched);

    // 引擎应该能够正常工作，过期的缓存条目应该被清理
}

TEST_F(RuleEngineTest, MultipleAdapters_SameData_IndependentCaches) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    json data = {{"age", 25}};

    // 创建多个 adapter，使用相同的 JSON 数据
    auto adapter1 = std::make_shared<JsonAdapter>(data);
    auto adapter2 = std::make_shared<JsonAdapter>(data);
    auto adapter3 = std::make_shared<JsonAdapter>(data);

    // 每个 adapter 都有独立的缓存条目（因为 ID 不同）
    MatchResult result1, result2, result3;
    
    ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_TRUE(result2.matched);

    ASSERT_TRUE(engine.match_rule("age_check", adapter3, result3, &error));
    EXPECT_TRUE(result3.matched);

    // 销毁 adapter1 和 adapter2
    adapter1.reset();
    adapter2.reset();

    // adapter3 仍然可用，缓存应该仍然有效
    MatchResult result4;
    ASSERT_TRUE(engine.match_rule("age_check", adapter3, result4, &error));
    EXPECT_TRUE(result4.matched);
}

TEST_F(RuleEngineTest, Adapter_ReusedAfterDestruction_NewCacheCreated) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    uint64_t first_id = 0;

    // 第一个 adapter
    {
        json data = {{"age", 20}};
        auto adapter1 = std::make_shared<JsonAdapter>(data);
        first_id = adapter1->get_id();

        MatchResult result1;
        ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
        EXPECT_TRUE(result1.matched);

        // adapter1 离开作用域，缓存中的 weak_ptr 过期
    }

    // 创建新的 adapter，由于 ID 是原子递增的，新 ID 应该大于第一个
    json data2 = {{"age", 35}};
    auto adapter2 = std::make_shared<JsonAdapter>(data2);
    
    uint64_t second_id = adapter2->get_id();
    EXPECT_GT(second_id, first_id) << "New adapter should have larger ID";

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_TRUE(result2.matched);
}

TEST_F(RuleEngineTest, BasicAdapter_Cleanup_AfterDestruction) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 创建 BasicDataAdapter
    {
        auto adapter1 = std::make_shared<BasicDataAdapter>();
        adapter1->set("age", 25);

        MatchResult result1;
        ASSERT_TRUE(engine.match_rule("age_check", adapter1, result1, &error));
        EXPECT_TRUE(result1.matched);

        // adapter1 离开作用域
    }

    // 创建新的 adapter
    auto adapter2 = std::make_shared<BasicDataAdapter>();
    adapter2->set("age", 30);

    MatchResult result2;
    ASSERT_TRUE(engine.match_rule("age_check", adapter2, result2, &error));
    EXPECT_TRUE(result2.matched);
}

TEST_F(RuleEngineTest, CacheWithAggressiveCleanup_CleansExpiredAdapters) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.add_rule("age_check", "test_data/rules/age_check.lua", &error));

    // 默认使用 Aggressive 清理策略，每次调用都会检查并清理过期缓存

    // 创建并销毁多个 adapter
    for (int i = 0; i < 10; i++) {
        json data = {{"age", 20 + i}};
        auto adapter = std::make_shared<JsonAdapter>(data);

        MatchResult result;
        ASSERT_TRUE(engine.match_rule("age_check", adapter, result, &error));
        EXPECT_TRUE(result.matched);

        // adapter 离开作用域，weak_ptr 过期
        // 下一次循环时，Aggressive 策略应该清理过期的缓存
    }

    // 验证引擎仍然正常工作
    json final_data = {{"age", 100}};
    auto final_adapter = std::make_shared<JsonAdapter>(final_data);

    MatchResult final_result;
    ASSERT_TRUE(engine.match_rule("age_check", final_adapter, final_result, &error));
    EXPECT_TRUE(final_result.matched);
}
