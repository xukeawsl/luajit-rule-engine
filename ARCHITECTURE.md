# LuaJIT Rule Engine 架构文档

## 1. 系统概述

LuaJIT Rule Engine 是一个基于 C++17 和 LuaJIT-2.1.0-beta3 的高性能规则引擎，通过将规则逻辑与业务逻辑分离，实现了规则的动态加载和热更新，无需重新编译 C++ 代码。

### 1.1 核心特性

- **高性能**：LuaJIT JIT 编译器，接近原生代码执行速度
- **动态更新**：运行时加载/重载/卸载规则
- **灵活适配**：适配器模式支持多种数据格式
- **安全可靠**：RAII 栈守卫 + Lua 沙箱环境
- **零依赖**：仅依赖 LuaJIT 和 nlohmann/json
- **多线程支持**：RuleEngineWrapper 提供线程安全的访问和热更新

### 1.2 设计目标

1. **性能优先**：利用 LuaJIT 的 JIT 编译能力实现高性能规则执行
2. **动态配置**：支持规则的热更新，无需重启服务
3. **易于集成**：提供简洁的 C++ API，支持多种数据格式
4. **安全稳定**：通过栈守卫和沙箱环境确保运行安全
5. **可测试性**：完善的单元测试和集成测试

---

## 1.3 编码规范

本项目遵循以下编码规范，以确保代码的一致性和可维护性：

- **私有成员变量**：使用 `_` 前缀（例如 `_lua_state`）
- **注释**：使用中文编写注释和文档
- **文件命名**：
  - 头文件使用 `.h` 后缀
  - 实现文件使用 `.cpp` 后缀
- **错误处理**：不使用异常，使用返回值和错误参数处理错误
- **代码风格**：
  - 使用 RAII 管理资源
  - 优先使用 `const` 和 `constexpr`
  - 使用智能指针管理动态内存

---

## 2. 系统架构

### 2.1 分层架构图

```
┌─────────────────────────────────────────────────────────────┐
│                        应用层 (Application)                    │
│                   业务逻辑代码 (用户代码)                       │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                       接口层 (Interface Layer)                 │
│  ┌──────────────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ RuleEngineWrapper    │  │ DataAdapter  │  │  LuaState    │ │
│  │ (多线程安全包装器)    │  │  (适配器模式) │  │   (RAII)     │ │
│  └──────────┬───────────┘  └──────────────┘  └──────────────┘ │
│             │                                                   │
│             │ 包含                                             │
│             ▼                                                   │
│  ┌──────────────┐                                              │
│  │  RuleEngine  │                                              │
│  │   (外观模式)  │                                              │
│  └──────────────┘                                              │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                       业务层 (Business Layer)                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │ 规则管理      │  │ 规则匹配      │  │  数据转换     │        │
│  │ (CRUD)       │  │ (Match)      │  │ (Adapter)    │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                       核心层 (Core Layer)                      │
│  ┌────────────────────────────────────────────────┐           │
│  │  LuaStackGuard (栈平衡管理 - RAII)             │           │
│  └────────────────────────────────────────────────┘           │
│  ┌────────────────────────────────────────────────┐           │
│  │  Lua State Management (Lua 状态管理)            │           │
│  └────────────────────────────────────────────────┘           │
└────────────────────────┬────────────────────────────────────┘
                         │
┌────────────────────────▼─────────────────────────────────────┐
│                      基础层 (Foundation Layer)                │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │   LuaJIT     │  │ nlohmann/    │  │  STL (C++17) │        │
│  │   2.1.0      │  │    json      │  │              │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 模块依赖关系

```
┌─────────────────────────────────────────────────────────┐
│                   RuleEngineWrapper                      │
│  ┌──────────────────────────────────────────────────┐  │
│  │  - 线程安全的引擎访问                                │  │
│  │  - 热更新支持                                       │  │
│  │  - Thread-Local Storage                           │  │
│  │  - Copy-on-Write                                  │  │
│  └──────────────────────────────────────────────────┘  │
│           │                                              │
│           │ 包含 (shared_ptr)                            │
│           ▼                                              │
│  ┌──────────────────────────────────────────────────┐  │
│  │                 RuleEngine                         │  │
│  │  ┌────────────────────────────────────────────┐  │  │
│  │  │  - 规则生命周期管理                           │  │  │
│  │  │  - 规则匹配调度                               │  │  │
│  │  │  - Lua 函数表管理                             │  │  │
│  │  └────────────────────────────────────────────┘  │  │
│  │           │                    │                    │  │
│  │           ▼                    ▼                    │  │
│  │  ┌──────────────┐      ┌──────────────┐            │  │
│  │  │   LuaState   │      │ DataAdapter  │            │  │
│  │  │  (组合关系)   │      │  (依赖关系)   │            │  │
│  │  └──────────────┘      └──────────────┘            │  │
│  │         │                                              │
│  │         ▼                                              │
│  │  ┌──────────────┐      ┌──────────────┐            │  │
│  │  │ LuaStackGuard│      │  JsonAdapter │            │  │
│  │  │  (临时引用)   │      │  (实现类)    │            │  │
│  │  └──────────────┘      └──────────────┘            │  │
│  └───────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘

图例:
── 组合关系 (Composition)
... 依赖关系 (Dependency)
~~~ 包含关系 (Containment via shared_ptr)
```

---

## 3. 核心组件设计

### 3.1 RuleEngine (规则引擎核心)

**职责**：
- 管理规则的生命周期（加载、移除、重载）
- 执行规则匹配逻辑
- 提供 C++ 与 Lua 规则之间的桥接

**类图**：

```
┌─────────────────────────────────────────────────────────┐
│                       RuleEngine                         │
├─────────────────────────────────────────────────────────┤
│ - _lua_state: LuaState                                   │
│ - _rule_functions: std::unordered_map<std::string, int>  │
├─────────────────────────────────────────────────────────┤
│ + RuleEngine()                                           │
│ + ~RuleEngine()                                          │
│ + load_rule_config(file): bool                           │
│ + add_rule(name, file): bool                             │
│ + remove_rule(name): bool                                │
│ + reload_rule(name): bool                                │
│ + match_rule(name, adapter, result): bool                │
│ + match_all_rules(adapter, results): bool                │
│ + get_all_rules(): vector<RuleInfo>                      │
│ + has_rule(name): bool                                   │
│ + get_rule_count(): size_t                               │
│ + clear_rules(): void                                    │
│ ├─────────────────────────────────────────────────────┤
│ │ C++ 函数注册功能                                        │
│ + register_function(name, func, error): bool             │
│ + register_function(name, dispatcher, instance, error): bool│
│ + unregister_function(name): bool                        │
│ + clear_registered_functions(): void                     │
│ + has_function(name): bool                               │
│ + get_registered_functions(): vector<string>             │
│ ├─────────────────────────────────────────────────────┤
│ │ Lua 公共函数加载功能                                    │
│ + add_lua_file(file_path): bool                         │
│ ├─────────────────────────────────────────────────────┤
│ │ 引擎克隆功能                                            │
│ + clone(options): unique_ptr<RuleEngine>                │
│ + clone_rules(): unique_ptr<RuleEngine>                 │
│ + clone_lua_files(): unique_ptr<RuleEngine>             │
│ + clone_cpp_functions(): unique_ptr<RuleEngine>         │
│ + clone_safe(): unique_ptr<RuleEngine>                  │
├─────────────────────────────────────────────────────────┤
│ - call_match_function(name, adapter, result): bool       │
│ - init_rule_functions_table(): void                      │
│ - ensure_ljre_table(): void                              │
│ - load_rule_file(file_path): bool                       │
└─────────────────────────────────────────────────────────┘
```

**关键设计决策**：

1. **禁止拷贝**：使用 `= delete` 禁止拷贝构造和赋值，确保唯一性
2. **错误传播**：通过可选的 `error_msg` 参数传递详细错误信息

**C++ 函数注册机制**：

```
┌─────────────────────────────────────────────────────────┐
│                 C++ 函数注册架构                          │
└─────────────────────────────────────────────────────────┘

