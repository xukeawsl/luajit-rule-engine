// UltraComplex 规则性能分析工具
// 用于详细分析 LuaJIT vs Native C++ 的性能差距

#include <iostream>
#include <chrono>
#include <iomanip>
#include "data_generator.h"
#include "native_rules.h"
#include <ljre/rule_engine.h>

using namespace benchmark;

// 性能计时器
class PerfTimer {
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }

    double elapsed_us() {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
    }

    double elapsed_ns() {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time;
};

void analyze_ultra_complex() {
    std::cout << "\n========================================\n";
    std::cout << "UltraComplex 规则性能分析\n";
    std::cout << "========================================\n\n";

    // 1. 准备测试数据
    std::cout << "1. 生成测试数据...\n";
    DataGenerator generator;
    PerfTimer timer;

    timer.start();
    auto data = generator.generate_data(DataSize::XLarge);
    double gen_time = timer.elapsed_us();
    std::cout << "   数据生成耗时: " << gen_time << " μs\n";
    std::cout << "   数据大小: " << data.dump().size() << " bytes\n";

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

    // 2. 测试 Native C++ 性能
    std::cout << "\n2. 测试 Native C++ 性能...\n";
    const int warmup = 100;
    const int iterations = 10000;

    // 预热
    for (int i = 0; i < warmup; i++) {
        std::string message;
        NativeComprehensiveRule::match(data, message);
    }

    // 正式测试
    timer.start();
    for (int i = 0; i < iterations; i++) {
        std::string message;
        bool matched = NativeComprehensiveRule::match(data, message);
    }
    double native_time = timer.elapsed_us();
    double native_avg_ns = native_time * 1000.0 / iterations;

    std::cout << "   总耗时: " << native_time << " μs (" << iterations << " 次迭代)\n";
    std::cout << "   平均耗时: " << std::fixed << std::setprecision(2) << native_avg_ns << " ns/op\n";
    std::cout << "   吞吐量: " << std::fixed << std::setprecision(0) << (1e9 / native_avg_ns) << " ops/s\n";

    // 3. 测试 LuaJIT 性能 - 分阶段分析
    std::cout << "\n3. 测试 LuaJIT 性能（分阶段）...\n";

    ljre::RuleEngine engine;

    // 3.1 规则加载时间
    timer.start();
    engine.add_rule("ultra_complex", "../benchmarks/src/rules/ultra_complex.lua");
    double load_time = timer.elapsed_us();
    std::cout << "\n   3.1 规则加载耗时: " << load_time << " μs\n";

    // 3.2 JsonAdapter 创建时间
    timer.start();
    ljre::JsonAdapter adapter(data);
    double adapter_time = timer.elapsed_us() * 1000.0;  // 转换为 ns
    std::cout << "   3.2 JsonAdapter 创建耗时: " << std::fixed << std::setprecision(2) << adapter_time << " ns\n";

    // 预热
    for (int i = 0; i < warmup; i++) {
        ljre::MatchResult result;
        engine.match_rule("ultra_complex", adapter, result);
    }

    // 正式测试
    timer.start();
    for (int i = 0; i < iterations; i++) {
        ljre::MatchResult result;
        engine.match_rule("ultra_complex", adapter, result);
    }
    double luajit_time = timer.elapsed_us();
    double luajit_avg_ns = luajit_time * 1000.0 / iterations;

    std::cout << "\n   3.3 LuaJIT 执行耗时:\n";
    std::cout << "       总耗时: " << luajit_time << " μs (" << iterations << " 次迭代)\n";
    std::cout << "       平均耗时: " << std::fixed << std::setprecision(2) << luajit_avg_ns << " ns/op\n";
    std::cout << "       吞吐量: " << std::fixed << std::setprecision(0) << (1e9 / luajit_avg_ns) << " ops/s\n";

    // 4. 性能对比分析
    std::cout << "\n========================================\n";
    std::cout << "性能对比分析\n";
    std::cout << "========================================\n\n";

    double slowdown = luajit_avg_ns / native_avg_ns;
    std::cout << "LuaJIT vs Native 性能比率: " << std::fixed << std::setprecision(2) << slowdown << "x\n";
    std::cout << "（LuaJIT 比 Native 慢 " << std::fixed << std::setprecision(1) << ((slowdown - 1) * 100) << "%）\n\n";

    // 5. 瓶颈分析
    std::cout << "瓶颈分析:\n\n";

    double adapter_overhead_pct = (adapter_time / luajit_avg_ns) * 100;
    std::cout << "1. 数据转换开销 (JsonAdapter):\n";
    std::cout << "   耗时: " << std::fixed << std::setprecision(2) << adapter_time << " ns\n";
    std::cout << "   占比: " << std::fixed << std::setprecision(1) << adapter_overhead_pct << "%\n\n";

    double pure_lua_time = luajit_avg_ns - adapter_time;
    std::cout << "2. 纯 Lua 执行时间:\n";
    std::cout << "   耗时: " << std::fixed << std::setprecision(2) << pure_lua_time << " ns\n";
    std::cout << "   占比: " << std::fixed << std::setprecision(1) << (100 - adapter_overhead_pct) << "%\n\n";

    std::cout << "3. Lua 代码特性分析:\n";
    std::cout << "   • 深度嵌套访问: data.user.profile.education (3层)\n";
    std::cout << "   • 大量 nil 检查: 每个字段访问前都检查 nil\n";
    std::cout << "   • 字符串比较: 多次字符串相等比较\n";
    std::cout << "   • 浮点运算: 多次浮点数乘法\n";
    std::cout << "   • 格式化输出: string.format 调用\n\n";

    std::cout << "4. 性能差距主要原因:\n\n";
    std::cout << "   a) Lua 表访问开销:\n";
    std::cout << "      - 每次嵌套访问 (data.user.profile.education) 需要 3 次哈希查找\n";
    std::cout << "      - Native C++ 直接内存访问，无哈希查找\n\n";

    std::cout << "   b) 字符串操作开销:\n";
    std::cout << "      - Lua 字符串比较比 C++ 慢\n";
    std::cout << "      - string.format 格式化开销较大\n\n";

    std::cout << "   c) 数据类型检查:\n";
    std::cout << "      - Lua 中每次访问前检查 nil\n";
    std::cout << "      - Native C++ 编译时类型检查，运行时无开销\n\n";

    std::cout << "   d) JIT 编译限制:\n";
    std::cout << "      - 复杂嵌套逻辑可能无法完全 JIT 编译\n";
    std::cout << "      - 部分代码回退到解释器执行\n\n";

    // 6. 优化建议
    std::cout << "========================================\n";
    std::cout << "优化建议\n";
    std::cout << "========================================\n\n";

    std::cout << "1. 减少嵌套深度:\n";
    std::cout << "   将 data.user.profile.education 改为扁平结构\n";
    std::cout << "   如: data.user_education\n\n";

    std::cout << "2. 预处理数据:\n";
    std::cout << "   在 C++ 端预处理数据，减少 Lua 中的 nil 检查\n";
    std::cout << "   保证数据结构完整\n\n";

    std::cout << "3. 使用局部变量缓存:\n";
    std::cout << "   local user = data.user\n";
    std::cout << "   local profile = user.profile\n";
    std::cout << "   减少重复的表访问\n\n";

    std::cout << "4. 优化字符串比较:\n";
    std::cout << "   使用枚举或整数代替字符串比较\n";
    std::cout << "   如: education = 1 (university), 2 (college)\n\n";

    std::cout << "5. 减少格式化输出:\n";
    std::cout << "   只在必要时调用 string.format\n";
    std::cout << "   考虑使用字符串拼接\n\n";

    std::cout << "6. 拆分复杂规则:\n";
    std::cout << "   将超复杂规则拆分为多个简单规则\n";
    std::cout << "   每个规则专注一个方面\n\n";

    std::cout << "========================================\n\n";
}

int main() {
    analyze_ultra_complex();
    return 0;
}
