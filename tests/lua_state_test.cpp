#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "ljre/lua_state.h"
#include "test_helpers.h"
#include <fstream>
#include <cstring>

using namespace ljre;
using namespace test_helpers;

// ============================================================================
// LuaState 构造和析构测试
// ============================================================================

TEST(LuaStateTest, DefaultConstructor_ValidState) {
    LuaState state;
    EXPECT_TRUE(state.is_valid());
    EXPECT_NE(state.get(), nullptr);
}

TEST(LuaStateTest, Destructor_CleansUpProperly) {
    {
        LuaState state;
        EXPECT_TRUE(state.is_valid());
    }
    // state 超出作用域，资源应被正确释放
    // 如果有内存泄漏，Valgrind 会检测到
}

TEST(LuaStateTest, MoveConstruction_TransfersOwnership) {
    LuaState state1;
    ASSERT_TRUE(state1.is_valid());

    LuaState state2(std::move(state1));

    // state1 应该失效
    EXPECT_FALSE(state1.is_valid());
    EXPECT_EQ(state1.get(), nullptr);

    // state2 应该有效
    EXPECT_TRUE(state2.is_valid());
    EXPECT_NE(state2.get(), nullptr);
}

TEST(LuaStateTest, MoveAssignment_TransfersOwnership) {
    LuaState state1;
    LuaState state2;
    ASSERT_TRUE(state1.is_valid());
    ASSERT_TRUE(state2.is_valid());

    auto* ptr1 = state1.get();

    state2 = std::move(state1);

    // state1 应该失效
    EXPECT_FALSE(state1.is_valid());
    EXPECT_EQ(state1.get(), nullptr);

    // state2 应该有效，且指向原来的 state1
    EXPECT_TRUE(state2.is_valid());
    EXPECT_EQ(state2.get(), ptr1);
}

TEST(LuaStateTest, SelfMoveAssignment_HandlesCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 自我移动赋值（虽然不是常见用法，但应该安全）
    state = std::move(state);

    // 状态应该仍然有效
    EXPECT_TRUE(state.is_valid());
}

// ============================================================================
// LuaState 文件加载测试
// ============================================================================

TEST(LuaStateTest, LoadFile_ValidLuaFile_Succeeds) {
    LuaState state;
    TempFile file(lua_code::valid_simple(), ".lua");

    std::string error;
    EXPECT_TRUE(state.load_file(file.c_str(), &error));
    EXPECT_TRUE(error.empty()) << "Error message: " << error;
}

TEST(LuaStateTest, LoadFile_NonExistentFile_Fails) {
    LuaState state;

    std::string error;
    EXPECT_FALSE(state.load_file("nonexistent_file.lua", &error));
    EXPECT_FALSE(error.empty());
}

TEST(LuaStateTest, LoadFile_SyntaxError_Fails) {
    LuaState state;
    TempFile file(lua_code::syntax_error(), ".lua");

    std::string error;
    EXPECT_FALSE(state.load_file(file.c_str(), &error));
    EXPECT_FALSE(error.empty());

    // 错误信息应包含关键字
    EXPECT_TRUE(error.find("syntax") != std::string::npos ||
                error.find("near") != std::string::npos ||
                error.find("expected") != std::string::npos);
}

TEST(LuaStateTest, LoadFile_RuntimeError_Fails) {
    LuaState state;
    TempFile file(lua_code::runtime_error(), ".lua");

    std::string error;
    EXPECT_FALSE(state.load_file(file.c_str(), &error));
    EXPECT_FALSE(error.empty());
}

TEST(LuaStateTest, LoadFile_WithoutErrorMsg_DoesNotCrash) {
    LuaState state;
    TempFile file(lua_code::valid_simple(), ".lua");

    // 不传递 error_msg 参数，不应该崩溃
    EXPECT_TRUE(state.load_file(file.c_str()));
}

TEST(LuaStateTest, LoadFile_InvalidState_Fails) {
    LuaState state1;
    LuaState state2(std::move(state1));  // state1 现在无效

    std::string error;
    EXPECT_FALSE(state1.load_file("test.lua", &error));
    EXPECT_TRUE(error.find("null") != std::string::npos);
}

TEST(LuaStateTest, LoadFile_ReturnTable_Succeeds) {
    LuaState state;
    TempFile file(lua_code::valid_table(), ".lua");

    std::string error;
    EXPECT_TRUE(state.load_file(file.c_str(), &error));
}

