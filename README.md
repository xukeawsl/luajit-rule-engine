# LuaJIT Rule Engine

基于 C++17 和 LuaJIT-2.1.0-beta3 的高性能规则引擎，支持动态更新规则而无需重新编译 C++ 代码。

## 特性

- **高性能**: 使用 LuaJIT JIT 编译器，中等复杂度规则优于原生性能（快 1.72x）
- **动态更新**: 支持运行时重新加载规则，无需重启程序
- **灵活适配**: 使用适配器模式，支持多种数据格式（JSON、Protobuf 等）
- **简洁易用**: 提供 C++17 友好的 API 接口
- **安全可靠**: 使用 RAII 栈守卫自动管理 Lua 栈平衡，避免内存泄漏
- **深度限制**: 支持配置最大嵌套深度（默认 8192），防止栈溢出
- **JIT 控制**: 支持运行时动态启用/禁用/刷新 JIT 编译器
- **最小权限**: 默认只加载必要的 Lua 标准库（base、table、string、math、jit），不开放 io/os/debug 等危险接口
- **零依赖（除 LuaJIT）**: 只依赖 LuaJIT 和 nlohmann/json（header-only）
- **完善的测试**: 包含 236 个单元测试，覆盖所有核心功能和错误场景
- **性能测试套件**: 45+ benchmark 测试用例，详细对比 LuaJIT vs Native 性能

## 编码规范

- 私有成员变量使用 `_` 前缀（例如 `_lua_state`）
- 注释使用中文
- 头文件使用 `.h` 后缀
- 实现文件使用 `.cpp` 后缀
- 不使用异常，使用返回值和错误参数处理错误

## 目录结构

```
luajit-rule-engine/
├── include/ljre/                  # 公共头文件
│   ├── lua_state.h                # Lua 状态管理
│   ├── data_adapter.h             # 数据适配器接口
│   ├── json_adapter.h             # JSON 适配器
│   └── rule_engine.h              # 规则引擎核心
├── src/                           # 实现文件
│   ├── lua_state.cpp
│   ├── json_adapter.cpp
│   └── rule_engine.cpp
├── benchmarks/                    # 性能测试
│   ├── include/                   # Benchmark 头文件
│   ├── src/                       # Benchmark 源文件
│   │   ├── benchmarks/            # 测试用例
│   │   └── rules/                 # Lua 规则文件
│   ├── generate_report.py         # 报告生成脚本
│   └── README.md                  # Benchmark 使用文档
├── examples/                      # 示例代码
│   ├── example.cpp                # 使用示例
│   ├── rule_config.lua            # 规则配置文件
│   └── rules/                     # 规则文件目录
│       ├── age_check.lua
│       ├── email_validation.lua
│       └── user_info_complete.lua
├── tests/                         # 单元测试
│   ├── lua_state_test.cpp
│   ├── data_adapter_test.cpp
│   ├── rule_engine_test.cpp
│   └── ...
├── docs/                          # 文档
│   ├── BENCHMARK_PLAN.md          # 性能测试计划
│   ├── ULTRA_COMPLEX_ANALYSIS.md  # 性能分析
│   └── COVERAGE_*.md              # 覆盖率指南
├── third-party/                   # 第三方库
│   └── json/                      # nlohmann/json
├── cmake/                         # CMake 配置
├── ARCHITECTURE.md                # 架构文档
├── README.md                      # 本文档
└── TESTING.md                     # 测试文档
```

## 依赖

### 核心依赖（必需）
- **LuaJIT-2.1.0-beta3**: 需要安装到 `/usr/local/3rd/luajit-2.1.0-beta3/`
- **nlohmann/json**: v3.11.3，已包含在 `third-party/` 目录中
- **CMake**: >= 3.15
- **C++ 编译器**: 支持 C++17（GCC 7+, Clang 5+, MSVC 2017+）

### 测试依赖
- **GoogleTest**: 用于单元测试（CMake 会自动下载）
- **Google Benchmark**: 用于性能测试（CMake 会自动下载）

### 可选依赖
- **Python 3**: 用于生成性能测试报告

## 编译

