#include "data_generator.h"
#include <sstream>
#include <iomanip>
#include <random>

namespace benchmark {

DataGenerator::DataGenerator()
    : _rng(std::random_device{}())
{
}

nlohmann::json DataGenerator::generate_data(DataSize size) {
    switch (size) {
        case DataSize::Small:
            return generate_simple_json();
        case DataSize::Medium:
            return generate_json_with_fields(15);
        case DataSize::Large:
            return generate_json_with_fields(75);
        case DataSize::XLarge:
            return generate_json_with_fields(350);
        default:
            return generate_simple_json();
    }
}

nlohmann::json DataGenerator::generate_simple_json() {
    nlohmann::json data;
    data["age"] = generate_random_int(18, 65);
    data["email"] = generate_random_email();
    data["name"] = generate_random_string(8);
    data["phone"] = generate_random_phone();
    return data;
}

nlohmann::json DataGenerator::generate_nested_json(int depth) {
    if (depth <= 0) {
        nlohmann::json leaf;
        leaf["value"] = generate_random_int();
        return leaf;
    }

    nlohmann::json obj;
    obj["field1"] = generate_random_int();
    obj["nested"] = generate_nested_json(depth - 1);
    obj["array"] = nlohmann::json::array();
    for (int i = 0; i < 3; ++i) {
        obj["array"].push_back(generate_random_int());
    }

    return obj;
}

nlohmann::json DataGenerator::generate_array_json(size_t length) {
    nlohmann::json data;
    data["items"] = nlohmann::json::array();

    for (size_t i = 0; i < length; ++i) {
        nlohmann::json item;
        item["id"] = static_cast<int>(i);
        item["value"] = generate_random_int();
        item["score"] = generate_random_double();
        data["items"].push_back(item);
    }

    return data;
}

nlohmann::json DataGenerator::generate_complex_json(DataSize size) {
    nlohmann::json data;

    // 基础字段
    data["id"] = generate_random_int(1000, 9999);
    data["name"] = generate_random_string(10);
    data["age"] = generate_random_int(18, 70);

    // 嵌套对象
    data["address"] = generate_random_address();

    // 数组
    size_t array_length = 5;
    switch (size) {
        case DataSize::Medium:   array_length = 20; break;
        case DataSize::Large:    array_length = 100; break;
        case DataSize::XLarge:   array_length = 500; break;
        default:                 array_length = 5; break;
    }

    data["scores"] = nlohmann::json::array();
    for (size_t i = 0; i < array_length; ++i) {
        data["scores"].push_back(generate_random_int(60, 100));
    }

    // 嵌套深度根据数据规模调整
    int depth = 2;
    switch (size) {
        case DataSize::Medium:   depth = 3; break;
        case DataSize::Large:    depth = 4; break;
        case DataSize::XLarge:   depth = 5; break;
        default:                 depth = 2; break;
    }

    data["nested"] = generate_nested_json(depth);

    // 添加更多字段
    int extra_fields = 0;
    switch (size) {
        case DataSize::Medium:   extra_fields = 10; break;
        case DataSize::Large:    extra_fields = 50; break;
        case DataSize::XLarge:   extra_fields = 200; break;
        default:                 extra_fields = 0; break;
    }

    for (int i = 0; i < extra_fields; ++i) {
        std::string key = "field_" + std::to_string(i);
        int type = generate_random_int(0, 2);
        switch (type) {
            case 0:
                data[key] = generate_random_int();
                break;
            case 1:
                data[key] = generate_random_string();
                break;
            case 2:
                data[key] = generate_random_double();
                break;
        }
    }

    return data;
}

nlohmann::json DataGenerator::generate_json_with_fields(int field_count) {
    nlohmann::json data;

    // 常用字段
    data["id"] = generate_random_int(1000, 99999);
    data["name"] = generate_random_string(10);
    data["age"] = generate_random_int(18, 70);
    data["email"] = generate_random_email();
    data["phone"] = generate_random_phone();
    data["active"] = generate_random_bool();

    // 地址信息
    data["address"] = generate_random_address();

    // 计算剩余需要的字段数
    int remaining_fields = field_count - 7;

    // 生成剩余字段
    for (int i = 0; i < remaining_fields; ++i) {
        std::string key = generate_field_name(i);
        int type = generate_random_int(0, 4);

        switch (type) {
            case 0:
                data[key] = generate_random_int();
                break;
            case 1:
                data[key] = generate_random_double();
                break;
            case 2:
                data[key] = generate_random_string();
                break;
            case 3:
                data[key] = generate_random_bool();
                break;
            case 4:
                // 小型数组
                data[key] = nlohmann::json::array();
                for (int j = 0; j < 3; ++j) {
                    data[key].push_back(generate_random_int());
                }
                break;
        }
    }

    return data;
}

std::string DataGenerator::generate_random_string(size_t length) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    std::uniform_int_distribution<int> dist(0, sizeof(charset) - 2);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(_rng)];
    }

    return result;
}

