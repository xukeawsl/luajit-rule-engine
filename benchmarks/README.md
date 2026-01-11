# LuaJIT Rule Engine 性能测试

本目录包含 LuaJIT Rule Engine 的性能和压力测试代码，使用 Google Benchmark 框架。

## 目录结构

```
benchmarks/
├── CMakeLists.txt              # CMake 构建配置
├── README.md                   # 本文档
├── include/                    # 头文件
│   ├── benchmark_common.h      # Benchmark 公共定义
│   ├── data_generator.h        # 测试数据生成器
│   └── native_rules.h          # 原生 C++ 规则实现
├── src/                        # 源文件
│   ├── benchmark_common.cpp
│   ├── data_generator.cpp
│   ├── native_rules.cpp
│   ├── benchmarks/             # Benchmark 测试用例
│   │   ├── basic_benchmark.cpp      # 基准测试
│   │   ├── stress_benchmark.cpp     # 压力测试
│   │   ├── comparison_benchmark.cpp # 对比测试
│   │   └── scaling_benchmark.cpp    # 扩展性测试
│   └── rules/                  # Lua 规则文件
│       ├── simple_age_check.lua
│       ├── medium_validation.lua
│       ├── complex_risk_control.lua
│       └── ultra_complex.lua
└── results/                    # 测试结果输出目录
```

## 依赖

### 必需依赖

- **LuaJIT 2.1.0-beta3**: 规则引擎核心依赖
- **nlohmann/json 3.11.3**: JSON 数据格式支持
- **Google Benchmark 1.8.3+**: 基准测试框架

### 可选依赖

- **Python 3**: 生成测试报告（可选）

## 编译

### 1. 安装 Google Benchmark

#### Ubuntu/Debian
```bash
sudo apt-get install libbenchmark-dev
```

#### 从源码编译
```bash
git clone https://github.com/google/benchmark.git
cd benchmark
git checkout v1.8.3
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
sudo make install
```

### 2. 编译 Benchmark

```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake，启用 benchmark
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3 \
         -DBUILD_BENCHMARKS=ON

# 编译
make -j$(nproc)
```

编译成功后，benchmark 可执行文件位于：
- `build/benchmarks/basic_benchmark`
- `build/benchmarks/stress_benchmark`
- `build/benchmarks/comparison_benchmark`
- `build/benchmarks/scaling_benchmark`

## 使用方法

### 运行所有基准测试

```bash
# 切换到 build 目录
cd build

# 运行基准测试
./benchmarks/basic_benchmark

# 运行压力测试
./benchmarks/stress_benchmark

# 运行对比测试
./benchmarks/comparison_benchmark

# 运行扩展性测试
./benchmarks/scaling_benchmark
```

### Google Benchmark 常用选项

```bash
# 只运行特定名称的测试
./benchmarks/basic_benchmark --benchmark_filter=BM_LuaJIT

# 设置最小运行时间（秒）
./benchmarks/basic_benchmark --benchmark_min_time=30

# 重复运行多次以获得更准确的结果
./benchmarks/basic_benchmark --benchmark_repetitions=10

# 输出格式（console, json, csv）
./benchmarks/basic_benchmark --benchmark_format=json > results.json

# 使用真实时间而非 CPU 时间
./benchmarks/basic_benchmark --benchmark_use_real_time

# 显示详细输出
./benchmarks/basic_benchmark --benchmark_counters_tabular=true

# 列出所有测试但不运行
./benchmarks/basic_benchmark --benchmark_list_tests
```

## 测试类型

### 1. 基准测试 (basic_benchmark)

测试不同场景下的基础性能：

- 简单规则 + 小数据
- 中等规则 + 中数据
- 复杂规则 + 大数据
- JIT 开启/关闭对比
- 多规则批量匹配

**运行示例**：
```bash
./benchmarks/basic_benchmark --benchmark_min_time=10
```

### 2. 压力测试 (stress_benchmark)

测试系统在高负载下的表现：

- 单线程持续高负载
- 多线程扩展性测试
- 突发流量测试
- 内存压力测试

**运行示例**：
```bash
# 单线程压力测试（运行 10 分钟）
./benchmarks/stress_benchmark --benchmark_min_time=600

# 多线程测试
./benchmarks/stress_benchmark --benchmark_threads=8
```

### 3. 对比测试 (comparison_benchmark)

对比 LuaJIT 与原生 C++ 的性能差异：

- 简单规则对比
- 复杂规则对比
- 数据转换开销对比
- 批量操作对比

**运行示例**：
```bash
./benchmarks/comparison_benchmark --benchmark_format=json > comparison_results.json
```

### 4. 扩展性测试 (scaling_benchmark)

测试不同数据规模下的性能表现：

- 不同字段数量的影响
- 不同数组长度的影响
- 不同嵌套深度的影响

**运行示例**：
```bash
# 测试不同数据规模
./benchmarks/scaling_benchmark --benchmark_filter=BM_DataSize
```