```bash
# 基础编译（包含单元测试）
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3
make -j$(nproc)

# 编译包含性能测试的版本
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_BENCHMARKS=ON
make -j$(nproc)

# 编译包含示例的版本
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_EXAMPLES=ON
make -j$(nproc)

# 编译所有内容（测试、示例、性能测试、覆盖率）
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_ALL=ON
make -j$(nproc)

# 同时编译所有内容（与 BUILD_ALL=ON 效果相同）
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 \
         -DBUILD_TESTING=ON \
         -DBUILD_EXAMPLES=ON \
         -DBUILD_BENCHMARKS=ON \
         -DBUILD_COVERAGE=ON
make -j$(nproc)
```

## 测试

项目使用 GoogleTest 框架进行单元测试。

> 💡 **快速开始**: 使用提供的测试脚本
> ```bash
> # 运行所有测试
> ./run_tests.sh
>
> # 生成覆盖率报告
> ./run_tests.sh -c
>
> # 查看更多选项
> ./run_tests.sh --help
> ```

详细的测试指南请参阅 [TESTING.md](TESTING.md)。

### 编译测试

```bash
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3
make -j$(nproc)
```

测试可执行文件会生成在 `build/tests/` 目录下。

### 运行所有测试

```bash
# 使用 CTest 运行所有测试
cd build
ctest --output-on-failure

# 或者查看详细输出
ctest --verbose

# 运行特定测试
ctest -R lua_state_test
ctest -R data_adapter_test
```

### 直接运行单个测试可执行文件

```bash
# 运行所有测试并显示简要结果
./build/tests/lua_state_test --gtest_brief=yes

# 运行特定测试用例
./build/tests/lua_state_test --gtest_filter="LuaStateTest.LoadFile*"

# 运行测试并显示详细输出
./build/tests/lua_state_test --gtest_print_time=1
```

### 测试覆盖率

项目支持使用 GCC/Clang 的 gcov/lcov 生成代码覆盖率报告。

#### 1. 编译带覆盖率信息的版本

```bash
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_COVERAGE=ON
make -j$(nproc)
```

#### 2. 运行测试

```bash
# 运行所有测试以生成覆盖率数据
ctest
```

#### 3. 生成覆盖率报告

```bash
# 方法1: 使用 lcov 生成 HTML 报告（推荐）
lcov --capture --directory . --output-file coverage.info
lcov --remove coverage.info '/usr/*' --output-file coverage.info
lcov --remove coverage.info 'third-party/*' --output-file coverage.info
lcov --remove coverage.info 'tests/*' --output-file coverage.info
genhtml coverage.info --output-directory coverage_html

# 在浏览器中打开报告
# firefox coverage_html/index.html  (Linux)
# open coverage_html/index.html     (macOS)
```

> 💡 **快捷方式**: 使用提供的脚本查看覆盖率
> ```bash
> # 生成并查看覆盖率（Ubuntu/Debian）
> ./view_coverage.sh
> ```

#### 4. 查看覆盖率摘要

```bash
lcov --summary coverage.info
```

示例输出：
```
Summary coverage rate:
  lines......: 90.4% (1945 of 2152 lines)
  functions..: 90.5% (813 of 898 functions)
  branches...: no data found
```

#### 5. 在浏览器中查看详细报告

**使用 Python HTTP 服务器（推荐）**

```bash
# 方法1: 使用快捷脚本（默认端口 8000）
./view_coverage.sh

# 方法2: 指定自定义端口
./view_coverage.sh 9000

# 方法3: 手动启动
cd build/coverage_html
python3 -m http.server 8000
# 然后在浏览器中访问: http://localhost:8000
```

服务器启动后，在浏览器中访问显示的地址即可查看覆盖率报告。按 `Ctrl+C` 停止服务器。

### 测试结构

测试文件位于 `tests/` 目录，按模块组织：

