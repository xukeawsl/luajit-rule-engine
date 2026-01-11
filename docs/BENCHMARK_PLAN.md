# LuaJIT Rule Engine 性能和压力测试计划

## 1. 测试目标

### 1.1 核心目标

1. **性能对比**：对比 LuaJIT 规则引擎与原生 C++ 实现的性能差异
2. **压力测试**：测试系统在高负载下的稳定性和性能表现
3. **瓶颈分析**：识别性能瓶颈，为优化提供数据支持
4. **扩展性验证**：验证系统在不同数据规模下的扩展性

### 1.2 测试指标

- **执行时间**：单次规则匹配的平均耗时
- **吞吐量**：每秒处理的规则匹配次数
- **延迟**：P50/P95/P99 延迟
- **内存使用**：内存分配和峰值内存
- **JIT 效果**：JIT 开启/关闭的性能对比

---

## 2. 测试环境

### 2.1 硬件环境

```
CPU: [根据实际环境填写]
内存: [根据实际环境填写]
操作系统: Linux 6.8.0-90-generic
编译器: GCC/Clang [版本]
```

### 2.2 软件环境

```
LuaJIT: 2.1.0-beta3
nlohmann/json: 3.11.3
Google Benchmark: [最新版本]
CMake: >= 3.15
C++ 标准: C++17
```

### 2.3 编译配置

```bash
# Release 模式（性能测试）
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 \
         -DBUILD_BENCHMARKS=ON

# 启用优化选项
- O3 优化
- 链接时优化 (LTO)
- PGO (Profile-Guided Optimization)
```

---

## 3. 测试场景设计

### 3.1 数据规模测试

#### 小数据集 (Small Dataset)
- **对象字段数**: 3-5 个
- **数组长度**: 0-10 个元素
- **嵌套深度**: 0-1 层
- **典型场景**: 简单字段验证

#### 中数据集 (Medium Dataset)
- **对象字段数**: 10-20 个
- **数组长度**: 10-100 个元素
- **嵌套深度**: 1-3 层
- **典型场景**: 中等复杂度业务对象

#### 大数据集 (Large Dataset)
- **对象字段数**: 50-100 个
- **数组长度**: 100-1000 个元素
- **嵌套深度**: 3-5 层
- **典型场景**: 复杂业务对象、批量数据处理

#### 超大数据集 (XLarge Dataset)
- **对象字段数**: 200-500 个
- **数组长度**: 1000-10000 个元素
- **嵌套深度**: 5-10 层
- **典型场景**: 极端压力测试

### 3.2 数据格式测试

#### 简单 JSON (Simple JSON)
```json
{
  "age": 25,
  "email": "test@example.com",
  "name": "John Doe"
}
```

#### 嵌套 JSON (Nested JSON)
```json
{
  "user": {
    "profile": {
      "age": 25,
      "address": {
        "city": "Beijing",
        "zip": "100000"
      }
    }
  }
}
```

#### 数组 JSON (Array JSON)
```json
{
  "items": [
    {"id": 1, "price": 100},
    {"id": 2, "price": 200}
  ],
  "scores": [95, 87, 92, 88]
}
```

#### 混合复杂 JSON (Complex JSON)
```json
{
  "user": {
    "name": "Alice",
    "contacts": [
      {"type": "email", "value": "alice@example.com"},
      {"type": "phone", "value": "13800138000"}
    ],
    "metadata": {
      "tags": ["vip", "active"],
      "settings": {
        "notifications": true,
        "privacy": "public"
      }
    }
  }
}
```

### 3.3 规则复杂度测试

#### 简单规则 (Simple Rule)
- 单字段检查
- 简单比较运算
- 示例：年龄 >= 18

#### 中等复杂规则 (Medium Rule)
- 多字段检查
- 逻辑运算符 (AND, OR)
- 字符串匹配
- 示例：年龄 >= 18 AND email 包含 "@"

