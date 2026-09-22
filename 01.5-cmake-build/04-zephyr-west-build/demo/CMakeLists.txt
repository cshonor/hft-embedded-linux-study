# 一个最小 Zephyr 应用的 CMakeLists——注意它和"裸 CMake"的两个差别
cmake_minimum_required(VERSION 3.20)

# 差别①：project() 之前必须先 find_package(Zephyr)。
# 它做了三件事：定位 Zephyr 内核源码（ZEPHYR_BASE）、
# 读 BOARD 变量确定目标板、把 Zephyr 的整套构建逻辑（几百个模块的
# add_subdirectory + Kconfig 处理）挂进这次构建。
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(blinky_demo C)

# 差别②：可执行文件 target 是 Zephyr 预先创建好的 app，
# 应用只往里面"挂"自己的源文件，而不是 add_executable。
target_sources(app PRIVATE src/main.c)
