#include <benchmark/benchmark.h>
#include <ljre/rule_engine.h>
#include <ljre/json_adapter.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <memory>

using namespace benchmark;
using json = nlohmann::json;

// ============================================================================
// 辅助函数：创建测试规则文件
// ============================================================================

static void ensure_test_dir() {
    system("mkdir -p test_data/bench_rules");
}

static std::string create_temp_rule_file(const std::string& name, const std::string& content) {
    ensure_test_dir();
    std::string path = "test_data/bench_rules/" + name + ".lua";
    std::ofstream file(path);
    file << content;
    file.close();
    return path;
}

static std::vector<std::string> create_rule_files(int count, const std::string& content_template) {
    std::vector<std::string> paths;
    for (int i = 0; i < count; ++i) {
        std::string content = content_template;
        // 替换模板中的 {index} 为实际索引
        size_t pos = 0;
        while ((pos = content.find("{index}", pos)) != std::string::npos) {
            content.replace(pos, 7, std::to_string(i));
            pos += 1;
        }
        paths.push_back(create_temp_rule_file("rule_" + std::to_string(i), content));
    }
    return paths;
}

// 在程序结束时清理测试文件
struct TestDirCleanup {
    ~TestDirCleanup() {
        system("rm -rf test_data");
    }
};

static TestDirCleanup g_cleanup;

// ============================================================================
// 基准测试：不同规模规则数量的克隆性能
// ============================================================================

static void BM_Clone_10Rules(benchmark::State& state) {
    const int rule_count = 10;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("10 rules");

    delete engine;
}

static void BM_Clone_50Rules(benchmark::State& state) {
    const int rule_count = 50;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("50 rules");

    delete engine;
}

static void BM_Clone_100Rules(benchmark::State& state) {
    const int rule_count = 100;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("100 rules");

    delete engine;
}

static void BM_Clone_200Rules(benchmark::State& state) {
    const int rule_count = 200;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("200 rules");

    delete engine;
}

static void BM_Clone_500Rules(benchmark::State& state) {
    const int rule_count = 500;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("500 rules");

    delete engine;
}

static void BM_Clone_1000Rules(benchmark::State& state) {
    const int rule_count = 1000;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("1000 rules");

    delete engine;
}

static void BM_Clone_5000Rules(benchmark::State& state) {
    const int rule_count = 5000;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("5000 rules");

    delete engine;
}

static void BM_Clone_10000Rules(benchmark::State& state) {
    const int rule_count = 10000;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("10000 rules");

    delete engine;
}

// ============================================================================
// 基准测试：克隆复杂规则
// ============================================================================

static void BM_Clone_ComplexRules_100(benchmark::State& state) {
    const int rule_count = 100;
    std::string complex_rule = R"(
        function match(data)
            local score = 0
            if data.age and data.age >= 18 then
                score = score + 10
            end
            if data.value and data.value > 100 then
                score = score + 20
            end
            if data.name and #data.name > 0 then
                score = score + 5
            end
            if data.active == true then
                score = score + 15
            end
            local result = score >= 30
            return result, "rule_{index} score=" .. score
        end
    )";
    auto rule_paths = create_rule_files(rule_count, complex_rule);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "complex_rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("100 complex rules");

    delete engine;
}

// ============================================================================
// 基准测试：克隆包含 Lua 公共文件的引擎
// ============================================================================

static void BM_Clone_WithLuaFiles(benchmark::State& state) {
    const int lua_file_count = 10;
    const int rule_count = 100;

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;

    for (int i = 0; i < lua_file_count; ++i) {
        std::string lua_content = R"(
            utils = utils or {}
            function utils.helper_)" + std::to_string(i) + R"((data)
                return data.value + )" + std::to_string(i) + R"(
            end
        )";
        std::string path = create_temp_rule_file("utils_" + std::to_string(i), lua_content);
        engine->add_lua_file(path, &error);
    }

    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("100 rules + 10 Lua files");

    delete engine;
}

// ============================================================================
// 基准测试：克隆包含 C++ 函数的引擎
// ============================================================================

static int test_function(lua_State* L) {
    lua_pushnumber(L, 42);
    return 1;
}

static void BM_Clone_WithCppFunctions(benchmark::State& state) {
    const int cpp_func_count = 50;
    const int rule_count = 100;

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;

    for (int i = 0; i < cpp_func_count; ++i) {
        std::string func_name = "cpp_func_" + std::to_string(i);
        engine->register_function(func_name, test_function, &error);
    }

    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::ALL, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("100 rules + 50 C++ functions");

    delete engine;
}

// ============================================================================
// 基准测试：不同克隆选项的性能对比
// ============================================================================

static void BM_Clone_OnlyRules_200(benchmark::State& state) {
    const int rule_count = 200;
    std::string rule_content = R"(
        function match(data)
            return data.value > 0, "rule_{index} matched"
        end
    )";
    auto rule_paths = create_rule_files(rule_count, rule_content);

    ljre::RuleEngine* engine = new ljre::RuleEngine();
    std::string error;
    for (int i = 0; i < rule_count; ++i) {
        std::string rule_name = "rule_" + std::to_string(i);
        engine->add_rule(rule_name, rule_paths[i], &error);
    }

    for (auto _ : state) {
        std::string clone_error;
        auto cloned = engine->clone(ljre::RuleEngine::RULES, &clone_error);
        benchmark::DoNotOptimize(cloned);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("200 rules (RULES only)");

    delete engine;
}

// ============================================================================
// 注册所有基准测试
// ============================================================================

BENCHMARK(BM_Clone_10Rules)
    ->Name("Clone_10Rules")
    ->Iterations(1000)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_50Rules)
    ->Name("Clone_50Rules")
    ->Iterations(500)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_100Rules)
    ->Name("Clone_100Rules")
    ->Iterations(200)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_200Rules)
    ->Name("Clone_200Rules")
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_500Rules)
    ->Name("Clone_500Rules")
    ->Iterations(50)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_1000Rules)
    ->Name("Clone_1000Rules")
    ->Iterations(20)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_5000Rules)
    ->Name("Clone_5000Rules")
    ->Iterations(10)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_10000Rules)
    ->Name("Clone_10000Rules")
    ->Iterations(5)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_ComplexRules_100)
    ->Name("Clone_ComplexRules_100")
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_WithLuaFiles)
    ->Name("Clone_WithLuaFiles")
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_WithCppFunctions)
    ->Name("Clone_WithCppFunctions")
    ->Iterations(100)
    ->Unit(benchmark::kMillisecond);

BENCHMARK(BM_Clone_OnlyRules_200)
    ->Name("Clone_OnlyRules_200")
    ->Iterations(200)
    ->Unit(benchmark::kMillisecond);

// 运行所有基准测试
BENCHMARK_MAIN();