#### 复杂规则 (Complex Rule)
- 嵌套字段访问
- 数组遍历
- 数学计算
- 字符串处理
- 示例：评分系统、复杂验证

#### 超复杂规则 (Ultra Complex Rule)
- 多层嵌套访问
- 数组和对象组合
- 复杂字符串处理
- 大量条件判断
- 示例：完整的风控规则

---

## 4. 测试用例

### 4.1 基准测试 (Benchmark Tests)

#### BM_LuaJIT_SimpleRule_SmallData
- **目的**: 测试简单规则 + 小数据的性能
- **迭代次数**: 1,000,000 次
- **对比**: 原生 C++ 实现
- **预期**: LuaJIT 接近原生性能 (80-95%)

#### BM_LuaJIT_MediumRule_MediumData
- **目的**: 测试中等复杂度规则 + 中数据的性能
- **迭代次数**: 100,000 次
- **对比**: 原生 C++ 实现
- **预期**: LuaJIT 有一定差距 (60-80%)

#### BM_LuaJIT_ComplexRule_LargeData
- **目的**: 测试复杂规则 + 大数据的性能
- **迭代次数**: 10,000 次
- **对比**: 原生 C++ 实现
- **预期**: LuaJIT 明显差距 (40-70%)

#### BM_LuaJIT_RuleReload
- **目的**: 测试规则热更新的性能影响
- **场景**: 加载规则 -> 执行 N 次 -> 重新加载 -> 执行 N 次
- **对比**: 静态规则
- **预期**: 热更新有一次性开销，后续无影响

#### BM_LuaJIT_JIT_On_vs_Off
- **目的**: 对比 JIT 开启和关闭的性能差异
- **场景**: 同一套规则，分别测试 JIT ON/OFF
- **预期**: JIT 开启提升 5-50x

#### BM_LuaJIT_MultipleRules
- **目的**: 测试多规则匹配的性能
- **场景**: 10/50/100 个规则批量匹配
- **对比**: 原生 C++ 实现
- **预期**: 规则数量增加，性能线性下降

### 4.2 压力测试 (Stress Tests)

#### ST_SingleThread_Continuous
- **目的**: 单线程持续高负载测试
- **持续时间**: 10 分钟
- **并发**: 1 个线程
- **指标**: CPU 使用率、内存稳定性、错误率

#### ST_MultiThread_Scaling
- **目的**: 多线程扩展性测试
- **线程数**: 1, 2, 4, 8, 16, 32
- **持续时间**: 每个配置 5 分钟
- **指标**: 吞吐量随线程数的变化、锁竞争

#### ST_BurstTraffic
- **目的**: 突发流量测试
- **场景**: 静默 10 秒 -> 突发 10000 请求 -> 静默 10 秒
- **重复**: 10 次
- **指标**: 峰值响应时间、系统稳定性

#### ST_MemoryPressure
- **目的**: 内存压力测试
- **场景**: 大数据集 + 长时间运行
- **持续时间**: 30 分钟
- **指标**: 内存泄漏检测、GC 行为

### 4.3 边界测试 (Edge Case Tests)

#### EC_EmptyData
- **目的**: 空数据性能
- **场景**: 空 JSON 对象 {}

#### EC_MaxDepthNesting
- **目的**: 最大嵌套深度性能
- **场景**: 10-20 层嵌套

#### EC_LargeArrays
- **目的**: 大数组性能
- **场景**: 10000+ 元素的数组

#### EC_UnicodeStrings
- **目的**: Unicode 字符串处理性能
- **场景**: 包含各种 Unicode 字符的字符串

### 4.4 对比测试 (Comparative Tests)

#### CT_LuaJIT_vs_Native_Simple
- **LuaJIT 实现**: 简单年龄检查
- **Native C++**: 相同逻辑的原生实现
- **指标**: 执行时间、内存使用

#### CT_LuaJIT_vs_Native_Complex
- **LuaJIT 实现**: 复杂风控规则
- **Native C++**: 相同逻辑的原生实现
- **指标**: 执行时间、内存使用