全局 ljre 表（Lua 全局命名空间）:
  Key: "ljre"
  Value: {
    ["get_time_ms"] = <C closure>,
    ["log"] = <C closure>,
    ["add"] = <C closure with upvalue>,
    ...
  }

注册流程:
  1. ensure_ljre_table() - 确保 ljre 全局表存在
  2. lua_getglobal(L, "ljre") - 获取表到栈顶
  3. lua_pushstring(L, function_name) - 压入函数名
  4. lua_pushlightuserdata(L, instance) - 压入实例指针（仅成员函数）
  5. lua_pushcclosure(L, dispatcher, n) - 创建闭包
  6. lua_rawset(L, -3) - ljre[function_name] = closure

两种注册方式:
  1. 普通 C++ 函数:
     - 直接使用函数指针
     - 无 upvalue
     - 使用 lua_pushcclosure(L, func, 0)

  2. 类成员函数:
     - 需要静态分发器
     - 使用 lua_pushlightuserdata 传递实例指针
     - 实例指针存储在 upvalue[1]
     - 使用 lua_pushcclosure(L, dispatcher, 1)
```

**函数注册使用示例**：

```cpp
// 1. 普通 C++ 函数
int get_current_time_ms(lua_State* L) {
    auto now = std::chrono::system_clock::now();
    auto millis = /* 计算毫秒 */;
    lua_pushnumber(L, static_cast<lua_Number>(millis));
    return 1;  // 返回 1 个值
}

RuleEngine engine;
std::string error;
engine.register_function("get_time_ms", get_current_time_ms, &error);

// 2. 类成员函数
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

// 3. 在 Lua 规则中调用
-- function_test.lua
function match(data)
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

**优势**：
- **扩展性**：允许 Lua 规则调用 C++ 功能，弥补禁用危险库后的功能缺失
- **类型安全**：通过静态分发器模式，支持类成员函数注册
- **灵活管理**：支持运行时动态注册、注销、查询函数
- **命名空间隔离**：所有函数注册在全局 `ljre` 表中，避免污染全局命名空间

#### C++ 异常处理注意事项

**✅ 重要发现**：LuaJIT 可以捕获 C++ 异常，注册的 C++ 函数抛出异常**不会导致程序崩溃**！

LuaJIT（本项目使用的 LuaJIT-2.1.0-beta3）具有特殊的异常处理机制，可以捕获未捕获的 C++ 异常并将其转换为 Lua 错误。

**⚠️ 但是**：LuaJIT 捕获的 C++ 异常只返回简短的错误信息（如 `"C++ exception"`），**原始异常的详细信息会丢失**。

**两种场景对比**：

**场景 1：不使用 pcall（直接调用）**
```lua
function match(data)
    local result = ljre.dangerous_function()
    return true, "Success: " .. result
end
```

**结果**：
- ✅ 程序不会崩溃
- ✅ `match_rule()` 返回 `false`
- ✅ `result.matched = false`
- ✅ `result.message = "Failed to call match: C++ exception"`
- ✅ `error` 参数也包含错误信息

**场景 2：使用 pcall 保护**
```lua
function match(data)
    local ok, result = pcall(function()
        return ljre.dangerous_function()
    end)

    if not ok then
        return false, "Error: " .. result
    end

    return true, "Success"
end
```

**结果**：
- ✅ 程序不会崩溃
- ✅ `match_rule()` 返回 `true`
- ✅ `result.matched = false`
- ✅ `result.message = "Error: C++ exception"`

**✅ 推荐做法**：
```cpp
int safe_function(lua_State* L) {
    try {
        // 可能抛出异常的代码
        if (error_condition) {
            throw std::runtime_error("Detailed error message");
        }
        lua_pushnumber(L, 42.0);
        return 1;
    } catch (const std::exception& e) {
        // 手动将 C++ 异常转换为 Lua 错误，保留详细信息
        lua_pushstring(L, e.what());
        lua_error(L);
        return 0;
    }
}
```

**异常处理流程对比**：

| 场景 | 是否安全 | 错误信息 | 推荐度 |
|------|---------|---------|--------|
| Lua `error()` | ✅ 安全 | 完整信息 | ⭐⭐⭐⭐⭐ |
| C++ `lua_error()` | ✅ 安全 | 完整信息 | ⭐⭐⭐⭐⭐ |
| C++ 异常（未捕获） | ✅ **LuaJIT 可捕获** | ❌ 仅 `"C++ exception"` | ⭐⭐ |
| C++ 异常（手动捕获） | ✅ 安全 | 完整信息 | ⭐⭐⭐⭐⭐ |

**最佳实践**：
1. ✅ **LuaJIT 可以捕获 C++ 异常，程序不会崩溃**
2. ⚠️ **但未捕获的异常错误信息会丢失**
3. ✅ **强烈建议手动捕获 C++ 异常并使用 `lua_error()` 转换**
4. ✅ **手动转换可以保留完整的错误信息，便于调试**
5. ✅ **无论是否手动捕获，都建议在 Lua 端使用 `pcall()` 保护**

### 3.2 引擎克隆功能

**职责**：
- 创建规则引擎的克隆副本
- 支持选择性克隆规则、Lua 文件、C++ 函数
- 保持克隆引擎与原引擎的独立性

**克隆选项**：

```cpp
enum CloneOption : uint32_t {
    NONE = 0,                      // 不克隆任何内容
    LUA_FILES = 1 << 0,            // 只克隆 Lua 公共函数文件
    CPP_FUNCTIONS = 1 << 1,        // 只克隆普通 C++ 函数
    CPP_MEMBER_FUNCTIONS = 1 << 2, // 只克隆类成员函数
    RULES = 1 << 3,                // 只克隆规则
    ALL = LUA_FILES | CPP_FUNCTIONS | CPP_MEMBER_FUNCTIONS | RULES
};
```

**克隆流程**：

