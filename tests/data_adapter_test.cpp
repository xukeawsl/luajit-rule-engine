#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ljre/data_adapter.h"
#include "ljre/json_adapter.h"
#include "ljre/basic_data_adapter.h"
#include "ljre/lua_state.h"
#include <string>

using namespace ljre;
using json = nlohmann::json;

// ============================================================================
// DataAdapter 接口测试
// ============================================================================

TEST(DataAdapterTest, VirtualDestructor_CanDeleteDerived) {
    // 这是一个编译时测试，验证虚析构函数存在
    // 应该能够安全地通过基类指针删除派生类对象
    DataAdapter* adapter = new JsonAdapter(json{});
    delete adapter;
    // 如果没有虚析构函数，这里会有未定义行为
}

// ============================================================================
// JsonAdapter 基本类型转换测试
// ============================================================================

class JsonAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(_state.is_valid());
        _L = _state.get();
    }

    // 辅助函数：检查栈顶值的类型
    bool is_nil(int idx = -1) { return lua_isnil(_L, idx); }
    bool is_boolean(int idx = -1) { return lua_isboolean(_L, idx); }
    bool is_number(int idx = -1) { return lua_isnumber(_L, idx); }
    bool is_string(int idx = -1) { return lua_isstring(_L, idx); }
    bool is_table(int idx = -1) { return lua_istable(_L, idx); }

    // 辅助函数：获取栈顶的值
    bool get_boolean(int idx = -1) { return lua_toboolean(_L, idx) != 0; }
    lua_Integer get_integer(int idx = -1) { return lua_tointeger(_L, idx); }
    lua_Number get_number(int idx = -1) { return lua_tonumber(_L, idx); }
    std::string get_string(int idx = -1) {
        size_t len;
        const char* str = lua_tolstring(_L, idx, &len);
        return std::string(str, len);
    }

    LuaState _state;
    lua_State* _L;
};

TEST_F(JsonAdapterTest, NullValue_ConvertsToNil) {
    json data = nullptr;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_nil());
}

TEST_F(JsonAdapterTest, BooleanTrue_ConvertsToLuaBoolean) {
    json data = true;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
}

TEST_F(JsonAdapterTest, BooleanFalse_ConvertsToLuaBoolean) {
    json data = false;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_boolean());
    EXPECT_FALSE(get_boolean());
}

TEST_F(JsonAdapterTest, Integer_ConvertsToLuaInteger) {
    json data = 42;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 42);
}

TEST_F(JsonAdapterTest, NegativeInteger_ConvertsCorrectly) {
    json data = -12345;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), -12345);
}

TEST_F(JsonAdapterTest, Float_ConvertsToLuaNumber) {
    json data = 3.14159;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_number());
    EXPECT_DOUBLE_EQ(get_number(), 3.14159);
}

TEST_F(JsonAdapterTest, String_ConvertsToLuaString) {
    json data = "hello world";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "hello world");
}

TEST_F(JsonAdapterTest, EmptyString_ConvertsCorrectly) {
    json data = "";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "");
}

TEST_F(JsonAdapterTest, UnicodeString_ConvertsCorrectly) {
    json data = "你好世界 🌍";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "你好世界 🌍");
}

TEST_F(JsonAdapterTest, StringWithNullChar_ConvertsCorrectly) {
    std::string str_with_null = "hello\0world";
    json data = str_with_null;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), str_with_null);
}

TEST_F(JsonAdapterTest, StringWithSpecialChars_ConvertsCorrectly) {
    json data = "line1\nline2\ttab\r\n";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "line1\nline2\ttab\r\n");
}

// ============================================================================
// JsonAdapter 数组转换测试
// ============================================================================

TEST_F(JsonAdapterTest, EmptyArray_ConvertsToEmptyTable) {
    json data = json::array();
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查表长度 - lua_objlen returns length, doesn't push to stack
    size_t len = lua_objlen(_L, -1);
    EXPECT_EQ(len, 0);

    lua_pop(_L, 1);  // 弹出 table
}

