# 测试指南

本文档提供 LuaJIT Rule Engine 项目的详细测试信息。

## 目录

- [快速开始](#快速开始)
- [测试架构](#测试架构)
- [运行测试](#运行测试)
- [代码覆盖率](#代码覆盖率)
- [编写测试](#编写测试)
- [测试最佳实践](#测试最佳实践)

## 快速开始

### 1. 编译测试

```bash
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3
make -j$(nproc)
```

### 2. 运行所有测试

```bash
# 在 build 目录下
ctest --output-on-failure
```

### 3. 查看测试结果

```bash
# 查看测试摘要
ctest

# 查看详细输出
ctest --verbose

# 查看失败的测试详情
ctest --rerun-failed --output-on-failure
```

## 测试架构

项目使用 GoogleTest 框架，测试分为以下模块：

### 模块概览

| 测试文件 | 测试内容 | 测试用例数 | 状态 |
|---------|---------|-----------|------|
| `lua_state_test.cpp` | Lua 状态管理 | 52 | ✅ 全部通过 |
| `lua_stack_guard_test.cpp` | Lua 栈守卫 | 17 | ✅ 全部通过 |
| `data_adapter_test.cpp` | 数据适配器 | 46 | ✅ 全部通过 |
| `rule_engine_test.cpp` | 规则引擎 | 78 | ✅ 全部通过 |
| `integration_test.cpp` | 集成测试 | 11 | ✅ 全部通过 |
| **总计** | | **204** | **✅ 100% 通过** |

### 测试分类

#### 1. 单元测试 (Unit Tests)

**lua_state_test.cpp** - LuaState 类测试 (52个测试用例)
- 构造和析构测试（移动语义）
- 文件加载测试（成功/失败场景）
- Buffer 加载测试
- 错误处理测试（包括栈顶非字符串场景：table、boolean、nil、function、userdata、thread）
- 栈操作测试
- 安全性测试
- 边界条件测试
- JIT 控制测试（enable/disable/flush 及各种组合场景）

**lua_stack_guard_test.cpp** - LuaStackGuard 类测试
- 基本栈恢复测试
- 多次 push/pop 测试
- 嵌套守卫测试
- Release 机制测试
- 空栈测试
- 函数调用场景测试
- 表迭代场景测试
- 错误处理场景测试

**data_adapter_test.cpp** - JsonAdapter 类测试 (46个测试用例)
- 基本类型转换（null, boolean, number, string）
- 数组转换测试
- 对象转换测试
- 嵌套结构测试
- 特殊字符处理（含空字符、Unicode 等）
- 错误处理测试（包括异常捕获和错误传播）
- 边界条件测试（深度嵌套、大量数据、混合类型）
- 栈平衡测试

#### 2. 集成测试 (Integration Tests)

**rule_engine_test.cpp** - 规则引擎集成测试 (78个测试用例)
- 规则加载和卸载
- 规则匹配（单个和批量）
- 规则热更新
- 配置文件加载（包含各种错误场景）
- 批量规则处理（使用 `std::map` 返回结果）
- 错误场景处理（包括 Lua 状态无效、规则函数表不存在等）
- call_match_function 错误路径完整覆盖

**integration_test.cpp** - 端到端场景测试
- 用户注册验证
- 规则动态管理
- 多引擎独立运行
- 复杂数据结构处理
- 大数据集处理

**重要说明**: `match_all_rules` 接口返回 `std::map<std::string, MatchResult>`，键为规则名，值为匹配结果。

## 运行测试

### 使用 CTest

```bash
# 运行所有测试
ctest

# 运行特定测试
ctest -R lua_state_test

# 并行运行测试（加速）
ctest -j$(nproc)

# 只运行失败的测试
ctest --rerun-failed

# 输出详细日志
ctest -V

# 输出失败测试的详细信息
ctest --output-on-failure
```

### 直接运行测试可执行文件

```bash
# 进入测试目录
cd build/tests

# 运行单个测试文件
./lua_state_test

# 运行特定测试用例
./lua_state_test --gtest_filter="LuaStateTest.LoadFile*"

# 显示简要输出
./lua_state_test --gtest_brief=yes

# 列出所有测试用例（不执行）
./lua_state_test --gtest_list_tests

# 重复运行测试（用于发现间歇性问题）
./lua_state_test --gtest_repeat=100

# 在第一个失败时停止
./lua_state_test --gtest_break_on_failure
```

### GoogleTest 过滤器语法

```bash
# 运行特定测试套件
--gtest_filter="LuaStateTest.*"

# 运行特定测试用例
--gtest_filter="LuaStateTest.LoadFile_ValidLuaFile_Succeeds"

# 运行多个测试
--gtest_filter="LuaStateTest.*:JsonAdapterTest.*"

# 排除特定测试
--gtest_filter="-*Disabled*"

# 通配符匹配
--gtest_filter="*LoadFile*"
--gtest_filter="*.ComplexScenario"
```

## 代码覆盖率

### 安装覆盖率工具

```bash
# Ubuntu/Debian
sudo apt-get install lcov gcov

# macOS (使用 Homebrew)
brew install lcov

# Fedora/RHEL
sudo dnf install lcov
```

### 生成覆盖率报告

#### 1. 编译带覆盖率信息的版本

```bash
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_COVERAGE=ON
make -j$(nproc)
```

#### 2. 运行测试生成数据

```bash
ctest
```

这会在 `build/` 目录下生成 `.gcda` 和 `.gcno` 文件。

#### 3. 生成覆盖率报告

**HTML 报告（推荐）**

```bash
# 捕获覆盖率数据
lcov --capture --directory . --output-file coverage.info

# 过滤掉不需要的文件
lcov --remove coverage.info '/usr/*' \
     --remove coverage.info 'third-party/*' \
     --remove coverage.info 'tests/*' \
     --output-file coverage_filtered.info

# 生成 HTML 报告
genhtml coverage_filtered.info --output-directory coverage_html

# 在浏览器中查看
firefox coverage_html/index.html
```

**命令行摘要**

```bash
# 查看总体覆盖率
lcov --summary coverage.info

# 查看特定文件的覆盖率
lcov --list coverage.info
```

#### 4. 查看覆盖率目标

项目覆盖率目标：

| 模块 | 行覆盖率 | 函数覆盖率 | 分支覆盖率 |
|-----|---------|-----------|-----------|
| LuaState | ≥90% | ≥95% | ≥85% |
| LuaStackGuard | ≥90% | ≥95% | ≥85% |
| JsonAdapter | ≥85% | ≥90% | ≥80% |
| RuleEngine | ≥85% | ≥90% | ≥80% |
| **总体** | **≥85%** | **≥90%** | **≥80%** |

### 持续监控覆盖率

```bash
# 完整的覆盖率检查流程
#!/bin/bash
set -e

mkdir -p build && cd build
cmake .. -DBUILD_COVERAGE=ON
make -j$(nproc)
ctest

lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info 'third-party/*' --output-file coverage.info
lcov --remove coverage.info 'tests/*' --output-file coverage.info

# 检查覆盖率是否达标
lcov --summary coverage.info | grep "lines.*%"
```

## 编写测试

### 测试文件结构

```cpp
#include <gtest/gtest.h>
#include "ljre/your_class.h"

// 测试套件
class YourClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 每个测试运行前调用
        // 初始化测试数据
        CreateTestRules();
    }

    void TearDown() override {
        // 每个测试运行后调用
        // 清理资源
        CleanupTestData();
    }

    // 测试辅助函数
    void CreateTestRules() {
        // 创建测试规则文件
        CreateRuleFile("test.lua", "function match(data) return true end");
    }

    void CleanupTestData() {
        // 清理测试数据目录
        system("rm -rf test_data");
    }

    void CreateRuleFile(const std::string& filename, const std::string& content) {
        system("mkdir -p test_data/rules");
        std::ofstream file("test_data/rules/" + filename);
        file << content;
        file.close();  // 必须关闭文件
    }

    void CreateConfigFile(const std::string& filename, const std::string& content) {
        system("mkdir -p test_data/configs");
        std::ofstream file("test_data/configs/" + filename);
        file << content;
        file.close();  // 必须关闭文件
    }

    // 测试数据成员
    YourClass object_;
};

// 测试用例
TEST_F(YourClassTest,MethodName_Scenario_ExpectedResult) {
    // Arrange（准备）
    int input = 42;

    // Act（执行）
    int result = object_.Method(input);

    // Assert（断言）
    EXPECT_EQ(result, 42);
}
```

### 批量规则测试示例

```cpp
TEST_F(RuleEngineTest, MatchAllRules_MultipleRules) {
    RuleEngine engine;
    std::string error;

    // 添加多个规则
    CreateRuleFile("age_check.lua", R"(
function match(data)
    if data.age < 18 then
        return false, "年龄不足"
    end
    return true, "年龄检查通过"
end
)");

    CreateRuleFile("email_check.lua", R"(
function match(data)
    if not string.match(data.email, "^[^@]+@[^@]+$") then
        return false, "邮箱格式错误"
    end
    return true, "邮箱检查通过"
end
)");

    ASSERT_TRUE(engine.add_rule("age", "test_data/rules/age_check.lua", &error));
    ASSERT_TRUE(engine.add_rule("email", "test_data/rules/email_check.lua", &error));

    // 测试数据
    json data = {
        {"age", 25},
        {"email", "test@example.com"}
    };
    JsonAdapter adapter(data);

    // 批量匹配
    std::map<std::string, MatchResult> results;
    EXPECT_TRUE(engine.match_all_rules(adapter, results, &error));

    // 验证结果 - 使用规则名访问
    EXPECT_EQ(results.size(), 2);
    EXPECT_TRUE(results.at("age").matched);
    EXPECT_TRUE(results.at("email").matched);

    // 测试失败场景
    json invalid_data = {
        {"age", 16},  // 年龄不足
        {"email", "test@example.com"}
    };
    JsonAdapter invalid_adapter(invalid_data);

    std::map<std::string, MatchResult> invalid_results;
    EXPECT_FALSE(engine.match_all_rules(invalid_adapter, invalid_results, &error));

    // 验证哪个规则失败了
    EXPECT_FALSE(invalid_results.at("age").matched);
    EXPECT_TRUE(invalid_results.at("email").matched);
}
```

### 测试命名规范

使用描述性的测试名称：`MethodName_Scenario_ExpectedResult`

```cpp
// ✅ 好的命名
TEST_F(JsonAdapterTest, IntegerArray_ConvertsToLuaTable)
TEST_F(RuleEngineTest, AddRule_DuplicateName_Fails)
TEST(LuaStateTest, LoadFile_SyntaxError_Fails)

// ❌ 不好的命名
TEST_F(JsonAdapterTest, Test1)
TEST_F(RuleEngineTest, TestAddRule)
```

### 断言选择

```cpp
// EXPECT_* - 失败后继续执行
EXPECT_EQ(a, b);
EXPECT_TRUE(condition);
EXPECT_STREQ(str1, str2);

// ASSERT_* - 失败后停止执行当前测试
ASSERT_EQ(a, b);
ASSERT_TRUE(condition);
ASSERT_STREQ(str1, str2);

// 浮点数比较
EXPECT_DOUBLE_EQ(a, b);
EXPECT_NEAR(a, b, 0.001);

// 异常测试
EXPECT_THROW(statement, exception_type);
EXPECT_ANY_THROW(statement);
EXPECT_NO_THROW(statement);
```

### 使用测试辅助工具

项目提供了 `test_helpers.h`，包含常用的测试辅助类和函数：

```cpp
#include "test_helpers.h"

// 创建临时文件（自动清理）
TempFile file(lua_code::valid_simple(), ".lua");
EXPECT_TRUE(state.load_file(file.c_str()));

// 预定义的测试代码
lua_code::valid_simple()
lua_code::syntax_error()
lua_code::runtime_error()
rule_code::always_pass()
rule_code::age_check()
```

## 测试最佳实践

### 1. 遵循 AAA 模式

```cpp
TEST_F(YourClassTest, Something) {
    // Arrange（准备测试数据和条件）
    int input = 42;
    object_.SetState(State::Ready);

    // Act（执行被测试的操作）
    int result = object_.Process(input);

    // Assert（验证结果）
    EXPECT_EQ(result, 84);
    EXPECT_EQ(object_.GetState(), State::Done);
}
```

### 2. 保持测试独立性

```cpp
// ✅ 好的例子 - 每个测试独立
TEST_F(YourClassTest, TestA) {
    // 独立的设置和验证
}

TEST_F(YourClassTest, TestB) {
    // 不依赖 TestA 的状态
}

// ❌ 不好的例子 - 测试之间有依赖
TEST_F(YourClassTest, TestA) {
    global_state = 1;  // 影响其他测试
}
```

### 3. 使用 RAII 管理资源

```cpp
TEST_F(YourClassTest, WithResourceCleanup) {
    // 临时文件会在测试结束时自动删除
    TempFile file(content, ".lua");

    // 使用文件
    // ...

    // 不需要手动清理
}
```

### 4. 管理测试文件

**重要**: 创建测试文件时必须：
1. 使用辅助函数创建文件和目录
2. 调用 `file.close()` 确保内容写入磁盘
3. 在 `TearDown()` 中清理测试数据

```cpp
class RuleEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 初始化测试规则文件
        CreateTestRules();
    }

    void TearDown() override {
        // 清理测试数据 - 避免测试间干扰
        CleanupTestData();
    }

    void CreateRuleFile(const std::string& filename, const std::string& content) {
        system("mkdir -p test_data/rules");
        std::ofstream file("test_data/rules/" + filename);
        file << content;
        file.close();  // ⚠️ 必须关闭文件！
    }

    void CleanupTestData() {
        system("rm -rf test_data");
    }
};
```

### 5. 测试边界条件

```cpp
TEST_F(YourClassTest, BoundaryConditions) {
    // 测试空值
    EXPECT_TRUE(obj.Process(""));

    // 测试最大值
    EXPECT_TRUE(obj.Process(INT_MAX));

    // 测试最小值
    EXPECT_TRUE(obj.Process(INT_MIN));

    // 测试非法值
    EXPECT_FALSE(obj.Process(-1));
}
```

### 6. 测试错误处理

```cpp
TEST_F(YourClassTest, ErrorHandling) {
    std::string error;

    // 测试 null 状态
    EXPECT_FALSE(obj.Process(nullptr, &error));
    EXPECT_FALSE(error.empty());

    // 测试无效输入
    EXPECT_FALSE(obj.Process("invalid", &error));
    EXPECT_THAT(error, HasSubstr("invalid"));
}
```

### 7. 使用参数化测试

```cpp
class YourClassTest : public ::testing::TestWithParam<int> {
    // ...
};

TEST_P(YourClassTest, HandlesVariousInputs) {
    int input = GetParam();
    EXPECT_TRUE(object_.Process(input));
}

INSTANTIATE_TEST_SUITE_P(
    ValidInputs,
    YourClassTest,
    ::testing::Values(1, 2, 3, 4, 5)
);
```

## 调试测试

### 使用调试器

```bash
# 使用 gdb
gdb --args ./tests/lua_state_test --gtest_filter="LuaStateTest.LoadFile*"

# 使用 lldb
lldb ./tests/lua_state_test -- --gtest_filter="LuaStateTest.LoadFile*"
```

### 启用详细日志

```cpp
// 在测试中添加日志
TEST_F(YourClassTest, DebuggingTest) {
    // 添加日志输出
    std::cout << "Debug info: " << value << std::endl;

    // 使用记录属性
    RecordProperty("custom_property", "value");
}
```

### 只运行失败的测试

```bash
# 第一次运行
ctest

# 只重新运行失败的测试
ctest --rerun-failed --output-on-failure
```

## 性能测试

对于性能关键的代码，可以使用 Google Benchmark：

```cpp
#include <benchmark/benchmark.h>

static void BM_JsonAdapterConversion(benchmark::State& state) {
    json data = {/* ... */};
    JsonAdapter adapter(data);

    for (auto _ : state) {
        lua_State* L = luaL_newstate();
        adapter.push_to_lua(L);
        lua_close(L);
    }
}
BENCHMARK(BM_JsonAdapterConversion);

BENCHMARK_MAIN();
```

## 常见问题

### Q: match_all_rules 返回什么类型？

`match_all_rules` 返回 `std::map<std::string, MatchResult>`，不是 `std::vector`。

```cpp
std::map<std::string, MatchResult> results;
engine.match_all_rules(adapter, results, &error);

// 使用规则名访问结果
for (const auto& pair : results) {
    std::cout << pair.first << ": " << pair.second.message << std::endl;
}

// 直接访问特定规则
if (results.count("age_check")) {
    EXPECT_TRUE(results.at("age_check").matched);
}
```

### Q: 为什么测试文件创建后读取失败？

可能的原因：
1. **忘记关闭文件**: 使用 `std::ofstream` 后必须调用 `close()`
   ```cpp
   std::ofstream file("test.lua");
   file << "content";
   file.close();  // ⚠️ 必须关闭！
   ```

2. **目录不存在**: 使用 `mkdir -p` 创建目录
   ```cpp
   system("mkdir -p test_data/rules");
   ```

3. **测试数据残留**: 在 `TearDown()` 中清理测试数据
   ```cpp
   void TearDown() override {
       system("rm -rf test_data");
   }
   ```

### Q: 测试失败了怎么办？

1. 查看详细错误信息
   ```bash
   ctest --output-on-failure
   ```

2. 单独运行失败的测试
   ```bash
   ./tests/your_test --gtest_filter="FailedTest.*"
   ```

3. 使用调试器定位问题

### Q: 如何测试内存泄漏？

```bash
# 使用 valgrind
valgrind --leak-check=full --show-leak-kinds=all ./tests/your_test
```

### Q: 覆盖率数据不准确？

确保：
1. 使用 `-DBUILD_COVERAGE=ON` 编译
2. 运行测试而不是直接执行
3. 清理旧的 `.gcda` 文件
   ```bash
   make clean
   ctest
   ```

### Q: 测试运行太慢？

1. 使用并行测试
   ```bash
   ctest -j$(nproc)
   ```

2. 只运行相关测试
   ```bash
   ctest -R ".*Test"
   ```

3. 使用测试过滤
   ```bash
   ./tests/your_test --gtest_filter="FastTests.*"
   ```

## 相关文档

- [GoogleTest 文档](https://google.github.io/googletest/)
- [GoogleTest 高级指南](https://google.github.io/googletest/advanced.html)
- [lcov 文档](http://ltp.sourceforge.net/coverage/lcov.php)
- [项目 README](README.md)
