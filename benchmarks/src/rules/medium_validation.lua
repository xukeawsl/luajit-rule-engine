-- 中等复杂规则：用户信息验证
-- 包含多字段检查和逻辑运算

function match(data)
    local score = 0

    -- 检查年龄
    if data.age and data.age >= 18 then
        score = score + 10
    end

    -- 检查邮箱
    if data.email and type(data.email) == "string" then
        local has_at = string.find(data.email, "@") ~= nil
        local has_dot = string.find(data.email, ".") ~= nil
        if has_at and has_dot then
            score = score + 20
        end
    end

    -- 检查手机号
    if data.phone and type(data.phone) == "string" then
        if string.len(data.phone) == 11 and string.sub(data.phone, 1, 1) == "1" then
            score = score + 20
        end
    end

    -- 检查姓名
    if data.name and type(data.name) == "string" and string.len(data.name) > 0 then
        score = score + 10
    end

    -- 检查地址
    if data.address and type(data.address) == "table" then
        if data.address.city and data.address.city ~= nil then
            score = score + 10
        end
        if data.address.zip and data.address.zip ~= nil then
            score = score + 10
        end
    end

    -- 评分要求
    if score >= 60 then
        return true, string.format("用户验证通过，评分: %d", score)
    else
        return false, string.format("用户验证失败，评分: %d (需要 >= 60)", score)
    end
end
