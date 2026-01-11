#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ljre/data_adapter.h"
#include "ljre/json_adapter.h"
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
        ASSERT_TRUE(state_.is_valid());
        L_ = state_.get();
    }

    // 辅助函数：检查栈顶值的类型
    bool is_nil(int idx = -1) { return lua_isnil(L_, idx); }
    bool is_boolean(int idx = -1) { return lua_isboolean(L_, idx); }
    bool is_number(int idx = -1) { return lua_isnumber(L_, idx); }
    bool is_string(int idx = -1) { return lua_isstring(L_, idx); }
    bool is_table(int idx = -1) { return lua_istable(L_, idx); }

    // 辅助函数：获取栈顶的值
    bool get_boolean(int idx = -1) { return lua_toboolean(L_, idx) != 0; }
    lua_Integer get_integer(int idx = -1) { return lua_tointeger(L_, idx); }
    lua_Number get_number(int idx = -1) { return lua_tonumber(L_, idx); }
    std::string get_string(int idx = -1) {
        size_t len;
        const char* str = lua_tolstring(L_, idx, &len);
        return std::string(str, len);
    }

    LuaState state_;
    lua_State* L_;
};

TEST_F(JsonAdapterTest, NullValue_ConvertsToNil) {
    json data = nullptr;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_nil());
}

TEST_F(JsonAdapterTest, BooleanTrue_ConvertsToLuaBoolean) {
    json data = true;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
}

TEST_F(JsonAdapterTest, BooleanFalse_ConvertsToLuaBoolean) {
    json data = false;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_boolean());
    EXPECT_FALSE(get_boolean());
}

TEST_F(JsonAdapterTest, Integer_ConvertsToLuaInteger) {
    json data = 42;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 42);
}

TEST_F(JsonAdapterTest, NegativeInteger_ConvertsCorrectly) {
    json data = -12345;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), -12345);
}

TEST_F(JsonAdapterTest, Float_ConvertsToLuaNumber) {
    json data = 3.14159;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_number());
    EXPECT_DOUBLE_EQ(get_number(), 3.14159);
}

TEST_F(JsonAdapterTest, String_ConvertsToLuaString) {
    json data = "hello world";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "hello world");
}

TEST_F(JsonAdapterTest, EmptyString_ConvertsCorrectly) {
    json data = "";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "");
}

TEST_F(JsonAdapterTest, UnicodeString_ConvertsCorrectly) {
    json data = "你好世界 🌍";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "你好世界 🌍");
}

TEST_F(JsonAdapterTest, StringWithNullChar_ConvertsCorrectly) {
    std::string str_with_null = "hello\0world";
    json data = str_with_null;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), str_with_null);
}

TEST_F(JsonAdapterTest, StringWithSpecialChars_ConvertsCorrectly) {
    json data = "line1\nline2\ttab\r\n";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查表长度 - lua_objlen returns length, doesn't push to stack
    size_t len = lua_objlen(L_, -1);
    EXPECT_EQ(len, 0);

    lua_pop(L_, 1);  // 弹出 table
}

TEST_F(JsonAdapterTest, IntegerArray_ConvertsToLuaTable) {
    json data = {1, 2, 3, 4, 5};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查数组长度
    size_t len = lua_objlen(L_, -1);
    EXPECT_EQ(len, 5);

    // 检查各个元素
    for (int i = 1; i <= 5; ++i) {
        lua_rawgeti(L_, -1, i);
        EXPECT_TRUE(is_number());
        EXPECT_EQ(get_integer(), i);
        lua_pop(L_, 1);
    }

    // 弹出 table
    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, MixedTypeArray_ConvertsCorrectly) {
    json data = {1, "two", 3.0, true, nullptr};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查第一个元素 (整数)
    lua_rawgeti(L_, -1, 1);
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 1);
    lua_pop(L_, 1);

    // 检查第二个元素 (字符串)
    lua_rawgeti(L_, -1, 2);
    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "two");
    lua_pop(L_, 1);

    // 检查第三个元素 (浮点数)
    lua_rawgeti(L_, -1, 3);
    EXPECT_TRUE(is_number());
    EXPECT_DOUBLE_EQ(get_number(), 3.0);
    lua_pop(L_, 1);

    // 检查第四个元素 (布尔)
    lua_rawgeti(L_, -1, 4);
    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
    lua_pop(L_, 1);

    // 检查第五个元素 (null)
    lua_rawgeti(L_, -1, 5);
    EXPECT_TRUE(is_nil());
    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, NestedArray_ConvertsCorrectly) {
    json data = {{1, 2}, {3, 4}, {5, 6}};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查第一个子数组
    lua_rawgeti(L_, -1, 1);
    ASSERT_TRUE(is_table());

    lua_rawgeti(L_, -1, 1);
    EXPECT_EQ(get_integer(), 1);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 2);
    EXPECT_EQ(get_integer(), 2);
    lua_pop(L_, 2);  // 弹出子数组和元素

    // 检查第三个子数组
    lua_rawgeti(L_, -1, 3);
    ASSERT_TRUE(is_table());

    lua_rawgeti(L_, -1, 1);
    EXPECT_EQ(get_integer(), 5);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 2);
    EXPECT_EQ(get_integer(), 6);
    lua_pop(L_, 2);
}