```
┌─────────────────────────────────────────────────────────┐
│                    引擎克隆流程                            │
└─────────────────────────────────────────────────────────┘

源引擎                     目标引擎（新建）
   │                            │
   │ 1. 创建新的 RuleEngine       │
   │    └── 创建新的 LuaState     │
   │                            │
   │ 2. 克隆规则（如果选项包含）  │
   │    ├── 遍历源引擎的规则      │
   │    ├── 读取规则文件路径      │
   │    └── 在目标引擎中加载      │
   │                            │
   │ 3. 克隆 Lua 文件（如果包含） │
   │    ├── 遍历已加载的 Lua 文件 │
   │    ├── 读取文件路径          │
   │    └── 在目标引擎中加载      │
   │                            │
   │ 4. 克隆 C++ 函数（如果包含） │
   │    ├── 遍历已注册的函数      │
   │    ├── 普通函数：重新注册    │
   │    └── 成员函数：使用相同实例 │
   │                            │
   ▼                            ▼
返回 unique_ptr<RuleEngine>
```

**克隆实现细节**：

```cpp
// 克隆规则
if (options & CloneOption::RULES) {
    for (const auto& [name, info] : source._metadata.rule_files) {
        // 使用原规则文件路径重新加载
        target.add_rule(name, info, &error);
    }
}

// 克隆 Lua 公共函数
if (options & CloneOption::LUA_FILES) {
    for (const auto& path : source._metadata.loaded_lua_files) {
        // 重新加载 Lua 文件
        target.add_lua_file(path, &error);
    }
}

// 克隆 C++ 函数
if (options & (CloneOption::CPP_FUNCTIONS | CloneOption::CPP_MEMBER_FUNCTIONS)) {
    for (const auto& [name, info] : source._metadata.registered_functions) {
        if (info.type == RegisteredFunctionInfo::NORMAL) {
            if (options & CloneOption::CPP_FUNCTIONS) {
                // 重新注册普通函数
                target.register_function(name, info.function_ptr, &error);
            }
        } else if (info.type == RegisteredFunctionInfo::MEMBER) {
            if (options & CloneOption::CPP_MEMBER_FUNCTIONS) {
                // 重新注册成员函数（使用相同实例指针）
                target.register_function(name, info.function_ptr,
                                        info.instance_ptr, &error);
            }
        }
    }
}
```

**独立性保证**：

```
源引擎                    克隆引擎
┌─────────────────┐     ┌─────────────────┐
│ LuaState #1     │     │ LuaState #2     │
│ ├─ Registry     │     │ ├─ Registry     │
│ ├─ Rules        │     │ ├─ Rules        │
│ └─ Functions    │     │ └─ Functions    │
└─────────────────┘     └─────────────────┘
         │                       │
         └─────── 独立 ───────────┘

- 各自独立的 Lua 状态
- 修改克隆不影响原引擎
- 各自可以独立加载/卸载规则
- C++ 成员函数共享实例指针
```

**便捷方法**：

```cpp
// 只克隆规则
auto rules_clone = engine.clone_rules();

// 只克隆 Lua 文件
auto lua_clone = engine.clone_lua_files();

// 只克隆 C++ 函数（普通函数 + 成员函数）
auto funcs_clone = engine.clone_cpp_functions();

// 克隆所有内容（安全方式）
auto full_clone = engine.clone_safe();
```

**使用场景**：

1. **多线程环境**
   ```cpp
   // 每个线程一个克隆引擎
   std::vector<std::thread> threads;
   for (int i = 0; i < 4; ++i) {
       threads.emplace_back([engine_ptr = engine.clone_safe()]() {
           // 每个线程独立的引擎实例
           process_data(*engine_ptr);
       });
   }
   ```

2. **批量处理隔离**
   ```cpp
   // 为每个批次创建独立引擎
   for (const auto& batch : batches) {
       auto batch_engine = engine.clone_rules();
       process_batch(batch_engine, batch);
   }
   ```

3. **测试和调试**
   ```cpp
   // 克隆引擎用于测试，不影响生产引擎
   auto test_engine = engine.clone_safe();
   run_tests(test_engine);
   ```

**优势**：
- **独立性**：克隆引擎完全独立，互不影响
- **灵活性**：支持选择性克隆，按需复制
- **安全性**：原引擎不受克隆影响
- **多线程友好**：每个线程可有自己的克隆
- **快速部署**：克隆比手动配置更快

### 3.3 RuleEngineWrapper (多线程安全包装器)

**职责**：
- 提供线程安全的规则引擎访问
- 支持运行时热更新
- 自动管理线程本地引擎副本
- 使用 shared_ptr 确保旧引擎仍然有效

**设计模式**：
- **Copy-on-Write**: 模板引擎使用 shared_ptr + 原子操作
- **Thread-Local Storage**: 每个线程维护独立的引擎副本
- **Versioning**: 通过版本号判断是否需要重新克隆
- **Shared Ownership**: 使用 shared_ptr 确保旧引擎在被使用时不会被销毁

**类图**：

```
┌─────────────────────────────────────────────────────────┐
│                 RuleEngineWrapper                        │
├─────────────────────────────────────────────────────────┤
│ - _template_engine: shared_ptr<RuleEngine>              │
│ - _global_version: atomic<uint64_t>                      │
├─────────────────────────────────────────────────────────┤
│ + RuleEngineWrapper()                                    │
│ + ~RuleEngineWrapper()                                   │
│ + set_template_engine(engine): void                      │
│ + get_engine(): shared_ptr<const RuleEngine>             │
│ + get_version(): uint64_t                                │
├─────────────────────────────────────────────────────────┤
│ - ThreadLocalEngine: struct                              │
│   - engine: shared_ptr<RuleEngine>                       │
│   - version: uint64_t                                    │
│ - get_thread_local(): ThreadLocalEngine*                 │
└─────────────────────────────────────────────────────────┘

禁止拷贝和移动:
- RuleEngineWrapper(const RuleEngineWrapper&) = delete
- RuleEngineWrapper& operator=(const RuleEngineWrapper&) = delete
- RuleEngineWrapper(RuleEngineWrapper&&) = delete;
- RuleEngineWrapper& operator=(RuleEngineWrapper&&) = delete;
```

**线程本地存储结构**：

```cpp
struct ThreadLocalEngine {
    std::shared_ptr<RuleEngine> engine;  // 线程本地引擎实例
    uint64_t version = 0;                 // 当前引擎的版本号
};

static thread_local ThreadLocalEngine local;
```

**工作流程**：

```
┌─────────────────────────────────────────────────────────┐
│                get_engine() 工作流程                      │
└─────────────────────────────────────────────────────────┘

1. 获取线程本地存储
   ThreadLocalEngine* local = get_thread_local();

2. 检查版本是否过期
   if (local->version != current_global_version) {

3. 原子加载模板引擎
     template_engine = atomic_load(&_template_engine);

4. 克隆模板引擎到线程本地
     local->engine = template_engine->clone(ALL, &error);
     local->version = current_global_version;
   }

5. 返回 shared_ptr 的拷贝（引用计数+1）
   return local->engine;
```

