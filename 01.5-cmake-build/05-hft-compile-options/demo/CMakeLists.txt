cmake_minimum_required(VERSION 3.21)
project(latency_demo C)

# ── ① 构建类型：没指定时给默认值（单配置生成器才生效）──
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)   # HFT 常用：优化保留调试信息
endif()

# ── ② sanitizer 开关：默认关，开发机上 -DENABLE_ASAN=ON 打开 ──
option(ENABLE_ASAN  "AddressSanitizer（内存错误）" OFF)
option(ENABLE_UBSAN "UndefinedBehaviorSanitizer（未定义行为）" OFF)

add_executable(latency_demo main.c)
set_target_properties(latency_demo PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)

# ── ③ 通用告警：所有配置都要（HFT 底线：-Wall -Wextra 零告警）──
target_compile_options(latency_demo PRIVATE -Wall -Wextra)

# ── ④ per-config 选项：用生成器表达式按构建类型挂不同 flag ──
# Release/RelWithDebInfo：针对"本机 CPU"深度优化（注意 5.3 的 -march=native 部署前提）
target_compile_options(latency_demo PRIVATE
    $<$<CONFIG:Release>:-O3>
    $<$<CONFIG:RelWithDebInfo>:-O3>
    $<$<CONFIG:Release>:-march=native>
    $<$<CONFIG:RelWithDebInfo>:-march=native>
    $<$<CONFIG:Debug>:-O0>
    $<$<CONFIG:Debug>:-g3>)

# ── ⑤ LTO：跨编译单元内联（行情热路径的收益大头之一）──
# CMake 抽象成 target 属性，不用手拼 -flto
set_target_properties(latency_demo PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)

# ── ⑥ sanitizer：编译和链接都要挂，成对出现 ──
if(ENABLE_ASAN)
    target_compile_options(latency_demo PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(latency_demo PRIVATE -fsanitize=address)
endif()
if(ENABLE_UBSAN)
    target_compile_options(latency_demo PRIVATE -fsanitize=undefined)
    target_link_options(latency_demo PRIVATE -fsanitize=undefined)
endif()