```
tests/
├── test_helpers.h              # 测试辅助工具和测试数据
├── CMakeLists.txt              # 测试构建配置
├── lua_state_test.cpp          # LuaState 类测试（52个测试用例）
│   ├── 构造和析构测试
│   ├── 文件加载测试
│   ├── Buffer 加载测试
│   ├── 错误处理测试（包括栈顶非字符串场景）
│   ├── 栈操作测试
│   ├── 安全性测试
│   ├── 边界条件测试
│   └── JIT 控制测试（enable/disable/flush）
├── lua_stack_guard_test.cpp    # LuaStackGuard 类测试（17个测试用例）
│   ├── 基本栈恢复测试
│   ├── 多次 push/pop 测试
│   ├── 嵌套守卫测试
│   ├── Release 机制测试
│   ├── 空栈测试
│   ├── 函数调用场景测试
│   ├── 表迭代场景测试
│   └── 错误处理场景测试
├── data_adapter_test.cpp       # 数据适配器测试（55个测试用例）
│   ├── 基本类型转换测试
│   ├── 数组转换测试
│   ├── 对象转换测试
│   ├── 嵌套结构测试
│   ├── 特殊字符处理
│   ├── 错误处理测试（包括异常捕获）
│   ├── 边界条件测试
│   ├── 栈平衡测试
│   └── 深度嵌套限制测试（9个测试用例）
├── rule_engine_test.cpp        # 规则引擎测试（127个测试用例）
│   ├── 规则加载和卸载测试
│   ├── 规则匹配测试（单个和批量）
│   ├── 规则热更新测试
│   ├── 配置文件加载测试
│   ├── 错误场景测试
│   ├── Lua 状态无效测试
│   ├── call_match_function 错误路径测试
│   ├── JIT 控制测试（11个测试用例）
│   │   ├── enable_jit/disable_jit/flush_jit 功能测试
│   │   ├── JIT 状态切换测试
│   │   ├── JIT 性能影响测试
│   │   └── 无效 Lua 状态下的 JIT 控制测试
│   ├── match_all_rules 边界情况测试（7个测试用例）
│   │   ├── push_to_lua 失败场景测试
│   │   ├── 函数表不存在场景测试
│   │   ├── 函数不存在场景测试
│   │   ├── 第一个返回值不是布尔值场景测试
│   │   ├── 第二个返回值不是字符串场景测试
│   │   ├── 混合错误场景测试
│   │   └── 只有错误场景测试
│   ├── match_rule (vector版本) 测试（6个测试用例）
│   │   ├── 所有规则都存在且通过
│   │   ├── 部分规则不存在
│   │   ├── 所有规则都不存在
│   │   ├── 空规则列表
│   │   ├── 混合存在和不存在的规则
│   │   └── 只请求不存在的规则
│   ├── 函数注册测试（30个测试用例）
│   │   ├── 普通 C++ 函数注册测试
│   │   ├── 类成员函数注册测试
│   │   ├── 函数管理测试（注销、清空、查询）
│   │   ├── 函数在规则中使用测试
│   │   ├── 多函数协同测试
│   │   └── 无效状态下的函数操作测试（6个）
│   └── 深度限制集成测试（4个测试用例）
└── integration_test.cpp        # 集成测试（15个测试用例）
    ├── 端到端工作流测试
    ├── 多规则协同测试
    └── 深度嵌套数据安全访问测试
```

**测试统计**：
- lua_state_test: 52 个测试用例
- lua_stack_guard_test: 17 个测试用例
- data_adapter_test: 55 个测试用例（+9 深度限制测试）
- rule_engine_test: 127 个测试用例（+11 JIT 控制测试、+7 边界情况测试、+6 多规则版本测试、+30 函数注册测试）
- integration_test: 15 个测试用例（+4 深度限制集成测试）
- **总计**: 266 个测试用例，100% 通过

### 测试覆盖率目标

- **总体目标**: ≥85% 代码覆盖率
- **核心模块**: ≥90% 代码覆盖率
  - `LuaState`: 核心状态管理
  - `LuaStackGuard`: 栈安全管理
  - `JsonAdapter`: 数据转换
  - `RuleEngine`: 规则引擎核心逻辑

### 持续集成

在提交代码前，请确保：

1. **所有测试通过**
   ```bash
   cd build && ctest
   ```

2. **代码覆盖率符合要求**
   ```bash
   # 生成覆盖率报告
   lcov --summary coverage.info
   ```