**热更新流程**：

```
┌─────────────────────────────────────────────────────────┐
│                热更新时序图                               │
└─────────────────────────────────────────────────────────┘

热加载线程              RuleEngineWrapper        工作线程
    │                         │                     │
    │ 创建新引擎               │                     │
    │─────────────────────────>│                     │
    │                         │                     │
    │ set_template_engine     │                     │
    │─────────────────────────>│                     │
    │                         │ atomic_store        │
    │                         │ 版本号 +1            │
    │                         │<────────────────────│
    │                         │ get_engine()        │
    │                         │ 检查版本过期         │
    │                         │────────────────────>│
    │                         │                     │ 克隆新引擎
    │                         │<────────────────────│
    │                         │ 返回新版本引擎       │
    │<─────────────────────────┘
    │                         │

关键点:
1. 热更新不影响正在使用的请求
2. 旧引擎的 shared_ptr 仍然有效
3. 各线程下次调用 get_engine() 时自动克隆新版本
```

**shared_ptr 安全性保证**：

```
场景 1: 热更新前的引擎
  auto engine1 = wrapper.get_engine();  // 版本 1
  // ... 热更新发生 ...
  // engine1 仍然有效，因为持有 shared_ptr

场景 2: 热更新后的引擎
  auto engine2 = wrapper.get_engine();  // 版本 2
  // engine2 是新的引擎实例

场景 3: 同一线程多次调用
  auto e1 = wrapper.get_engine();
  auto e2 = wrapper.get_engine();
  // e1 和 e2 指向同一个对象（相同 shared_ptr）

内存管理:
- 旧引擎在所有 shared_ptr 引用释放后自动销毁
- 无需手动管理生命周期
```

**性能特性**：

```
第一次调用 get_engine():
  - 检查版本、克隆模板引擎
  - 开销较大（取决于引擎复杂度）
  - 后续调用直接返回 shared_ptr 拷贝

后续调用 get_engine():
  - 只需返回 shared_ptr 拷贝
  - 开销极小（< 1 微秒）
  - 无锁、无克隆

热更新后的第一次调用:
  - 检测到版本过期
  - 重新克隆新版本
  - 后续调用又回到快速路径
```

**使用示例**：

```cpp
// 全局包装器
RuleEngineWrapper g_wrapper;

// 初始化线程（只执行一次）
void init() {
    auto engine = std::make_shared<RuleEngine>();
    engine->load_rule_config("config.lua");
    g_wrapper.set_template_engine(engine);
}

// 热加载线程
void reload_thread() {
    while (running) {
        wait_for_change();

        auto new_engine = std::make_shared<RuleEngine>();
        new_engine->load_rule_config("new_config.lua");

        // 原子替换，无阻塞
        g_wrapper.set_template_engine(new_engine);
    }
}

// brpc 处理线程
void process_request(const Request& req) {
    // 获取引擎（自动处理版本更新）
    auto engine = g_wrapper.get_engine();

    // 使用引擎
    auto adapter = std::make_shared<JsonAdapter>(req.json());
    MatchResult result;
    engine->match_rule("rule1", adapter, result);
}
```

**Const 正确性**：

```cpp
// 返回 shared_ptr<const RuleEngine>
// 只能调用 const 方法：
auto engine = wrapper.get_engine();

// ✅ 允许：
engine->match_rule(...);
engine->match_all_rules(...);
engine->has_rule(...);
engine->get_all_rules();
engine->get_rule_count();

// ❌ 禁止：
engine->add_rule(...);           // 编译错误
engine->remove_rule(...);        // 编译错误
engine->load_rule_config(...);   // 编译错误
engine->register_function(...);  // 编译错误
engine->clear_rules();           // 编译错误
```

**线程安全性**：

```
set_template_engine():
  - 使用 atomic_store，原子操作
  - 无阻塞，不影响其他线程
  - 版本号自动递增

get_engine():
  - 无锁设计（只读操作使用 atomic_load）
  - 每个线程独立存储
  - 热更新不影响正在使用的引擎

内存安全:
  - shared_ptr 保证引用计数正确
  - 旧引擎在所有引用释放后自动销毁
  - 不会出现悬空指针
```

**优势**：
- **线程安全**：无锁设计，高并发性能好
- **热更新友好**：不影响正在处理的请求
- **自动管理**：无需手动处理版本更新
- **类型安全**：const 正确性，防止误操作
- **高性能**：后续调用开销极小（< 1 μs）

**注意事项**：
- 首次调用 `get_engine()` 会克隆模板引擎（开销较大）
- 不推荐缓存 `shared_ptr`，应该在每次请求时获取最新版本
- 热更新后，各线程会在下次调用时自动克隆新版本
- RuleEngineWrapper 本身不应该被拷贝或移动

### 3.4 LuaState (Lua 状态管理)

**职责**：
- 管理 Lua 状态的生命周期（RAII）
- 加载和执行 Lua 文件/缓冲区
- 控制 JIT 编译器的开启/关闭

**类图**：

```
┌─────────────────────────────────────────────────────────┐
│                       LuaState                           │
├─────────────────────────────────────────────────────────┤
│ - _L: lua_State*                                         │
├─────────────────────────────────────────────────────────┤
│ + LuaState()                                             │
│ + LuaState(LuaState&&) noexcept                          │
│ + operator=(LuaState&&) noexcept                         │
│ + ~LuaState()                                            │
│ + get(): lua_State*                                      │
│ + load_file(file): bool                                  │
│ + load_buffer(buffer, size, name): bool                  │
│ + enable_jit(): bool                                     │
│ + disable_jit(): bool                                    │
│ + flush_jit(): bool                                      │
├─────────────────────────────────────────────────────────┤
│ - create_lua_state(): void                               │
│ - open_libraries(): void                                 │
└─────────────────────────────────────────────────────────┘

移动语义支持:
- 支持移动构造和移动赋值
- 禁止拷贝构造和拷贝赋值
```

**安全沙箱设计**：

```cpp
// 仅加载安全的 Lua 标准库
void LuaState::open_libraries() {
    luaopen_base(_L);    // ✅ 基础函数库
    luaopen_table(_L);   // ✅ 表操作库
    luaopen_string(_L);  // ✅ 字符串处理库
    luaopen_math(_L);    // ✅ 数学函数库
    luaopen_jit(_L);     // ✅ JIT 控制库

    // ❌ 不加载危险库:
    // - luaopen_io()      // 文件操作
    // - luaopen_os()      // 系统操作
    // - luaopen_debug()   // 调试功能
    // - luaopen_package() // 模块加载
}
```

**JIT 控制机制**：