#### CT_JsonAdapter_vs_DirectAccess
- **对比**: 通过 JsonAdapter vs 直接访问 C++ 结构体
- **目的**: 测试数据转换的开销

#### CT_Batch_vs_Individual
- **对比**: match_all_rules vs 多次 match_rule
- **目的**: 测试批量操作的优势

---

## 5. 测试实现

### 5.1 目录结构

```
benchmarks/
├── CMakeLists.txt              # Benchmark 构建配置
├── README.md                   # Benchmark 使用说明
├── run_benchmarks.sh           # Benchmark 运行脚本
├── generate_report.py          # 测试报告生成脚本
│
├── include/
│   ├── benchmark_common.h      # Benchmark 公共定义
│   ├── data_generator.h        # 测试数据生成器
│   └── native_rules.h          # 原生 C++ 规则实现
│
├── src/
│   ├── benchmark_common.cpp
│   ├── data_generator.cpp
│   ├── native_rules.cpp
│   │
│   ├── benchmarks/
│   │   ├── basic_benchmark.cpp      # 基准测试
│   │   ├── stress_benchmark.cpp     # 压力测试
│   │   ├── comparison_benchmark.cpp # 对比测试
│   │   └── scaling_benchmark.cpp    # 扩展性测试
│   │
│   └── rules/
│       ├── simple_age_check.lua    # 简单规则
│       ├── medium_validation.lua   # 中等规则
│       ├── complex_risk_control.lua # 复杂规则
│       └── ultra_complex.lua       # 超复杂规则
│
└── results/
    ├── [timestamp]_basic.json
    ├── [timestamp]_stress.json
    └── [timestamp]_comparison.json
```

### 5.2 核心组件

#### BenchmarkCommon
- 基础设施初始化
- 性能计时器
- 统计数据收集

#### DataGenerator
- 生成不同规模的测试数据
- 支持多种数据格式
- 可配置的数据生成策略

#### NativeRules
- 原生 C++ 规则实现
- 与 Lua 规则逻辑完全一致
- 用于性能对比

### 5.3 Google Benchmark 集成

```cpp
#include <benchmark/benchmark.h>

// 基本用法
static void BM_LuaJIT_SimpleRule(benchmark::State& state) {
    RuleEngine engine;
    engine.add_rule("age_check", "benchmarks/src/rules/simple_age_check.lua");

    json data = {{"age", 25}};
    JsonAdapter adapter(data);

    for (auto _ : state) {
        MatchResult result;
        engine.match_rule("age_check", adapter, result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.size());
}

// 注册基准测试
BENCHMARK(BM_LuaJIT_SimpleRule)
    ->Iterations(1000000)
    ->Unit(benchmark::kMicrosecond);

// 运行所有基准测试
BENCHMARK_MAIN();
```

### 5.4 测试参数化

```cpp
// 参数化测试：不同数据规模
static void BM_DataSize(benchmark::State& state) {
    int field_count = state.range(0);

    RuleEngine engine;
    // ...

    for (auto _ : state) {
        json data = generate_data(field_count);
        // 执行测试
    }
}

BENCHMARK(BM_DataSize)
    ->Arg(3)    // 小数据
    ->Arg(10)   // 中数据
    ->Arg(50)   // 大数据
    ->Arg(200); // 超大数据

// 参数化测试：不同规则复杂度
BENCHMARK(BM_RuleComplexity)
    ->ArgNames("Simple", "Medium", "Complex", "UltraComplex")
    ->Arg(0)->Arg(1)->Arg(2)->Arg(3);
```

---

## 6. 性能基线

### 6.1 预期性能目标

