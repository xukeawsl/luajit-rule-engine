#include <gtest/gtest.h>
#include "ljre/rule_engine.h"
#include "ljre/json_adapter.h"
#include <fstream>
#include <map>

using namespace ljre;
using json = nlohmann::json;

// ============================================================================
// 完整工作流集成测试
// ============================================================================

class IntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        SetupTestEnvironment();
    }

    void TearDown() override {
        CleanupTestEnvironment();
    }

    void SetupTestEnvironment() {
        // 创建测试目录
        system("mkdir -p test_data/rules");
        system("mkdir -p test_data/configs");

        // 创建测试规则文件
        CreateRuleFile("age_validation.lua", R"(
function match(data)
    if data["age"] == nil then
        return false, "年龄字段不能为空"
    end

    if type(data["age"]) ~= "number" then
        return false, "年龄必须是数字"
    end

    if data["age"] < 18 then
        return false, "未满18岁，无法注册"
    end

    if data["age"] > 120 then
        return false, "年龄超出合理范围"
    end

    return true, "年龄验证通过"
end
)");

        CreateRuleFile("email_validation.lua", R"(
function match(data)
    if data["email"] == nil then
        return false, "邮箱不能为空"
    end

    local email = data["email"]
    if type(email) ~= "string" then
        return false, "邮箱必须是字符串"
    end

    -- 简单的邮箱格式验证
    -- 在 Lua 模式中，%. 匹配点号
    if not string.match(email, "^[A-Za-z0-9._%%+-]+@[A-Za-z0-9.-]+%.[A-Za-z]+$") then
        return false, "邮箱格式不正确"
    end

    return true, "邮箱验证通过"
end
)");

        CreateRuleFile("phone_validation.lua", R"(
function match(data)
    if data["phone"] == nil then
        return false, "手机号不能为空"
    end

    local phone = data["phone"]
    if type(phone) ~= "string" then
        return false, "手机号必须是字符串"
    end

    -- 去除所有非数字字符
    -- 在 C++ raw string 中需要写成 %%D 才能传给 Lua 的 %D
    local digits = string.gsub(phone, "%%D", "")

    if string.len(digits) ~= 11 then
        return false, "手机号必须是11位数字"
    end

    -- LuaJIT 不支持 %d{10} 这种量词语法，需要使用字符集 [0-9]
    if not string.match(digits, "^1[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]$") then
        return false, "手机号格式不正确"
    end

    return true, "手机号验证通过"
end
)");

        CreateRuleFile("completeness_check.lua", R"(
function match(data)
    local required_fields = {"username", "age", "email", "phone"}
    local missing = {}

    for _, field in ipairs(required_fields) do
        if data[field] == nil then
            table.insert(missing, field)
        end
    end

    if #missing > 0 then
        local msg = "缺少必填字段: " .. table.concat(missing, ", ")
        return false, msg
    end

    return true, "信息完整性检查通过"
end
)");

        // 创建配置文件
        std::ofstream config("test_data/configs/user_validation.lua");
        config << R"(
return {
    { name = "completeness", file = "test_data/rules/completeness_check.lua" },
    { name = "age_validation", file = "test_data/rules/age_validation.lua" },
    { name = "email_validation", file = "test_data/rules/email_validation.lua" },
    { name = "phone_validation", file = "test_data/rules/phone_validation.lua" }
}
)";
        config.close();
    }

    void CleanupTestEnvironment() {
        // 清理测试文件
        system("rm -rf test_data");
    }

    void CreateRuleFile(const std::string& filename, const std::string& content) {
        std::ofstream file("test_data/rules/" + filename);
        file << content;
    }
};

// ============================================================================
// 完整用户注册验证场景
// ============================================================================