```
┌─────────────────────────────────────────────────────────┐
│                      JIT 控制流程                          │
└─────────────────────────────────────────────────────────┘

启用 JIT:
  luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_ON)
  → LuaJIT 编译器启动，热点代码被编译为机器码

禁用 JIT:
  luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_OFF)
  → 切换到解释模式，所有代码解释执行

刷新 JIT:
  luaJIT_setmode(L, 0, LUAJIT_MODE_ENGINE | LUAJIT_MODE_FLUSH)
  → 清除 JIT 缓存，重新编译代码
```

### 3.5 LuaStackGuard (栈守卫)

**职责**：
- 自动管理 Lua 栈的平衡
- 防止栈泄漏和内存错误
- 支持嵌套使用

**类图**：

```
┌─────────────────────────────────────────────────────────┐
│                    LuaStackGuard                         │
├─────────────────────────────────────────────────────────┤
│ - _L: lua_State*                                         │
│ - _top: int                                              │
├─────────────────────────────────────────────────────────┤
│ + LuaStackGuard(L)                                       │
│ + ~LuaStackGuard()                                       │
│ + release(): void                                        │
└─────────────────────────────────────────────────────────┘

工作原理:
  构造时: _top = lua_gettop(_L)  // 记录当前栈顶
  析构时: lua_settop(_L, _top)   // 恢复到原始栈顶
  release(): _L = nullptr        // 手动释放保护
```

**使用场景**：

```cpp
bool some_function(lua_State* L) {
    LuaStackGuard guard(L);  // 记录当前栈位置

    lua_pushstring(L, "hello");
    lua_pushnumber(L, 42);
    // ... 执行各种 Lua 操作 ...

    // 函数返回时，自动恢复栈位置，防止泄漏
    return true;
}
```

**嵌套场景**：

```
栈帧:  [bottom] ......................... [top]

初始状态:  [ ]                          (栈顶 = 0)

guard1:   [ ] ←── guard1._top = 0
          push: [ "hello" ]              (栈顶 = 1)

guard2:        [ "hello" ] ←── guard2._top = 1
               push: [ "hello", 42 ]     (栈顶 = 2)

guard2 析构:    [ "hello" ]               (恢复到 guard2._top = 1)

guard1 析构:  [ ]                        (恢复到 guard1._top = 0)

最终结果: 栈恢复平衡
```

### 3.6 DataAdapter (数据适配器接口)

**职责**：
- 提供不同数据类型的统一接口
- 将 C++ 数据类型转换为 Lua table
- 支持扩展性和多态

**类图**：

```
┌─────────────────────────────────────────────────────────┐
│                    DataAdapter                          │
│                   (抽象接口)                             │
├─────────────────────────────────────────────────────────┤
│ + push_to_lua(L): bool {pure virtual}                   │
│ + get_type_name(): const char* {pure virtual}           │
│ + ~DataAdapter() {virtual}                              │
└─────────────────────────────────────────────────────────┘
                          △
                          │ 继承
                          │
┌─────────────────────────┴───────────────────────────────┐
│                    JsonAdapter                          │
│                  (具体实现类)                            │
├─────────────────────────────────────────────────────────┤
│ - _json: nlohmann::json                                 │
├─────────────────────────────────────────────────────────┤
│ + JsonAdapter(const json&)                              │
│ + push_to_lua(L): bool override                         │
│ + get_type_name(): const char* override                 │
├─────────────────────────────────────────────────────────┤
│ - push_json_object(L, const json&): bool               │
│ - push_json_array(L, const json&): bool                │
└─────────────────────────────────────────────────────────┘
```

**适配器模式**：

```
C++ 数据类型 ──── DataAdapter ──── Lua Table
                   (接口)              (目标)
                        │
                        ├── JsonAdapter ──── nlohmann::json
                        ├── ProtobufAdapter ──── protobuf::Message
                        ├── XMLAdapter ──── tinyxml2::XMLDocument
                        └── ... (用户自定义)
```

### 3.7 JsonAdapter (JSON 适配器实现)

**类型转换映射**：

```
┌────────────────┬──────────────────────────────────────┐
│   JSON 类型     │           Lua 类型                    │
├────────────────┼──────────────────────────────────────┤
│ null           │ nil                                   │
│ boolean        │ boolean                               │
│ number         │ number                                │
│ string         │ string (支持含\0字符串)                │
│ array          │ table (1-based 索引)                  │
│ object         │ table (字符串键)                       │
└────────────────┴──────────────────────────────────────┘
```

**转换示例**：

```json
{
  "name": "Alice",
  "age": 25,
  "scores": [95, 87, 92],
  "address": {
    "city": "Beijing",
    "zip": "100000"
  },
  "active": true,
  "notes": null
}
```

↓ 转换为 Lua table ↓

```lua
{
  name = "Alice",
  age = 25,
  scores = {95, 87, 92},
  address = {
    city = "Beijing",
    zip = "100000"
  },
  active = true,
  notes = nil
}
```

---

## 4. 数据流和交互流程

### 4.1 规则加载流程

```
┌─────────────────────────────────────────────────────────┐
│                   规则加载时序图                           │
└─────────────────────────────────────────────────────────┘

User              RuleEngine         LuaState          Lua VM
 │                    │                  │                │
 │ load_rule_config  │                  │                │
 │─────────────────> │                  │                │
 │                    │ load_file       │                │
 │                    │ (config.lua)    │                │
 │                    │────────────────>│                │
 │                    │                  │ luaL_loadfile │
 │                    │                  │──────────────>│
 │                    │                  │ lua_pcall     │
 │                    │                  │──────────────>│
 │                    │                  │ 返回配置表     │
 │                    │<────────────────│                │
 │                    │                  │                │
 │                    │ 解析配置表       │                │
 │                    │                  │                │
 │                    │ add_rule(name, file) for each    │
 │                    │────────────────>│                │
 │                    │                  │ load_file     │
 │                    │                  │ (rule.lua)    │
 │                    │                  │──────────────>│
 │                    │                  │ lua_pcall     │
 │                    │                  │──────────────>│
 │                    │                  │ 获取 match 函数 │
 │                    │                  │ 存入 Registry │
 │                    │<────────────────│                │
 │                    │                  │                │
 │<───────────────────│                  │                │
 │                    │                  │                │

说明:
1. 加载配置文件并解析规则列表
2. 逐个加载规则文件
3. 每个规则文件的 match 函数存储在 Lua Registry 中
4. 使用规则名作为键，函数引用作为值
```

### 4.2 规则匹配流程

