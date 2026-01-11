#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <random>
#include "benchmark_common.h"

namespace benchmark {

// 测试数据生成器
class DataGenerator {
public:
    DataGenerator();
    ~DataGenerator() = default;

    // 生成指定规模的测试数据
    nlohmann::json generate_data(DataSize size);

    // 生成简单 JSON（用于简单规则测试）
    nlohmann::json generate_simple_json();

    // 生成嵌套 JSON（用于嵌套访问测试）
    nlohmann::json generate_nested_json(int depth = 3);

    // 生成数组 JSON（用于数组遍历测试）
    nlohmann::json generate_array_json(size_t length);

    // 生成混合复杂 JSON
    nlohmann::json generate_complex_json(DataSize size);

    // 生成特定字段数量的 JSON
    nlohmann::json generate_json_with_fields(int field_count);

    // 生成随机字符串
    std::string generate_random_string(size_t length = 10);

    // 生成随机整数
    int generate_random_int(int min = 0, int max = 100);

    // 生成随机浮点数
    double generate_random_double(double min = 0.0, double max = 100.0);

    // 生成随机布尔值
    bool generate_random_bool();

    // 生成随机邮箱
    std::string generate_random_email();

    // 生成随机手机号
    std::string generate_random_phone();

    // 生成随机地址
    nlohmann::json generate_random_address();

    // 设置随机种子
    void set_seed(unsigned int seed) { _rng.seed(seed); }

private:
    std::mt19937 _rng;

    // 辅助函数：生成随机字段名
    std::string generate_field_name(int index);

    // 辅助函数：生成随机的嵌套结构
    nlohmann::json generate_nested_structure(int current_depth, int max_depth);
};

// 批量数据生成器（用于压力测试）
class BatchDataGenerator {
public:
    BatchDataGenerator();
    ~BatchDataGenerator() = default;

    // 生成一批测试数据
    std::vector<nlohmann::json> generate_batch(DataSize size, size_t batch_size);

    // 生成可变大小的数据批次（用于扩展性测试）
    std::vector<nlohmann::json> generate_variable_batch(
        const std::vector<DataSize>& sizes);

private:
    DataGenerator _generator;
};

} // namespace benchmark