TEST_F(IntegrationTest, UserRegistration_FullScenario) {
    // 1. 创建规则引擎并加载配置
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.load_rule_config("test_data/configs/user_validation.lua", &error))
        << "加载配置失败: " << error;

    EXPECT_EQ(engine.get_rule_count(), 4);

    // 2. 测试完整的有效用户数据
    json valid_user = {
        {"username", "zhang_san"},
        {"age", 25},
        {"email", "zhangsan@example.com"},
        {"phone", "13800138000"}  // 手机号需要是字符串
    };

    JsonAdapter adapter1(valid_user);
    std::map<std::string, MatchResult> results1;

    bool all_passed = engine.match_all_rules(adapter1, results1, &error);

    EXPECT_TRUE(all_passed) << "有效用户应该通过所有验证";
    ASSERT_EQ(results1.size(), 4);

    for (const auto& pair : results1) {
        EXPECT_TRUE(pair.second.matched) << "规则应该通过: " << pair.first << " - " << pair.second.message;
    }

    // 3. 测试年龄不足的用户
    json underage_user = {
        {"username", "li_si"},
        {"age", 16},
        {"email", "lisi@example.com"},
        {"phone", "13900139000"}
    };

    JsonAdapter adapter2(underage_user);
    std::map<std::string, MatchResult> results2;

    all_passed = engine.match_all_rules(adapter2, results2, &error);

    // 注意：修改后的语义是"只要有一个规则通过就返回 true"
    // 这里年龄验证失败，但手机号、邮箱等验证通过，所以返回 true
    EXPECT_TRUE(all_passed) << "部分验证通过，应该返回 true";
    ASSERT_EQ(results2.size(), 4);

    // 验证具体哪些规则通过了，哪些失败了
    EXPECT_FALSE(results2.at("age_validation").matched) << "年龄验证应该失败";
    EXPECT_TRUE(results2.at("phone_validation").matched) << "手机号验证应该通过";
}

TEST_F(IntegrationTest, UserRegistration_PartialData) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.load_rule_config("test_data/configs/user_validation.lua", &error));

    // 测试缺少部分字段的用户数据
    json incomplete_user = {
        {"username", "wang_wu"},
        {"age", 30}
        // 缺少 email 和 phone
    };

    JsonAdapter adapter(incomplete_user);
    std::map<std::string, MatchResult> results;

    engine.match_all_rules(adapter, results, &error);

    ASSERT_EQ(results.size(), 4);

    // 应该有规则失败
    EXPECT_FALSE(results.at("completeness").matched) << "完整性检查应该失败";
    EXPECT_FALSE(results.at("email_validation").matched) << "邮箱验证应该失败";
    EXPECT_FALSE(results.at("phone_validation").matched) << "手机号验证应该失败";
}

TEST_F(IntegrationTest, UserRegistration_InvalidEmailFormat) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.load_rule_config("test_data/configs/user_validation.lua", &error));

    // 测试邮箱格式错误的用户
    json invalid_email_user = {
        {"username", "zhao_liu"},
        {"age", 28},
        {"email", "invalid-email-format"},
        {"phone", "13700137000"}
    };

    JsonAdapter adapter(invalid_email_user);
    std::map<std::string, MatchResult> results;

    engine.match_all_rules(adapter, results, &error);

    // 检查邮箱验证是否失败
    EXPECT_FALSE(results.at("email_validation").matched) << "错误的邮箱格式应该被拒绝";
}

TEST_F(IntegrationTest, UserRegistration_InvalidPhoneFormat) {
    RuleEngine engine;
    std::string error;

    ASSERT_TRUE(engine.load_rule_config("test_data/configs/user_validation.lua", &error));

    // 测试手机号格式错误的用户
    json invalid_phone_user = {
        {"username", "qian_qi"},
        {"age", 35},
        {"email", "qianqi@example.com"},
        {"phone", "12345"}  // 格式错误
    };

    JsonAdapter adapter(invalid_phone_user);
    std::map<std::string, MatchResult> results;

    engine.match_all_rules(adapter, results, &error);

    // 检查手机号验证是否失败
    EXPECT_FALSE(results.at("phone_validation").matched) << "错误的手机号格式应该被拒绝";
}

// ============================================================================
// 规则热重载场景
// ============================================================================