```
┌─────────────────────────────────────────────────────────┐
│                   规则匹配时序图                           │
└─────────────────────────────────────────────────────────┘

User              RuleEngine       DataAdapter      LuaState
 │                    │                  │               │
 │ match_rule        │                  │               │
 │─────────────────> │                  │               │
 │                    │ 检查规则存在     │               │
 │                    │                  │               │
 │                    │ push_to_lua      │               │
 │                    │─────────────────>│               │
 │                    │                  │ 将 C++ 数据   │
 │                    │                  │ 转为 Lua table│
 │                    │<─────────────────│               │
 │                    │                  │               │
 │                    │ 从 Registry      │               │
 │                    │ 获取 match 函数   │               │
 │                    │                  │               │
 │                    │ lua_pcall        │               │
 │                    │ (调用 match 函数) │               │
 │                    │─────────────────────────────────>│
 │                    │                  │               │
 │                    │ 返回结果         │               │
 │                    │ (matched, msg)   │               │
 │                    │                  │               │
 │<───────────────────│                  │               │
 │                    │                  │               │

说明:
1. 验证规则是否存在
2. 适配器将 C++ 数据转换为 Lua table
3. 从 Lua Registry 获取 match 函数
4. 调用 Lua 函数，传入数据 table
5. 获取返回值：boolean（必需）+ string（可选）
   - 第二个返回值必须是 string 类型才会被使用
   - 如果是数字、nil 或其他类型，message 字段将为空字符串
```

### 4.3 内存管理流程

```
┌─────────────────────────────────────────────────────────┐
│                    内存管理生命周期                        │
└─────────────────────────────────────────────────────────┘

1. RuleEngine 构造
   │
   ├── 创建 LuaState 实例
   │   │
   │   ├── lua_newstate()    → 分配 Lua 内存
   │   ├── open_libraries()  → 加载标准库
   │   └── init_rule_functions_table() → 初始化函数表
   │
   └── 持有 LuaState (组合关系)

2. 规则匹配期间
   │
   ├── LuaStackGuard 自动管理栈平衡
   │   ├── 构造时记录栈位置
   │   └── 析构时恢复栈位置
   │
   └── Lua 函数从 Registry 获取
       ├── 获取函数引用
       ├── lua_pcall() 调用
       └── 自动清理栈

3. RuleEngine 析构
   │
   ├── 清空规则表
   │   └── 释放 Lua Registry 中的函数引用
   │
   ├── LuaState 析构
   │   │
   │   ├── lua_close()  → 释放所有 Lua 内存
   │   └── _L = nullptr
   │
   └── 内存完全释放

内存安全保证:
- RAII 自动管理生命周期
- 栈守卫防止栈泄漏
- Registry 管理函数引用，避免 GC 问题
```

---

## 5. 关键数据结构

### 5.1 MatchResult (匹配结果)

```cpp
struct MatchResult {
    bool matched;           // 匹配成功标志
    std::string message;    // 错误或提示信息

    // 默认构造
    MatchResult() : matched(false), message("") {}

    // 便捷构造
    MatchResult(bool m, const std::string& msg)
        : matched(m), message(msg) {}
};
```

### 5.2 RuleInfo (规则信息)

```cpp
struct RuleInfo {
    std::string name;       // 规则名称
    std::string file_path;  // 规则文件路径

    // 默认构造
    RuleInfo() = default;

    // 便捷构造
    RuleInfo(const std::string& n, const std::string& p)
        : name(n), file_path(p) {}
};
```

### 5.3 规则函数表 (内部数据结构)

```cpp
// 存储在 Lua Registry 中
// Key: "ljre_rule_functions"
// Value: {
//   ["rule_name_1"] = <Lua function reference>,
//   ["rule_name_2"] = <Lua function reference>,
//   ...
// }

// C++ 中通过整数引用访问
std::unordered_map<std::string, int> _rule_functions;
// Key: 规则名称
// Value: Lua Registry 中的函数引用
```

---

## 6. 设计模式应用

### 6.1 适配器模式 (Adapter Pattern)

**目的**：将不同数据格式统一转换为 Lua table

```
┌─────────────────────────────────────────────────────────┐
│                    适配器模式结构                          │
└─────────────────────────────────────────────────────────┘

    Target Interface           Client
    (DataAdapter)           (RuleEngine)
         △                         │
         │ implements              │ uses
         │                         │
    ┌────┴────────────┐
    │                 │
Concrete Adapter1  Concrete Adapter2
(JsonAdapter)     (ProtobufAdapter)
    │                 │
    │ adapts          │ adapts
    ▼                 ▼
Adaptee1          Adaptee2
(nlohmann::json)  (protobuf::Message)
```

**优势**：
- 符合开闭原则：对扩展开放，对修改关闭
- 符合单一职责：每个适配器只负责一种数据类型
- 提高灵活性：用户可实现自定义适配器

### 6.2 RAII 模式 (Resource Acquisition Is Initialization)

**目的**：自动管理资源生命周期

```
┌─────────────────────────────────────────────────────────┐
│                     RAII 模式                            │
└─────────────────────────────────────────────────────────┘

资源获取即初始化:
  LuaState()
    ├── create_lua_state()    → 获取资源
    ├── open_libraries()      → 初始化
    └── _L 有效

使用资源:
  调用 load_file(), load_buffer() 等

资源释放即析构:
  ~LuaState()
    ├── lua_close(_L)         → 释放资源
    └── _L = nullptr          → 避免悬空指针

栈守卫 RAII:
  LuaStackGuard(L)
    ├── _top = lua_gettop(L)  → 记录状态

  ~LuaStackGuard()
    └── lua_settop(L, _top)   → 恢复状态
```

**优势**：
- 异常安全：即使发生异常也能正确释放资源
- 防止泄漏：自动管理，无需手动释放
- 代码简洁：减少手动资源管理代码

### 6.3 外观模式 (Facade Pattern)

**目的**：简化复杂子系统的接口

```
┌─────────────────────────────────────────────────────────┐
│                    外观模式                              │
└─────────────────────────────────────────────────────────┘

Client Code
   │
   │ 使用
   ▼
┌──────────────────────────────────┐
│      RuleEngine (Facade)         │
│  ┌────────────────────────────┐  │
│  │ 简化的接口:                 │  │
│  │ - load_rule_config()       │  │
│  │ - match_rule()             │  │
│  │ - match_all_rules()        │  │
│  └────────────────────────────┘  │
│            │ delegates           │
│            ▼                     │
│  ┌────────────────────────────┐  │
│  │  复杂的子系统:              │  │
│  │  - LuaState 管理           │  │
│  │  - Lua 栈操作              │  │
│  │  - 数据转换                │  │
│  │  - 函数调用                │  │
│  └────────────────────────────┘  │
└──────────────────────────────────┘
```

**优势**：
- 简化接口：隐藏 Lua API 的复杂性
- 降低耦合：客户端不直接依赖 LuaJIT
- 易于使用：提供高层次的业务接口

### 6.4 策略模式 (Strategy Pattern)

**目的**：规则逻辑可动态替换

```
┌─────────────────────────────────────────────────────────┐
│                    策略模式                              │
└─────────────────────────────────────────────────────────┘

Context (RuleEngine)
   │
   │ 使用
   ▼
Strategy Interface (match 函数)
   △
   │
   │ 具体策略
   ├── age_check.lua       → 年龄检查策略
   ├── email_validation.lua → 邮箱验证策略
   └── user_info_complete.lua → 用户信息完整性策略

运行时动态选择:
  engine.match_rule("age_check", adapter, result)
  engine.match_rule("email_validation", adapter, result)
```