| 测试场景 | LuaJIT (JIT ON) | LuaJIT (JIT OFF) | Native C++ | LuaJIT/Native 比率 |
|---------|----------------|------------------|------------|-------------------|
| 简单规则 + 小数据 | 50-100 ns | 500-1000 ns | 20-50 ns | 2-5x 慢 |
| 中等规则 + 中数据 | 200-500 ns | 2000-5000 ns | 100-200 ns | 2-5x 慢 |
| 复杂规则 + 大数据 | 1-5 μs | 10-50 μs | 500 ns-2 μs | 2-5x 慢 |
| 批量规则匹配 (10个) | 5-10 μs | 50-100 μs | 2-5 μs | 2-5x 慢 |

### 6.2 吞吐量目标

| 配置 | 目标吞吐量 | 说明 |
|------|-----------|------|
| 单线程简单规则 | > 1M ops/s | 每秒百万次操作 |
| 单线程复杂规则 | > 100K ops/s | 每秒十万次操作 |
| 多线程 (8核) | > 5M ops/s | 线性扩展 |

### 6.3 延迟目标

| 配置 | P50 | P95 | P99 | P999 |
|------|-----|-----|-----|------|
| 简单规则 | < 100 ns | < 200 ns | < 500 ns | < 1 μs |
| 复杂规则 | < 2 μs | < 5 μs | < 10 μs | < 20 μs |

---

## 7. 测试执行

### 7.1 准备阶段

1. **环境检查**
   ```bash
   # 检查 LuaJIT 安装
   ls /usr/local/3rd/luajit-2.1.0-beta3/

   # 检查 Google Benchmark
   pkg-config --modversion benchmark
   ```

2. **编译 Benchmark**
   ```bash
   mkdir build && cd build
   cmake .. -DCMAKE_BUILD_TYPE=Release \
            -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 \
            -DBUILD_BENCHMARKS=ON
   make -j$(nproc)
   ```

3. **预热系统**
   ```bash
   # 运行几次预热测试，使 JIT 预热
   ./benchmarks/basic_benchmark --benchmark_min_time=10
   ```

### 7.2 执行测试

#### 运行所有基准测试
```bash
./run_benchmarks.sh --all
```

#### 运行特定测试
```bash
# 基准测试
./benchmarks/basic_benchmark

# 压力测试
./benchmarks/stress_benchmark

# 对比测试
./benchmarks/comparison_benchmark
```

#### Google Benchmark 选项
```bash
# 运行所有测试，重复 10 次
./benchmarks/basic_benchmark --benchmark_repetitions=10

# 只运行特定测试
./benchmarks/basic_benchmark --benchmark_filter=BM_LuaJIT

# 输出到 JSON
./benchmarks/basic_benchmark --benchmark_format=json > results.json

# 最小运行时间（秒）
./benchmarks/basic_benchmark --benchmark_min_time=30

# 使用 CPU 定时器（避免频率缩放影响）
./benchmarks/basic_benchmark --benchmark_use_real_time
```

### 7.3 测试报告

#### 生成报告
```bash
./generate_report.py --input results/*.json --output report.html
```

#### 报告内容
1. **执行摘要**: 关键指标汇总
2. **性能图表**:
   - 条形图：LuaJIT vs Native 对比
   - 折线图：不同数据规模的性能趋势
   - 热力图：多线程扩展性
3. **详细数据表**: 原始测试数据
4. **结论和建议**: 基于测试结果的优化建议

---

## 8. 测试结果分析

### 8.1 性能分析维度

#### 8.1.1 绝对性能
- 单次操作耗时
- 每秒操作数 (ops/s)
- 与预期目标对比

#### 8.1.2 相对性能
- LuaJIT vs Native C++ 对比
- JIT ON vs JIT OFF 对比
- 不同数据规模的影响

#### 8.1.3 扩展性
- 线性扩展性 (多线程)
- 数据规模扩展性
- 规则数量扩展性

#### 8.1.4 稳定性
- 长时间运行稳定性
- 突发流量处理能力
- 内存使用稳定性

### 8.2 瓶颈识别

#### 常见性能瓶颈

1. **Lua 栈操作**
   - 频繁的 push/pop
   - 栈查找开销

2. **数据转换**
   - JSON → Lua table 转换
   - 复杂嵌套结构的处理