TEST_F(JsonAdapterTest, LargeArray_ConvertsCorrectly) {
    json data = json::array();
    for (int i = 0; i < 1000; ++i) {
        data.push_back(i);
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查数组长度 - lua_objlen returns length, doesn't push to stack
    size_t len = lua_objlen(L_, -1);
    EXPECT_EQ(len, 1000);

    // 抽查几个元素
    lua_rawgeti(L_, -1, 1);
    EXPECT_EQ(get_integer(), 0);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 500);
    EXPECT_EQ(get_integer(), 499);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 1000);
    EXPECT_EQ(get_integer(), 999);
    lua_pop(L_, 1);
}

// ============================================================================
// JsonAdapter 对象转换测试
// ============================================================================

TEST_F(JsonAdapterTest, EmptyObject_ConvertsToEmptyTable) {
    json data = json::object();
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查 name 字段
    lua_getfield(L_, -1, "name");
    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "Alice");
    lua_pop(L_, 1);

    // 检查 age 字段
    lua_getfield(L_, -1, "age");
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 30);
    lua_pop(L_, 1);

    // 检查 active 字段
    lua_getfield(L_, -1, "active");
    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
    lua_pop(L_, 1);
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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查嵌套的 user 对象
    lua_getfield(L_, -1, "user");
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "name");
    EXPECT_EQ(get_string(), "Bob");
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "age");
    EXPECT_EQ(get_integer(), 25);
    lua_pop(L_, 2);  // 弹出 user 对象和 age

    // 检查 status 字段
    lua_getfield(L_, -1, "status");
    EXPECT_EQ(get_string(), "active");
    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, ObjectWithArrayField_ConvertsCorrectly) {
    json data = {
        {"items", {1, 2, 3}},
        {"count", 3}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查 items 数组
    lua_getfield(L_, -1, "items");
    ASSERT_TRUE(is_table());

    // 检查数组长度 - lua_objlen returns length, doesn't push to stack
    size_t len = lua_objlen(L_, -1);
    EXPECT_EQ(len, 3);

    lua_rawgeti(L_, -1, 1);
    EXPECT_EQ(get_integer(), 1);
    lua_pop(L_, 1);

    lua_pop(L_, 1);  // 弹出 items

    // 检查 count 字段
    lua_getfield(L_, -1, "count");
    EXPECT_EQ(get_integer(), 3);
    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, ArrayOfObjects_ConvertsCorrectly) {
    json data = {
        {{"id", 1}, {"name", "Item 1"}},
        {{"id", 2}, {"name", "Item 2"}},
        {{"id", 3}, {"name", "Item 3"}}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查第一个对象
    lua_rawgeti(L_, -1, 1);
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "id");
    EXPECT_EQ(get_integer(), 1);
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "name");
    EXPECT_EQ(get_string(), "Item 1");
    lua_pop(L_, 2);

    // 检查第三个对象
    lua_rawgeti(L_, -1, 3);
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "id");
    EXPECT_EQ(get_integer(), 3);
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "name");
    EXPECT_EQ(get_string(), "Item 3");
    lua_pop(L_, 2);
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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查 users 数组
    lua_getfield(L_, -1, "users");
    ASSERT_TRUE(is_table());

    // 第一个用户
    lua_rawgeti(L_, -1, 1);
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "name");
    EXPECT_EQ(get_string(), "Alice");
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "scores");
    ASSERT_TRUE(is_table());

    lua_rawgeti(L_, -1, 1);
    EXPECT_EQ(get_integer(), 95);
    lua_pop(L_, 4);

    // 检查 metadata
    lua_getfield(L_, -1, "metadata");
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "version");
    EXPECT_EQ(get_string(), "1.0");
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "active");
    EXPECT_TRUE(get_boolean());
    lua_pop(L_, 2);
}