## 性能测试建议

### 测试环境准备

1. **设置 CPU 性能模式**
   ```bash
   # 设置为性能模式，避免频率缩放影响
   sudo cpupower frequency-set -g performance
   ```

2. **关闭不必要的进程**
   ```bash
   # 关闭占用资源的服务
   sudo systemctl stop docker
   ```

3. **固定 CPU 频率（可选）**
   ```bash
   # 查看可用频率
   sudo cpupower frequency-info

   # 设置固定频率
   sudo cpupower frequency-set -f 3.0GHz
   ```

### 预热系统

```bash
# 运行几次预热测试，使 JIT 编译器预热
./benchmarks/basic_benchmark --benchmark_min_time=5

# 然后运行正式测试
./benchmarks/basic_benchmark --benchmark_min_time=30
```

### 收集结果

#### 运行测试并保存 JSON 结果

```bash
# 切换到 build 目录
cd build

# 创建结果目录
mkdir -p benchmarks/results

# 运行测试并保存结果
./benchmarks/basic_benchmark --benchmark_format=json > benchmarks/results/basic.json
./benchmarks/stress_benchmark --benchmark_format=json > benchmarks/results/stress.json
./benchmarks/comparison_benchmark --benchmark_format=json > benchmarks/results/comparison.json
./benchmarks/scaling_benchmark --benchmark_format=json > benchmarks/results/scaling.json
```

#### 生成测试报告

项目提供了 Python 脚本来自动生成 HTML、Markdown 和 JSON 格式的测试报告。

```bash
# 从项目根目录运行
python3 benchmarks/generate_report.py

# 指定结果目录（默认搜索 build/benchmarks/results 和 benchmarks/results）
python3 benchmarks/generate_report.py --results-dir build/benchmarks/results

# 只生成特定格式
python3 benchmarks/generate_report.py --format html      # 只生成 HTML
python3 benchmarks/generate_report.py --format markdown  # 只生成 Markdown
python3 benchmarks/generate_report.py --format json      # 只生成 JSON
```

**生成的报告**：
- **HTML 报告**: `benchmarks/results/benchmark_report_<timestamp>.html`
  - 包含性能对比表格、可视化图表、详细测试数据
  - 可在浏览器中直接查看
- **Markdown 报告**: `benchmarks/results/benchmark_report_<timestamp>.md`
  - 适合文档归档和版本控制
- **JSON 摘要**: `benchmarks/results/benchmark_summary_<timestamp>.json`
  - 机器可读格式，适合自动化分析

**报告特性**：
- ✅ 自动从 Google Benchmark JSON 结果提取数据
- ✅ 计算 LuaJIT vs Native 性能比率
- ✅ 根据性能数据给出推荐方案
- ✅ 提取系统信息（CPU、主机名等）
- ✅ 支持多个结果文件合并分析

## 性能基线参考

| 测试场景 | 预期性能 (JIT ON) | 预期性能 (Native) | 性能比率 |
|---------|------------------|------------------|---------|
| 简单规则 + 小数据 | 50-100 ns | 20-50 ns | 2-5x |
| 中等规则 + 中数据 | 200-500 ns | 100-200 ns | 2-5x |
| 复杂规则 + 大数据 | 1-5 μs | 500 ns-2 μs | 2-5x |

## 性能分析工具

### 使用 perf 分析

```bash
# 安装 perf
sudo apt-get install linux-tools-common linux-tools-generic

# 记录性能数据
perf record -g ./benchmarks/basic_benchmark

# 查看报告
perf report
```

### 使用 valgrind 分析内存

```bash
# 安装 valgrind
sudo apt-get install valgrind

# 内存分析
valgrind --tool=massif ./benchmarks/basic_benchmark

# 查看结果
ms_print massif.out.xxxxx
```

## 故障排查

### 编译错误

1. **找不到 Google Benchmark**
   ```bash
   # 确保 Google Benchmark 已安装
   pkg-config --modversion benchmark

   # 或指定 benchmark 路径
   cmake .. -Dbenchmark_DIR=/path/to/benchmark/cmake
   ```

2. **找不到 LuaJIT**
   ```bash
   # 指定 LuaJIT 路径
   cmake .. -DLUAJIT_ROOT=/usr/local/3rd/luajit-2.1.0-beta3
   ```

### 运行时错误

1. **找不到规则文件**
   ```bash
   # 确保在正确目录运行
   cd build
   ./benchmarks/basic_benchmark
   ```

2. **性能波动大**
   - 关闭其他占用 CPU 的程序
   - 使用性能模式而非节能模式
   - 增加运行时间以获得更稳定的结果

## 更多信息

- [测试计划文档](../docs/BENCHMARK_PLAN.md) - 详细的测试计划和设计
- [Google Benchmark 文档](https://github.com/google/benchmark) - Google Benchmark 使用指南

## 贡献

欢迎提交新的测试用例和改进建议！
