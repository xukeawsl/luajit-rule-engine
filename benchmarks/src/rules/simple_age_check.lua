-- 简单规则：年龄检查
-- 用于基准测试的简单场景

function match(data)
    -- 检查 age 字段是否存在
    if data.age == nil then
        return false, "缺少 age 字段"
    end

    -- 检查 age 是否 >= 18
    if data.age < 18 then
        return false, string.format("年龄不足: %d", data.age)
    end

    return true, "年龄检查通过"
end