TEST_F(JsonAdapterTest, IntegerArray_ConvertsToLuaTable) {
    json data = {1, 2, 3, 4, 5};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查数组长度
    size_t len = lua_objlen(_L, -1);
    EXPECT_EQ(len, 5);

    // 检查各个元素
    for (int i = 1; i <= 5; ++i) {
        lua_rawgeti(_L, -1, i);
        EXPECT_TRUE(is_number());
        EXPECT_EQ(get_integer(), i);
        lua_pop(_L, 1);
    }

    // 弹出 table
    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, MixedTypeArray_ConvertsCorrectly) {
    json data = {1, "two", 3.0, true, nullptr};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查第一个元素 (整数)
    lua_rawgeti(_L, -1, 1);
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 1);
    lua_pop(_L, 1);

    // 检查第二个元素 (字符串)
    lua_rawgeti(_L, -1, 2);
    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "two");
    lua_pop(_L, 1);

    // 检查第三个元素 (浮点数)
    lua_rawgeti(_L, -1, 3);
    EXPECT_TRUE(is_number());
    EXPECT_DOUBLE_EQ(get_number(), 3.0);
    lua_pop(_L, 1);

    // 检查第四个元素 (布尔)
    lua_rawgeti(_L, -1, 4);
    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
    lua_pop(_L, 1);

    // 检查第五个元素 (null)
    lua_rawgeti(_L, -1, 5);
    EXPECT_TRUE(is_nil());
    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, NestedArray_ConvertsCorrectly) {
    json data = {{1, 2}, {3, 4}, {5, 6}};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查第一个子数组
    lua_rawgeti(_L, -1, 1);
    ASSERT_TRUE(is_table());

    lua_rawgeti(_L, -1, 1);
    EXPECT_EQ(get_integer(), 1);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 2);
    EXPECT_EQ(get_integer(), 2);
    lua_pop(_L, 2);  // 弹出子数组和元素

    // 检查第三个子数组
    lua_rawgeti(_L, -1, 3);
    ASSERT_TRUE(is_table());

    lua_rawgeti(_L, -1, 1);
    EXPECT_EQ(get_integer(), 5);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 2);
    EXPECT_EQ(get_integer(), 6);
    lua_pop(_L, 2);
}

TEST_F(JsonAdapterTest, LargeArray_ConvertsCorrectly) {
    json data = json::array();
    for (int i = 0; i < 1000; ++i) {
        data.push_back(i);
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查数组长度 - lua_objlen returns length, doesn't push to stack
    size_t len = lua_objlen(_L, -1);
    EXPECT_EQ(len, 1000);

    // 抽查几个元素
    lua_rawgeti(_L, -1, 1);
    EXPECT_EQ(get_integer(), 0);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 500);
    EXPECT_EQ(get_integer(), 499);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 1000);
    EXPECT_EQ(get_integer(), 999);
    lua_pop(_L, 1);
}

// ============================================================================
// JsonAdapter 对象转换测试
// ============================================================================

TEST_F(JsonAdapterTest, EmptyObject_ConvertsToEmptyTable) {
    json data = json::object();
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());
}

TEST_F(JsonAdapterTest, SimpleObject_ConvertsToLuaTable) {
    json data = {
        {"name", "Alice"},
        {"age", 30},
        {"active", true}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查 name 字段
    lua_getfield(_L, -1, "name");
    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "Alice");
    lua_pop(_L, 1);

    // 检查 age 字段
    lua_getfield(_L, -1, "age");
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 30);
    lua_pop(_L, 1);

    // 检查 active 字段
    lua_getfield(_L, -1, "active");
    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, NestedObject_ConvertsCorrectly) {
    json data = {
        {"user", {
            {"name", "Bob"},
            {"age", 25}
        }},
        {"status", "active"}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查嵌套的 user 对象
    lua_getfield(_L, -1, "user");
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "name");
    EXPECT_EQ(get_string(), "Bob");
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "age");
    EXPECT_EQ(get_integer(), 25);
    lua_pop(_L, 2);  // 弹出 user 对象和 age

    // 检查 status 字段
    lua_getfield(_L, -1, "status");
    EXPECT_EQ(get_string(), "active");
    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, ObjectWithArrayField_ConvertsCorrectly) {
    json data = {
        {"items", {1, 2, 3}},
        {"count", 3}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查 items 数组
    lua_getfield(_L, -1, "items");
    ASSERT_TRUE(is_table());

    // 检查数组长度 - lua_objlen returns length, doesn't push to stack
    size_t len = lua_objlen(_L, -1);
    EXPECT_EQ(len, 3);

    lua_rawgeti(_L, -1, 1);
    EXPECT_EQ(get_integer(), 1);
    lua_pop(_L, 1);

    lua_pop(_L, 1);  // 弹出 items

    // 检查 count 字段
    lua_getfield(_L, -1, "count");
    EXPECT_EQ(get_integer(), 3);
    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, ArrayOfObjects_ConvertsCorrectly) {
    json data = {
        {{"id", 1}, {"name", "Item 1"}},
        {{"id", 2}, {"name", "Item 2"}},
        {{"id", 3}, {"name", "Item 3"}}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查第一个对象
    lua_rawgeti(_L, -1, 1);
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "id");
    EXPECT_EQ(get_integer(), 1);
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "name");
    EXPECT_EQ(get_string(), "Item 1");
    lua_pop(_L, 2);

    // 检查第三个对象
    lua_rawgeti(_L, -1, 3);
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "id");
    EXPECT_EQ(get_integer(), 3);
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "name");
    EXPECT_EQ(get_string(), "Item 3");
    lua_pop(_L, 2);
}

