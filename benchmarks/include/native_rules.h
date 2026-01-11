#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include "benchmark_common.h"

namespace benchmark {

// 原生 C++ 规则实现（用于性能对比）

// 简单规则：年龄检查
struct NativeAgeCheckRule {
    static bool match(const nlohmann::json& data, std::string& message) {
        if (!data.contains("age")) {
            message = "缺少 age 字段";
            return false;
        }

        try {
            int age = data["age"].get<int>();
            if (age < 18) {
                message = "年龄不足: " + std::to_string(age);
                return false;
            }

            message = "年龄检查通过";
            return true;
        } catch (const std::exception& e) {
            message = std::string("年龄检查失败: ") + e.what();
            return false;
        }
    }
};

// 中等规则：用户信息验证
struct NativeUserValidationRule {
    static bool match(const nlohmann::json& data, std::string& message) {
        int score = 0;

        // 检查年龄
        if (data.contains("age")) {
            try {
                int age = data["age"].get<int>();
                if (age >= 18) {
                    score += 10;
                }
            } catch (...) {
                message = "年龄字段格式错误";
                return false;
            }
        }

        // 检查邮箱
        if (data.contains("email")) {
            try {
                std::string email = data["email"].get<std::string>();
                if (email.find("@") != std::string::npos &&
                    email.find(".") != std::string::npos) {
                    score += 20;
                }
            } catch (...) {
                message = "邮箱字段格式错误";
                return false;
            }
        }

        // 检查手机号
        if (data.contains("phone")) {
            try {
                std::string phone = data["phone"].get<std::string>();
                if (phone.length() == 11 && phone[0] == '1') {
                    score += 20;
                }
            } catch (...) {
                message = "手机号字段格式错误";
                return false;
            }
        }

        // 检查姓名
        if (data.contains("name")) {
            try {
                std::string name = data["name"].get<std::string>();
                if (!name.empty()) {
                    score += 10;
                }
            } catch (...) {
                message = "姓名字段格式错误";
                return false;
            }
        }

        // 检查地址
        if (data.contains("address")) {
            try {
                auto address = data["address"];
                if (address.contains("city") && !address["city"].is_null()) {
                    score += 10;
                }
                if (address.contains("zip") && !address["zip"].is_null()) {
                    score += 10;
                }
            } catch (...) {
                message = "地址字段格式错误";
                return false;
            }
        }

        // 评分要求
        if (score >= 60) {
            message = "用户验证通过，评分: " + std::to_string(score);
            return true;
        } else {
            message = "用户验证失败，评分: " + std::to_string(score) + " (需要 >= 60)";
            return false;
        }
    }
};

// 复杂规则：风控评分系统
struct NativeRiskControlRule {
    static bool match(const nlohmann::json& data, std::string& message) {
        int risk_score = 0;
        std::vector<std::string> risk_factors;

        // 1. 年龄风险
        if (data.contains("age")) {
            try {
                int age = data["age"].get<int>();
                if (age < 18) {
                    risk_score += 30;
                    risk_factors.push_back("未成年用户");
                } else if (age > 70) {
                    risk_score += 10;
                    risk_factors.push_back("高龄用户");
                }
            } catch (...) {}
        }

        // 2. 交易金额风险
        if (data.contains("transaction")) {
            try {
                auto transaction = data["transaction"];
                if (transaction.contains("amount")) {
                    double amount = transaction["amount"].get<double>();
                    if (amount > 10000) {
                        risk_score += 20;
                        risk_factors.push_back("大额交易");
                    } else if (amount > 5000) {
                        risk_score += 10;
                        risk_factors.push_back("中等金额交易");
                    }
                }
            } catch (...) {}
        }

        // 3. 历史行为分析
        if (data.contains("history")) {
            try {
                auto history = data["history"];
                if (history.contains("failed_transactions")) {
                    int failed = history["failed_transactions"].get<int>();
                    if (failed > 5) {
                        risk_score += 30;
                        risk_factors.push_back("多次交易失败");
                    } else if (failed > 2) {
                        risk_score += 15;
                        risk_factors.push_back("有交易失败记录");
                    }
                }

                if (history.contains("total_transactions")) {
                    int total = history["total_transactions"].get<int>();
                    if (total < 10) {
                        risk_score += 10;
                        risk_factors.push_back("新用户");
                    }
                }
            } catch (...) {}
        }

        // 4. 设备信息
        if (data.contains("device")) {
            try {
                auto device = data["device"];
                if (device.contains("is_new_device")) {
                    bool is_new = device["is_new_device"].get<bool>();
                    if (is_new) {
                        risk_score += 15;
                        risk_factors.push_back("新设备");
                    }
                }

                if (device.contains("is_rooted")) {
                    bool is_rooted = device["is_rooted"].get<bool>();
                    if (is_rooted) {
                        risk_score += 25;
                        risk_factors.push_back("设备已root");
                    }
                }
            } catch (...) {}
        }

        // 5. 地理位置异常
        if (data.contains("location")) {
            try {
                auto location = data["location"];
                if (location.contains("is_abnormal")) {
                    bool is_abnormal = location["is_abnormal"].get<bool>();
                    if (is_abnormal) {
                        risk_score += 20;
                        risk_factors.push_back("地理位置异常");
                    }
                }
            } catch (...) {}
        }

        // 6. 交易时间
        if (data.contains("transaction")) {
            try {
                auto transaction = data["transaction"];
                if (transaction.contains("hour")) {
                    int hour = transaction["hour"].get<int>();
                    if (hour >= 0 && hour <= 6) {
                        risk_score += 10;
                        risk_factors.push_back("凌晨交易");
                    }
                }
            } catch (...) {}
        }

        // 风险评估
        if (risk_score >= 80) {
            message = "高风险交易 (风险值: " + std::to_string(risk_score) + ")";
            return false;
        } else if (risk_score >= 50) {
            message = "中风险交易 (风险值: " + std::to_string(risk_score) + ")";
            return true;  // 允许但需监控
        } else {
            message = "低风险交易 (风险值: " + std::to_string(risk_score) + ")";
            return true;
        }
    }
};

// 超复杂规则：综合评分系统
struct NativeComprehensiveRule {
    static bool match(const nlohmann::json& data, std::string& message) {
        double total_score = 0.0;
        std::vector<std::string> details;

        // 1. 基础信息评分 (30分)
        double base_score = 0.0;
        if (data.contains("user")) {
            try {
                auto user = data["user"];
                if (user.contains("age")) {
                    int age = user["age"].get<int>();
                    if (age >= 25 && age <= 45) {
                        base_score += 10;
                    } else if (age >= 18 && age < 25) {
                        base_score += 7;
                    } else if (age > 45 && age <= 65) {
                        base_score += 8;
                    }
                }

                if (user.contains("profile")) {
                    auto profile = user["profile"];
                    if (profile.contains("education")) {
                        std::string edu = profile["education"].get<std::string>();
                        if (edu == "university" || edu == "master" || edu == "phd") {
                            base_score += 10;
                        } else if (edu == "college") {
                            base_score += 7;
                        } else if (edu == "high_school") {
                            base_score += 5;
                        }
                    }

                    if (profile.contains("occupation")) {
                        std::string occupation = profile["occupation"].get<std::string>();
                        if (occupation == "engineer" || occupation == "doctor" ||
                            occupation == "teacher" || occupation == "lawyer") {
                            base_score += 10;
                        } else if (!occupation.empty()) {
                            base_score += 5;
                        }
                    }
                }
            } catch (...) {}
        }
        total_score += base_score * 0.3;

        // 2. 财务状况评分 (25分)
        double finance_score = 0.0;
        if (data.contains("finance")) {
            try {
                auto finance = data["finance"];
                if (finance.contains("income")) {
                    double income = finance["income"].get<double>();
                    if (income >= 10000) {
                        finance_score += 10;
                    } else if (income >= 5000) {
                        finance_score += 7;
                    } else if (income >= 3000) {
                        finance_score += 5;
                    }
                }

                if (finance.contains("assets")) {
                    double assets = finance["assets"].get<double>();
                    if (assets >= 500000) {
                        finance_score += 10;
                    } else if (assets >= 200000) {
                        finance_score += 7;
                    } else if (assets >= 50000) {
                        finance_score += 5;
                    }
                }

                if (finance.contains("credit_score")) {
                    int credit = finance["credit_score"].get<int>();
                    if (credit >= 750) {
                        finance_score += 5;
                    } else if (credit >= 650) {
                        finance_score += 3;
                    } else if (credit >= 550) {
                        finance_score += 1;
                    }
                }
            } catch (...) {}
        }
        total_score += finance_score * 0.25;

        // 3. 行为历史评分 (25分)
        double behavior_score = 0.0;
        if (data.contains("behavior")) {
            try {
                auto behavior = data["behavior"];
                if (behavior.contains("punctuality")) {
                    double punctuality = behavior["punctuality"].get<double>();
                    behavior_score += punctuality * 5;
                }

                if (behavior.contains("stability")) {
                    double stability = behavior["stability"].get<double>();
                    behavior_score += stability * 5;
                }

                if (behavior.contains("transaction_frequency")) {
                    int freq = behavior["transaction_frequency"].get<int>();
                    if (freq > 0 && freq <= 10) {
                        behavior_score += 5;
                    } else if (freq > 10 && freq <= 30) {
                        behavior_score += 10;
                    } else if (freq > 30) {
                        behavior_score += 7;
                    }
                }
            } catch (...) {}
        }
        total_score += behavior_score * 0.25;

        // 4. 社交关系评分 (20分)
        double social_score = 0.0;
        if (data.contains("social")) {
            try {
                auto social = data["social"];
                if (social.contains("connections")) {
                    int connections = social["connections"].get<int>();
                    if (connections >= 100) {
                        social_score += 10;
                    } else if (connections >= 50) {
                        social_score += 7;
                    } else if (connections >= 20) {
                        social_score += 5;
                    }
                }

                if (social.contains("influence_score")) {
                    double influence = social["influence_score"].get<double>();
                    social_score += influence * 2;
                }

                if (social.contains("community_activities")) {
                    int activities = social["community_activities"].get<int>();
                    if (activities >= 5) {
                        social_score += 8;
                    } else if (activities >= 2) {
                        social_score += 5;
                    } else if (activities >= 1) {
                        social_score += 3;
                    }
                }
            } catch (...) {}
        }
        total_score += social_score * 0.2;

        // 最终评估
        if (total_score >= 80) {
            message = "优秀用户 (总分: " + std::to_string(total_score).substr(0, 5) + ")";
            return true;
        } else if (total_score >= 60) {
            message = "良好用户 (总分: " + std::to_string(total_score).substr(0, 5) + ")";
            return true;
        } else if (total_score >= 40) {
            message = "一般用户 (总分: " + std::to_string(total_score).substr(0, 5) + ")";
            return true;
        } else {
            message = "风险用户 (总分: " + std::to_string(total_score).substr(0, 5) + ")";
            return false;
        }
    }
};

} // namespace benchmark