int DataGenerator::generate_random_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(_rng);
}

double DataGenerator::generate_random_double(double min, double max) {
    std::uniform_real_distribution<double> dist(min, max);
    return dist(_rng);
}

bool DataGenerator::generate_random_bool() {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(_rng) == 1;
}

std::string DataGenerator::generate_random_email() {
    std::string username = generate_random_string(8);
    std::string domain = generate_random_string(6);

    static const char* tlds[] = {".com", ".org", ".net", ".edu", ".cn"};
    std::uniform_int_distribution<int> dist(0, 4);
    const char* tld = tlds[dist(_rng)];

    return username + "@" + domain + tld;
}

std::string DataGenerator::generate_random_phone() {
    std::string phone = "1";
    std::uniform_int_distribution<int> dist_3(3, 9);
    phone += std::to_string(dist_3(_rng));

    std::uniform_int_distribution<int> dist_0(0, 9);
    for (int i = 0; i < 9; ++i) {
        phone += std::to_string(dist_0(_rng));
    }

    return phone;
}

nlohmann::json DataGenerator::generate_random_address() {
    nlohmann::json address;
    address["street"] = generate_random_string(10) + " Street";
    address["city"] = generate_random_string(8) + " City";
    address["state"] = generate_random_string(2);
    address["zip"] = generate_random_string(5);
    address["country"] = "China";

    return address;
}

std::string DataGenerator::generate_field_name(int index) {
    static const char* prefixes[] = {
        "field", "attr", "prop", "item", "value",
        "data", "info", "param", "arg", "var"
    };
    std::uniform_int_distribution<int> dist(0, 9);
    const char* prefix = prefixes[dist(_rng)];

    return std::string(prefix) + "_" + std::to_string(index);
}

nlohmann::json DataGenerator::generate_nested_structure(int current_depth, int max_depth) {
    nlohmann::json obj;

    if (current_depth >= max_depth) {
        obj["leaf_value"] = generate_random_int();
        return obj;
    }

    // 添加一些基本字段
    obj["level"] = current_depth;
    obj["value"] = generate_random_int();
    obj["name"] = generate_random_string(5);

    // 添加嵌套对象
    obj["child"] = generate_nested_structure(current_depth + 1, max_depth);

    // 添加数组
    obj["items"] = nlohmann::json::array();
    for (int i = 0; i < 3; ++i) {
        nlohmann::json item;
        item["index"] = i;
        item["value"] = generate_random_double();
        obj["items"].push_back(item);
    }

    return obj;
}

// 批量数据生成器实现
BatchDataGenerator::BatchDataGenerator()
    : _generator()
{
}

std::vector<nlohmann::json> BatchDataGenerator::generate_batch(
    DataSize size, size_t batch_size) {
    std::vector<nlohmann::json> batch;
    batch.reserve(batch_size);

    for (size_t i = 0; i < batch_size; ++i) {
        batch.push_back(_generator.generate_data(size));
    }

    return batch;
}

std::vector<nlohmann::json> BatchDataGenerator::generate_variable_batch(
    const std::vector<DataSize>& sizes) {
    std::vector<nlohmann::json> batch;
    batch.reserve(sizes.size());

    for (auto size : sizes) {
        batch.push_back(_generator.generate_data(size));
    }

    return batch;
}

} // namespace benchmark