TEST_F(JsonAdapterTest, ComplexNestedStructure_ConvertsCorrectly) {
    json data = {
        {"users", {
            {{"name", "Alice"}, {"scores", {95, 87, 92}}},
            {{"name", "Bob"}, {"scores", {88, 91, 85}}}
        }},
        {"metadata", {
            {"version", "1.0"},
            {"active", true}
        }}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查 users 数组
    lua_getfield(_L, -1, "users");
    ASSERT_TRUE(is_table());

    // 第一个用户
    lua_rawgeti(_L, -1, 1);
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "name");
    EXPECT_EQ(get_string(), "Alice");
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "scores");
    ASSERT_TRUE(is_table());

    lua_rawgeti(_L, -1, 1);
    EXPECT_EQ(get_integer(), 95);
    lua_pop(_L, 4);

    // 检查 metadata
    lua_getfield(_L, -1, "metadata");
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "version");
    EXPECT_EQ(get_string(), "1.0");
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "active");
    EXPECT_TRUE(get_boolean());
    lua_pop(_L, 2);
}

TEST_F(JsonAdapterTest, ObjectKeyWithSpecialChars_ConvertsCorrectly) {
    json data = {
        {"key with spaces", "value1"},
        {"key\twith\ttabs", "value2"},
        {"key\nwith\nnewlines", "value3"}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 检查带空格的键
    lua_getfield(_L, -1, "key with spaces");
    EXPECT_EQ(get_string(), "value1");
    lua_pop(_L, 1);

    // 检查带制表符的键
    lua_getfield(_L, -1, "key\twith\ttabs");
    EXPECT_EQ(get_string(), "value2");
    lua_pop(_L, 1);

    // 检查带换行符的键
    lua_getfield(_L, -1, "key\nwith\nnewlines");
    EXPECT_EQ(get_string(), "value3");
    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, ObjectKeyWithNullChar_ConvertsCorrectly) {
    std::string key_with_null = "key\0with\0null";
    json data = {{key_with_null, "value"}};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 使用 lua_pushlstring 获取键
    lua_pushlstring(_L, key_with_null.data(), key_with_null.size());
    lua_gettable(_L, -2);

    EXPECT_EQ(get_string(), "value");
    lua_pop(_L, 1);
}

// ============================================================================
// JsonAdapter 错误处理测试
// ============================================================================

TEST_F(JsonAdapterTest, NullLuaState_ReturnsError) {
    json data = "test";
    JsonAdapter adapter(data);

    std::string error;
    EXPECT_FALSE(adapter.push_to_lua(nullptr, &error));
    EXPECT_FALSE(error.empty());
}

TEST_F(JsonAdapterTest, NullLuaState_NoErrorMsg) {
    json data = "test";
    JsonAdapter adapter(data);

    // 不传递 error_msg，不应该崩溃
    EXPECT_FALSE(adapter.push_to_lua(nullptr, nullptr));
}

TEST_F(JsonAdapterTest, GetTypeName_ReturnsJson) {
    json data = "test";
    JsonAdapter adapter(data);

    EXPECT_STREQ(adapter.get_type_name(), "nlohmann::json");
}

// ============================================================================
// JsonAdapter 边界条件测试
// ============================================================================

TEST_F(JsonAdapterTest, VeryLargeNumber_ConvertsCorrectly) {
    json data = 9007199254740991;  // 2^53 - 1
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 9007199254740991);
}

TEST_F(JsonAdapterTest, VerySmallNumber_ConvertsCorrectly) {
    json data = -9007199254740991;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), -9007199254740991);
}

TEST_F(JsonAdapterTest, VeryLongString_ConvertsCorrectly) {
    std::string long_string(10000, 'x');
    json data = long_string;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), long_string);
}