TEST_F(IntegrationTest, HotReload_RuleModification) {
    RuleEngine engine;
    std::string error;

    // 创建初始规则
    CreateRuleFile("dynamic_rule.lua", R"(
function match(data)
    return data["value"] > 10, "值必须大于10"
end
)");

    ASSERT_TRUE(engine.add_rule("dynamic", "test_data/rules/dynamic_rule.lua", &error));

    // 测试原始规则
    json data1 = {{"value", 15}};
    JsonAdapter adapter1(data1);
    MatchResult result1;

    ASSERT_TRUE(engine.match_rule("dynamic", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    json data2 = {{"value", 5}};
    JsonAdapter adapter2(data2);
    MatchResult result2;

    ASSERT_TRUE(engine.match_rule("dynamic", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);

    // 修改规则
    CreateRuleFile("dynamic_rule.lua", R"(
function match(data)
    return data["value"] > 100, "值必须大于100"
end
)");

    // 热重载
    ASSERT_TRUE(engine.reload_rule("dynamic", &error));

    // 测试修改后的规则
    json data3 = {{"value", 50}};
    JsonAdapter adapter3(data3);
    MatchResult result3;

    ASSERT_TRUE(engine.match_rule("dynamic", adapter3, result3, &error));
    EXPECT_FALSE(result3.matched) << "50应该不大于100";

    json data4 = {{"value", 150}};
    JsonAdapter adapter4(data4);
    MatchResult result4;

    ASSERT_TRUE(engine.match_rule("dynamic", adapter4, result4, &error));
    EXPECT_TRUE(result4.matched) << "150应该大于100";
}

// ============================================================================
// 动态规则管理场景
// ============================================================================

TEST_F(IntegrationTest, DynamicRuleManagement_AddRemove) {
    RuleEngine engine;
    std::string error;

    // 创建一些规则
    CreateRuleFile("rule1.lua", R"(
function match(data)
    return data["field1"] ~= nil, "field1不能为空"
end
)");

    CreateRuleFile("rule2.lua", R"(
function match(data)
    return data["field2"] ~= nil, "field2不能为空"
end
)");

    // 动态添加规则
    ASSERT_TRUE(engine.add_rule("check_field1", "test_data/rules/rule1.lua", &error));
    ASSERT_TRUE(engine.add_rule("check_field2", "test_data/rules/rule2.lua", &error));

    EXPECT_EQ(engine.get_rule_count(), 2);

    // 测试两个规则
    json data = {{"field1", "value1"}, {"field2", "value2"}};
    JsonAdapter adapter(data);
    std::map<std::string, MatchResult> results;

    engine.match_all_rules(adapter, results, &error);

    EXPECT_TRUE(results.at("check_field1").matched);
    EXPECT_TRUE(results.at("check_field2").matched);

    // 移除一个规则
    ASSERT_TRUE(engine.remove_rule("check_field2"));
    EXPECT_EQ(engine.get_rule_count(), 1);

    // 再次测试
    std::map<std::string, MatchResult> results2;
    engine.match_all_rules(adapter, results2, &error);

    ASSERT_EQ(results2.size(), 1);
    EXPECT_TRUE(results2.at("check_field1").matched);
}

// ============================================================================
// 多引擎协作场景
// ============================================================================

TEST_F(IntegrationTest, MultipleEngines_IndependentOperation) {
    std::string error;

    // 创建第一个引擎用于测试环境
    CreateRuleFile("test_rule.lua", R"(
function match(data)
    return data["env"] == "test", "必须是测试环境"
end
)");

    RuleEngine test_engine;
    ASSERT_TRUE(test_engine.add_rule("env_check", "test_data/rules/test_rule.lua", &error));

    // 创建第二个引擎用于生产环境
    CreateRuleFile("prod_rule.lua", R"(
function match(data)
    return data["env"] == "production", "必须是生产环境"
end
)");

    RuleEngine prod_engine;
    ASSERT_TRUE(prod_engine.add_rule("env_check", "test_data/rules/prod_rule.lua", &error));

    // 测试不同环境的数据
    json test_data = {{"env", "test"}};
    JsonAdapter test_adapter(test_data);
    MatchResult test_result;

    ASSERT_TRUE(test_engine.match_rule("env_check", test_adapter, test_result, &error));
    EXPECT_TRUE(test_result.matched) << "测试引擎应该接受测试环境数据";

    json prod_data = {{"env", "production"}};
    JsonAdapter prod_adapter(prod_data);
    MatchResult prod_result;

    ASSERT_TRUE(prod_engine.match_rule("env_check", prod_adapter, prod_result, &error));
    EXPECT_TRUE(prod_result.matched) << "生产引擎应该接受生产环境数据";

    // 交叉测试（应该失败）
    MatchResult cross_result;
    ASSERT_TRUE(test_engine.match_rule("env_check", prod_adapter, cross_result, &error));
    EXPECT_FALSE(cross_result.matched) << "测试引擎应该拒绝生产环境数据";
}

// ============================================================================
// 复杂数据结构验证
// ============================================================================

TEST_F(IntegrationTest, ComplexDataStructure_NestedValidation) {
    RuleEngine engine;
    std::string error;

    // 创建验证嵌套结构的规则
    CreateRuleFile("address_validation.lua", R"(
function match(data)
    if data["address"] == nil then
        return false, "地址信息不能为空"
    end

    local addr = data["address"]
    if addr["province"] == nil or addr["city"] == nil or addr["district"] == nil then
        return false, "地址必须包含省市区信息"
    end

    if addr["detail"] == nil or addr["detail"] == "" then
        return false, "详细地址不能为空"
    end

    return true, "地址验证通过"
end
)");

    ASSERT_TRUE(engine.add_rule("address_check", "test_data/rules/address_validation.lua", &error));

    // 测试有效的地址
    json valid_address = {
        {"address", {
            {"province", "北京市"},
            {"city", "北京市"},
            {"district", "朝阳区"},
            {"detail", "某某街道123号"}
        }}
    };

    JsonAdapter adapter1(valid_address);
    MatchResult result1;

    ASSERT_TRUE(engine.match_rule("address_check", adapter1, result1, &error));
    EXPECT_TRUE(result1.matched);

    // 测试缺少字段的地址
    json invalid_address = {
        {"address", {
            {"province", "上海市"},
            {"city", "上海市"}
            // 缺少 district 和 detail
        }}
    };

    JsonAdapter adapter2(invalid_address);
    MatchResult result2;

    ASSERT_TRUE(engine.match_rule("address_check", adapter2, result2, &error));
    EXPECT_FALSE(result2.matched);
}

// ============================================================================
// 边界条件和异常场景
// ============================================================================

TEST_F(IntegrationTest, EmptyData_HandledCorrectly) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("non_empty_check.lua", R"(
function match(data)
    for key, value in pairs(data) do
        -- 只要有数据就算通过
        return true, "有数据"
    end
    return false, "数据为空"
end
)");

    ASSERT_TRUE(engine.add_rule("empty_check", "test_data/rules/non_empty_check.lua", &error));

    // 测试空对象
    json empty_data = json::object();
    JsonAdapter adapter1(empty_data);
    MatchResult result1;

    ASSERT_TRUE(engine.match_rule("empty_check", adapter1, result1, &error));
    EXPECT_FALSE(result1.matched);

    // 测试有数据的对象
    json non_empty_data = {{"key", "value"}};
    JsonAdapter adapter2(non_empty_data);
    MatchResult result2;

    ASSERT_TRUE(engine.match_rule("empty_check", adapter2, result2, &error));
    EXPECT_TRUE(result2.matched);
}

TEST_F(IntegrationTest, LargeDataSet_HandledCorrectly) {
    RuleEngine engine;
    std::string error;

    CreateRuleFile("array_check.lua", R"(
function match(data)
    if data["items"] == nil then
        return false, "items字段不能为空"
    end

    if type(data["items"]) ~= "table" then
        return false, "items必须是数组"
    end

    local count = 0
    for k, v in pairs(data["items"]) do
        count = count + 1
    end

    if count < 10 then
        return false, "至少需要10个项目"
    end

    return true, "项目数量检查通过"
end
)");

    ASSERT_TRUE(engine.add_rule("items_check", "test_data/rules/array_check.lua", &error));

    // 创建包含大量项目的数据
    json large_data;
    large_data["items"] = json::array();
    for (int i = 0; i < 100; ++i) {
        large_data["items"].push_back({{"id", i}, {"name", "item" + std::to_string(i)}});
    }

    JsonAdapter adapter(large_data);
    MatchResult result;

    ASSERT_TRUE(engine.match_rule("items_check", adapter, result, &error));
    EXPECT_TRUE(result.matched);
}

// ============================================================================
// 真实业务场景模拟
// ============================================================================

TEST_F(IntegrationTest, ECommerce_OrderValidation) {
    RuleEngine engine;
    std::string error;

    // 创建订单验证规则
    CreateRuleFile("order_amount.lua", R"(
function match(data)
    local amount = data["amount"]
    if amount == nil then
        return false, "订单金额不能为空"
    end

    if amount <= 0 then
        return false, "订单金额必须大于0"
    end

    if amount > 1000000 then
        return false, "订单金额超出限制"
    end

    return true, "金额验证通过"
end
)");

    CreateRuleFile("order_items.lua", R"(
function match(data)
    local items = data["items"]
    if items == nil then
        return false, "订单商品不能为空"
    end

    if #items == 0 then
        return false, "订单至少包含一件商品"
    end

    for _, item in ipairs(items) do
        if item["product_id"] == nil then
            return false, "商品必须包含product_id"
        end
        if item["quantity"] == nil or item["quantity"] <= 0 then
            return false, "商品数量必须大于0"
        end
    end

    return true, "商品验证通过"
end
)");

    ASSERT_TRUE(engine.add_rule("amount_check", "test_data/rules/order_amount.lua", &error));
    ASSERT_TRUE(engine.add_rule("items_check", "test_data/rules/order_items.lua", &error));

    // 测试有效订单
    json valid_order = {
        {"amount", 299.99},
        {"items", {
            {{"product_id", "P001"}, {"quantity", 2}},
            {{"product_id", "P002"}, {"quantity", 1}}
        }}
    };

    JsonAdapter adapter1(valid_order);
    std::map<std::string, MatchResult> results1;

    EXPECT_TRUE(engine.match_all_rules(adapter1, results1, &error));

    // 测试无效订单（金额为0）
    json invalid_order1 = {
        {"amount", 0},
        {"items", {{{"product_id", "P001"}, {"quantity", 1}}}}
    };

    JsonAdapter adapter2(invalid_order1);
    std::map<std::string, MatchResult> results2;

    // 注意：修改后的语义是"只要有一个规则通过就返回 true"
    // 这里金额验证失败，但商品验证通过，所以返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter2, results2, &error));
    EXPECT_FALSE(results2.at("amount_check").matched) << "金额验证应该失败";
    EXPECT_TRUE(results2.at("items_check").matched) << "商品验证应该通过";

    // 测试无效订单（商品数量为0）
    json invalid_order2 = {
        {"amount", 100.00},
        {"items", {{{"product_id", "P001"}, {"quantity", 0}}}}
    };

    JsonAdapter adapter3(invalid_order2);
    std::map<std::string, MatchResult> results3;

    // 注意：修改后的语义是"只要有一个规则通过就返回 true"
    // 这里商品验证失败，但金额验证通过，所以返回 true
    EXPECT_TRUE(engine.match_all_rules(adapter3, results3, &error));
    EXPECT_TRUE(results3.at("amount_check").matched) << "金额验证应该通过";
    EXPECT_FALSE(results3.at("items_check").matched) << "商品验证应该失败";
}

// ============================================================================
// 嵌套深度限制集成测试
// ============================================================================

TEST_F(IntegrationTest, NestedDepthLimit_DeeplyNestedDataTruncatesCorrectly) {
    // 创建规则引擎
    RuleEngine engine;
    std::string error;

    // 创建一个规则，用于测试深度嵌套数据的访问
    CreateRuleFile("nested_depth_check.lua", R"(
function match(data)
    -- 检查第1层（应该存在）
    if data["level1"] == nil then
        return false, "第1层数据丢失"
    end

    -- 检查第2层（应该存在）
    if data["level1"]["level2"] == nil then
        return false, "第2层数据丢失"
    end

    -- 检查第3层（应该存在）
    if data["level1"]["level2"]["level3"] == nil then
        return false, "第3层数据丢失"
    end

    -- 检查第4层（应该是 nil，因为超出了深度限制）
    if data["level1"]["level2"]["level3"]["level4"] ~= nil then
        return false, "第4层数据应该被截断为nil"
    end

    return true, "深度限制验证通过"
end
)");

    ASSERT_TRUE(engine.add_rule("nested_depth_check", "test_data/rules/nested_depth_check.lua", &error));

    // 创建4层嵌套的JSON数据
    // 结构：{"level1": {"level2": {"level3": {"level4": "deep_value"}}}}
    json nested_data = {
        {"level1", {
            {"level2", {
                {"level3", {
                    {"level4", "deep_value"}
                }}
            }}
        }}
    };

    // 使用最大嵌套深度为4的 JsonAdapter
    // 这意味着：根对象(深度0) -> level1(深度1) -> level2(深度2) -> level3(深度3) -> level4(深度4)
    // 当 max_depth=4 时，处理 level3 的值时（current_depth=3），检查 current_depth + 1 >= 4 为真
    // 所以 level3 的子元素 level4 会被截断为 nil
    JsonAdapter adapter(nested_data, 4);

    // 执行规则
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("nested_depth_check", adapter, result, &error));

    // 规则应该通过，因为超出的深度访问到了 nil
    EXPECT_TRUE(result.matched) << "规则验证失败: " << result.message;
    EXPECT_STREQ(result.message.c_str(), "深度限制验证通过");
}

TEST_F(IntegrationTest, NestedDepthLimit_ArrayWithinObjectTruncatesCorrectly) {
    // 测试嵌套数组中的深度限制
    RuleEngine engine;
    std::string error;

    CreateRuleFile("array_nested_depth_check.lua", R"(
function match(data)
    -- 检查根层的 user 字段
    if data["user"] == nil then
        return false, "user字段不存在"
    end

    -- 检查 user.name（第2层）
    if data["user"]["name"] == nil then
        return false, "name字段不存在"
    end

    -- 检查 user.profile（第2层）
    if data["user"]["profile"] == nil then
        return false, "profile字段不存在"
    end

    -- 检查 user.profile.settings（第3层，应该存在）
    if data["user"]["profile"]["settings"] == nil then
        return false, "settings字段不存在"
    end

    -- 检查 user.profile.settings.theme（第4层，应该为nil）
    if data["user"]["profile"]["settings"]["theme"] ~= nil then
        return false, "theme应该被截断为nil"
    end

    -- 检查数组元素中的深层嵌套
    if data["user"]["orders"] == nil then
        return false, "orders数组不存在"
    end

    -- orders 数组存在，数组元素也存在
    if data["user"]["orders"][1] == nil then
        return false, "orders数组元素不存在"
    end

    -- 但数组元素的字段（第5层）应该被截断为nil
    if data["user"]["orders"][1]["id"] ~= nil then
        return false, "orders数组元素的id字段应该被截断为nil"
    end

    if data["user"]["orders"][1]["amount"] ~= nil then
        return false, "orders数组元素的amount字段应该被截断为nil"
    end

    return true, "数组嵌套深度限制验证通过"
end
)");

    ASSERT_TRUE(engine.add_rule("array_nested_depth_check", "test_data/rules/array_nested_depth_check.lua", &error));

    // 创建包含数组的嵌套JSON（4层）
    // 结构：
    // {
    //   "user": {
    //     "name": "Alice",                    // 第2层
    //     "profile": {
    //       "settings": {"theme": "dark"}     // 第4层
    //     },
    //     "orders": [
    //       {"id": 1, "amount": 100}          // 第4层
    //     ]
    //   }
    // }
    json data = {
        {"user", {
            {"name", "Alice"},
            {"profile", {
                {"settings", {
                    {"theme", "dark"}
                }}
            }},
            {"orders", {
                {{"id", 1}, {"amount", 100}},
                {{"id", 2}, {"amount", 200}}
            }}
        }}
    };

    // 限制最大深度为4
    JsonAdapter adapter(data, 4);

    // 执行规则
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("array_nested_depth_check", adapter, result, &error));

    // 规则应该通过
    EXPECT_TRUE(result.matched) << "规则验证失败: " << result.message;
    EXPECT_STREQ(result.message.c_str(), "数组嵌套深度限制验证通过");
}

