# LuaJIT Rule Engine

基于 C++17 和 LuaJIT-2.1.0-beta3 的高性能规则引擎，支持动态更新规则而无需重新编译 C++ 代码。

## 特性

- **高性能**: 使用 LuaJIT JIT 编译器，执行效率高，支持动态开启/关闭 JIT
- **动态更新**: 支持运行时重新加载规则，无需重启程序
- **灵活适配**: 使用适配器模式，支持多种数据格式（JSON、Protobuf 等）
- **简洁易用**: 提供 C++17 友好的 API 接口
- **安全可靠**: 使用 RAII 栈守卫自动管理 Lua 栈平衡，避免内存泄漏
- **最小权限**: 默认只加载必要的 Lua 标准库（base、table、string、math），不开放 io/os/debug 等危险接口
- **零依赖（除 LuaJIT）**: 只依赖 LuaJIT 和 nlohmann/json（header-only）

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
├── examples/                      # 示例代码
│   ├── example.cpp                # 使用示例
│   ├── rule_config.lua            # 规则配置文件
│   └── rules/                     # 规则文件目录
│       ├── age_check.lua
│       ├── email_validation.lua
│       └── user_info_complete.lua
├── third-party/                   # 第三方库
│   └── json/                      # nlohmann/json
└── cmake/                         # CMake 配置
```

## 依赖

- **LuaJIT-2.1.0-beta3**: 需要安装到 `/usr/local/3rd/luajit-2.1.0-beta3/`
- **nlohmann/json**: v3.11.3，已包含在 `third-party/` 目录中
- **CMake**: >= 3.15
- **C++ 编译器**: 支持 C++17（GCC 7+, Clang 5+, MSVC 2017+）

## 编译

```bash
# 方法1: 使用构建脚本（推荐）
./build.sh

# 方法2: 手动编译
mkdir build && cd build
cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3
make -j$(nproc)
```

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
- 第一个返回值：`boolean`，表示是否匹配成功
- 第二个返回值：`string`，错误信息或提示信息

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

    // 匹配所有规则
    std::vector<MatchResult> results;
    if (engine.match_all_rules(adapter, results)) {
        std::cout << "所有规则匹配成功" << std::endl;
    } else {
        std::cout << "部分规则匹配失败" << std::endl;
        for (const auto& r : results) {
            std::cout << "  - " << (r.matched ? "✓" : "✗") << " " << r.message << std::endl;
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

// 检查 JIT 是否启用
bool is_jit_enabled() const;
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

#### 匹配所有规则
```cpp
bool match_all_rules(const DataAdapter& data_adapter,
                     std::vector<MatchResult>& results,
                     std::string* error_msg = nullptr);
```

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

## 性能优化

1. **启用 JIT**: LuaJIT 默认启用 JIT，会自动将热点的 Lua 代码编译为机器码，可以通过 `enable_jit()`/`disable_jit()` 动态控制
2. **减少数据转换**: 适配器实现时避免不必要的拷贝
3. **规则复用**: 规则文件只需加载一次，后续调用直接使用缓存
4. **批量匹配**: 使用 `match_all_rules` 一次性匹配所有规则
5. **栈管理**: 使用 `LuaStackGuard` 自动管理栈平衡，避免手动管理错误

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
3. `match` 函数的第二个返回值必须是 `string`（可选）
4. 规则名称必须唯一，重复添加会失败
5. 规则文件路径可以是相对路径或绝对路径

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！