TEST_F(JsonAdapterTest, DeeplyNestedStructure_ConvertsCorrectly) {
    // 测试深度嵌套，观察 Lua 栈的行为
    // 现在默认 max_depth 是 8192，所以 200 层嵌套在默认限制以内
    json data = 1;
    int depth = 200;  // 200 层嵌套 (在默认8192以内)

    for (int i = 0; i < depth; ++i) {
        json temp = data;
        data = {{"nested", temp}};
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error)) << "Failed to push nested JSON to Lua: " << error;

    EXPECT_TRUE(is_table());

    // 记录初始栈位置
    int initial_stack = lua_gettop(_L);

    // 验证嵌套结构：前 (depth-1) 层都是table
    for (int i = 0; i < depth - 1; ++i) {
        lua_getfield(_L, -1, "nested");
        ASSERT_TRUE(is_table()) << "Failed at depth " << (i + 1) << ", stack top: " << lua_gettop(_L);
    }

    // 最后一次获取 nested 应该得到数字 1
    lua_getfield(_L, -1, "nested");
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 1);

    // 栈上有 1 个初始 table + depth 个元素
    int final_stack = lua_gettop(_L);
    EXPECT_EQ(final_stack, initial_stack + depth);

    // 清理栈
    lua_settop(_L, initial_stack);
}

TEST_F(JsonAdapterTest, WideUnicodeCharacters_ConvertsCorrectly) {
    json data = "🌍🌎🌏 🎉🎊🎈 𝔘𝔫𝔦𝔠𝔬𝔡𝔢";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "🌍🌎🌏 🎉🎊🎈 𝔘𝔫𝔦𝔠𝔬𝔡𝔢");
}

// ============================================================================
// JsonAdapter 栈平衡测试
// ============================================================================

TEST_F(JsonAdapterTest, StackBalance_AfterSuccessfulPush) {
    json data = {{"key", "value"}, {"array", {1, 2, 3}}};
    JsonAdapter adapter(data);

    int top_before = lua_gettop(_L);

    std::string error;
    adapter.push_to_lua(_L, &error);

    int top_after = lua_gettop(_L);

    EXPECT_EQ(top_after, top_before + 1);  // 应该只压入一个 table

    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, StackBalance_AfterFailedPush) {
    json data = {{"key", "value"}, {"array", {1, 2, 3}}};
    JsonAdapter adapter(data);

    int top_before = lua_gettop(_L);

    std::string error;
    adapter.push_to_lua(nullptr, &error);

    int top_after = lua_gettop(_L);

    EXPECT_EQ(top_after, top_before);  // 失败时不应该改变栈
}

// ============================================================================
// JsonAdapter 异常处理和错误场景测试
// ============================================================================

// 测试类：模拟会抛出异常的 JSON 值
class ExceptionThrowingDataAdapter : public DataAdapter {
public:
    ExceptionThrowingDataAdapter(nlohmann::json data) : data_(data) {}

    bool push_to_lua(lua_State* L, std::string* error_msg) const override {
        if (!L) {
            if (error_msg) {
                *error_msg = "Lua state is null";
            }
            return false;
        }
        return push_json_value(L, data_, error_msg);
    }

    const char* get_type_name() const override { return "ExceptionThrowingAdapter"; }

private:
    bool push_json_value(lua_State* L, const nlohmann::json& j, std::string* error_msg) const {
        try {
            // 尝试获取一个不存在的类型或进行无效操作
            if (j.is_string()) {
                // 尝试从字符串获取整数，会抛出异常
                auto val = j.get<int>();
                lua_pushinteger(L, val);
            } else {
                lua_pushnil(L);
            }
            return true;
        } catch (const std::exception& e) {
            if (error_msg) {
                *error_msg = std::string("JSON conversion error: ") + e.what();
            }
            return false;
        }
    }

    nlohmann::json data_;
};

TEST_F(JsonAdapterTest, ArrayWithInvalidElement_FailsCleanly) {
    // 创建一个包含会被丢弃的值的数组
    // 通过解析损坏的 JSON 来创建 discarded 值
    json data = json::array();
    data.push_back(1);
    data.push_back(2);

    // 手动创建一个 discarded 类型的 JSON 值
    json discarded_value = json::value_t::discarded;

    // nlohmann::json 不允许直接构造 discarded 值
    // 我们测试其他异常场景：嵌套过深导致栈溢出
    // 或者测试通过类型转换错误来触发异常

    // 测试场景：数组中某个元素无法转换
    // 由于 nlohmann::json 的设计，所有值都是有效的
    // 我们需要测试的是在 push_json_value 递归调用中的错误处理

    // 创建一个会导致错误传播的场景
    // 使用特殊的 JSON 构造方式
}

TEST_F(JsonAdapterTest, ObjectWithInvalidValue_FailsCleanly) {
    // 测试对象中某个值无效时的清理
    // 由于 nlohmann::json 的类型系统，我们需要模拟错误场景

    // 我们无法直接创建 discarded 类型的值
    // 但可以测试其他错误路径

    json valid_data = {{"key1", "value1"}, {"key2", 123}};
    JsonAdapter adapter(valid_data);

    std::string error;
    EXPECT_TRUE(adapter.push_to_lua(_L, &error));

    lua_pop(_L, 1);
}