**优势**：
- 算法可变：运行时切换规则
- 独立演化：规则修改不影响引擎代码
- 易于测试：每个规则可独立测试

---

## 7. 错误处理机制

### 7.1 错误传播策略

```cpp
// 统一的错误处理模式
bool function(..., std::string* error_msg = nullptr) {
    if (error_condition) {
        if (error_msg) {
            *error_msg = "详细的错误信息";
        }
        return false;
    }
    return true;
}
```

### 7.2 Lua 错误处理

```
┌─────────────────────────────────────────────────────────┐
│                  Lua 错误处理流程                         │
└─────────────────────────────────────────────────────────┘

Lua 代码执行
   │
   ├── lua_pcall()
   │   │
   │   ├── 成功: 返回 0
   │   │
   │   └── 失败: 返回 ≠ 0
   │       │
   │       ├── 栈顶: 错误对象 (通常是字符串)
   │       │
   │       ├── lua_tostring() 提取错误信息
   │       │
   │       └── 传播给 C++ error_msg
   │
   └── 返回 bool + 可选的错误信息
```

### 7.3 错误处理示例

```cpp
// 文件加载错误
bool load_file(const char* filename, std::string* error_msg) {
    if (luaL_loadfile(_L, filename) != LUA_OK) {
        if (error_msg) {
            *error_msg = lua_tostring(_L, -1);  // 获取 Lua 错误信息
        }
        lua_pop(_L, 1);  // 弹出错误对象
        return false;
    }
    // ... 执行代码 ...
}

// 函数调用错误
bool call_match_function(...) {
    if (lua_pcall(_L, 1, 2, 0) != LUA_OK) {
        if (error_msg) {
            *error_msg = lua_tostring(_L, -1);
        }
        lua_pop(_L, 1);
        return false;
    }
    // ... 处理返回值 ...
}
```

---

## 8. 性能优化策略

### 8.1 JIT 编译优化

```
┌─────────────────────────────────────────────────────────┐
│                  JIT 性能优化                             │
└─────────────────────────────────────────────────────────┘

1. 启用 JIT (默认)
   engine.get_lua_state()->enable_jit()
   → 热点代码编译为机器码
   → 性能提升: 5-50x (取决于代码特性)

2. 禁用 JIT (特殊场景)
   engine.get_lua_state()->disable_jit()
   → 适合调试或不确定的代码
   → 避免编译开销

3. 刷新 JIT 缓存
   engine.get_lua_state()->flush_jit()
   → 规则热更新后清除旧编译代码
   → 重新编译最新的规则代码
```

### 8.2 内存优化

```
1. 零拷贝设计
   - 使用移动语义避免不必要的拷贝
   - LuaState 支持移动构造和移动赋值
   - DataAdapter 使用引用传递

2. 对象复用
   - 规则函数缓存在 Lua Registry
   - 避免重复加载规则文件
   - LuaState 长生命周期，避免频繁创建

3. 栈管理
   - LuaStackGuard 自动管理栈平衡
   - 避免栈泄漏导致的内存泄漏
```

### 8.3 批量操作优化

```cpp
// 批量匹配所有规则 (比逐个匹配更高效)
std::map<std::string, MatchResult> results;
engine.match_all_rules(adapter, results);

// 优势:
// 1. 一次性传递数据到 Lua
// 2. 减少数据转换次数
// 3. 减少 Lua C API 调用开销
```

---

## 9. 安全性设计

### 9.1 Lua 沙箱环境

```
┌─────────────────────────────────────────────────────────┐
│                    安全沙箱设计                           │
└─────────────────────────────────────────────────────────┘

允许的库:
  ✅ base     - 基础函数 (print, type, tonumber 等)
  ✅ table    - 表操作 (table.insert, table.remove 等)
  ✅ string   - 字符串处理 (string.sub, string.find 等)
  ✅ math     - 数学函数 (math.sin, math.cos 等)
  ✅ jit      - JIT 控制 (jit.on, jit.off 等)

禁止的库:
  ❌ io       - 文件操作 (防止读写系统文件)
  ❌ os       - 系统操作 (防止执行系统命令)
  ❌ debug    - 调试功能 (防止访问内部状态)
  ❌ package  - 模块加载 (防止加载外部模块)
```

### 9.2 栈安全

```
1. 自动栈平衡
   - LuaStackGuard 自动恢复栈状态
   - 防止栈溢出和泄漏

2. 栈深度检查
   - 每次调用前检查栈空间
   - 确保有足够的空间存放返回值

3. 错误恢复
   - 即使发生错误，栈守卫也能恢复栈状态
   - 避免错误传播导致的栈破坏
```

### 9.3 类型安全

```cpp
// 使用引用而非指针，避免空指针异常
bool match_rule(const std::string& rule_name,
                const DataAdapter& data_adapter,
                MatchResult& result,  // 引用，必须提供
                std::string* error_msg = nullptr);  // 可选
```

---

## 10. 扩展性设计

### 10.1 自定义数据适配器

```cpp
// 实现自定义适配器
class ProtobufAdapter : public DataAdapter {
public:
    explicit ProtobufAdapter(const Message& msg) : _msg(msg) {}

    bool push_to_lua(lua_State* L, std::string* error_msg) const override {
        lua_createtable(L, 0, _msg.GetDescriptor()->field_count());

        // 遍历 Protobuf 字段
        const Reflection* reflection = _msg.GetReflection();
        for (int i = 0; i < _msg.GetDescriptor()->field_count(); ++i) {
            const FieldDescriptor* field = _msg.GetDescriptor()->field(i);
            if (reflection->HasField(_msg, field)) {
                // 转换字段值到 Lua
                // ...
            }
        }

        return true;
    }

    const char* get_type_name() const override {
        return "Protobuf";
    }

private:
    const Message& _msg;
};

// 使用自定义适配器
ProtobufAdapter adapter(protobuf_msg);
engine.match_rule("some_rule", adapter, result);
```

### 10.2 规则扩展

```lua
-- 支持复杂的规则逻辑
function match(data)
    -- 可以使用所有安全的 Lua 库
    local score = 0

    -- 复杂条件判断
    if data.age and data.age >= 18 then
        score = score + 10
    end

    if data.email and string.match(data.email, ".*@example%.com") then
        score = score + 20
    end

    -- 返回匹配结果和详细信息
    if score >= 30 then
        return true, string.format("评分通过: %d", score)
    else
        return false, string.format("评分不足: %d (需要 ≥30)", score)
    end
end
```

---

## 11. 构建和部署

### 11.1 构建架构

```
┌─────────────────────────────────────────────────────────┐
│                     构建流程                              │
└─────────────────────────────────────────────────────────┘

源代码
   │
   ├── CMake 配置
   │   ├── 设置 C++17 标准
   │   ├── 配置 LuaJIT 路径
   │   └── 配置 nlohmann/json 路径
   │
   ├── 编译
   │   ├── 生成静态库 (libljre.a)
   │   ├── 编译示例程序
   │   └── 编译测试程序
   │
   ├── 链接
   │   ├── 链接 LuaJIT 库
   │   └── 链接 nlohmann/json (header-only)
   │
   └── 安装
       ├── 安装头文件到 include/ljre/
       ├── 安装静态库到 lib/
       └── 安装 CMake 配置到 lib/cmake/ljre/
```

