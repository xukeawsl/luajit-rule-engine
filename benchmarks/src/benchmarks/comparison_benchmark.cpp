#include <benchmark/benchmark.h>
#include "benchmark_common.h"
#include "data_generator.h"
#include "native_rules.h"

using namespace benchmark;

// ============================================================================
// 对比测试：LuaJIT vs Native C++ - 简单规则
// ============================================================================

static void BM_Comp_LuaJIT_Simple(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);

    DataGenerator generator;
    auto data = generator.generate_simple_json();

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }
}

static void BM_Comp_Native_Simple(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_simple_json();

    for (auto _ : state) {
        std::string message;
        bool matched = NativeAgeCheckRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }
}

// ============================================================================
// 对比测试：LuaJIT vs Native C++ - 中等规则
// ============================================================================

static void BM_Comp_LuaJIT_Medium(benchmark::State& state) {
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
}

static void BM_Comp_Native_Medium(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

    for (auto _ : state) {
        std::string message;
        bool matched = NativeUserValidationRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }
}

// ============================================================================
// 对比测试：LuaJIT vs Native C++ - 复杂规则
// ============================================================================

static void BM_Comp_LuaJIT_Complex(benchmark::State& state) {
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
}

static void BM_Comp_Native_Complex(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Large);

    // 添加复杂规则需要的字段
    data["transaction"] = nlohmann::json{{"amount", 8000.0}, {"hour", 3}};
    data["history"] = nlohmann::json{{"failed_transactions", 3}, {"total_transactions", 15}};
    data["device"] = nlohmann::json{{"is_new_device", false}, {"is_rooted", false}};
    data["location"] = nlohmann::json{{"is_abnormal", false}};

    for (auto _ : state) {
        std::string message;
        bool matched = NativeRiskControlRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }
}

// ============================================================================
// 对比测试：LuaJIT vs Native C++ - 超复杂规则
// ============================================================================

static void BM_Comp_LuaJIT_UltraComplex(benchmark::State& state) {
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
}

static void BM_Comp_Native_UltraComplex(benchmark::State& state) {
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
        std::string message;
        bool matched = NativeComprehensiveRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }
}

// ============================================================================
// 对比测试：数据转换开销
// ============================================================================

static void BM_Comp_JsonAdapter_Overhead(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

    ljre::RuleEngine engine;
    engine.add_rule("simple_age_check", "../benchmarks/src/rules/simple_age_check.lua");

    for (auto _ : state) {
        ljre::JsonAdapter adapter(data);
        ljre::MatchResult result;
        engine.match_rule("simple_age_check", adapter, result);
        benchmark::DoNotOptimize(result);
    }
}

static void BM_Comp_Direct_JSON_No_Adapter(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_simple_json();

    // 直接使用原生规则，不经过 JsonAdapter
    for (auto _ : state) {
        std::string message;
        bool matched = NativeAgeCheckRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }
}

// ============================================================================
// 对比测试：批量 vs 单次
// ============================================================================

static void BM_Comp_Batch_MatchAll(benchmark::State& state) {
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
}

static void BM_Comp_Individual_Match(benchmark::State& state) {
    ljre::RuleEngine engine;
    engine.add_rule("rule1", "../benchmarks/src/rules/simple_age_check.lua");
    engine.add_rule("rule2", "../benchmarks/src/rules/medium_validation.lua");

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

    for (auto _ : state) {
        ljre::MatchResult result1;
        ljre::MatchResult result2;
        engine.match_rule("rule1", ljre::JsonAdapter(data), result1);
        engine.match_rule("rule2", ljre::JsonAdapter(data), result2);
        benchmark::DoNotOptimize(result1);
        benchmark::DoNotOptimize(result2);
    }
}

// ============================================================================
// 注册对比测试
// ============================================================================

// 简单规则对比
BENCHMARK(BM_Comp_LuaJIT_Simple)
    ->Name("LuaJIT_Simple")
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Comp_Native_Simple)
    ->Name("Native_Simple")
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond);

// 中等规则对比
BENCHMARK(BM_Comp_LuaJIT_Medium)
    ->Name("LuaJIT_Medium")
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Comp_Native_Medium)
    ->Name("Native_Medium")
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond);

// 复杂规则对比
BENCHMARK(BM_Comp_LuaJIT_Complex)
    ->Name("LuaJIT_Complex")
    ->Iterations(10000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Comp_Native_Complex)
    ->Name("Native_Complex")
    ->Iterations(10000)
    ->Unit(benchmark::kNanosecond);

// 超复杂规则对比
BENCHMARK(BM_Comp_LuaJIT_UltraComplex)
    ->Name("LuaJIT_UltraComplex")
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Comp_Native_UltraComplex)
    ->Name("Native_UltraComplex")
    ->Iterations(1000)
    ->Unit(benchmark::kNanosecond);

// 数据转换对比
BENCHMARK(BM_Comp_JsonAdapter_Overhead)
    ->Name("JsonAdapter_WithConversion")
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Comp_Direct_JSON_No_Adapter)
    ->Name("Direct_JSON_NoConversion")
    ->Iterations(100000)
    ->Unit(benchmark::kNanosecond);

// 批量 vs 单次对比
BENCHMARK(BM_Comp_Batch_MatchAll)
    ->Name("Batch_MatchAllRules")
    ->Iterations(10000)
    ->Unit(benchmark::kNanosecond);

BENCHMARK(BM_Comp_Individual_Match)
    ->Name("Individual_Match_Sequential")
    ->Iterations(10000)
    ->Unit(benchmark::kNanosecond);

// 运行所有对比测试
BENCHMARK_MAIN();