// ============================================================================
// LuaState Buffer 加载测试
// ============================================================================

TEST(LuaStateTest, LoadBuffer_ValidCode_Succeeds) {
    LuaState state;
    auto code = lua_code::valid_simple();
    std::string error;

    EXPECT_TRUE(state.load_buffer(code.c_str(), code.size(), "test_buffer", &error));
    EXPECT_TRUE(error.empty());
}

TEST(LuaStateTest, LoadBuffer_SyntaxError_Fails) {
    LuaState state;
    auto code = lua_code::syntax_error();
    std::string error;

    EXPECT_FALSE(state.load_buffer(code.c_str(), code.size(), "test_buffer", &error));
    EXPECT_FALSE(error.empty());
}

TEST(LuaStateTest, LoadBuffer_RuntimeError_Fails) {
    LuaState state;
    auto code = lua_code::runtime_error();
    std::string error;

    EXPECT_FALSE(state.load_buffer(code.c_str(), code.size(), "test_buffer", &error));
    EXPECT_FALSE(error.empty());
}

TEST(LuaStateTest, LoadBuffer_EmptyString_Succeeds) {
    LuaState state;
    auto code = lua_code::empty();
    std::string error;

    // 空字符串在 Lua 中是有效的
    EXPECT_TRUE(state.load_buffer(code.c_str(), code.size(), "empty_test", &error));
}

TEST(LuaStateTest, LoadBuffer_OnlyComments_Succeeds) {
    LuaState state;
    auto code = lua_code::only_comments();
    std::string error;

    EXPECT_TRUE(state.load_buffer(code.c_str(), code.size(), "comment_test", &error));
}

TEST(LuaStateTest, LoadBuffer_WithNameParameter_UsesNameInErrors) {
    LuaState state;
    auto code = lua_code::syntax_error();
    std::string error;

    state.load_buffer(code.c_str(), code.size(), "my_custom_name", &error);

    // 错误信息应包含自定义名称
    EXPECT_TRUE(error.find("my_custom_name") != std::string::npos);
}

TEST(LuaStateTest, LoadBuffer_InvalidState_Fails) {
    LuaState state1;
    LuaState state2(std::move(state1));  // state1 现在无效

    auto code = lua_code::valid_simple();
    std::string error;

    EXPECT_FALSE(state1.load_buffer(code.c_str(), code.size(), "test", &error));
    EXPECT_TRUE(error.find("null") != std::string::npos);
}

TEST(LuaStateTest, LoadBuffer_WithoutErrorMsg_DoesNotCrash) {
    LuaState state;
    auto code = lua_code::valid_simple();

    // 不传递 error_msg 参数，不应该崩溃
    EXPECT_TRUE(state.load_buffer(code.c_str(), code.size(), "test"));
}

// ============================================================================
// LuaState 错误处理测试
// ============================================================================

TEST(LuaStateTest, GetErrorString_WithStringOnStack_ReturnsString) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 手动在栈上放一个错误字符串
    lua_pushstring(state.get(), "test error message");

    std::string error = state.get_error_string();
    EXPECT_EQ(error, "test error message");

    // 栈应该恢复到初始位置（get_error_string 会弹出一个元素）
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top);
}

TEST(LuaStateTest, GetErrorString_WithNonStringOnStack_ReturnsError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上放一个数字
    // 注意：lua_isstring 对数字也返回 true（因为数字可以转字符串）
    // lua_tostring 会将数字转换为字符串
    lua_pushnumber(state.get(), 42);

    std::string error = state.get_error_string();
    // 数字会被转换为字符串 "42"
    EXPECT_EQ(error, "42");

    // 栈应该恢复到初始位置
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top);
}

TEST(LuaStateTest, GetErrorString_InvalidState_ReturnsError) {
    LuaState state1;
    LuaState state2(std::move(state1));  // state1 现在无效

    std::string error = state1.get_error_string();
    EXPECT_TRUE(error.find("null") != std::string::npos);
}

