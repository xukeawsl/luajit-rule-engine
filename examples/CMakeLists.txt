# 示例程序 CMakeLists.txt
# 注意: 当作为子目录添加时,ljre 库会从父项目继承

# 创建示例可执行程序
add_executable(example example.cpp)

# 链接 ljre 库(从父项目继承)
target_link_libraries(example PRIVATE ljre)
