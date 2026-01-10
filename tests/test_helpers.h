#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <cstdio>
#include <memory>

namespace test_helpers {

// 临时文件管理类 - RAII 方式创建和删除临时文件
class TempFile {
public:
    // 创建临时文件并写入内容
    TempFile(const std::string& content, const std::string& suffix)
        : path_(create_temp_file(content, suffix)) {}

    ~TempFile() {
        if (!path_.empty()) {
            std::remove(path_.c_str());
        }
    }

    // 禁止拷贝和移动
    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;
    TempFile(TempFile&&) = delete;
    TempFile& operator=(TempFile&&) = delete;

    // 获取文件路径
    const std::string& path() const { return path_; }
    const char* c_str() const { return path_.c_str(); }

private:
    static std::string create_temp_file(const std::string& content, const std::string& suffix) {
        std::string temp_dir = "test_data";

        // 确保目录存在
        std::string mkdir_cmd = "mkdir -p " + temp_dir;
        system(mkdir_cmd.c_str());

        // 生成唯一文件名
        std::string template_name = temp_dir + "/ljre_test_XXXXXX" + suffix;

        // 创建临时文件
        int fd = mkstemps(const_cast<char*>(template_name.data()), suffix.length());
        if (fd == -1) {
            return "";
        }

        // 写入内容
        write(fd, content.c_str(), content.size());
        close(fd);

        return template_name;
    }

    std::string path_;
};

// 在测试数据目录创建文件
class TestDataFile {
public:
    TestDataFile(const std::string& filename, const std::string& content)
        : path_("test_data/" + filename) {
        // 确保目录存在
        std::size_t pos = path_.find_last_of('/');
        if (pos != std::string::npos) {
            std::string dir = path_.substr(0, pos);
            std::string mkdir_cmd = "mkdir -p " + dir;
            system(mkdir_cmd.c_str());
        }

        // 写入文件
        std::ofstream file(path_);
        file << content;
    }

    ~TestDataFile() {
        // 可选：清理测试文件
        // std::remove(path_.c_str());
    }

    const std::string& path() const { return path_; }
    const char* c_str() const { return path_.c_str(); }

private:
    std::string path_;
};

// Lua 代码辅助函数
namespace lua_code {

// 有效的简单 Lua 代码
inline std::string valid_simple() {
    return R"(
local x = 10
local y = 20
return x + y
)";
}

// 返回 table 的代码
inline std::string valid_table() {
    return R"(
return {
    name = "test",
    value = 42,
    items = {1, 2, 3}
}
)";
}

// 语法错误的代码
inline std::string syntax_error() {
    return R"(
local x = 10
-- 缺少 then
if x > 5
    print("x is large")
end
)";
}

// 运行时错误的代码
inline std::string runtime_error() {
    return R"(
local x = nil
-- 尝试对 nil 调用方法会出错
x:method()
)";
}

// 访问不存在的全局变量
inline std::string undefined_variable() {
    return R"(
return this_variable_does_not_exist
)";
}

// 空代码
inline std::string empty() {
    return "";
}

// 仅注释
inline std::string only_comments() {
    return R"(-- This is only a comment
-- Another comment
)";
}

// 无限循环（用于测试超时）
inline std::string infinite_loop() {
    return R"(
while true do
    -- loop forever
end
)";
}

// 使用已禁用的 io 库
inline std::string use_io_library() {
    return R"(
local file = io.open("test.txt", "r")
)";
}

} // namespace lua_code

// 规则代码辅助函数
namespace rule_code {

// 总是通过的规则
inline std::string always_pass() {
    return R"(
function match(data)
    return true, "规则通过"
end
)";
}

// 总是失败的规则
inline std::string always_fail() {
    return R"(
function match(data)
    return false, "规则失败"
end
)";
}

// 检查年龄的规则
inline std::string age_check() {
    return R"(
function match(data)
    if data["age"] == nil then
        return false, "缺少age字段"
    end

    if type(data["age"]) ~= "number" then
        return false, "age字段必须是数字类型"
    end

    if data["age"] < 18 then
        return false, string.format("年龄不足，当前年龄: %d, 要求年龄 >= 18", data["age"])
    end

    return true, "年龄检查通过"
end
)";
}

// 检查字段完整性的规则
inline std::string field_complete() {
    return R"(
function match(data)
    local required_fields = {"name", "email", "phone"}
    local missing = {}

    for _, field in ipairs(required_fields) do
        if data[field] == nil then
            table.insert(missing, field)
        end
    end

    if #missing > 0 then
        return false, "缺少必填字段: " .. table.concat(missing, ", ")
    end

    return true, "字段完整性检查通过"
end
)";
}

// 会抛出错误的规则
inline std::string throws_error() {
    return R"(
function match(data)
    error("这是一个测试错误")
end
)";
}

// 没有 match 函数的规则
inline std::string no_match_function() {
    return R"(
local x = 10
-- 没有 match 函数
)";
}

} // namespace rule_code

// 配置文件辅助函数
namespace config_code {

// 有效的配置
inline std::string valid_config() {
    return R"(
return {
    { name = "rule1", file = "test_data/rules/rule1.lua" },
    { name = "rule2", file = "test_data/rules/rule2.lua" }
}
)";
}

// 空配置
inline std::string empty_config() {
    return R"(
return {}
)";
}

// 缺少 name 字段的配置
inline std::string missing_name() {
    return R"(
return {
    { file = "test_data/rules/rule1.lua" }
}
)";
}

// 缺少 file 字段的配置
inline std::string missing_file() {
    return R"(
return {
    { name = "rule1" }
}
)";
}

// 重复的规则名
inline std::string duplicate_names() {
    return R"(
return {
    { name = "rule1", file = "test_data/rules/rule1.lua" },
    { name = "rule1", file = "test_data/rules/rule2.lua" }
}
)";
}

} // namespace config_code

} // namespace test_helpers