TEST(LuaStateTest, GetErrorString_WithTableOnStack_ReturnsUnknownError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上放一个 table
    lua_newtable(state.get());
    lua_pushstring(state.get(), "key");
    lua_pushnumber(state.get(), 42);
    lua_rawset(state.get(), -3);  // t["key"] = 42

    // 栈顶现在是 table
    std::string error = state.get_error_string();
    EXPECT_EQ(error, "Failed to get error string: Unknown error");

    // 栈不应该被弹出（因为 lua_isstring 返回 false，没有执行 lua_pop）
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top + 1);

    // 清理栈
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, GetErrorString_WithBooleanOnStack_ReturnsUnknownError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上放一个 boolean
    lua_pushboolean(state.get(), 1);

    // 栈顶现在是 boolean
    std::string error = state.get_error_string();
    EXPECT_EQ(error, "Failed to get error string: Unknown error");

    // 栈不应该被弹出
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top + 1);

    // 清理栈
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, GetErrorString_WithNilOnStack_ReturnsUnknownError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上放 nil
    lua_pushnil(state.get());

    // 栈顶现在是 nil
    std::string error = state.get_error_string();
    EXPECT_EQ(error, "Failed to get error string: Unknown error");

    // 栈不应该被弹出
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top + 1);

    // 清理栈
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, GetErrorString_WithFunctionOnStack_ReturnsUnknownError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上放一个函数
    lua_pushcfunction(state.get(), [](lua_State* L) -> int {
        lua_pushstring(L, "test");
        return 1;
    });

    // 栈顶现在是 function（C function）
    std::string error = state.get_error_string();
    EXPECT_EQ(error, "Failed to get error string: Unknown error");

    // 栈不应该被弹出
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top + 1);

    // 清理栈
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, GetErrorString_WithUserDataOnStack_ReturnsUnknownError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上放一个 lightuserdata
    int dummy_data = 42;
    lua_pushlightuserdata(state.get(), &dummy_data);

    // 栈顶现在是 userdata
    std::string error = state.get_error_string();
    EXPECT_EQ(error, "Failed to get error string: Unknown error");

    // 栈不应该被弹出
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top + 1);

    // 清理栈
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, GetErrorString_WithThreadOnStack_ReturnsUnknownError) {
    LuaState state;

    // 记录初始栈位置
    int initial_top = lua_gettop(state.get());

    // 在栈上创建一个新线程（coroutine）
    lua_newthread(state.get());

    // 栈顶现在是 thread
    std::string error = state.get_error_string();
    EXPECT_EQ(error, "Failed to get error string: Unknown error");

    // 栈不应该被弹出
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top + 1);

    // 清理栈
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, LoadFile_ErrorString_Populated) {
    LuaState state;
    TempFile file(lua_code::syntax_error(), ".lua");

    int initial_top = lua_gettop(state.get());

    std::string error;
    state.load_file(file.c_str(), &error);

    // load_file 失败后，错误字符串应该被获取并填充到 error
    EXPECT_FALSE(error.empty());

    // 栈应该恢复到初始位置
    int top = lua_gettop(state.get());
    EXPECT_EQ(top, initial_top);
}

// ============================================================================
// LuaState 栈操作测试
// ============================================================================

TEST(LuaStateTest, StackBalance_AfterLoadFile) {
    LuaState state;
    TempFile file(lua_code::valid_simple(), ".lua");

    int top_before = lua_gettop(state.get());

    std::string error;
    state.load_file(file.c_str(), &error);

    int top_after = lua_gettop(state.get());

    // load_file 使用 luaL_dofile，它会保留返回值在栈上
    // valid_simple() 返回 x + y，所以栈会多一个元素
    EXPECT_EQ(top_after, top_before + 1);

    // 清理返回值
    lua_pop(state.get(), 1);
}

TEST(LuaStateTest, StackBalance_AfterLoadBuffer) {
    LuaState state;
    auto code = lua_code::valid_simple();

    int top_before = lua_gettop(state.get());

    std::string error;
    state.load_buffer(code.c_str(), code.size(), "test", &error);

    int top_after = lua_gettop(state.get());

    // 栈应该保持平衡
    EXPECT_EQ(top_before, top_after);
}

TEST(LuaStateTest, StackBalance_AfterFailedLoad) {
    LuaState state;
    TempFile file(lua_code::syntax_error(), ".lua");

    int top_before = lua_gettop(state.get());

    std::string error;
    state.load_file(file.c_str(), &error);

    int top_after = lua_gettop(state.get());

    // 即使加载失败，栈也应该保持平衡
    EXPECT_EQ(top_before, top_after);
}

// ============================================================================
// LuaState 安全性测试
// ============================================================================

