#include "ljre/rule_engine_wrapper.h"
#include "gtest/gtest.h"
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <mutex>
#include <set>

using namespace ljre;

// ============== 基础功能测试 ==============

TEST(RuleEngineWrapperTest, DefaultConstruction) {
    RuleEngineWrapper wrapper;
    EXPECT_EQ(wrapper.get_version(), 0);
}

TEST(RuleEngineWrapperTest, GetEngineBeforeSet) {
    RuleEngineWrapper wrapper;

    // 未设置模板引擎时获取，应该返回空的 shared_ptr
    auto engine = wrapper.get_engine();
    EXPECT_EQ(engine, nullptr);
}

TEST(RuleEngineWrapperTest, SetAndGetTemplateEngine_Simple) {
    RuleEngineWrapper wrapper;

    // 创建简单的模板引擎（不加载规则）
    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    EXPECT_EQ(wrapper.get_version(), 1);

    // 获取引擎
    auto retrieved_engine = wrapper.get_engine();
    ASSERT_NE(retrieved_engine, nullptr);

    // 验证基本功能
    EXPECT_EQ(retrieved_engine->get_rule_count(), 0);
    EXPECT_FALSE(retrieved_engine->has_rule("any_rule"));
}

TEST(RuleEngineWrapperTest, MultipleGetReturnsSamePointer) {
    RuleEngineWrapper wrapper;

    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    // 多次获取应该返回同一个 shared_ptr（指向同一对象）
    auto engine1 = wrapper.get_engine();
    auto engine2 = wrapper.get_engine();
    auto engine3 = wrapper.get_engine();

    EXPECT_EQ(engine1.get(), engine2.get());
    EXPECT_EQ(engine2.get(), engine3.get());
}

// ============== shared_ptr 安全性测试 ==============

TEST(RuleEngineWrapperTest, SharedPtrSafety_OldPointerStillValid) {
    RuleEngineWrapper wrapper;

    // 创建第一个引擎
    auto engine1 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine1);

    // 获取引擎（版本1）
    auto engine_v1 = wrapper.get_engine();
    ASSERT_NE(engine_v1, nullptr);

    // 热更新到第二个引擎
    auto engine2 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine2);

    // 再次获取引擎（应该自动克隆到新版本）
    auto engine_v2 = wrapper.get_engine();

    // engine_v1 和 engine_v2 应该指向不同的对象
    EXPECT_NE(engine_v1.get(), engine_v2.get());

    // 关键测试：旧引擎仍然有效（shared_ptr 保证）
    EXPECT_NE(engine_v1.get(), nullptr);
    EXPECT_EQ(engine_v1->get_rule_count(), 0);
}

TEST(RuleEngineWrapperTest, SharedPtrSafety_MultipleHolders) {
    RuleEngineWrapper wrapper;

    auto engine1 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine1);

    // 多个 shared_ptr 持有同一个引擎
    auto holder1 = wrapper.get_engine();
    auto holder2 = holder1;  // 拷贝，引用计数+1
    auto holder3 = wrapper.get_engine();  // 同一个对象，引用计数再+1

    EXPECT_EQ(holder1.get(), holder3.get());

    // 热更新
    auto engine2 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine2);

    // 所有 holder 仍然有效
    EXPECT_NE(holder1.get(), nullptr);
    EXPECT_NE(holder2.get(), nullptr);
    EXPECT_NE(holder3.get(), nullptr);
}

// ============== 热更新测试 ==============

TEST(RuleEngineWrapperTest, HotReloadVersionUpdate) {
    RuleEngineWrapper wrapper;

    auto engine1 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine1);
    EXPECT_EQ(wrapper.get_version(), 1);

    auto engine2 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine2);
    EXPECT_EQ(wrapper.get_version(), 2);

    auto engine3 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine3);
    EXPECT_EQ(wrapper.get_version(), 3);
}

TEST(RuleEngineWrapperTest, HotReloadDoesNotCrash) {
    RuleEngineWrapper wrapper;
    std::atomic<bool> ready{false};
    std::atomic<bool> done{false};
    std::atomic<int> completed{0};

    // 创建初始引擎
    auto engine1 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine1);

    // 工作线程：持续使用引擎
    std::thread worker([&]() {
        auto engine = wrapper.get_engine();
        ready = true;

        // 持续使用引擎
        while (!done) {
            EXPECT_NE(engine.get(), nullptr);
            EXPECT_EQ(engine->get_rule_count(), 0);
            completed++;
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    // 等待工作线程准备就绪
    while (!ready) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    // 等待工作线程运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 执行热更新
    auto engine2 = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine2);

    // 再等待一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 停止工作线程
    done = true;

    worker.join();

    EXPECT_GT(completed, 0);
}

// ============== 线程本地存储测试 ==============

TEST(RuleEngineWrapperTest, ThreadLocalIndependence) {
    RuleEngineWrapper wrapper;

    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    // 同步变量
    std::atomic<int> ready_count{0};
    std::atomic<bool> start_flag{false};
    std::atomic<bool> done_flag{false};

    // 多个线程获取引擎，应该得到不同的指针（每个线程独立）
    std::set<const RuleEngine*> engine_pointers;
    std::mutex pointers_mutex;
    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&wrapper, &engine_pointers, &pointers_mutex,
                              &ready_count, &start_flag, &done_flag, i]() {
            // 通知：线程已准备就绪
            ready_count.fetch_add(1, std::memory_order_relaxed);

            // 等待所有线程准备就绪
            while (!start_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }

            // 获取引擎
            auto thread_engine = wrapper.get_engine();

            // 通知：线程已完成工作
            std::lock_guard<std::mutex> lock(pointers_mutex);
            engine_pointers.insert(thread_engine.get());

            // 等待所有线程完成
            while (!done_flag.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
        });
    }

    // 等待所有线程准备就绪
    while (ready_count.load(std::memory_order_acquire) < 5) {
        std::this_thread::sleep_for(std::chrono::microseconds(100));
    }

    // 通知所有线程同时开始
    start_flag.store(true, std::memory_order_release);

    // 等待一小段时间让所有线程完成工作
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 通知所有线程可以结束
    done_flag.store(true, std::memory_order_release);

    for (auto& t : threads) {
        t.join();
    }

    // 每个线程应该有自己的副本（严格 5 个）
    EXPECT_EQ(engine_pointers.size(), 5);
}

