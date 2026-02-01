#pragma once

#include "rule_engine.h"
#include <memory>
#include <atomic>
#include <thread>

namespace ljre {

/**
 * @brief 规则引擎包装器，支持多线程安全访问和热更新
 *
 * 设计模式：
 * - Copy-on-Write: 模板引擎使用 shared_ptr + 原子操作
 * - Thread-Local Storage: 每个线程维护独立的引擎副本
 * - Versioning: 通过版本号判断是否需要重新克隆
 * - Shared Ownership: 使用 shared_ptr 确保旧引擎在被使用时不会被销毁
 *
 * 适用场景：
 * - 多线程服务（如 brpc）
 * - 支持热更新配置
 * - 引擎本身非线程安全
 *
 * 线程安全性保证：
 * - get_engine(): 无锁，返回 shared_ptr<const RuleEngine>
 * - set_template_engine(): 使用原子操作，无阻塞
 * - 热更新不影响正在使用的请求
 * - 旧引擎会在所有引用释放后自动销毁
 *
 * 使用示例：
 * @code
 * // 全局包装器实例
 * RuleEngineWrapper g_engine_wrapper;
 *
 * // 初始化引擎（程序启动时）
 * void init_engine() {
 *     auto engine = std::make_shared<RuleEngine>();
 *     std::string error;
 *     engine->load_rule_config("config.lua", &error);
 *     engine->add_lua_file("utils.lua", &error);
 *     engine->register_function("my_func", my_func_ptr, &error);
 *     g_engine_wrapper.set_template_engine(engine);
 * }
 *
 * // 热加载线程
 * void hot_reload_thread() {
 *     while (running) {
 *         wait_for_config_change();
 *
 *         // 创建新的引擎实例
 *         auto new_engine = std::make_shared<RuleEngine>();
 *         std::string error;
 *
 *         // 加载新配置
 *         new_engine->load_rule_config("new_config.lua", &error);
 *         new_engine->add_lua_file("new_utils.lua", &error);
 *         new_engine->register_function("new_func", new_func_ptr, &error);
 *
 *         // 原子替换模板引擎（其他线程会在下次调用时自动克隆）
 *         g_engine_wrapper.set_template_engine(new_engine);
 *     }
 * }
 *
 * // brpc 服务处理
 * void YourService::ProcessRequest(const HttpRequest& req, HttpResponse* resp) {
 *     // 获取当前线程的引擎（自动处理版本更新和克隆）
 *     auto engine = g_engine_wrapper.get_engine();
 *
 *     // 使用引擎进行规则判断
 *     JsonAdapter adapter(req_json);
 *     MatchResult result;
 *
 *     if (engine->match_rule("rule1", adapter, result)) {
 *         if (result.matched) {
 *             resp->set_body("Matched: " + result.message);
 *         } else {
 *             resp->set_body("Not matched: " + result.message);
 *         }
 *     }
 * }
 * @endcode
 */
class RuleEngineWrapper {
public:
    RuleEngineWrapper() = default;
    ~RuleEngineWrapper() = default;

    // 禁止拷贝和移动
    RuleEngineWrapper(const RuleEngineWrapper&) = delete;
    RuleEngineWrapper& operator=(const RuleEngineWrapper&) = delete;
    RuleEngineWrapper(RuleEngineWrapper&&) = delete;
    RuleEngineWrapper& operator=(RuleEngineWrapper&&) = delete;

    /**
     * @brief 设置新的模板引擎（线程安全）
     *
     * 使用场景：热加载线程调用此方法更新配置
     *
     * 线程安全性：
     * - 使用 atomic_store 原子替换指针
     * - 旧的模板引擎会在所有引用释放后自动销毁
     * - 其他线程可能仍在使用旧的模板引擎克隆的副本
     * - 正在处理的请求不会被打断
     *
     * @param engine 新的模板引擎（使用 shared_ptr 管理生命周期）
     */
    void set_template_engine(std::shared_ptr<RuleEngine> engine) {
        // 原子存储新的模板引擎
        std::atomic_store(&_template_engine, std::move(engine));

        // 更新版本号（通知其他线程模板已更新）
        _global_version.fetch_add(1, std::memory_order_release);
    }

