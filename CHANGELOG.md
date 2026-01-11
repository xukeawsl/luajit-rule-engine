# 变更日志

所有重要的项目变更都将记录在此文件中。

## [未发布] - 2025-01

### 新增 (Added)

#### LuaState 功能增强
- **JIT 动态控制**: 新增 JIT 编译器的运行时控制功能
  - `enable_jit()`: 启用 JIT 编译引擎
  - `disable_jit()`: 禁用 JIT 编译引擎，切换到解释模式
  - `flush_jit()`: 刷新 JIT 编译器缓存，清除已编译的代码
- 支持 LuaJIT jit 库的自动加载

#### 测试覆盖率大幅提升
- **LuaState 测试**: 从 32 个增加到 52 个测试用例
  - 新增 14 个 JIT 控制相关测试
  - 新增 6 个错误处理测试（覆盖栈顶非字符串场景）
- **JsonAdapter 测试**: 从 35 个增加到 46 个测试用例
  - 新增异常处理测试（自定义 ExceptionThrowingDataAdapter）
  - 新增边界条件测试（深度嵌套、大量数据、混合类型）
  - 完整覆盖异常捕获和错误传播路径
- **RuleEngine 测试**: 从 43 个增加到 78 个测试用例
  - 新增 Lua 状态无效场景测试
  - 完整覆盖 call_match_function 所有错误路径
  - 新增规则配置文件边界条件测试
- **总测试数量**: 从 138 个增加到 204 个测试用例（+48%）

### 改进 (Changed)

#### 代码质量优化
- 移除 `Rule` 和 `RuleInfo` 结构体中不必要的 `loaded` 成员
  - 规则在 map 中存在即表示已加载，简化状态管理
  - 减少了不必要的复杂度

#### 错误处理增强
- 完善了 `LuaState::get_error_string()` 对各种栈顶类型的处理
  - 正确处理 table、boolean、nil、function、userdata、thread 等非字符串类型
  - 栈顶非字符串时返回 "Unknown error" 且不弹出栈元素

#### 测试辅助工具
- 新增 `ExceptionThrowingDataAdapter` 用于测试异常处理路径
- 新增 `RuleEngineInternalTest` 测试子类，支持访问内部 Lua 状态
- 新增 `RuleEngineInvalidStateTest` 测试子类，支持模拟无效 Lua 状态

### 文档更新 (Documentation)
- 更新 README.md：
  - 反映最新的测试用例数量（204 个）
  - 更新特性列表，注明包含 jit 库
  - 详细列出各测试文件的测试类别
- 更新 TESTING.md：
  - 更新测试统计表格
  - 详细说明各测试模块的覆盖范围
- 新增 CHANGELOG.md 记录所有重要变更

### 测试覆盖率详情

#### LuaState (52 个测试)
- ✅ 构造和析构：移动语义、自我赋值
- ✅ 文件加载：成功/失败/语法错误/运行时错误
- ✅ Buffer 加载：各种代码格式和大小
- ✅ 错误处理：栈顶各种类型（string、number、table、boolean、nil、function、userdata、thread）
- ✅ 栈操作：栈平衡检查
- ✅ 安全性：禁用 io 库、状态独立性
- ✅ 边界条件：零大小、超大代码、Unicode 路径
- ✅ JIT 控制：enable/disable/flush 及各种组合场景

#### LuaStackGuard (17 个测试)
- ✅ 基本功能：栈恢复、多次 push/pop
- ✅ 嵌套场景：多层守卫嵌套
- ✅ Release 机制：手动释放、多次调用
- ✅ 空栈处理：栈为空时的行为
- ✅ 函数调用场景：实际使用模式
- ✅ 表迭代场景：遍历表时的栈管理
- ✅ 错误处理：异常情况下的栈恢复

#### JsonAdapter (46 个测试)
- ✅ 基本类型：null、boolean、integer、float、string
- ✅ 数组转换：空数组、嵌套数组、大数组、混合类型
- ✅ 对象转换：空对象、嵌套对象、大量键、特殊键
- ✅ 特殊字符：含空字符字符串、Unicode 字符、特殊字符键
- ✅ 异常处理：标准异常捕获、错误消息生成、无 error_msg 场景
- ✅ 边界条件：深度嵌套（500+ 层）、1000 个键的大对象
- ✅ 栈平衡：成功和失败时的栈平衡保证

#### RuleEngine (78 个测试)
- ✅ 规则管理：添加、移除、重载、清空规则
- ✅ 配置加载：从配置文件加载规则列表
- ✅ 规则匹配：单个规则匹配、所有规则匹配
- ✅ 错误场景：无效状态、规则不存在、规则函数表不存在、match 函数不存在
- ✅ 返回值验证：非布尔返回值、单返回值
- ✅ 数据适配器：push_to_lua 失败场景
- ✅ 边界条件：空配置、重复规则、无效字段

#### Integration Test (11 个测试)
- ✅ 端到端工作流：完整的规则引擎使用流程
- ✅ 用户注册验证：实际业务场景模拟
- ✅ 规则动态管理：运行时添加/删除规则
- ✅ 多引擎独立：多个引擎实例互不干扰
- ✅ 复杂数据结构：嵌套对象和数组
- ✅ 大数据集处理：大量规则和数据

### 技术债务 (Technical Debt)
无

### 废弃 (Deprecated)
无

### 移除 (Removed)
无

### 安全性 (Security)
无安全相关问题修复

### 已知问题 (Known Issues)
无

### 升级指南 (Migration Guide)

#### 从旧版本升级

如果你是从之前的版本升级，请注意以下变更：

1. **规则结构变更**:
   - `RuleInfo` 结构体不再包含 `loaded` 字段
   - 规则引擎内部也不再跟踪 `loaded` 状态
   - 这不影响公共 API，仅影响内部实现

2. **JIT 功能**:
   - LuaState 现在会自动加载 jit 库
   - 可以通过 `enable_jit()`/`disable_jit()` 动态控制 JIT
   - 默认情况下 JIT 是启用的

3. **测试要求**:
   - 所有新增测试必须通过
   - 代码覆盖率要求 ≥85%

---

## 版本命名规范

本项目遵循语义化版本 (Semantic Versioning) 规范：
- **主版本号**: 不兼容的 API 修改
- **次版本号**: 向下兼容的功能性新增
- **修订号**: 向下兼容的问题修正
