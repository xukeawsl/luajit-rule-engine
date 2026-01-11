#include <benchmark/benchmark.h>
#include "benchmark_common.h"
#include "data_generator.h"

using namespace benchmark;

// ============================================================================
// 压力测试：单线程持续负载
// ============================================================================

static void BM_Stress_SingleThread_Continuous(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        auto data = generator.generate_data(DataSize::Large);
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 压力测试：大数据集压力
// ============================================================================

static void BM_Stress_LargeDataset(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    // 生成超大数据
    auto data = generator.generate_data(DataSize::XLarge);

    // 添加复杂规则需要的字段
    data["transaction"] = nlohmann::json{
        {"amount", 8000.0},
        {"hour", 3}
    };
    data["history"] = nlohmann::json{
        {"failed_transactions", 3},
        {"total_transactions", 15}
    };
    data["device"] = nlohmann::json{
        {"is_new_device", false},
        {"is_rooted", false}
    };
    data["location"] = nlohmann::json{
        {"is_abnormal", false}
    };

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.dump().size());
}

// ============================================================================
// 压力测试：频繁规则切换
// ============================================================================

static void BM_Stress_RuleSwitching(benchmark::State& state) {
    // 创建多个引擎，每个加载不同规则
    RuleEngineWrapper engine_simple;
    engine_simple.initialize(RuleComplexity::Simple);

    RuleEngineWrapper engine_medium;
    engine_medium.initialize(RuleComplexity::Medium);

    RuleEngineWrapper engine_complex;
    engine_complex.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    auto data_simple = generator.generate_simple_json();
    auto data_medium = generator.generate_data(DataSize::Medium);
    auto data_complex = generator.generate_data(DataSize::Large);

    // 添加复杂规则需要的字段
    data_complex["transaction"] = nlohmann::json{{"amount", 8000.0}, {"hour", 3}};
    data_complex["history"] = nlohmann::json{{"failed_transactions", 3}, {"total_transactions", 15}};
    data_complex["device"] = nlohmann::json{{"is_new_device", false}, {"is_rooted", false}};
    data_complex["location"] = nlohmann::json{{"is_abnormal", false}};

    int counter = 0;

    for (auto _ : state) {
        bool matched = false;
        std::string message;

        // 轮流使用不同规则
        switch (counter % 3) {
            case 0:
                engine_simple.match_rule(data_simple, matched, message);
                break;
            case 1:
                engine_medium.match_rule(data_medium, matched, message);
                break;
            case 2:
                engine_complex.match_rule(data_complex, matched, message);
                break;
        }

        counter++;
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 压力测试：内存分配压力
// ============================================================================

static void BM_Stress_MemoryAllocation(benchmark::State& state) {
    DataGenerator generator;

    for (auto _ : state) {
        // 每次迭代都创建新引擎和大量数据
        RuleEngineWrapper engine;
        engine.initialize(RuleComplexity::Medium);

        bool matched = false;
        std::string message;
        auto data = generator.generate_data(DataSize::Large);

        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 压力测试：批量处理
// ============================================================================

static void BM_Stress_BatchProcessing(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);

    DataGenerator generator;
    BatchDataGenerator batch_generator;

    const size_t batch_size = 100;
    auto batch = batch_generator.generate_batch(DataSize::Small, batch_size);

    size_t total_processed = 0;

    for (auto _ : state) {
        for (const auto& data : batch) {
            bool matched = false;
            std::string message;
            engine.match_rule(data, matched, message);
            benchmark::DoNotOptimize(matched);
            benchmark::DoNotOptimize(message);
            total_processed++;
        }
    }

    state.SetItemsProcessed(total_processed);
}

// ============================================================================
// 注册压力测试
// ============================================================================

BENCHMARK(BM_Stress_SingleThread_Continuous)
    ->Name("Stress_SingleThread_Continuous")
    ->Iterations(50000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Stress_LargeDataset)
    ->Name("Stress_LargeDataset")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Stress_RuleSwitching)
    ->Name("Stress_RuleSwitching")
    ->Iterations(50000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Stress_MemoryAllocation)
    ->Name("Stress_MemoryAllocation")
    ->Iterations(5000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Stress_BatchProcessing)
    ->Name("Stress_BatchProcessing")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

// 运行所有压力测试
BENCHMARK_MAIN();
