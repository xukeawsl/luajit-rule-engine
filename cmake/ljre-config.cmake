include(CMakeFindDependencyMacro)

# 查找依赖
# nlohmann/json 是 header-only 库，ljre 自己包含了 nlohmann/json
# LuaJIT 需要用户设置环境变量 LUAJIT_ROOT

# 检查并设置 LuaJIT 路径
if(NOT DEFINED LUAJIT_ROOT)
    set(LUAJIT_ROOT "/usr/local/3rd/luajit-2.1.0-beta3" CACHE PATH "LuaJIT installation directory")
endif()

if(NOT EXISTS "${LUAJIT_ROOT}")
    message(WARNING "LUAJIT_ROOT (${LUAJIT_ROOT}) does not exist. Please set LUAJIT_ROOT to the LuaJIT installation directory.")
endif()

# 引入目标
include("${CMAKE_CURRENT_LIST_DIR}/ljre_targets.cmake")

# 手动添加依赖的 include 路径
if(TARGET ljre::ljre)
    # 添加 LuaJIT include 路径
    set_property(TARGET ljre::ljre APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${LUAJIT_ROOT}/include/luajit-2.1")

    # 添加 nlohmann/json include 路径（已安装到 ljre 的 include 目录）
    # nlohmann/json 的头文件会被安装到 <install_prefix>/include/nlohmann/json.hpp
endif()
