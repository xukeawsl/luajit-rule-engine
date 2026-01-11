-- 复杂规则：风控评分系统
-- 包含嵌套访问、数组遍历、复杂条件判断

function match(data)
    local risk_score = 0
    local risk_factors = {}

    -- 1. 年龄风险
    if data.age then
        if data.age < 18 then
            risk_score = risk_score + 30
            table.insert(risk_factors, "未成年用户")
        elseif data.age > 70 then
            risk_score = risk_score + 10
            table.insert(risk_factors, "高龄用户")
        end
    end

    -- 2. 交易金额风险
    if data.transaction then
        if data.transaction.amount then
            if data.transaction.amount > 10000 then
                risk_score = risk_score + 20
                table.insert(risk_factors, "大额交易")
            elseif data.transaction.amount > 5000 then
                risk_score = risk_score + 10
                table.insert(risk_factors, "中等金额交易")
            end
        end
    end

    -- 3. 历史行为分析
    if data.history then
        if data.history.failed_transactions then
            if data.history.failed_transactions > 5 then
                risk_score = risk_score + 30
                table.insert(risk_factors, "多次交易失败")
            elseif data.history.failed_transactions > 2 then
                risk_score = risk_score + 15
                table.insert(risk_factors, "有交易失败记录")
            end
        end

        if data.history.total_transactions then
            if data.history.total_transactions < 10 then
                risk_score = risk_score + 10
                table.insert(risk_factors, "新用户")
            end
        end
    end

    -- 4. 设备信息
    if data.device then
        if data.device.is_new_device then
            if data.device.is_new_device then
                risk_score = risk_score + 15
                table.insert(risk_factors, "新设备")
            end
        end

        if data.device.is_rooted then
            if data.device.is_rooted then
                risk_score = risk_score + 25
                table.insert(risk_factors, "设备已root")
            end
        end
    end

    -- 5. 地理位置异常
    if data.location then
        if data.location.is_abnormal then
            if data.location.is_abnormal then
                risk_score = risk_score + 20
                table.insert(risk_factors, "地理位置异常")
            end
        end
    end

    -- 6. 交易时间
    if data.transaction then
        if data.transaction.hour then
            local hour = data.transaction.hour
            if hour >= 0 and hour <= 6 then
                risk_score = risk_score + 10
                table.insert(risk_factors, "凌晨交易")
            end
        end
    end

    -- 风险评估
    if risk_score >= 80 then
        return false, string.format("高风险交易 (风险值: %d)", risk_score)
    elseif risk_score >= 50 then
        return true, string.format("中风险交易 (风险值: %d)", risk_score)
    else
        return true, string.format("低风险交易 (风险值: %d)", risk_score)
    end
end
