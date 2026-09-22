cmake_minimum_required(VERSION 3.16)
project(ma_feed C)

# 与第 1 章完全相同的工程描述——交叉编译不改这一份文件，
# 全部差异都在 toolchain file 里（见 arm-linux-gnueabihf.cmake / arm-none-eabi.cmake）
add_executable(ma_feed main.c math_utils.c)
target_compile_options(ma_feed PRIVATE -Wall -Wextra)
set_target_properties(ma_feed PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