TEST(LuaStateTest, LoadFile_WithIoLibrary_Fails) {
    LuaState state;
    TempFile file(lua_code::use_io_library(), ".lua");

    std::string error;
    EXPECT_FALSE(state.load_file(file.c_str(), &error));

    // io 库应该不可用
    EXPECT_TRUE(error.find("io") != std::string::npos ||
                error.find("global") != std::string::npos);
}

TEST(LuaStateTest, MultipleStates_Independent) {
    LuaState state1;
    LuaState state2;

    EXPECT_TRUE(state1.is_valid());
    EXPECT_TRUE(state2.is_valid());

    EXPECT_NE(state1.get(), state2.get());

    // 在 state1 中设置全局变量
    lua_pushstring(state1.get(), "test_value");
    lua_setglobal(state1.get(), "test_var");

    // 在 state2 中获取该全局变量，应该不存在
    lua_getglobal(state2.get(), "test_var");
    EXPECT_EQ(lua_type(state2.get(), -1), LUA_TNIL);
    lua_pop(state2.get(), 1);
}

// ============================================================================
// LuaState 边界条件测试
// ============================================================================

TEST(LuaStateTest, LoadBuffer_ZeroSize) {
    LuaState state;
    std::string error;

    // 零大小 buffer（但非空指针）
    const char* data = "";
    EXPECT_TRUE(state.load_buffer(data, 0, "zero_size", &error));
}

TEST(LuaStateTest, LoadBuffer_VeryLargeCode) {
    LuaState state;

    // 创建一个较大的代码块
    std::string code = "local x = 0\n";
    for (int i = 0; i < 1000; ++i) {
        code += "x = x + " + std::to_string(i) + "\n";
    }
    code += "return x\n";

    std::string error;
    EXPECT_TRUE(state.load_buffer(code.c_str(), code.size(), "large_code", &error));
}

TEST(LuaStateTest, LoadFile_UnicodeInPath) {
    LuaState state;

    // 创建包含 Unicode 的文件名
    std::string content = lua_code::valid_simple();
    TempFile file(content, ".lua");

    std::string error;
    // TempFile 使用 /tmp，应该支持 Unicode
    EXPECT_TRUE(state.load_file(file.c_str(), &error));
}

// ============================================================================
// LuaState JIT 控制测试
// ============================================================================
//
// 注意：LuaState 在初始化时会加载 jit 库（luaopen_jit），
// 这使得 JIT 控制功能可用。在加载 jit 库后，
// luaJIT_setmode 调用应该能够正常工作。
// ============================================================================
//
// JIT 控制说明：
// - enable_jit(): 启用 JIT 编译引擎
// - disable_jit(): 禁用 JIT 编译引擎（切换到解释模式）
// - flush_jit(): 刷新 JIT 编译器缓存，清除已编译的代码
// ============================================================================

TEST(LuaStateTest, EnableJIT_ValidState_Succeeds) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // LuaState 初始化时会加载 jit 库，因此 enable_jit 应该成功
    // 先禁用再启用，以确保测试的可靠性
    EXPECT_TRUE(state.disable_jit());
    EXPECT_TRUE(state.enable_jit());
}

TEST(LuaStateTest, DisableJIT_ValidState_Succeeds) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 禁用 JIT
    EXPECT_TRUE(state.disable_jit());

    // 再次禁用也应该成功
    EXPECT_TRUE(state.disable_jit());
}

TEST(LuaStateTest, FlushJIT_ValidState_Succeeds) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 刷新 JIT 缓存
    EXPECT_TRUE(state.flush_jit());

    // 再次刷新也应该成功
    EXPECT_TRUE(state.flush_jit());
}

TEST(LuaStateTest, EnableJIT_InvalidState_Fails) {
    LuaState state1;
    LuaState state2(std::move(state1));  // state1 现在无效

    // 对无效状态启用 JIT 应该失败
    EXPECT_FALSE(state1.enable_jit());
}

TEST(LuaStateTest, DisableJIT_InvalidState_Fails) {
    LuaState state1;
    LuaState state2(std::move(state1));  // state1 现在无效

    // 对无效状态禁用 JIT 应该失败
    EXPECT_FALSE(state1.disable_jit());
}

TEST(LuaStateTest, FlushJIT_InvalidState_Fails) {
    LuaState state1;
    LuaState state2(std::move(state1));  // state1 现在无效

    // 对无效状态刷新 JIT 应该失败
    EXPECT_FALSE(state1.flush_jit());
}

