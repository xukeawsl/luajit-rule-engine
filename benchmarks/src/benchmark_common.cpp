#include "benchmark_common.h"
#include <cstdio>
#include <cmath>

namespace benchmark {

// 规则引擎包装器实现
RuleEngineWrapper::RuleEngineWrapper() {
}

bool RuleEngineWrapper::initialize(RuleComplexity complexity, const std::string& rule_file) {
    std::string file = rule_file;
    if (file.empty()) {
        file = get_rule_file(complexity);
    }

    std::string error_msg;
    std::string rule_name = rule_complexity_to_string(complexity);

    if (!_engine.add_rule(rule_name, file, &error_msg)) {
        fprintf(stderr, "Failed to load rule: %s\n", error_msg.c_str());
        return false;
    }

    _current_rule_name = rule_name;
    return true;
}

bool RuleEngineWrapper::match_rule(const nlohmann::json& data, bool& matched, std::string& message) {
    ljre::JsonAdapter adapter(data);
    ljre::MatchResult result;

    std::string error_msg;
    if (!_engine.match_rule(_current_rule_name, adapter, result, &error_msg)) {
        fprintf(stderr, "Failed to match rule: %s\n", error_msg.c_str());
        return false;
    }

    matched = result.matched;
    message = result.message;
    return true;
}

bool RuleEngineWrapper::enable_jit() {
    return _engine.enable_jit();
}

bool RuleEngineWrapper::disable_jit() {
    return _engine.disable_jit();
}

bool RuleEngineWrapper::flush_jit() {
    return _engine.flush_jit();
}

// 辅助函数实现
const char* data_size_to_string(DataSize size) {
    switch (size) {
        case DataSize::Small:     return "Small";
        case DataSize::Medium:    return "Medium";
        case DataSize::Large:     return "Large";
        case DataSize::XLarge:    return "XLarge";
        default:                  return "Unknown";
    }
}

const char* rule_complexity_to_string(RuleComplexity complexity) {
    switch (complexity) {
        case RuleComplexity::Simple:        return "simple_age_check";
        case RuleComplexity::Medium:        return "medium_validation";
        case RuleComplexity::Complex:       return "complex_risk_control";
        case RuleComplexity::UltraComplex:  return "ultra_complex";
        default:                            return "unknown";
    }
}

std::string get_rule_file(RuleComplexity complexity) {
    // 返回规则文件路径
    // 由于可执行文件在 build/benchmarks/ 目录，需要相对路径回到项目根目录
    switch (complexity) {
        case RuleComplexity::Simple:
            return "../benchmarks/src/rules/simple_age_check.lua";
        case RuleComplexity::Medium:
            return "../benchmarks/src/rules/medium_validation.lua";
        case RuleComplexity::Complex:
            return "../benchmarks/src/rules/complex_risk_control.lua";
        case RuleComplexity::UltraComplex:
            return "../benchmarks/src/rules/ultra_complex.lua";
        default:
            return "";
    }
}

void print_test_config(DataSize size, RuleComplexity complexity, size_t iterations) {
    printf("\n========================================\n");
    printf("测试配置:\n");
    printf("  数据规模:     %s\n", data_size_to_string(size));
    printf("  规则复杂度:   %s\n", rule_complexity_to_string(complexity));
    printf("  迭代次数:     %zu\n", iterations);
    printf("========================================\n\n");
}

} // namespace benchmark
