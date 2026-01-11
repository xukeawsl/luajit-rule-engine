# Benchmark CMakeLists.txt
# LuaJIT Rule Engine 性能和压力测试

# 查找 Google Benchmark
find_package(benchmark QUIET)

if(NOT benchmark_FOUND)
    message(STATUS "Google Benchmark not found, will download from GitHub")
    include(FetchContent)
    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    # 对于 Windows，防止覆盖父项目的编译器/链接器设置
    set(g_benchmarks_disable_gtest_requirements ON CACHE INTERNAL "")
    FetchContent_MakeAvailable(benchmark)
endif()

message(STATUS "Google Benchmark found: ${benchmark_FOUND}")

# Benchmark 公共库
add_library(benchmark_common STATIC
    src/benchmark_common.cpp
    src/data_generator.cpp
    src/native_rules.cpp
)

target_include_directories(benchmark_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include
    ${LUAJIT_ROOT}/include/luajit-2.1
    ${JSON_ROOT}
)

target_link_libraries(benchmark_common PUBLIC
    ljre::ljre
    benchmark::benchmark
    benchmark::benchmark_main
)

# 基准测试可执行文件
add_executable(basic_benchmark
    src/benchmarks/basic_benchmark.cpp
)

target_link_libraries(basic_benchmark PRIVATE
    benchmark_common
)

# 压力测试可执行文件
add_executable(stress_benchmark
    src/benchmarks/stress_benchmark.cpp
)

target_link_libraries(stress_benchmark PRIVATE
    benchmark_common
)

# 对比测试可执行文件
add_executable(comparison_benchmark
    src/benchmarks/comparison_benchmark.cpp
)

target_link_libraries(comparison_benchmark PRIVATE
    benchmark_common
)

# 扩展性测试可执行文件
add_executable(scaling_benchmark
    src/benchmarks/scaling_benchmark.cpp
)

target_link_libraries(scaling_benchmark PRIVATE
    benchmark_common
)

# 性能分析工具
add_executable(analyze_ultra_complex
    analyze_ultra_complex.cpp
)

target_link_libraries(analyze_ultra_complex PRIVATE
    ljre::ljre
)

target_include_directories(analyze_ultra_complex PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${LUAJIT_ROOT}/include/luajit-2.1
    ${JSON_ROOT}
)

# 创建 results 目录
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/benchmarks/results)

# 打印配置信息
message(STATUS "Benchmark targets:")
message(STATUS "  - basic_benchmark: 基准性能测试")
message(STATUS "  - stress_benchmark: 压力测试")
message(STATUS "  - comparison_benchmark: LuaJIT vs Native 对比测试")
message(STATUS "  - scaling_benchmark: 扩展性测试")