3. **无内存泄漏**
   ```bash
   # 使用 valgrind 检查
   valgrind --leak-check=full ./tests/lua_state_test
   ```

4. **符合编码规范**
   - 私有成员变量使用 `_` 前缀
   - 注释使用中文
   - 不使用异常

## 运行示例

```bash
mkdir build && cd build
cmake .. -DBUILD_EXAMPLES=ON
make -j$(nproc)

# 运行示例（需要在 examples 目录下运行，因为要读取配置文件）
cd ../examples
../build/examples/example
```

## 在你的项目中使用 luajit-rule-engine

### 方法1: 源码集成（add_subdirectory）

如果你的项目还没有安装 luajit-rule-engine，可以直接将源码作为子目录集成：

```cmake
# 你的 CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(your_project)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)

# 添加 luajit-rule-engine 作为子目录
# 假设引擎源码在 third-party/luajit-rule-engine
add_subdirectory(third-party/luajit-rule-engine)

# 创建你的可执行程序
add_executable(your_app main.cpp)

# 链接 luajit-rule-engine（推荐使用命名空间版本）
target_link_libraries(your_app PRIVATE ljre::ljre)
```

### 方法2: 安装后使用（find_package）

如果你已经安装了 luajit-rule-engine 到系统：

```cmake
# 你的 CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(your_project)

# 查找已安装的 luajit-rule-engine
find_package(ljre REQUIRED)

# 创建你的可执行程序
add_executable(your_app main.cpp)

# 链接 luajit-rule-engine（统一使用命名空间版本）
target_link_libraries(your_app PRIVATE ljre::ljre)
```

### 安装到系统

#### 安装到 /usr/local（默认）

```bash
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(nproc)
sudo make install
```

#### 安装到自定义路径（推荐）

```bash
# 安装到 /usr/local/3rd/ljre-1.0.0
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DCMAKE_INSTALL_PREFIX=/usr/local/3rd/ljre-1.0.0
make -j$(nproc)
sudo make install
```

安装后的文件结构：
```
/usr/local/3rd/ljre-1.0.0/
├── include/ljre/              # 头文件
│   ├── lua_state.h
│   ├── data_adapter.h
│   ├── json_adapter.h
│   └── rule_engine.h
└── lib/
    ├── libljre.a              # 静态库
    └── cmake/ljre/            # CMake 配置文件
        ├── ljre-config.cmake
        └── ljre_targets.cmake
```

#### 使用已安装的库

安装后,在你的项目中使用需要指定 `CMAKE_PREFIX_PATH`:

```cmake
# 你的 CMakeLists.txt
cmake_minimum_required(VERSION 3.15)
project(your_project)

# 设置 ljre 的安装路径
set(CMAKE_PREFIX_PATH "/usr/local/3rd/ljre-1.0.0")

# 查找已安装的 luajit-rule-engine
find_package(ljre REQUIRED)

# 创建你的可执行程序
add_executable(your_app main.cpp)

# 链接 luajit-rule-engine（推荐使用命名空间版本）
target_link_libraries(your_app PRIVATE ljre::ljre)
```

或者在命令行中指定:

```bash
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/usr/local/3rd/ljre-1.0.0
make -j$(nproc)
```

## 使用方法

### 1. 定义规则

每条规则是一个独立的 Lua 文件，必须实现 `match` 函数：

```lua
-- age_check.lua
function match(data)
    -- 检查 age 字段是否存在
    if data.age == nil then
        return false, "缺少age字段"
    end

    -- 检查 age 是否 >= 18
    if data.age < 18 then
        return false, string.format("年龄不足: %d", data.age)
    end

    return true, "年龄检查通过"
end
```

`match` 函数的返回值：
- 第一个返回值：`boolean`，表示是否匹配成功（必需）
- 第二个返回值：`string`，错误信息或提示信息（可选）
  - 如果第二个返回值不是字符串类型（如数字、nil、省略），则 message 字段为空字符串

### 2. 创建规则配置文件

配置文件是 Lua table 格式：