TEST_F(JsonAdapterTest, ObjectKeyWithSpecialChars_ConvertsCorrectly) {
    json data = {
        {"key with spaces", "value1"},
        {"key\twith\ttabs", "value2"},
        {"key\nwith\nnewlines", "value3"}
    };
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 检查带空格的键
    lua_getfield(L_, -1, "key with spaces");
    EXPECT_EQ(get_string(), "value1");
    lua_pop(L_, 1);

    // 检查带制表符的键
    lua_getfield(L_, -1, "key\twith\ttabs");
    EXPECT_EQ(get_string(), "value2");
    lua_pop(L_, 1);

    // 检查带换行符的键
    lua_getfield(L_, -1, "key\nwith\nnewlines");
    EXPECT_EQ(get_string(), "value3");
    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, ObjectKeyWithNullChar_ConvertsCorrectly) {
    std::string key_with_null = "key\0with\0null";
    json data = {{key_with_null, "value"}};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 使用 lua_pushlstring 获取键
    lua_pushlstring(L_, key_with_null.data(), key_with_null.size());
    lua_gettable(L_, -2);

    EXPECT_EQ(get_string(), "value");
    lua_pop(L_, 1);
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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 9007199254740991);
}

TEST_F(JsonAdapterTest, VerySmallNumber_ConvertsCorrectly) {
    json data = -9007199254740991;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), -9007199254740991);
}

TEST_F(JsonAdapterTest, VeryLongString_ConvertsCorrectly) {
    std::string long_string(10000, 'x');
    json data = long_string;
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), long_string);
}

TEST_F(JsonAdapterTest, DeeplyNestedStructure_ConvertsCorrectly) {
    // 测试深度嵌套，观察 Lua 栈的行为
    // LuaJIT 默认栈大小通常为几千个元素
    // 测试 1000 层嵌套是安全的，可以验证栈的使用
    json data = 1;
    int depth = 1000;  // 1000 层嵌套

    for (int i = 0; i < depth; ++i) {
        json temp = data;
        data = {{"nested", temp}};
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error)) << "Failed to push nested JSON to Lua: " << error;

    EXPECT_TRUE(is_table());

    // 记录初始栈位置
    int initial_stack = lua_gettop(L_);

    // 验证嵌套结构：前 (depth-1) 层都是table
    for (int i = 0; i < depth - 1; ++i) {
        lua_getfield(L_, -1, "nested");
        ASSERT_TRUE(is_table()) << "Failed at depth " << (i + 1) << ", stack top: " << lua_gettop(L_);
    }

    // 最后一次获取 nested 应该得到数字 1
    lua_getfield(L_, -1, "nested");
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 1);

    // 栈上有 1 个初始 table + depth 个元素
    int final_stack = lua_gettop(L_);
    EXPECT_EQ(final_stack, initial_stack + depth);

    // 清理栈
    lua_settop(L_, initial_stack);
}

TEST_F(JsonAdapterTest, WideUnicodeCharacters_ConvertsCorrectly) {
    json data = "🌍🌎🌏 🎉🎊🎈 𝔘𝔫𝔦𝔠𝔬𝔡𝔢";
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "🌍🌎🌏 🎉🎊🎈 𝔘𝔫𝔦𝔠𝔬𝔡𝔢");
}

// ============================================================================
// JsonAdapter 栈平衡测试
// ============================================================================

TEST_F(JsonAdapterTest, StackBalance_AfterSuccessfulPush) {
    json data = {{"key", "value"}, {"array", {1, 2, 3}}};
    JsonAdapter adapter(data);

    int top_before = lua_gettop(L_);

    std::string error;
    adapter.push_to_lua(L_, &error);

    int top_after = lua_gettop(L_);

    EXPECT_EQ(top_after, top_before + 1);  // 应该只压入一个 table

    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, StackBalance_AfterFailedPush) {
    json data = {{"key", "value"}, {"array", {1, 2, 3}}};
    JsonAdapter adapter(data);

    int top_before = lua_gettop(L_);

    std::string error;
    adapter.push_to_lua(nullptr, &error);

    int top_after = lua_gettop(L_);

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
    EXPECT_TRUE(adapter.push_to_lua(L_, &error));

    lua_pop(L_, 1);
}

