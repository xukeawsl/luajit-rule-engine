#pragma once

#include <ljre/rule_engine.h>
#include <ljre/json_adapter.h>
#include <chrono>
#include <string>
#include <vector>

namespace benchmark {

// 测试数据规模枚举
enum class DataSize {
    Small,      // 小数据: 3-5 个字段
    Medium,     // 中数据: 10-20 个字段
    Large,      // 大数据: 50-100 个字段
    XLarge      // 超大数据: 200-500 个字段
};

// 规则复杂度枚举
enum class RuleComplexity {
    Simple,        // 简单规则: 单字段检查
    Medium,        // 中等规则: 多字段 + 逻辑运算
    Complex,       // 复杂规则: 嵌套 + 数组遍历
    UltraComplex   // 超复杂规则: 多层嵌套 + 大量条件
};

// 测试结果统计
struct TestStats {
    double mean_time_us;      // 平均时间（微秒）
    double min_time_us;       // 最小时间（微秒）
    double max_time_us;       // 最大时间（微秒）
    double std_dev_us;        // 标准差（微秒）
    size_t iterations;        // 迭代次数
    double throughput_ops;    // 吞吐量（ops/s）

    TestStats()
        : mean_time_us(0.0)
        , min_time_us(0.0)
        , max_time_us(0.0)
        , std_dev_us(0.0)
        , iterations(0)
        , throughput_ops(0.0)
    {}
};

// 规则引擎包装器，用于测试
class RuleEngineWrapper {
public:
    RuleEngineWrapper();
    ~RuleEngineWrapper() = default;

    // 初始化规则引擎（加载指定复杂度的规则）
    bool initialize(RuleComplexity complexity, const std::string& rule_file = "");

    // 执行规则匹配
    bool match_rule(const nlohmann::json& data, bool& matched, std::string& message);

    // 获取规则引擎实例
    ljre::RuleEngine& get_engine() { return _engine; }

    // 启用/禁用 JIT
    bool enable_jit();
    bool disable_jit();
    bool flush_jit();

private:
    ljre::RuleEngine _engine;
    std::string _current_rule_name;
};

// 简单计时器
class Timer {
public:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;

    Timer() : _start(Clock::now()) {}

    // 重置计时器
    void reset() {
        _start = Clock::now();
    }

    // 获取经过的时间（秒）
    double elapsed_seconds() const {
        return std::chrono::duration_cast<Duration>(Clock::now() - _start).count();
    }

    // 获取经过的时间（微秒）
    double elapsed_microseconds() const {
        return elapsed_seconds() * 1e6;
    }

    // 获取经过的时间（纳秒）
    double elapsed_nanoseconds() const {
        return elapsed_seconds() * 1e9;
    }

private:
    TimePoint _start;
};

// 辅助函数

// 将 DataSize 枚举转换为字符串
const char* data_size_to_string(DataSize size);

// 将 RuleComplexity 枚举转换为字符串
const char* rule_complexity_to_string(RuleComplexity complexity);

// 获取规则文件路径
std::string get_rule_file(RuleComplexity complexity);

// 打印测试配置信息
void print_test_config(DataSize size, RuleComplexity complexity, size_t iterations);

// 性能测试辅助类
class PerformanceTester {
public:
    PerformanceTester(const std::string& test_name)
        : _test_name(test_name)
        , _total_iterations(0)
        , _total_time_us(0.0)
        , _min_time_us(1e9)
        , _max_time_us(0.0)
    {}

    // 记录一次迭代
    void record_iteration(double time_us) {
        _total_iterations++;
        _total_time_us += time_us;
        _min_time_us = std::min(_min_time_us, time_us);
        _max_time_us = std::max(_max_time_us, time_us);
        _times.push_back(time_us);
    }

    // 计算统计数据
    TestStats get_stats() const {
        TestStats stats;
        stats.iterations = _total_iterations;
        stats.mean_time_us = _total_time_us / _total_iterations;
        stats.min_time_us = _min_time_us;
        stats.max_time_us = _max_time_us;

        // 计算标准差
        if (_total_iterations > 1) {
            double variance = 0.0;
            for (double time : _times) {
                double diff = time - stats.mean_time_us;
                variance += diff * diff;
            }
            variance /= _total_iterations;
            stats.std_dev_us = std::sqrt(variance);
        }

        // 计算吞吐量
        if (stats.mean_time_us > 0) {
            stats.throughput_ops = 1.0 / (stats.mean_time_us / 1e6);
        }

        return stats;
    }

    // 打印统计信息
    void print_stats() const {
        auto stats = get_stats();
        printf("\n=== %s 测试结果 ===\n", _test_name.c_str());
        printf("迭代次数:     %zu\n", stats.iterations);
        printf("平均时间:     %.3f μs\n", stats.mean_time_us);
        printf("最小时间:     %.3f μs\n", stats.min_time_us);
        printf("最大时间:     %.3f μs\n", stats.max_time_us);
        printf("标准差:       %.3f μs\n", stats.std_dev_us);
        printf("吞吐量:       %.2f ops/s\n", stats.throughput_ops);
        printf("============================\n");
    }

private:
    std::string _test_name;
    size_t _total_iterations;
    double _total_time_us;
    double _min_time_us;
    double _max_time_us;
    std::vector<double> _times;
};

} // namespace benchmark
