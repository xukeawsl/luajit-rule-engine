#include <benchmark/benchmark.h>
#include "benchmark_common.h"
#include "data_generator.h"

using namespace benchmark;

// ============================================================================
// 扩展性测试：不同数据规模
// ============================================================================

static void BM_Scale_DataSize_Small(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Small);

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

static void BM_Scale_DataSize_Medium(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Medium);

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

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

static void BM_Scale_DataSize_Large(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Large);

    // 添加复杂规则需要的字段
    data["transaction"] = nlohmann::json{{"amount", 8000.0}, {"hour", 3}};
    data["history"] = nlohmann::json{{"failed_transactions", 3}, {"total_transactions", 15}};
    data["device"] = nlohmann::json{{"is_new_device", false}, {"is_rooted", false}};
    data["location"] = nlohmann::json{{"is_abnormal", false}};

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

static void BM_Scale_DataSize_XLarge(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::UltraComplex);

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::XLarge);

    // 添加超复杂规则需要的字段
    data["user"] = nlohmann::json{
        {"age", 35},
        {"profile", nlohmann::json{
            {"education", "university"},
            {"occupation", "engineer"}
        }}
    };
    data["finance"] = nlohmann::json{
        {"income", 8000.0},
        {"assets", 300000.0},
        {"credit_score", 720}
    };
    data["behavior"] = nlohmann::json{
        {"punctuality", 0.9},
        {"stability", 0.85},
        {"transaction_frequency", 25}
    };
    data["social"] = nlohmann::json{
        {"connections", 80},
        {"influence_score", 3.5},
        {"community_activities", 3}
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
// 扩展性测试：不同数组长度
// ============================================================================

static void BM_Scale_ArrayLength_10(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Medium);

    DataGenerator generator;
    auto data = generator.generate_array_json(10);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_ArrayLength_100(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Medium);

    DataGenerator generator;
    auto data = generator.generate_array_json(100);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_ArrayLength_1000(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Medium);

    DataGenerator generator;
    auto data = generator.generate_array_json(1000);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 扩展性测试：不同嵌套深度
// ============================================================================

static void BM_Scale_NestingDepth_1(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);

    DataGenerator generator;
    auto data = generator.generate_nested_json(1);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_NestingDepth_3(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Medium);

    DataGenerator generator;
    auto data = generator.generate_nested_json(3);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_NestingDepth_5(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    auto data = generator.generate_nested_json(5);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 扩展性测试：规则数量
// ============================================================================

static void BM_Scale_RuleCount_1(benchmark::State& state) {
    ljre::RuleEngine engine;
    engine.add_rule("rule1", "../benchmarks/src/rules/simple_age_check.lua");

    DataGenerator generator;
    auto data = generator.generate_simple_json();

    for (auto _ : state) {
        std::map<std::string, ljre::MatchResult> results;
        engine.match_all_rules(ljre::JsonAdapter(data), results);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_RuleCount_2(benchmark::State& state) {
    ljre::RuleEngine engine;
    engine.add_rule("rule1", "../benchmarks/src/rules/simple_age_check.lua");
    engine.add_rule("rule2", "../benchmarks/src/rules/medium_validation.lua");

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

    for (auto _ : state) {
        std::map<std::string, ljre::MatchResult> results;
        engine.match_all_rules(ljre::JsonAdapter(data), results);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * 2);
}

static void BM_Scale_RuleCount_4(benchmark::State& state) {
    ljre::RuleEngine engine;
    engine.add_rule("rule1", "../benchmarks/src/rules/simple_age_check.lua");
    engine.add_rule("rule2", "../benchmarks/src/rules/medium_validation.lua");
    engine.add_rule("rule3", "../benchmarks/src/rules/complex_risk_control.lua");
    engine.add_rule("rule4", "../benchmarks/src/rules/ultra_complex.lua");

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Large);

    for (auto _ : state) {
        std::map<std::string, ljre::MatchResult> results;
        engine.match_all_rules(ljre::JsonAdapter(data), results);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * 4);
}

// ============================================================================
// 扩展性测试：字段数量
// ============================================================================

static void BM_Scale_FieldCount_5(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);

    DataGenerator generator;
    auto data = generator.generate_json_with_fields(5);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_FieldCount_20(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Medium);

    DataGenerator generator;
    auto data = generator.generate_json_with_fields(20);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_FieldCount_50(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    auto data = generator.generate_json_with_fields(50);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_Scale_FieldCount_100(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    auto data = generator.generate_json_with_fields(100);

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

// ============================================================================
// 注册扩展性测试
// ============================================================================

// 数据规模扩展性
BENCHMARK(BM_Scale_DataSize_Small)
    ->Name("DataSize_Small")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_DataSize_Medium)
    ->Name("DataSize_Medium")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_DataSize_Large)
    ->Name("DataSize_Large")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_DataSize_XLarge)
    ->Name("DataSize_XLarge")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

// 数组长度扩展性
BENCHMARK(BM_Scale_ArrayLength_10)
    ->Name("ArrayLength_10")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_ArrayLength_100)
    ->Name("ArrayLength_100")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_ArrayLength_1000)
    ->Name("ArrayLength_1000")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

// 嵌套深度扩展性
BENCHMARK(BM_Scale_NestingDepth_1)
    ->Name("NestingDepth_1")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_NestingDepth_3)
    ->Name("NestingDepth_3")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_NestingDepth_5)
    ->Name("NestingDepth_5")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

// 规则数量扩展性
BENCHMARK(BM_Scale_RuleCount_1)
    ->Name("RuleCount_1")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_RuleCount_2)
    ->Name("RuleCount_2")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_RuleCount_4)
    ->Name("RuleCount_4")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// 字段数量扩展性
BENCHMARK(BM_Scale_FieldCount_5)
    ->Name("FieldCount_5")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_FieldCount_20)
    ->Name("FieldCount_20")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_FieldCount_50)
    ->Name("FieldCount_50")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Scale_FieldCount_100)
    ->Name("FieldCount_100")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

// 运行所有扩展性测试
BENCHMARK_MAIN();