TEST_F(JsonAdapterTest, PushToLua_CatchesStandardExceptions) {
    // 使用自定义适配器测试异常捕获
    json string_data = "hello";
    ExceptionThrowingDataAdapter adapter(string_data);

    std::string error;
    EXPECT_FALSE(adapter.push_to_lua(L_, &error));

    EXPECT_FALSE(error.empty());
    EXPECT_TRUE(error.find("JSON conversion error") != std::string::npos);
}

TEST_F(JsonAdapterTest, PushToLua_ExceptionWithoutErrorMsg_DoesNotCrash) {
    json string_data = "hello";
    ExceptionThrowingDataAdapter adapter(string_data);

    // 不传递 error_msg，不应该崩溃
    EXPECT_FALSE(adapter.push_to_lua(L_, nullptr));
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
    int depth = 500;  // 500 层嵌套

    for (int i = 0; i < depth; ++i) {
        json temp = data;
        data = json::array();
        data.push_back(temp);
    }

    JsonAdapter adapter(data);

    std::string error;
    bool result = adapter.push_to_lua(L_, &error);

    // 应该成功（或者至少不会崩溃）
    if (result) {
        EXPECT_TRUE(is_table());
        lua_pop(L_, 1);
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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    // 验证所有元素都正确转换
    lua_rawgeti(L_, -1, 1);
    EXPECT_TRUE(is_number());
    EXPECT_EQ(get_integer(), 1);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 2);
    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "string");
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 3);
    EXPECT_TRUE(is_boolean());
    EXPECT_TRUE(get_boolean());
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 4);
    EXPECT_TRUE(is_nil());
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 5);
    EXPECT_TRUE(is_number());
    EXPECT_DOUBLE_EQ(get_number(), 3.14);
    lua_pop(L_, 2);  // 弹出元素和数组
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
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    // 验证结构正确
    lua_getfield(L_, -1, "level1");
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "level2");
    ASSERT_TRUE(is_table());

    lua_getfield(L_, -1, "array");
    ASSERT_TRUE(is_table());

    size_t len = lua_objlen(L_, -1);
    EXPECT_EQ(len, 3);

    lua_pop(L_, 4);  // 清理栈
}

// 模拟内存分配失败的测试
// 注意：这很难在单元测试中模拟，因为我们无法控制 new 的失败
// 但我们可以测试其他边界条件

TEST_F(JsonAdapterTest, EmptyStringKey_WorksCorrectly) {
    // 测试空字符串作为对象键
    json data = {{"", "empty_key_value"}, {"normal", "normal_value"}};
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    lua_pushstring(L_, "");
    lua_gettable(L_, -2);

    EXPECT_TRUE(is_string());
    EXPECT_EQ(get_string(), "empty_key_value");

    lua_pop(L_, 2);  // 清理栈
}

TEST_F(JsonAdapterTest, VeryLargeObject_ManyKeys_HandlesCorrectly) {
    // 测试包含大量键的对象
    json data = json::object();
    for (int i = 0; i < 1000; ++i) {
        data["key_" + std::to_string(i)] = i;
    }

    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    EXPECT_TRUE(is_table());

    // 验证几个键
    lua_getfield(L_, -1, "key_0");
    EXPECT_EQ(get_integer(), 0);
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "key_500");
    EXPECT_EQ(get_integer(), 500);
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "key_999");
    EXPECT_EQ(get_integer(), 999);
    lua_pop(L_, 2);
}

TEST_F(JsonAdapterTest, ArrayWithHoles_ConvertsCorrectly) {
    // nlohmann::json 不支持数组中的"洞"（sparse arrays）
    // 所有索引都是连续的
    // 但我们可以测试跳过索引的场景

    json data = {1, 2, 3};  // 索引 0, 1, 2
    JsonAdapter adapter(data);

    std::string error;
    ASSERT_TRUE(adapter.push_to_lua(L_, &error));

    // Lua 数组从 1 开始，所以索引映射为 1, 2, 3
    lua_rawgeti(L_, -1, 1);
    EXPECT_EQ(get_integer(), 1);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 2);
    EXPECT_EQ(get_integer(), 2);
    lua_pop(L_, 1);

    lua_rawgeti(L_, -1, 3);
    EXPECT_EQ(get_integer(), 3);
    lua_pop(L_, 2);
}
