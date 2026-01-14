-- 函数注册测试规则
-- 测试从 Lua 调用 C++ 注册的函数

function match(data)
    -- 测试普通C++函数：获取时间戳
    local time_ms = ljre.get_time_ms()

    -- 测试普通C++函数：打印日志
    ljre.log("Processing data at timestamp: " .. time_ms)

    -- 测试类成员函数：加法运算
    local sum = ljre.add(data.value1, data.value2)

    -- 再次打印日志
    ljre.log("Sum of " .. data.value1 .. " + " .. data.value2 .. " = " .. sum)

    -- 判断结果
    if sum > 100 then
        return true, "Sum is greater than 100: " .. sum
    else
        return false, "Sum is not greater than 100: " .. sum
    end
end