TEST(LuaStateTest, JIT_ToggleOperation_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 测试禁用和启用的循环
    // 由于加载了 jit 库，所有操作都应该成功
    for (int i = 0; i < 3; ++i) {
        EXPECT_TRUE(state.disable_jit());
        EXPECT_TRUE(state.enable_jit());
    }
}

TEST(LuaStateTest, JIT_FlushAfterDisable_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 先禁用 JIT
    EXPECT_TRUE(state.disable_jit());

    // 刷新 JIT 缓存（即使在禁用状态下）
    EXPECT_TRUE(state.flush_jit());
}

TEST(LuaStateTest, JIT_FlushAfterEnable_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 禁用和启用 JIT
    EXPECT_TRUE(state.disable_jit());
    EXPECT_TRUE(state.enable_jit());

    // 刷新 JIT 缓存
    EXPECT_TRUE(state.flush_jit());
}

TEST(LuaStateTest, JIT_WithCodeExecution_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 执行一些代码，无论 JIT 是否可用都应该正常工作
    auto code = R"(
        local function add(a, b)
            return a + b
        end

        -- 多次调用以触发 JIT 编译（如果 JIT 可用）
        local sum = 0
        for i = 1, 100 do
            sum = add(sum, i)
        end

        return sum
    )";

    std::string error;
    EXPECT_TRUE(state.load_buffer(code, strlen(code), "jit_test", &error));
    EXPECT_TRUE(error.empty());
}

TEST(LuaStateTest, JIT_CodeExecutionWithDisabledJIT_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 禁用 JIT
    EXPECT_TRUE(state.disable_jit());

    // 执行代码（应该使用解释模式）
    auto code = R"(
        local function multiply(a, b)
            return a * b
        end

        local result = 1
        for i = 1, 10 do
            result = multiply(result, 2)
        end

        return result
    )";

    std::string error;
    EXPECT_TRUE(state.load_buffer(code, strlen(code), "jit_interpreter_test", &error));
    EXPECT_TRUE(error.empty());
}

TEST(LuaStateTest, JIT_MultipleFlushDuringExecution_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 第一次刷新 JIT 缓存
    EXPECT_TRUE(state.flush_jit());

    // 执行一些代码
    auto code1 = R"(
        local x = 0
        for i = 1, 50 do x = x + i end
        return x
    )";

    std::string error;
    EXPECT_TRUE(state.load_buffer(code1, strlen(code1), "test1", &error));
    EXPECT_TRUE(error.empty());

    // 第二次刷新 JIT 缓存
    EXPECT_TRUE(state.flush_jit());

    // 执行更多代码
    auto code2 = R"(
        local y = 1
        for i = 1, 50 do y = y * i end
        return y
    )";

    EXPECT_TRUE(state.load_buffer(code2, strlen(code2), "test2", &error));
    EXPECT_TRUE(error.empty());

    // 第三次刷新 JIT 缓存
    EXPECT_TRUE(state.flush_jit());
}

TEST(LuaStateTest, JIT_RapidToggle_WorksCorrectly) {
    LuaState state;
    ASSERT_TRUE(state.is_valid());

    // 快速切换 JIT 状态
    for (int i = 0; i < 10; ++i) {
        EXPECT_TRUE(state.disable_jit());
        EXPECT_TRUE(state.flush_jit());
        EXPECT_TRUE(state.enable_jit());
        EXPECT_TRUE(state.flush_jit());
    }

    // 最后确保恢复正常状态
    EXPECT_TRUE(state.disable_jit());
    EXPECT_TRUE(state.enable_jit());
}

TEST(LuaStateTest, JIT_StateIndependence_DifferentStatesHaveIndependentJIT) {
    LuaState state1;
    LuaState state2;
    ASSERT_TRUE(state1.is_valid());
    ASSERT_TRUE(state2.is_valid());

    // state1 禁用 JIT
    EXPECT_TRUE(state1.disable_jit());

    // state2 的 JIT 状态应该独立
    EXPECT_TRUE(state1.disable_jit()); // state1 再次禁用应该成功
    EXPECT_TRUE(state2.disable_jit()); // state2 禁用也应该成功
    EXPECT_TRUE(state2.enable_jit()); // state2 启用应该成功
}
