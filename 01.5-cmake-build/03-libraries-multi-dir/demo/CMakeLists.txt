cmake_minimum_required(VERSION 3.16)
project(feed_handler C)

# 顶层 CMakeLists 只做两件事：定工程、把子目录拉进来。
# 每个子目录有自己的 CMakeLists，各自创建自己的 target——
# 顶层不需要知道库里有哪些 .c 文件
add_subdirectory(libs/ma)
add_subdirectory(libs/risk)
add_subdirectory(app)
