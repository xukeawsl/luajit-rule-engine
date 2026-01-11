#include <benchmark/benchmark.h>
#include "benchmark_common.h"
#include "data_generator.h"
#include "native_rules.h"

using namespace benchmark;

// ============================================================================
// 基准测试：简单规则 + 小数据
// ============================================================================

static void BM_LuaJIT_SimpleRule_SmallData(benchmark::State& state) {
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

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.dump().size());
}

static void BM_Native_SimpleRule_SmallData(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_simple_json();

    for (auto _ : state) {
        std::string message;
        bool matched = NativeAgeCheckRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.dump().size());
}

// ============================================================================
// 基准测试：中等规则 + 中数据
// ============================================================================

static void BM_LuaJIT_MediumRule_MediumData(benchmark::State& state) {
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

static void BM_Native_MediumRule_MediumData(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

    for (auto _ : state) {
        std::string message;
        bool matched = NativeUserValidationRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.dump().size());
}

// ============================================================================
// 基准测试：复杂规则 + 大数据
// ============================================================================

static void BM_LuaJIT_ComplexRule_LargeData(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Complex);

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Large);

    // 准备复杂规则所需的数据
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

static void BM_Native_ComplexRule_LargeData(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Large);

    // 准备复杂规则所需的数据
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
        std::string message;
        bool matched = NativeRiskControlRule::match(data, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.dump().size());
}

// ============================================================================
// 基准测试：超复杂规则 + 超大数据
// ============================================================================

static void BM_LuaJIT_UltraComplexRule_XLargeData(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::UltraComplex);

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::XLarge);

    // 准备超复杂规则所需的数据
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

static void BM_Native_UltraComplexRule_XLargeData(benchmark::State& state) {
    DataGenerator generator;
    auto data = generator.generate_data(DataSize::XLarge);

    // 准备超复杂规则所需的数据
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

    state.SetItemsProcessed(state.iterations());
    state.SetBytesProcessed(state.iterations() * data.dump().size());
}

// ============================================================================
// JIT 开启/关闭对比测试
// ============================================================================

static void BM_LuaJIT_JIT_On_SimpleRule(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);
    engine.enable_jit();

    DataGenerator generator;
    auto data = generator.generate_simple_json();

    for (auto _ : state) {
        bool matched = false;
        std::string message;
        engine.match_rule(data, matched, message);
        benchmark::DoNotOptimize(matched);
        benchmark::DoNotOptimize(message);
    }

    state.SetItemsProcessed(state.iterations());
}

static void BM_LuaJIT_JIT_Off_SimpleRule(benchmark::State& state) {
    RuleEngineWrapper engine;
    engine.initialize(RuleComplexity::Simple);
    engine.disable_jit();

    DataGenerator generator;
    auto data = generator.generate_simple_json();

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
// 批量规则匹配测试
// ============================================================================

static void BM_LuaJIT_MatchAllRules(benchmark::State& state) {
    ljre::RuleEngine engine;

    // 加载多个规则
    engine.add_rule("simple_age_check", "../benchmarks/src/rules/simple_age_check.lua");
    engine.add_rule("medium_validation", "../benchmarks/src/rules/medium_validation.lua");

    DataGenerator generator;
    auto data = generator.generate_data(DataSize::Medium);

    for (auto _ : state) {
        std::map<std::string, ljre::MatchResult> results;
        engine.match_all_rules(ljre::JsonAdapter(data), results);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations() * 2);  // 2个规则
}

// ============================================================================
// 注册所有基准测试
// ============================================================================

BENCHMARK(BM_LuaJIT_SimpleRule_SmallData)
    ->Name("LuaJIT_SimpleRule_SmallData")
    ->Iterations(1000000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Native_SimpleRule_SmallData)
    ->Name("Native_SimpleRule_SmallData")
    ->Iterations(1000000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LuaJIT_MediumRule_MediumData)
    ->Name("LuaJIT_MediumRule_MediumData")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Native_MediumRule_MediumData)
    ->Name("Native_MediumRule_MediumData")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LuaJIT_ComplexRule_LargeData)
    ->Name("LuaJIT_ComplexRule_LargeData")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Native_ComplexRule_LargeData)
    ->Name("Native_ComplexRule_LargeData")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LuaJIT_UltraComplexRule_XLargeData)
    ->Name("LuaJIT_UltraComplexRule_XLargeData")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_Native_UltraComplexRule_XLargeData)
    ->Name("Native_UltraComplexRule_XLargeData")
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LuaJIT_JIT_On_SimpleRule)
    ->Name("LuaJIT_JIT_On")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LuaJIT_JIT_Off_SimpleRule)
    ->Name("LuaJIT_JIT_Off")
    ->Iterations(100000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK(BM_LuaJIT_MatchAllRules)
    ->Name("LuaJIT_MatchAllRules")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// 运行所有基准测试
BENCHMARK_MAIN();