```lua
-- rule_config.lua
return {
    { name = "age_check", file = "rules/age_check.lua" },
    { name = "email_validation", file = "rules/email_validation.lua" },
    { name = "user_info_complete", file = "rules/user_info_complete.lua" }
}
```

### 3. 在 C++ 中使用规则引擎

```cpp
#include "ljre/rule_engine.h"
#include "ljre/json_adapter.h"
#include <iostream>

using namespace ljre;
using json = nlohmann::json;

int main() {
    // 创建规则引擎
    RuleEngine engine;

    // 从配置文件加载规则
    std::string error_msg;
    if (!engine.load_rule_config("rule_config.lua", &error_msg)) {
        std::cerr << "加载规则配置失败: " << error_msg << std::endl;
        return 1;
    }

    // 准备测试数据
    json data = {
        {"username", "zhang_san"},
        {"email", "zhangsan@example.com"},
        {"age", 25},
        {"phone", "13800138000"}
    };

    // 创建 JSON 适配器
    JsonAdapter adapter(data);

    // 匹配单个规则
    MatchResult result;
    if (engine.match_rule("age_check", adapter, result)) {
        std::cout << "匹配结果: " << (result.matched ? "成功" : "失败")
                  << ", 信息: " << result.message << std::endl;
    }

    // 匹配所有规则（只要有一个规则通过就返回 true）
    std::map<std::string, MatchResult> results;
    if (engine.match_all_rules(adapter, results)) {
        std::cout << "至少一个规则匹配成功" << std::endl;
    } else {
        std::cout << "所有规则匹配失败" << std::endl;
        for (const auto& pair : results) {
            std::cout << "  - [" << pair.first << "] "
                      << (pair.second.matched ? "✓" : "✗") << " "
                      << pair.second.message << std::endl;
        }
    }

    // 动态添加规则
    engine.add_rule("new_rule", "path/to/new_rule.lua", &error_msg);

    // 重新加载规则（热更新）
    engine.reload_rule("age_check", &error_msg);

    // 移除规则
    engine.remove_rule("old_rule");

    return 0;
}
```

## 实现自定义适配器

如果你需要支持其他数据格式（如 Protobuf），可以实现 `DataAdapter` 接口：

```cpp
#include "ljre/data_adapter.h"

class ProtobufAdapter : public DataAdapter {
public:
    explicit ProtobufAdapter(const YourMessage& msg) : _msg(msg) {}

    bool push_to_lua(lua_State* L, std::string* error_msg) const override {
        // 创建 Lua table
        lua_createtable(L, 0, 0);

        // 将 Protobuf 消息字段转换为 Lua table
        lua_pushstring(L, _msg.field_name().c_str());
        lua_pushstring(L, _msg.field_value().c_str());
        lua_rawset(L, -3);

        // ... 转换其他字段

        return true;
    }

    const char* get_type_name() const override {
        return "Protobuf";
    }

private:
    const YourMessage& _msg;
};
```

## API 参考

### LuaState 类

Lua 状态管理类，提供 RAII 方式的生命周期管理。

#### 构造函数
```cpp
LuaState();
```

#### JIT 控制方法
```cpp
// 启用 JIT 编译
bool enable_jit();

// 禁用 JIT 编译（切换到解释模式）
bool disable_jit();

// 刷新 JIT 编译器缓存（清除已编译的代码）
bool flush_jit();
```

#### 加载 Lua 代码
```cpp
// 加载并执行 Lua 文件
bool load_file(const char* filename, std::string* error_msg = nullptr);

// 加载并执行 Lua 代码缓冲区
bool load_buffer(const char* buffer, size_t size, const char* name,
                std::string* error_msg = nullptr);
```

### LuaStackGuard 类

RAII 栈守卫，自动管理 Lua 栈平衡。

```cpp
{
    lua_State* L = lua_state.get();
    LuaStackGuard guard(L);  // 记录当前栈位置

    // ... 执行 Lua 操作，可能会修改栈 ...

    // 离开作用域时，自动恢复栈位置
}
```

### RuleEngine 类

#### 构造函数
```cpp
RuleEngine();
```

