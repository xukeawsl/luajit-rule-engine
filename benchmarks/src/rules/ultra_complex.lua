-- 超复杂规则：综合评分系统
-- 包含多层嵌套、大量条件判断、数学计算

function match(data)
    local total_score = 0.0

    -- 1. 基础信息评分 (30分)
    local base_score = 0.0
    if data.user then
        -- 年龄评分
        if data.user.age then
            local age = data.user.age
            if age >= 25 and age <= 45 then
                base_score = base_score + 10
            elseif age >= 18 and age < 25 then
                base_score = base_score + 7
            elseif age > 45 and age <= 65 then
                base_score = base_score + 8
            end
        end

        -- 教育背景评分
        if data.user.profile then
            if data.user.profile.education then
                local edu = data.user.profile.education
                if edu == "university" or edu == "master" or edu == "phd" then
                    base_score = base_score + 10
                elseif edu == "college" then
                    base_score = base_score + 7
                elseif edu == "high_school" then
                    base_score = base_score + 5
                end
            end

            -- 职业评分
            if data.user.profile.occupation then
                local occupation = data.user.profile.occupation
                if occupation == "engineer" or occupation == "doctor" or
                   occupation == "teacher" or occupation == "lawyer" then
                    base_score = base_score + 10
                elseif occupation ~= "" and occupation ~= nil then
                    base_score = base_score + 5
                end
            end
        end
    end
    total_score = total_score + base_score * 0.3

    -- 2. 财务状况评分 (25分)
    local finance_score = 0.0
    if data.finance then
        -- 收入评分
        if data.finance.income then
            local income = data.finance.income
            if income >= 10000 then
                finance_score = finance_score + 10
            elseif income >= 5000 then
                finance_score = finance_score + 7
            elseif income >= 3000 then
                finance_score = finance_score + 5
            end
        end

        -- 资产评分
        if data.finance.assets then
            local assets = data.finance.assets
            if assets >= 500000 then
                finance_score = finance_score + 10
            elseif assets >= 200000 then
                finance_score = finance_score + 7
            elseif assets >= 50000 then
                finance_score = finance_score + 5
            end
        end

        -- 信用评分
        if data.finance.credit_score then
            local credit = data.finance.credit_score
            if credit >= 750 then
                finance_score = finance_score + 5
            elseif credit >= 650 then
                finance_score = finance_score + 3
            elseif credit >= 550 then
                finance_score = finance_score + 1
            end
        end
    end
    total_score = total_score + finance_score * 0.25

    -- 3. 行为历史评分 (25分)
    local behavior_score = 0.0
    if data.behavior then
        -- 守时性
        if data.behavior.punctuality then
            behavior_score = behavior_score + data.behavior.punctuality * 5
        end

        -- 稳定性
        if data.behavior.stability then
            behavior_score = behavior_score + data.behavior.stability * 5
        end

        -- 交易频率
        if data.behavior.transaction_frequency then
            local freq = data.behavior.transaction_frequency
            if freq > 0 and freq <= 10 then
                behavior_score = behavior_score + 5
            elseif freq > 10 and freq <= 30 then
                behavior_score = behavior_score + 10
            elseif freq > 30 then
                behavior_score = behavior_score + 7
            end
        end
    end
    total_score = total_score + behavior_score * 0.25

    -- 4. 社交关系评分 (20分)
    local social_score = 0.0
    if data.social then
        -- 社交连接数
        if data.social.connections then
            local connections = data.social.connections
            if connections >= 100 then
                social_score = social_score + 10
            elseif connections >= 50 then
                social_score = social_score + 7
            elseif connections >= 20 then
                social_score = social_score + 5
            end
        end

        -- 影响力评分
        if data.social.influence_score then
            social_score = social_score + data.social.influence_score * 2
        end

        -- 社区活动
        if data.social.community_activities then
            local activities = data.social.community_activities
            if activities >= 5 then
                social_score = social_score + 8
            elseif activities >= 2 then
                social_score = social_score + 5
            elseif activities >= 1 then
                social_score = social_score + 3
            end
        end
    end
    total_score = total_score + social_score * 0.2

    -- 最终评估
    if total_score >= 80 then
        return true, string.format("优秀用户 (总分: %.1f)", total_score)
    elseif total_score >= 60 then
        return true, string.format("良好用户 (总分: %.1f)", total_score)
    elseif total_score >= 40 then
        return true, string.format("一般用户 (总分: %.1f)", total_score)
    else
        return false, string.format("风险用户 (总分: %.1f)", total_score)
    end
end