3. **Lua 函数调用**
   - lua_pcall 开销
   - 参数传递开销

4. **内存分配**
   - Lua 对象分配
   - JSON 解析分配

5. **JIT 编译**
   - 首次执行编译开销
   - 热点识别延迟

### 8.3 优化建议

#### 8.3.1 短期优化
1. **减少数据转换**: 预先转换，缓存结果
2. **批量操作**: 使用 match_all_rules
3. **规则预编译**: 启动时预加载并预热规则
4. **JIT 预热**: 启动后执行预热运行

#### 8.3.2 中期优化
1. **Lua 代码优化**: 减少全局变量访问，使用局部变量
2. **缓存策略**: 缓存常用的 Lua 对象
3. **内存池**: 减少动态分配
4. **线程池**: 复用 LuaState 实例（如果线程安全）

#### 8.3.3 长期优化
1. **LuaJIT 升级**: 跟进最新版本
2. **自定义 C 模块**: 热点代码用 C 实现
3. **规则并行化**: 多规则并行执行
4. **规则编译**: 预编译规则为字节码

---

## 9. 持续性能监控

### 9.1 回归测试

每次代码变更后运行完整测试套件：

```bash
# CI/CD 集成
./run_benchmarks.sh --regression --baseline baseline.json
```

### 9.2 性能阈值

设置性能回归阈值：

- **警告**: 性能下降 > 5%
- **失败**: 性能下降 > 15%

### 9.3 性能追踪

- 维护历史性能数据
- 生成性能趋势图
- 识别性能退化

---

## 10. 文档和报告

### 10.1 测试文档

- **BENCHMARK_PLAN.md**: 本文档，测试计划
- **BENCHMARK_GUIDE.md**: 测试执行指南
- **BENCHMARK_REPORT.md**: 测试结果报告

### 10.2 代码文档

- 每个测试用例的注释说明
- 测试数据生成器的文档
- 原生实现与 Lua 实现的对应关系

### 10.3 结果归档

- 按日期归档测试结果
- 保存原始 JSON 数据
- 保留可视化图表

---

## 11. 附录

### 11.1 Google Benchmark 安装

```bash
# Ubuntu/Debian
sudo apt-get install libbenchmark-dev

# 从源码编译
git clone https://github.com/google/benchmark.git
cd benchmark
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 11.2 测试环境配置

```bash
# 设置 CPU 性能模式（避免频率缩放影响）
sudo cpupower frequency-set -g performance

# 禁用 CPU 节能
echo 0 | sudo tee /sys/devices/system/cpu/cpu*/cpuidle/state*/disable

# 固定 CPU 频率（可选）
sudo cpupower frequency-set -f 3.0GHz
```

### 11.3 测试数据示例

#### 小数据集
```json
{"age": 25, "email": "test@example.com", "name": "John"}
```

#### 中数据集
```json
{
  "user_id": 12345,
  "username": "john_doe",
  "email": "john@example.com",
  "age": 25,
  "phone": "13800138000",
  "address": {
    "street": "Main St",
    "city": "Beijing",
    "zip": "100000"
  },
  "preferences": {
    "language": "zh-CN",
    "timezone": "UTC+8"
  },
  "tags": ["vip", "active"],
  "score": 95
}
```

#### 大数据集
生成 50-100 个字段，包含嵌套和数组。

---

## 12. 总结

本测试计划提供了全面的性能和压力测试方案，涵盖：

1. **多种数据规模**: 从小数据到超大数据
2. **多种数据格式**: 简单到复杂的 JSON 结构
3. **多种规则复杂度**: 从简单到超复杂规则
4. **多种测试类型**: 基准、压力、边界、对比测试
5. **详细的对比**: LuaJIT vs Native C++
6. **完整的工具链**: 自动化测试和报告生成

通过本测试计划，可以全面评估 LuaJIT Rule Engine 的性能表现，识别优化机会，并为生产部署提供数据支持。
