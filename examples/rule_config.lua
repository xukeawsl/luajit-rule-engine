-- 规则配置文件示例
-- 定义规则引擎需要加载的所有规则

return {
    { name = "age_check", file = "rules/age_check.lua" },
    { name = "email_validation", file = "rules/email_validation.lua" },
    { name = "user_info_complete", file = "rules/user_info_complete.lua" }
}