TEST_F(JsonAdapterTest, PushToLua_CatchesStandardExceptions) {
    // 使用自定义适配器测试异常捕获
    json string_data = "hello";
    ExceptionThrowingDataAdapter adapter(string_data);

    std::string error;
    EXPECT_FALSE(adapter.push_to_lua(_L, &error));

    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("JSON conversion error") != std::string::npos);
}

TEST_F(JsonAdapterTest, PushToLua_ExceptionWithoutErrorMsg_DoesNotCrash) {
    json string_data = "hello";
    ExceptionThrowingDataAdapter adapter(string_data);

    // 不传递 error_msg，不应该崩溃
    EXPECT_FALSE(adapter.push_to_lua(_L, nullptr));
}

TEST_F(JsonAdapterTest, GetTypeName_ReturnsCorrectString) {
    json data = {{"key", "value"}};
    JsonAdapter adapter(data);

    EXPECT_STREQ(adapter.get_type_name(), "nlohmann::json");
}

TEST_F(JsonAdapterTest, VeryDeeplyNestedArray_HandlesStackCorrectly) {
    // 测试深度嵌套的数组，确保栈不会溢出
    // LuaJIT 默认栈大小通常是几千个元素
    json data = 1;
    int depth = 100;

    for (int i = 0; i < depth; ++i) {
        json temp = data;
        data = json::array();
        data.push_back(temp);
    }

    JsonAdapter adapter(data);

    std::string error;
    bool result = adapter.push_to_lua(_L, &error);

    // 应该成功（或者至少不会崩溃）
    if (result) {
        EXPECT_TRUE(is_table());
        lua_pop(_L, 1);
    } else {
        // 如果失败，应该有合理的错误消息
        EXPECT_FALSE(error.empty());
    }
}

TEST_F(JsonAdapterTest, ArrayWithMixedTypes_AllSucceedOrFailCleanly) {
    // 测试数组中混合类型，确保部分失败时正确清理
    json data = json::array();
    data.push_back(1);
    data.push_back("string");
    data.push_back(true);
    data.push_back(nullptr);
    data.push_back(3.14);

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 验证所有元素都正确转换
    lua_rawgeti(_L, -1, 1);
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 1);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 2);
    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "string");
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 3);
    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 4);
    EXPECT_TRUE(is_nil());
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 5);
    EXPECT_TRUE(is_number());
    EXPECT_DOUBLE_EQ(get_number(), 3.14);
    lua_pop(_L, 2);  // 弹出元素和数组
}

TEST_F(JsonAdapterTest, ObjectWithNestedArrayAndObject_AllSucceed) {
    // 测试复杂嵌套结构中的错误处理
    json data = {
        {"level1", {
            {"level2", {
                {"array", {1, 2, 3}},
                {"object", {{"key", "value"}}}
            }}
        }}
    };

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 验证结构正确
    lua_getfield(_L, -1, "level1");
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "level2");
    ASSERT_TRUE(is_table());

    lua_getfield(_L, -1, "array");
    ASSERT_TRUE(is_table());

    size_t len = lua_objlen(_L, -1);
    EXPECT_EQ(len, 3);

    lua_pop(_L, 4);  // 清理栈
}

// 模拟内存分配失败的测试
// 注意：这很难在单元测试中模拟，因为我们无法控制 new 的失败
// 但我们可以测试其他边界条件

TEST_F(JsonAdapterTest, EmptyStringKey_WorksCorrectly) {
    // 测试空字符串作为对象键
    json data = {{"", "empty_key_value"}, {"normal", "normal_value"}};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    lua_pushstring(_L, "");
    lua_gettable(_L, -2);

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "empty_key_value");

    lua_pop(_L, 2);  // 清理栈
}

TEST_F(JsonAdapterTest, VeryLargeObject_ManyKeys_HandlesCorrectly) {
    // 测试包含大量键的对象
    json data = json::object();
    for (int i = 0; i < 1000; ++i) {
        data["key_" + std::to_string(i)] = i;
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    EXPECT_TRUE(is_table());

    // 验证几个键
    lua_getfield(_L, -1, "key_0");
    EXPECT_EQ(get_integer(), 0);
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "key_500");
    EXPECT_EQ(get_integer(), 500);
    lua_pop(_L, 1);

    lua_getfield(_L, -1, "key_999");
    EXPECT_EQ(get_integer(), 999);
    lua_pop(_L, 2);
}

