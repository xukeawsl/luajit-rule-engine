#include <gtest/gtest.h>
#include "ljre/lua_state.h"
#include "test_helpers.h"

using namespace ljre;

// ============================================================================
// LuaStackGuard 基本功能测试
// ============================================================================

TEST(LuaStackGuardTest, RestoresStack_BasicOperations) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    // 记录初始栈位置
    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 在栈上压入一些元素
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 2);
        lua_pushnumber(L, 3);

        EXPECT_EQ(lua_gettop(L), initial_top + 3);
    }

    // guard 析构后，栈应该恢复
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, RestoresStack_MultiplePushPop) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 复杂的栈操作
        lua_pushnumber(L, 10);       // +1
        lua_pushstring(L, "test");   // +2
        lua_pushnumber(L, 20);       // +3
        lua_pop(L, 1);               // +2 (弹出 20)
        lua_pushnil(L);              // +3
        lua_pushboolean(L, true);    // +4

        int current_top = lua_gettop(L);
        EXPECT_EQ(current_top, initial_top + 4);  // 压入了 4 个元素
    }

    // 栈应该恢复
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, GetTop_ReturnsRecordedPosition) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    // 压入一些元素
    lua_pushnumber(L, 1);
    lua_pushnumber(L, 2);
    int expected_top = lua_gettop(L);

    LuaStackGuard guard(L);
    EXPECT_EQ(guard.get_top(), expected_top);
}

// ============================================================================
// LuaStackGuard 手动释放测试
// ============================================================================

TEST(LuaStackGuardTest, Release_PreventsStackRestoration) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        lua_pushnumber(L, 1);
        lua_pushnumber(L, 2);

        EXPECT_EQ(lua_gettop(L), initial_top + 2);

        // 手动释放
        guard.release();

        // 栈不应该被恢复
    }

    // 栈没有被恢复，仍然有 2 个元素
    EXPECT_EQ(lua_gettop(L), initial_top + 2);

    // 清理
    lua_pop(L, 2);
}

TEST(LuaStackGuardTest, MultipleRelease_Safe) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    LuaStackGuard guard(L);

    lua_pushnumber(L, 1);

    // 多次调用 release 应该是安全的
    guard.release();
    guard.release();
    guard.release();

    // 栈不应该被恢复
    int top = lua_gettop(L);
    EXPECT_GT(top, 0);

    // 清理
    lua_pop(L, top);
}

TEST(LuaStackGuardTest, Release_BeforeOperations_KeepStack) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 立即释放
        guard.release();

        // 现在可以自由操作栈
        lua_pushnumber(L, 1);
        lua_pushnumber(L, 2);
        lua_pushnumber(L, 3);
    }

    // 栈保持不变
    EXPECT_EQ(lua_gettop(L), initial_top + 3);

    // 清理
    lua_pop(L, 3);
}

// ============================================================================
// LuaStackGuard 嵌套测试
// ============================================================================

TEST(LuaStackGuardTest, NestedGuards_RestoreCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard1(L);

        lua_pushnumber(L, 1);

        {
            LuaStackGuard guard2(L);

            lua_pushnumber(L, 2);
            lua_pushnumber(L, 3);

            EXPECT_EQ(lua_gettop(L), initial_top + 3);
        }

        // guard2 析构，栈应该恢复到 guard1 的位置
        EXPECT_EQ(lua_gettop(L), initial_top + 1);
    }

    // guard1 析构，栈应该恢复到初始位置
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, NestedGuards_InnerRelease) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard1(L);

        lua_pushnumber(L, 1);

        {
            LuaStackGuard guard2(L);

            lua_pushnumber(L, 2);
            lua_pushnumber(L, 3);

            // 释放内层 guard
            guard2.release();
        }

        // guard2 已释放，栈保持不变
        EXPECT_EQ(lua_gettop(L), initial_top + 3);
    }

    // guard1 恢复到它的位置
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, NestedGuards_OuterRelease) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard1(L);

        lua_pushnumber(L, 1);

        {
            LuaStackGuard guard2(L);

            lua_pushnumber(L, 2);
            lua_pushnumber(L, 3);
        }

        // guard2 析构时已经恢复了，现在栈上只有数字 1
        // 释放外层 guard1，不会恢复栈
        guard1.release();
    }

    // guard2 恢复了（弹出 2 和 3），guard1 被释放（不恢复）
    // 所以栈上只剩下数字 1
    EXPECT_EQ(lua_gettop(L), initial_top + 1);

    // 清理
    lua_pop(L, 1);
}