#### 加载规则配置
```cpp
bool load_rule_config(const char* config_file, std::string* error_msg = nullptr);
```

#### 添加规则
```cpp
bool add_rule(const std::string& rule_name,
              const std::string& file_path,
              std::string* error_msg = nullptr);
```

#### 移除规则
```cpp
bool remove_rule(const std::string& rule_name);
```

#### 重新加载规则
```cpp
bool reload_rule(const std::string& rule_name, std::string* error_msg = nullptr);
```

#### 匹配单个规则
```cpp
bool match_rule(const std::string& rule_name,
                const DataAdapter& data_adapter,
                MatchResult& result,
                std::string* error_msg = nullptr);
```

#### 匹配多个指定规则
```cpp
bool match_rule(const std::vector<std::string>& rule_names,
                const DataAdapter& data_adapter,
                std::map<std::string, MatchResult>& results,
                std::string* error_msg = nullptr);
```
返回的 `results` 是一个 `std::map`，键为规则名，值为匹配结果，按规则名字母顺序排序。

**返回值**：
- `true`: 至少有一个规则匹配成功
- `false`: 所有规则都匹配失败

**注意**：
- 如果 `rule_names` 中包含未添加的规则，该规则会被添加到 `results` 中，`matched = false`，`message` 包含 "not found" 信息
- 即使某个规则调用失败（如抛出异常），也会将其结果添加到 `results` 中，并设置 `matched = false` 和相应的错误信息
- 函数会继续处理其他规则，不会因为某个规则失败而提前返回

#### 匹配所有规则
```cpp
bool match_all_rules(const DataAdapter& data_adapter,
                     std::map<std::string, MatchResult>& results,
                     std::string* error_msg = nullptr);
```
返回的 `results` 是一个 `std::map`，键为规则名，值为匹配结果，按规则名字母顺序排序。

**返回值**：
- `true`: 至少有一个规则匹配成功
- `false`: 所有规则都匹配失败

**注意**：即使某个规则调用失败（如抛出异常），也会将其结果添加到 `results` 中，并设置 `matched = false` 和相应的错误信息。

#### 获取规则信息
```cpp
std::vector<RuleInfo> get_all_rules() const;
bool has_rule(const std::string& rule_name) const;
size_t get_rule_count() const;
```

#### 清空规则
```cpp
void clear_rules();
```

#### JIT 控制方法
```cpp
// 启用 JIT 编译（默认已启用）
bool enable_jit();

// 禁用 JIT 编译，切换到解释模式
bool disable_jit();

// 刷新 JIT 编译器缓存，清除已编译的代码
bool flush_jit();
```

#### 函数注册

允许将 C++ 函数注册到 Lua 的全局 `ljre` 命名空间，使得 Lua 规则文件可以调用这些函数。这对于禁用了某些 Lua 标准库（如 `os` 库）的情况下特别有用。

**注册普通 C 函数**：
```cpp
// C 函数签名必须是 int (*)(lua_State* L)，返回值为返回值个数
int get_current_time_ms(lua_State* L) {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    lua_pushnumber(L, static_cast<lua_Number>(millis));
    return 1;  // 返回 1 个值
}

RuleEngine engine;
std::string error;
engine.register_function("get_time_ms", get_current_time_ms, &error);
```

**注册类成员函数**：
```cpp
class MathHelper {
public:
    int add(lua_State* L) {
        double a = lua_tonumber(L, 1);
        double b = lua_tonumber(L, 2);
        lua_pushnumber(L, a + b);
        return 1;
    }

    // 静态分发器（必需）
    static int add_dispatcher(lua_State* L) {
        auto* self = static_cast<MathHelper*>(lua_touserdata(L, lua_upvalueindex(1)));
        return self->add(L);
    }
};

MathHelper helper;
engine.register_function("add", &MathHelper::add_dispatcher, &helper, &error);
```

在 Lua 规则中调用：
```lua
function match(data)
    -- 调用注册的 C 函数
    local time = ljre.get_time_ms()
    ljre.log("Processing at: " .. time)

    local sum = ljre.add(data.value1, data.value2)
    return sum > 100, "Sum is " .. sum
end
```