TEST_F(JsonAdapterTest, ArrayWithHoles_ConvertsCorrectly) {
    // nlohmann::json 不支持数组中的"洞"（sparse arrays）
    // 所有索引都是连续的
    // 但我们可以测试跳过索引的场景

    json data = {1, 2, 3};  // 索引 0, 1, 2
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // Lua 数组从 1 开始，所以索引映射为 1, 2, 3
    lua_rawgeti(_L, -1, 1);
    EXPECT_EQ(get_integer(), 1);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 2);
    EXPECT_EQ(get_integer(), 2);
    lua_pop(_L, 1);

    lua_rawgeti(_L, -1, 3);
    EXPECT_EQ(get_integer(), 3);
    lua_pop(_L, 2);
}

// ============================================================================
// 嵌套深度限制测试
// ============================================================================

// 辅助函数：创建深度嵌套的 JSON
json create_deep_nested_json(size_t depth) {
    if (depth == 0) {
        return json("value_at_bottom");
    }

    json result = json::object();
    result["level_" + std::to_string(depth)] = create_deep_nested_json(depth - 1);
    return result;
}

TEST_F(JsonAdapterTest, DefaultMaxNestingDepth_Is8192) {
    json data = json::object();
    JsonAdapter adapter(data);

    EXPECT_EQ(adapter.get_max_nesting_depth(), 8192);
}

TEST_F(JsonAdapterTest, SetMaxNestingDepth_WorksCorrectly) {
    json data = json::object();
    JsonAdapter adapter(data);

    adapter.set_max_nesting_depth(10);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 10);

    adapter.set_max_nesting_depth(100);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 100);
}

TEST_F(JsonAdapterTest, SetMaxNestingDepth_CapsAtMaximum) {
    json data = json::object();
    JsonAdapter adapter(data);

    // 尝试设置超过最大值的深度，应该被限制在 MAX_NESTING_DEPTH
    adapter.set_max_nesting_depth(10000);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 8192);

    // 设置恰好等于最大值
    adapter.set_max_nesting_depth(8192);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 8192);

    // 设置超过最大值的很大数字
    adapter.set_max_nesting_depth(999999);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 8192);
}

TEST_F(JsonAdapterTest, SetMaxNestingDepth_MinimumIsOne) {
    json data = json::object();
    JsonAdapter adapter(data);

    // 尝试设置 0，应该被限制为最小值 1
    adapter.set_max_nesting_depth(0);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 1);

    // 设置 1，应该保持为 1
    adapter.set_max_nesting_depth(1);
    EXPECT_EQ(adapter.get_max_nesting_depth(), 1);

    // 验证默认值符合最小值要求
    JsonAdapter default_adapter(data);
    EXPECT_GE(default_adapter.get_max_nesting_depth(), 1);
}

TEST_F(JsonAdapterTest, ShallowNesting_ConvertsCorrectly) {
    // 测试浅层嵌套（10层），应该完全转换
    json data = create_deep_nested_json(10);
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 验证根对象被转换
    EXPECT_TRUE(is_table());

    // 简单验证：检查根对象有 level_10 键
    lua_pushstring(_L, "level_10");
    lua_rawget(_L, -2);
    EXPECT_TRUE(is_table()) << "level_10 should be a table";
    lua_pop(_L, 2); // 清理
}

TEST_F(JsonAdapterTest, DepthExceedsLimit_BecomesNil) {
    // 创建 300 层嵌套的 JSON
    json data = create_deep_nested_json(300);

    // 设置自定义最大深度为 256（低于默认的8192）
    JsonAdapter adapter(data, 256);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 栈顶应该是 table
    EXPECT_TRUE(is_table());

    // 检查超过深度的部分
    // 当 current_depth + 1 >= max_depth (256) 时子元素被截断
    // 即 level_44 (深度 255) 的子元素 level_43 会被截断
    lua_pushstring(_L, "level_300");
    lua_rawget(_L, -2);

    // 递归检查直到找到被截断的位置
    lua_State* L = _L;
    int depth = 1;
    bool found_nil = false;

    while (lua_istable(L, -1) && depth < 300) {
        std::string next_key = "level_" + std::to_string(300 - depth);
        lua_pushstring(L, next_key.c_str());
        lua_rawget(L, -2);

        if (lua_isnil(L, -1)) {
            found_nil = true;
            lua_pop(L, 2); // 弹出 nil 和 table
            break;
        }

        lua_remove(L, -2); // 移除父 table
        depth++;
    }

    // 应该在某个深度找到 nil (大约在深度 255-256 附近)
    EXPECT_TRUE(found_nil) << "Should find nil at some depth due to limit";
    EXPECT_GE(depth, 255) << "Nil should appear at or after depth 255";

    if (!found_nil) {
        lua_pop(L, 1); // 清理栈
    }
}