TEST_F(IntegrationTest, NestedDepthLimit_SafeAccessPattern) {
    // 测试在深度限制场景下如何安全地访问嵌套数据
    RuleEngine engine;
    std::string error;

    CreateRuleFile("safe_access_pattern.lua", R"(
function match(data)
    -- 4层嵌套结构，限制深度为2
    -- 结构：{"level1": {"level2": {"level3": {"level4": "value"}}}}

    -- 方式1：直接访问 level3 会报错（因为 level2 是 nil）
    -- data["level1"]["level2"]["level3"]  -- 这会报错！

    -- 方式2：使用 and 短路求值安全访问
    local level3 = data["level1"] and data["level1"]["level2"] and data["level1"]["level2"]["level3"]
    if level3 ~= nil then
        return false, "level3应该为nil"
    end

    -- 方式3：逐层检查
    local level1 = data["level1"]
    if level1 == nil then
        return false, "level1不应该为nil"
    end

    local level2 = level1["level2"]
    if level2 == nil then
        -- level2 被截断为 nil，这是预期的
        -- 不能再访问 level2["level3"]，否则会报错
    else
        return false, "level2应该为nil"
    end

    -- 方式4：使用 pcall 捕获错误（不推荐，性能较差）
    local ok, value = pcall(function()
        return data["level1"]["level2"]["level3"]
    end)
    if ok then
        -- 如果没有报错，说明能访问到（不应该发生）
        return false, "访问level3应该失败"
    end
    -- err 包含错误信息，这是预期的

    return true, "安全访问模式验证通过"
end
)");

    ASSERT_TRUE(engine.add_rule("safe_access_pattern", "test_data/rules/safe_access_pattern.lua", &error));

    // 创建4层嵌套的JSON数据
    json nested_data = {
        {"level1", {
            {"level2", {
                {"level3", {
                    {"level4", "deep_value"}
                }}
            }}
        }}
    };

    // 限制最大深度为2
    // level1 存在，level2 被截断为 nil
    JsonAdapter adapter(nested_data, 2);

    // 执行规则
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("safe_access_pattern", adapter, result, &error));

    // 规则应该通过
    EXPECT_TRUE(result.matched) << "规则验证失败: " << result.message;
    EXPECT_STREQ(result.message.c_str(), "安全访问模式验证通过");
}

