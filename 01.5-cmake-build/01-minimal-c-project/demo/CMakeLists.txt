# 最小可用的现代 CMake 工程 —— 三行起步
cmake_minimum_required(VERSION 3.16)   # ① 声明最低版本：语法以这个版本为准
project(demo C)                        # ② 工程名 + 语言（触发编译器探测）

add_executable(demo main.c math_utils.c)  # ③ 一切皆 target：可执行文件名 + 源文件清单

# —— 下面是「属性挂在 target 上」的现代写法（区别于全局 add_compile_options）——
target_compile_options(demo PRIVATE -Wall -Wextra)  # 只作用于 demo 这个 target
set_target_properties(demo PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