**函数管理**：
```cpp
// 检查函数是否已注册
bool has = engine.has_function("get_time_ms");

// 获取所有已注册的函数名
std::vector<std::string> funcs = engine.get_registered_functions();

// 注销单个函数
engine.unregister_function("log");

// 清空所有已注册的函数
engine.clear_registered_functions();
```

## 性能优化

### 基本优化建议

1. **启用 JIT**: LuaJIT 默认启用 JIT，会自动将热点的 Lua 代码编译为机器码，可以通过 `enable_jit()`/`disable_jit()` 动态控制
2. **减少数据转换**: 适配器实现时避免不必要的拷贝
3. **规则复用**: 规则文件只需加载一次，后续调用直接使用缓存
4. **批量匹配**: 使用 `match_all_rules` 一次性匹配所有规则
5. **栈管理**: 使用 `LuaStackGuard` 自动管理栈平衡，避免手动管理错误

### 根据规则复杂度选择实现方式

根据最新性能测试结果（2026-01-12），不同复杂度的规则有不同的最佳实践：

| 规则复杂度 | 推荐方案 | 性能比率 | 说明 |
|-----------|---------|---------|------|
| **简单规则** | Native C++ | 2.85x 慢 | 性能关键路径的首选 |
| **中等规则** | **LuaJIT** ✅ | **1.72x 快** | **优于原生性能**，支持动态更新 |
| **复杂规则** | LuaJIT | 1.11x 慢 | 接近原生性能，灵活性高 |
| **超复杂规则** | Native C++ 或拆分 | 3.51x 慢 | 性能差距较大，建议拆分或使用 Native |

### Lua 规则优化建议

如果你的规则使用 LuaJIT 实现，可以通过以下方式优化性能：

1. **使用局部变量缓存** (提升 15-20%)
   ```lua
   -- 不推荐
   if data.user.profile.education == "university" then
       base_score = base_score + 10
   end
   if data.user.profile.occupation == "engineer" then
       base_score = base_score + 10
   end

   -- 推荐
   local profile = data.user and data.user.profile
   if profile then
       if profile.education == "university" then
           base_score = base_score + 10
       end
       if profile.occupation == "engineer" then
           base_score = base_score + 10
       end
   end
   ```

2. **扁平化数据结构** (提升 30-40%)
   ```lua
   -- 不推荐: 深度嵌套
   data.user.profile.education  -- 3 层嵌套

   -- 推荐: 扁平结构
   data.user_education
   data.user_occupation
   ```

3. **使用枚举/整数代替字符串** (提升 20-30%)
   ```lua
   -- 不推荐
   if edu == "university" or edu == "master" then

   -- 推荐（在 C++ 端预处理）
   -- education: 1=university, 2=master, 3=phd
   if edu == 1 or edu == 2 then
   ```

4. **拆分复杂规则** (单规则提升 40-50%)
   - 将超复杂规则拆分为多个中等规则
   - 每个规则专注一个方面
   - 可以并行执行

## 安全性

1. **最小权限原则**: 默认只加载必要的 Lua 标准库
   - ✅ 加载：base、table、string、math
   - ❌ 不加载：io（文件操作）、os（系统操作）、debug（调试）、package（模块加载）
2. **栈安全**: 使用 RAII 栈守卫自动管理 Lua 栈，避免栈溢出和内存泄漏
3. **类型安全**: 使用引用而非指针传递结果，避免空指针异常
4. **错误处理**: 所有可能失败的操作都返回状态码和错误信息

## 注意事项

1. 规则文件必须实现 `match` 函数
2. `match` 函数的第一个返回值必须是 `boolean`
3. `match` 函数的第二个返回值应该是 `string`（可选）
   - 如果不是字符串类型（如数字、nil），message 字段将为空字符串
   - 只有当类型精确为字符串时才会被使用
4. 规则名称必须唯一，重复添加会失败
5. 规则文件路径可以是相对路径或绝对路径

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！

## 性能测试

项目提供了完整的性能测试套件，使用 Google Benchmark 框架进行 LuaJIT vs Native C++ 的性能对比测试。

### 快速开始

