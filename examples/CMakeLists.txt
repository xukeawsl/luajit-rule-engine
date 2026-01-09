# 示例程序 CMakeLists.txt
# 注意: 当作为子目录添加时,ljre 库会从父项目继承

# 创建示例可执行程序
add_executable(example example.cpp)

# 链接 ljre 库(从父项目继承)
# 统一使用 ljre::ljre，无论是通过 add_subdirectory 还是 find_package
target_link_libraries(example PRIVATE ljre::ljre)