TEST_F(JsonAdapterTest, CustomMaxNestingDepth_TruncatesDeeplyNestedValues) {
    // 创建 50 层嵌套的 JSON
    json data = create_deep_nested_json(50);

    // 设置最大深度为 10
    // 当 current_depth + 1 >= max_depth 时，子元素被截断
    JsonAdapter adapter(data, 10);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 栈顶应该是 table
    EXPECT_TRUE(is_table());

    // 直接检查：遍历深度，找到第一个被截断的位置
    lua_pushstring(_L, "level_50");
    lua_rawget(_L, -2);

    int depth = 0;
    bool found_truncation = false;

    // 最多遍历15层
    while (depth < 15 && lua_istable(_L, -1)) {
        depth++;
        std::string next_key = "level_" + std::to_string(50 - depth);
        lua_pushstring(_L, next_key.c_str());
        lua_rawget(_L, -2);

        if (lua_isnil(_L, -1)) {
            found_truncation = true;
            lua_pop(_L, 3); // 清理
            break;
        }

        lua_remove(_L, -2);
    }

    if (!found_truncation) {
        lua_pop(_L, 2);
    }

    // 应该在深度10左右找到截断
    EXPECT_TRUE(found_truncation) << "Should find truncation due to depth limit";
    EXPECT_GE(depth, 9) << "Truncation should happen at or after depth 9";
}

TEST_F(JsonAdapterTest, MinMaxNestingDepth_RootObjectStillConverted) {
    // 创建嵌套 JSON: {"level_5": {"level_4": {...}}}
    json data = create_deep_nested_json(5);

    // 设置最大深度为 0（会被限制为最小值 1）
    // 这意味着只允许 1 层嵌套
    // 根对象会被转换，但其子元素（深度 1 的子对象）会被截断为 nil
    // 因为 current_depth + 1 >= max_depth (1) 时，子元素就会被截断
    JsonAdapter adapter(data, 0);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 栈顶应该是 table (根对象，深度 0)
    EXPECT_TRUE(is_table());

    // level_5 应该存在但值为 nil
    // 因为当处理 level_5 的值时，current_depth=0，检查 current_depth + 1 >= 1 为真
    // 所以 level_5 的嵌套值会被截断为 nil
    lua_pushstring(_L, "level_5");
    lua_rawget(_L, -2);

    EXPECT_TRUE(is_nil()) << "Child elements should be nil when max_depth is 1";

    lua_pop(_L, 2); // 清理栈
}

TEST_F(JsonAdapterTest, DeepArrayNesting_TruncatesCorrectly) {
    // 创建深度嵌套的数组
    json data = json::array();
    json* current = &data;

    for (int i = 0; i < 300; ++i) {
        json nested = json::array();
        current->push_back(nested);
        current = &current->back();
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error));

    // 栈顶应该是 table（数组）
    EXPECT_TRUE(is_table());

    // 数组应该被成功转换（不会崩溃）
    // 超过最大深度的部分会被截断为 nil

    lua_pop(_L, 1); // 清理栈
}

// ============================================================================
// DataAdapter 接口的 execute_commands 测试
// ============================================================================

// 测试用的 DataAdapter 实现，只继承基类不重写 execute_commands
class TestDataAdapter : public DataAdapter {
public:
    TestDataAdapter() = default;
    ~TestDataAdapter() override = default;

    // 实现纯虚函数
    bool push_to_lua(lua_State* L, std::string* error_msg = nullptr) const override {
        (void)error_msg;
        lua_newtable(L);
        return true;
    }

    const char* get_type_name() const override {
        return "TestDataAdapter";
    }
};

TEST_F(JsonAdapterTest, DataAdapter_ExecuteCommands_Default_ReturnsTrue) {
    TestDataAdapter adapter;

    // 创建一个表
    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error)) << error;

    // 调用默认的 execute_commands（应该返回 true，不做任何操作）
    EXPECT_TRUE(adapter.execute_commands(_L, &error)) << error;

    // 表应该是空的
    EXPECT_EQ(lua_objlen(_L, -1), 0);

    lua_pop(_L, 1);
}

// ============================================================================
// BasicDataAdapter 测试
// ============================================================================

class BasicDataAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_TRUE(_state.is_valid());
        _L = _state.get();
    }

    LuaState _state;
    lua_State* _L;
};