TEST(RuleEngineWrapperTest, SameThreadSamePointer) {
    RuleEngineWrapper wrapper;

    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    // 同一线程多次获取应该返回同一 shared_ptr
    std::vector<std::thread> threads;
    std::atomic<int> same_pointer_count{0};

    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([&wrapper, &same_pointer_count]() {
            auto p1 = wrapper.get_engine();
            auto p2 = wrapper.get_engine();
            auto p3 = wrapper.get_engine();

            if (p1.get() == p2.get() && p2.get() == p3.get()) {
                same_pointer_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 所有线程都应该满足条件
    EXPECT_EQ(same_pointer_count, 3);
}

// ============== 并发压力测试 ==============

TEST(RuleEngineWrapperTest, ConcurrentReadStressTest) {
    RuleEngineWrapper wrapper;
    std::atomic<int> total_operations{0};

    // 创建引擎
    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    const int num_threads = 10;
    const int operations_per_thread = 1000;
    std::vector<std::thread> threads;

    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&, i]() {
            auto thread_engine = wrapper.get_engine();

            for (int j = 0; j < operations_per_thread; ++j) {
                EXPECT_NE(thread_engine, nullptr);
                total_operations++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(total_operations, num_threads * operations_per_thread);
}

TEST(RuleEngineWrapperTest, ConcurrentReadWriteStressTest) {
    RuleEngineWrapper wrapper;
    std::atomic<int> read_ops{0};
    std::atomic<int> write_ops{0};
    std::atomic<bool> stop{false};

    // 创建初始引擎
    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    // 读取线程
    const int num_readers = 8;
    std::vector<std::thread> readers;

    for (int i = 0; i < num_readers; ++i) {
        readers.emplace_back([&, i]() {
            while (!stop) {
                auto thread_engine = wrapper.get_engine();
                EXPECT_NE(thread_engine, nullptr);
                read_ops++;
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });
    }

    // 写入线程（热更新）
    const int num_writers = 2;
    std::vector<std::thread> writers;

    for (int i = 0; i < num_writers; ++i) {
        writers.emplace_back([&]() {
            while (!stop) {
                auto new_engine = std::make_shared<RuleEngine>();
                wrapper.set_template_engine(new_engine);
                write_ops++;
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        });
    }

    // 运行一段时间
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 停止所有线程
    stop = true;

    for (auto& t : readers) {
        t.join();
    }

    for (auto& t : writers) {
        t.join();
    }

    EXPECT_GT(read_ops, 0);
    EXPECT_GT(write_ops, 0);
}

// ============== 边界情况测试 ==============

TEST(RuleEngineWrapperTest, MultipleHotReloads) {
    RuleEngineWrapper wrapper;

    const int num_reloads = 10;

    for (int i = 0; i < num_reloads; ++i) {
        auto engine = std::make_shared<RuleEngine>();
        wrapper.set_template_engine(engine);

        EXPECT_EQ(wrapper.get_version(), i + 1);

        // 验证新版本
        auto retrieved = wrapper.get_engine();
        EXPECT_NE(retrieved, nullptr);
    }
}

TEST(RuleEngineWrapperTest, PointerValidityAcrossRequests) {
    RuleEngineWrapper wrapper;

    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    // 模拟多个请求处理
    for (int i = 0; i < 100; ++i) {
        auto request_engine = wrapper.get_engine();
        EXPECT_NE(request_engine, nullptr);
    }
}

// ============== 性能测试 ==============

TEST(RuleEngineWrapperTest, GetEnginePerformance) {
    RuleEngineWrapper wrapper;

    auto engine = std::make_shared<RuleEngine>();
    wrapper.set_template_engine(engine);

    const int num_iterations = 10000;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_iterations; ++i) {
        auto e = wrapper.get_engine();
        (void)e;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    // 平均每次调用应该小于 1 微秒（因为后续调用只是返回 shared_ptr 拷贝）
    double avg_us = static_cast<double>(duration.count()) / num_iterations;
    EXPECT_LT(avg_us, 1.0) << "Average get_engine() time: " << avg_us << " us";
}
