# LuaJIT Rule Engine Tests

# 查找 GoogleTest
find_package(GTest REQUIRED)

# 通用的测试宏
macro(add_ljre_test test_name)
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name}
        PRIVATE
        ljre
        GTest::GTest
        GTest::Main
    )
    target_include_directories(${test_name}
        PRIVATE
        ${LUAJIT_ROOT}/include/luajit-2.1
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endmacro()

# LuaState 测试
add_ljre_test(lua_state_test)

# LuaStackGuard 测试
add_ljre_test(lua_stack_guard_test)

# DataAdapter 测试
add_ljre_test(data_adapter_test)

# RuleEngine 测试
add_ljre_test(rule_engine_test)

# 集成测试
add_ljre_test(integration_test)

# 覆盖率选项（如果使用 GCC 或 Clang）
option(BUILD_COVERAGE "Build with coverage reporting" OFF)
if(BUILD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(lua_state_test PRIVATE --coverage)
        target_link_options(lua_state_test PRIVATE --coverage)
        target_compile_options(lua_stack_guard_test PRIVATE --coverage)
        target_link_options(lua_stack_guard_test PRIVATE --coverage)
        target_compile_options(data_adapter_test PRIVATE --coverage)
        target_link_options(data_adapter_test PRIVATE --coverage)
        target_compile_options(rule_engine_test PRIVATE --coverage)
        target_link_options(rule_engine_test PRIVATE --coverage)
        target_compile_options(integration_test PRIVATE --coverage)
        target_link_options(integration_test PRIVATE --coverage)
    endif()
endif()