```bash
# 1. 编译 benchmark
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 -DBUILD_BENCHMARKS=ON
make -j$(nproc)

# 2. 运行基准测试
cd build
./benchmarks/basic_benchmark --benchmark_min_time=10

# 3. 生成测试报告
python3 ../benchmarks/generate_report.py
```

### 性能测试结果

基于最新测试数据（2026-01-12，Linux 6.8.0-90-generic, 4 x 3600 MHz CPU）：

| 规则类型 | LuaJIT 性能 | Native 性能 | 性能比率 | 推荐方案 |
|---------|------------|------------|---------|---------|
| 简单规则 + 小数据 | 1.04 μs | 0.36 μs | 2.85x | Native |
| **中等规则 + 中数据** | **4.54 μs** | **7.82 μs** | **0.58x** | **LuaJIT** ✅ |
| 复杂规则 + 大数据 | 20.57 μs | 18.48 μs | 1.11x | LuaJIT |
| 超复杂规则 + 超大数据 | 92.63 μs | 26.38 μs | 3.51x | Native |

**关键发现**：
- ✅ **中等复杂度规则优于原生性能**（快 1.72x），这是 LuaJIT 的最佳应用场景
- ✅ 复杂规则性能接近原生（仅慢 11%），灵活性优势明显
- ✅ 支持动态规则更新，无需重新编译
- ✅ JIT 编译优化效果显著（启用后性能提升 4.5%）
- ⚠️ 超复杂规则（3.51x 慢）建议使用 Native 或拆分为多个中等规则

### 运行完整测试套件

```bash
# 生成 JSON 格式结果
./benchmarks/basic_benchmark --benchmark_format=json > benchmarks/results/basic.json
./benchmarks/comparison_benchmark --benchmark_format=json > benchmarks/results/comparison.json
./benchmarks/stress_benchmark --benchmark_format=json > benchmarks/results/stress.json
./benchmarks/scaling_benchmark --benchmark_format=json > benchmarks/results/scaling.json

# 生成 HTML/Markdown/JSON 报告
python3 benchmarks/generate_report.py

# 查看报告
# HTML: benchmarks/results/benchmark_report_*.html
# Markdown: benchmarks/results/benchmark_report_*.md
# JSON: benchmarks/results/benchmark_summary_*.json
```

### 详细的性能分析

- **[UltraComplex 深度分析 (docs/ULTRA_COMPLEX_ANALYSIS.md)](docs/ULTRA_COMPLEX_ANALYSIS.md)** - 详细分析超复杂规则的性能瓶颈
  - Lua 表访问开销（35-47%）
  - 数据转换开销（12-16%）
  - 字符串比较开销（7-12%）
  - 优化建议和预期提升

详细的使用说明请参阅：
- **[性能测试指南 (benchmarks/README.md)](benchmarks/README.md)** - 完整的 benchmark 使用文档
- **[测试计划文档 (docs/BENCHMARK_PLAN.md)](docs/BENCHMARK_PLAN.md)** - 详细的测试计划和设计

## 文档

- [架构文档 (ARCHITECTURE.md)](ARCHITECTURE.md) - 详细的系统架构设计、模块关系、数据流、设计模式和最佳实践
- [变更日志 (CHANGELOG.md)](CHANGELOG.md) - 详细的版本变更记录
- [测试指南 (TESTING.md)](TESTING.md) - 详细的测试说明、覆盖率报告生成、测试最佳实践
- **[性能测试指南 (benchmarks/README.md)](benchmarks/README.md)** - Benchmark 使用说明和性能分析
- **[UltraComplex 性能分析 (docs/ULTRA_COMPLEX_ANALYSIS.md)](docs/ULTRA_COMPLEX_ANALYSIS.md)** - 超复杂规则性能瓶颈深度分析
- [覆盖率快速指南 (docs/COVERAGE_QUICKSTART.md)](docs/COVERAGE_QUICKSTART.md) - 快速查看覆盖率报告
- [Ubuntu 覆盖率指南 (docs/COVERAGE_UBUNTU.md)](docs/COVERAGE_UBUNTU.md) - Ubuntu 用户覆盖率查看详细说明
