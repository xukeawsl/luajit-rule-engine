-- 用户信息完整性检查规则
-- 检查必需的用户字段是否都存在

function match(data)
    -- 定义必需的字段
    local required_fields = {"username", "email", "age", "phone"}

    -- 检查每个必需字段
    for _, field in ipairs(required_fields) do
        if data[field] == nil then
            return false, string.format("缺少必需字段: %s", field)
        end

        if data[field] == "" then
            return false, string.format("字段 %s 不能为空", field)
        end
    end

    return true, "用户信息完整"
end