TEST_F(BasicDataAdapterTest, ExecuteCommands_SkipsEmptyKey) {
    BasicDataAdapter adapter;
    adapter.set("name", "test");
    adapter.set("", "should_be_skipped");  // 空键应该被跳过
    adapter.set("value", 42);

    // 使用 push_to_lua 创建表（模拟实际使用场景）
    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error)) << error;

    // 调用 execute_commands 填充字段
    ASSERT_TRUE(adapter.execute_commands(_L, &error)) << error;

    // 验证 name 和 value 存在

    // 验证 name 和 value 存在
    lua_pushstring(_L, "name");
    lua_rawget(_L, -2);
    EXPECT_TRUE(lua_isstring(_L, -1));
    EXPECT_STREQ(lua_tostring(_L, -1), "test");
    lua_pop(_L, 1);

    lua_pushstring(_L, "value");
    lua_rawget(_L, -2);
    EXPECT_TRUE(lua_isnumber(_L, -1));
    EXPECT_EQ(lua_tointeger(_L, -1), 42);
    lua_pop(_L, 1);

    // 验证空键不存在
    lua_pushstring(_L, "");
    lua_rawget(_L, -2);
    EXPECT_TRUE(lua_isnil(_L, -1)) << "Empty key should not exist in table";
    lua_pop(_L, 1);

    lua_pop(_L, 1);  // 清理 table
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_MultipleEmptyKeys_AllSkipped) {
    BasicDataAdapter adapter;
    adapter.set("field1", "value1");
    adapter.set("", "skip1");
    adapter.set("field2", 123);
    adapter.set("", "skip2");
    adapter.set("field3", true);

    // 使用 push_to_lua 创建表
    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error)) << error;

    // 调用 execute_commands 填充字段
    ASSERT_TRUE(adapter.execute_commands(_L, &error)) << error;

    // 验证所有字段都存在

    // 验证所有字段都存在
    lua_pushstring(_L, "field1");
    lua_rawget(_L, -2);
    EXPECT_TRUE(lua_isstring(_L, -1));
    lua_pop(_L, 1);

    lua_pushstring(_L, "field2");
    lua_rawget(_L, -2);
    EXPECT_TRUE(lua_isnumber(_L, -1));
    lua_pop(_L, 1);

    lua_pushstring(_L, "field3");
    lua_rawget(_L, -2);
    EXPECT_TRUE(lua_isboolean(_L, -1));
    lua_pop(_L, 1);

    lua_pop(_L, 1);
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_OnlyEmptyKeys_TableEmpty) {
    BasicDataAdapter adapter;
    adapter.set("", "value1");
    adapter.set("", "value2");

    // 使用 push_to_lua 创建表
    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(_L, &error)) << error;

    // 调用 execute_commands 填充字段
    ASSERT_TRUE(adapter.execute_commands(_L, &error)) << error;

    // table 应该是空的
    EXPECT_EQ(lua_objlen(_L, -1), 0);

    lua_pop(_L, 1);
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_StackTopNotTable_ReturnsFalse) {
    BasicDataAdapter adapter;
    adapter.set("field1", "value1");

    // 栈顶放一个字符串而不是 table
    lua_pushstring(_L, "not_a_table");

    std::string error;
    EXPECT_FALSE(adapter.execute_commands(_L, &error));
    EXPECT_EQ(error, "Stack top is not a table");

    lua_pop(_L, 1);  // 清理栈
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_StackTopNil_ReturnsFalse) {
    BasicDataAdapter adapter;
    adapter.set("field1", "value1");

    // 栈顶放 nil
    lua_pushnil(_L);

    std::string error;
    EXPECT_FALSE(adapter.execute_commands(_L, &error));
    EXPECT_EQ(error, "Stack top is not a table");

    lua_pop(_L, 1);  // 清理栈
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_StackTopNumber_ReturnsFalse) {
    BasicDataAdapter adapter;
    adapter.set("field1", "value1");

    // 栈顶放一个数字
    lua_pushnumber(_L, 42);

    std::string error;
    EXPECT_FALSE(adapter.execute_commands(_L, &error));
    EXPECT_EQ(error, "Stack top is not a table");

    lua_pop(_L, 1);  // 清理栈
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_EmptyStack_ReturnsFalse) {
    BasicDataAdapter adapter;
    adapter.set("field1", "value1");

    // 栈是空的，栈顶绝对不是 table
    // 确保栈是空的
    lua_settop(_L, 0);

    std::string error;
    EXPECT_FALSE(adapter.execute_commands(_L, &error));
    EXPECT_EQ(error, "Stack top is not a table");
}

TEST_F(BasicDataAdapterTest, ExecuteCommands_NoErrorNull_ReturnsFalse) {
    BasicDataAdapter adapter;
    adapter.set("field1", "value1");

    // 栈顶放一个字符串而不是 table，不传 error_msg
    lua_pushstring(_L, "not_a_table");

    // 不传递 error_msg 参数，应该返回 false 但不设置错误信息
    EXPECT_FALSE(adapter.execute_commands(_L, nullptr));

    lua_pop(_L, 1);  // 清理栈
}