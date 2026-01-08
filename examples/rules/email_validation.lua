-- 邮箱验证规则
-- 检查数据中是否存在email字段，并且格式正确

function match(data)
    -- 检查email字段是否存在
    if data.email == nil then
        return false, "缺少email字段"
    end

    -- 检查email类型是否为字符串
    if type(data.email) ~= "string" then
        return false, "email字段必须是字符串类型"
    end

    -- 简单的邮箱格式验证
    local email = data.email
    if not string.match(email, "^[%w._-]+@[%w._-]+%.[%w]+$") then
        return false, string.format("邮箱格式不正确: %s", email)
    end

    return true, "邮箱验证通过"
end
