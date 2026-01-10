cmake_minimum_required(VERSION 3.15)
project(luajit-rule-engine VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 覆盖率选项
option(BUILD_COVERAGE "Build with coverage reporting" OFF)
if(BUILD_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

# 设置 LuaJIT 路径
set(LUAJIT_ROOT "/usr/local/3rd/luajit-2.1.0-beta3" CACHE PATH "LuaJIT installation directory")

# 设置 nlohmann/json 路径
set(JSON_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third-party/json/single_include" CACHE PATH "nlohmann/json include directory")

# 规则引擎库
add_library(ljre STATIC
    src/lua_state.cpp
    src/json_adapter.cpp
    src/rule_engine.cpp
)

target_include_directories(ljre PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${LUAJIT_ROOT}/include/luajit-2.1>
    $<BUILD_INTERFACE:${JSON_ROOT}>
)

target_link_libraries(ljre PUBLIC
    ${LUAJIT_ROOT}/lib/libluajit-5.1.a
)

target_link_libraries(ljre PRIVATE
    -lm -ldl
)

# 安装规则
include(GNUInstallDirs)

install(TARGETS ljre
    EXPORT ljre_targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# 安装 ljre 头文件
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# 安装 nlohmann/json 头文件（ljre 依赖此库）
install(FILES ${JSON_ROOT}/nlohmann/json.hpp DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nlohmann)

# 导出目标
install(EXPORT ljre_targets
    FILE ljre_targets.cmake
    NAMESPACE ljre::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ljre
)

# 为 ljre 目标创建别名，既可以使用 ljre 也可以使用 ljre::ljre
add_library(ljre::ljre ALIAS ljre)

# 创建配置文件
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ljre-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ljre-config-version.cmake"
    cmake/ljre-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ljre
)

# 启用测试
option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

# 启用示例
option(BUILD_EXAMPLES "Build examples" OFF)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