TEST_F(IntegrationTest, NestedDepthLimit_ArraySafeAccessPattern) {
    // 测试数组嵌套在深度限制场景下的安全访问
    RuleEngine engine;
    std::string error;

    CreateRuleFile("array_safe_access.lua", R"(
function match(data)
    -- 4层数组嵌套结构，限制深度为2
    -- 结构：{"items": [[[[1, 2, 3]]]]}
    -- items 是数组，items[1] 是数组，items[1][1] 是数组，items[1][1][1] 是数字

    -- 方式1：直接深层访问会报错（因为中间层被截断为 nil）
    -- data["items"][1][1][1]  -- 这会报错！

    -- 方式2：使用 and 短路求值安全访问数组
    local value = data["items"] and data["items"][1] and data["items"][1][1] and data["items"][1][1][1]
    if value ~= nil then
        return false, "深层元素应该为nil"
    end

    -- 方式3：逐层检查数组
    local items = data["items"]
    if items == nil then
        return false, "items数组不应该为nil"
    end

    -- items 数组存在（深度2）
    -- 但数组元素（深度3）被截断为 nil
    local level1 = items[1]
    if level1 ~= nil then
        return false, "第一层数组元素应该为nil"
    end

    -- 方式4：检查数组类型
    if type(items) ~= "table" then
        return false, "items应该是table"
    end

    -- 数组本身存在，但元素都被设置为 nil
    -- 在深度限制 max_depth=2 下，处理 items 数组时 current_depth=1
    -- 检查 current_depth + 1 >= 2，即 2 >= 2 为真
    -- 所以数组元素全部被设置为 nil
    -- ipairs 在遇到 nil 时会停止，所以我们用 rawget 手动检查
    local element = rawget(items, 1)
    if element ~= nil then
        return false, "数组元素[1]应该为nil"
    end

    -- 检查确实设置了元素（虽然值为nil）
    -- 使用 # 操作符获取数组长度
    local len = #items
    -- 长度可能为0，因为所有元素都是nil

    return true, "数组安全访问模式验证通过"
end
)");

    ASSERT_TRUE(engine.add_rule("array_safe_access", "test_data/rules/array_safe_access.lua", &error));

    // 创建4层数组嵌套的JSON数据
    // 结构：{"items": [[[[1, 2, 3]]]]}
    // items 是数组，items[0] 是数组，items[0][0] 是数组，items[0][0][0] 是数组
    json array_data = {
        {"items", {
            {{{{1, 2, 3}}}}
        }}
    };

    // 限制最大深度为2
    // items 存在（深度1），items[1] 存在（深度2）
    // items[1][1] 被截断为 nil（深度3）
    JsonAdapter adapter(array_data, 2);

    // 执行规则
    MatchResult result;
    ASSERT_TRUE(engine.match_rule("array_safe_access", adapter, result, &error));

    // 规则应该通过
    EXPECT_TRUE(result.matched) << "规则验证失败: " << result.message;
    EXPECT_STREQ(result.message.c_str(), "数组安全访问模式验证通过");
}

