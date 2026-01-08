-- 年龄检查规则
-- 检查数据中是否存在age字段，并且age >= 18

function match(data)
    -- 检查age字段是否存在
    if data.age == nil then
        return false, "缺少age字段"
    end

    -- 检查age类型是否为数字
    if type(data.age) ~= "number" then
        return false, "age字段必须是数字类型"
    end

    -- 检查age是否大于等于18
    if data.age < 18 then
        return false, string.format("年龄不足，当前年龄: %d, 要求年龄 >= 18", data.age)
    end

    return true, "年龄检查通过"
end