// ============================================================================
// LuaStackGuard 空栈测试
// ============================================================================

TEST(LuaStackGuardTest, EmptyStack_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    // 确保栈是空的
    lua_settop(L, 0);

    {
        LuaStackGuard guard(L);

        EXPECT_EQ(guard.get_top(), 0);
        EXPECT_EQ(lua_gettop(L), 0);
    }

    // 栈仍然应该是空的
    EXPECT_EQ(lua_gettop(L), 0);
}

TEST(LuaStackGuardTest, EmptyStack_ThenPush) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    lua_settop(L, 0);

    {
        LuaStackGuard guard(L);

        lua_pushnumber(L, 1);
        lua_pushnumber(L, 2);

        EXPECT_EQ(lua_gettop(L), 2);
    }

    // 栈应该恢复到空
    EXPECT_EQ(lua_gettop(L), 0);
}

// ============================================================================
// LuaStackGuard 实际使用场景测试
// ============================================================================

TEST(LuaStackGuardTest, RealWorldUsage_FunctionCall) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    // 加载一个简单的 Lua 函数
    const char* code = R"(
function add(a, b)
    return a + b
end
)";

    std::string error;
    ASSERT_TRUE(state.load_buffer(code, strlen(code), "add_func", &error));

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 调用函数
        lua_getglobal(L, "add");
        lua_pushnumber(L, 10);
        lua_pushnumber(L, 20);

        if (lua_pcall(L, 2, 1, 0) == LUA_OK) {
            // 获取结果
            double result = lua_tonumber(L, -1);
            EXPECT_EQ(result, 30.0);
        }
    }

    // 栈应该恢复
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, RealWorldUsage_TableIteration) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    // 创建一个 table
    lua_newtable(L);
    lua_pushstring(L, "key1");
    lua_pushnumber(L, 100);
    lua_settable(L, -3);

    lua_pushstring(L, "key2");
    lua_pushnumber(L, 200);
    lua_settable(L, -3);

    int table_index = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 遍历 table
        lua_pushnil(L);  // 第一个 key
        int count = 0;
        while (lua_next(L, table_index) != 0) {
            // key 在 -2，value 在 -1
            lua_pop(L, 1);  // 弹出 value，保留 key
            count++;
        }

        EXPECT_EQ(count, 2);
    }

    // 栈应该恢复到只有 table
    EXPECT_EQ(lua_gettop(L), table_index);

    // 清理
    lua_pop(L, 1);
}

TEST(LuaStackGuardTest, RealWorldUsage_ErrorHandling) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    // 加载一个会出错的代码
    const char* code = R"(
function error_func()
    error("test error")
end
)";

    std::string error;
    ASSERT_TRUE(state.load_buffer(code, strlen(code), "error_func", &error));

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 尝试调用会出错的函数
        lua_getglobal(L, "error_func");

        int result = lua_pcall(L, 0, 0, 0);

        // 调用失败，栈上有错误消息
        EXPECT_NE(result, LUA_OK);
        EXPECT_GT(lua_gettop(L), initial_top);
    }

    // 即使出错，栈也应该恢复
    EXPECT_EQ(lua_gettop(L), initial_top);
}

// ============================================================================
// LuaStackGuard 边界条件测试
// ============================================================================

TEST(LuaStackGuardTest, LargeStackDepth) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 压入大量元素
        for (int i = 0; i < 1000; ++i) {
            lua_pushnumber(L, i);
        }

        EXPECT_EQ(lua_gettop(L), initial_top + 1000);
    }

    // 栈应该恢复
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, ZeroChanges_RestoresCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    {
        LuaStackGuard guard(L);

        // 不做任何栈操作
        EXPECT_EQ(lua_gettop(L), initial_top);
    }

    // 栈应该保持不变
    EXPECT_EQ(lua_gettop(L), initial_top);
}

TEST(LuaStackGuardTest, MultipleGuardsInSequence) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    lua_State* L = state.get();

    int initial_top = lua_gettop(L);

    // 第一个 guard
    {
        LuaStackGuard guard1(L);
        lua_pushnumber(L, 1);
    }

    EXPECT_EQ(lua_gettop(L), initial_top);

    // 第二个 guard
    {
        LuaStackGuard guard2(L);
        lua_pushnumber(L, 2);
    }

    EXPECT_EQ(lua_gettop(L), initial_top);

    // 第三个 guard
    {
        LuaStackGuard guard3(L);
        lua_pushnumber(L, 3);
    }

    EXPECT_EQ(lua_gettop(L), initial_top);
}