### 11.2 依赖关系

```
┌─────────────────────────────────────────────────────────┐
│                    依赖关系图                             │
└─────────────────────────────────────────────────────────┘

luajit-rule-engine
   │
   ├── LuaJIT 2.1.0-beta3 (外部依赖)
   │   ├── 需要预安装
   │   └── 静态链接或动态链接
   │
   ├── nlohmann/json 3.11.3 (外部依赖)
   │   ├── Header-only 库
   │   └── 已包含在 third-party/
   │
   ├── CMake 3.15+ (构建工具)
   │   └── 构建系统
   │
   └── C++17 编译器
       ├── GCC 7+
       ├── Clang 5+
       └── MSVC 2017+
```

---

## 12. 测试架构

### 12.1 测试金字塔

```
┌─────────────────────────────────────────────────────────┐
│                    测试金字塔                             │
└─────────────────────────────────────────────────────────┘

              ┌────────────────┐
              │  E2E Tests     │  数十个测试用例
              │ (集成测试)      │  < 10%
              └────────────────┘
          ┌──────────────────────┐
          │  Integration Tests   │
          │  (多组件协同测试)     │
          └──────────────────────┘
      ┌──────────────────────────────────┐
      │     Unit Tests (单元测试)         │  上百个测试用例
      │  - RuleEngine: 数百个             │  > 90%
      │  - DataAdapter/JsonAdapter: 数十个│
      │  - Integration: 数十个            │
      │  - 其他组件测试                   │
      └──────────────────────────────────┘

测试工具:
  - Google Test (gtest)
  - Google Mock (gmock)
  - lcov (覆盖率工具)
```

### 12.2 测试覆盖

```
模块                 测试用例数    覆盖率目标
────────────────────────────────────────
RuleEngine              数百个      ≥90%
JsonAdapter              数十个     ≥90%
Integration              数十个     ≥80%
其他组件                  数十个     ≥90%
────────────────────────────────────────
总计                     上百个     ≥85%
```

---

## 13. 最佳实践

### 13.1 使用建议

1. **规则设计**
   - 每个规则文件只做一件事
   - 使用清晰的错误信息
   - 避免复杂的全局状态

2. **性能优化**
   - 启用 JIT 编译
   - 使用 `match_all_rules` 批量匹配
   - 避免在规则中使用循环和递归

3. **错误处理**
   - 始终检查返回值
   - 使用 `error_msg` 参数获取详细信息
   - 规则文件返回有意义的错误信息

4. **内存管理**
   - 使用 `LuaStackGuard` 自动管理栈
   - 避免在循环中创建临时对象
   - 复用 RuleEngine 实例

### 13.2 代码示例

```cpp
// 最佳实践示例
ljre::RuleEngine engine;

// 1. 加载规则配置
std::string error;
if (!engine.load_rule_config("rules.lua", &error)) {
    std::cerr << "加载失败: " << error << std::endl;
    return 1;
}

// 2. 准备数据
json data = { {"age", 25}, {"email", "test@example.com"} };
auto adapter = std::make_shared<JsonAdapter>(data);

// 3. 批量匹配 (比逐个匹配更高效)
// 注意：match_all_rules 返回 true 表示至少有一个规则匹配成功
std::map<std::string, MatchResult> results;
if (!engine.match_all_rules(adapter, results, &error)) {
    std::cerr << "所有规则匹配失败: " << error << std::endl;
    return 1;
}

// 4. 处理结果
for (const auto& [name, result] : results) {
    if (result.matched) {
        std::cout << name << ": ✓ " << result.message << std::endl;
    } else {
        std::cout << name << ": ✗ " << result.message << std::endl;
    }
}

// 5. 热更新规则 (无需重启)
if (engine.reload_rule("age_check", &error)) {
    std::cout << "规则已更新" << std::endl;
}
```

---

## 14. 总结

LuaJIT Rule Engine 是一个设计精良、实现规范的高性能规则引擎项目，具有以下特点：

### 14.1 技术优势

1. **高性能**：LuaJIT JIT 编译，接近原生代码执行速度
2. **动态配置**：支持规则热更新，无需重启服务
3. **安全可靠**：沙箱环境 + 栈守卫，确保运行安全
4. **易于扩展**：适配器模式支持多种数据格式；支持 C++ 函数注册扩展功能；支持 Lua 公共函数加载；支持引擎克隆

### 14.2 架构特点

1. **分层架构**：清晰的职责分离
2. **设计模式**：适配器、RAII、外观、策略模式
3. **内存管理**：RAII 自动管理，防止泄漏
4. **错误处理**：统一的错误传播机制
5. **性能优化**：JIT 控制、批量操作、对象复用

### 14.3 适用场景

- 风控系统（反欺诈规则引擎）
- 权限控制系统（动态权限规则）
- 数据验证系统（复杂数据校验）
- 业务规则引擎（动态业务逻辑）
- 配置管理系统（动态配置验证）

---

## 附录

### A. 文件组织

```
luajit-rule-engine/
├── include/ljre/                  # 公共头文件
│   ├── lua_state.h                # Lua 状态管理
│   ├── data_adapter.h             # 数据适配器接口
│   ├── basic_data_adapter.h       # 基础数据适配器（支持字段修改）
│   ├── json_adapter.h             # JSON 适配器
│   ├── rule_engine.h              # 规则引擎核心
│   └── rule_engine_wrapper.h      # 多线程包装器
├── src/                           # 实现文件
│   ├── lua_state.cpp
│   ├── data_adapter.cpp
│   ├── basic_data_adapter.cpp     # 基础数据适配器实现
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
│   ├── test_helpers.h             # 测试辅助工具
│   ├── lua_state_test.cpp
│   ├── lua_stack_guard_test.cpp
│   ├── data_adapter_test.cpp
│   ├── rule_engine_test.cpp
│   ├── rule_engine_wrapper_test.cpp
│   └── integration_test.cpp
├── docs/                          # 文档
│   ├── ULTRA_COMPLEX_ANALYSIS.md  # 性能分析
│   └── COVERAGE_QUICKSTART.md     # 覆盖率快速指南
├── third-party/                   # 第三方库
│   └── json/                      # nlohmann/json
├── cmake/                         # CMake 配置
├── ARCHITECTURE.md                # 架构文档（本文档）
├── README.md                      # 项目概述和快速开始
├── TESTING.md                     # 测试文档
└── CHANGELOG.md                   # 变更日志
```

### B. 相关文档

- [README.md](README.md) - 项目概述和快速开始
- [TESTING.md](TESTING.md) - 测试指南
- [CHANGELOG.md](CHANGELOG.md) - 变更日志
- [ARCHITECTURE.md](ARCHITECTURE.md) - 本文档