    /**
     * @brief 获取当前线程的引擎实例（线程安全）
     *
     * 使用场景：brpc 处理线程调用此方法获取引擎
     *
     * 工作流程：
     * 1. 检查线程本地引擎的版本号
     * 2. 如果版本过期，从模板引擎重新克隆
     * 3. 返回 shared_ptr<const RuleEngine>
     *
     * 线程安全性：
     * - 每个线程有独立的引擎实例，无需加锁
     * - 只在版本过期时执行一次原子操作
     * - 热更新不会影响正在使用的 shared_ptr
     *
     * @shared_ptr 安全性保证：
     * - 返回 shared_ptr 的拷贝，引用计数+1
     * - 即使发生热更新，旧的 shared_ptr 仍然有效
     * - 旧引擎会在所有 shared_ptr 引用释放后自动销毁
     * - 可以安全地保存 shared_ptr 供后续使用（虽然不推荐）
     *
     * @warning 推荐用法：
     * @code
     * // ✅ 推荐：每次请求都获取最新的引擎
     * void handle_request(const Request& req) {
     *     auto engine = wrapper.get_engine();  // 获取最新版本
     *     engine->match_rule(...);
     * }  // engine 在请求结束时自动释放
     *
     * // ✅ 可接受：在同一次请求中多次使用
     * void handle_request(const Request& req) {
     *     auto engine1 = wrapper.get_engine();
     *     // ... 热更新发生 ...
     *     auto engine2 = wrapper.get_engine();  // 获取新版本
     *
     *     engine1->match_rule(...);  // ✅ 使用旧版本，仍然有效
     *     engine2->match_rule(...);  // ✅ 使用新版本
     * }
     *
     * // ⚠️ 不推荐但安全：保存 shared_ptr 供后续使用
     * std::shared_ptr<const RuleEngine> cached = wrapper.get_engine();
     * // ... 后续请求
     * cached->match_rule(...);  // ✅ 安全，但使用的是旧版本
     * @endcode
     *
     * @note const 正确性保证：
     * - 返回 shared_ptr<const RuleEngine>，只能调用 const 方法
     * - 允许调用：match_rule, match_all_rules, has_rule, get_all_rules, get_rule_count
     * - 禁止调用：add_rule, remove_rule, load_rule_config, register_function, clear_rules
     *
     * @return shared_ptr<const RuleEngine> 线程本地引擎的智能指针（保证非空）
     */
    std::shared_ptr<const RuleEngine> get_engine() {
        // 获取当前线程的本地存储
        ThreadLocalEngine* local = get_thread_local();

        // 获取当前全局版本号
        uint64_t current_version = _global_version.load(std::memory_order_acquire);

        // 检查版本是否需要更新
        if (local->version != current_version) {
            // 原子加载模板引擎的 shared_ptr（增加引用计数）
            std::shared_ptr<RuleEngine> template_engine =
                std::atomic_load(&_template_engine);

            // 如果模板引擎存在，克隆到线程本地
            if (template_engine) {
                std::string error;
                // unique_ptr 转换为 shared_ptr
                local->engine = template_engine->clone(RuleEngine::CloneOption::ALL, &error);
                local->version = current_version;
            }
        }

        // 返回 shared_ptr 的拷贝（引用计数+1，确保调用者持有有效引用）
        return local->engine;
    }

    /**
     * @brief 获取当前全局版本号
     *
     * 版本号在每次 set_template_engine() 时递增
     *
     * @return uint64_t 当前版本号
     */
    uint64_t get_version() const {
        return _global_version.load(std::memory_order_acquire);
    }

private:
    // 线程本地引擎条目
    struct ThreadLocalEngine {
        std::shared_ptr<RuleEngine> engine;  // 使用 shared_ptr 管理生命周期
        uint64_t version = 0;  // 当前引擎的版本号
    };

    /**
     * @brief 获取当前线程的本地存储
     *
     * 使用 thread_local 保证每个线程有独立的副本
     * 线程结束时自动销毁
     */
    static ThreadLocalEngine* get_thread_local() {
        static thread_local ThreadLocalEngine local;
        return &local;
    }

    // 模板引擎（使用 shared_ptr 管理生命周期）
    std::shared_ptr<RuleEngine> _template_engine;

    // 全局版本号（每次 set_template_engine 递增）
    std::atomic<uint64_t> _global_version{0};
};

} // namespace ljre
