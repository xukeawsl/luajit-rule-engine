include(CMakeFindDependencyMacro)

# 查找依赖
# nlohmann/json 是 header-only 库，已包含在项目中
# LuaJIT 需要用户安装到指定位置

include("${CMAKE_CURRENT_LIST_DIR}/ljre_targets.cmake")
