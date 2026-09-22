# toolchain file：Cortex-M 裸机（STM32 等，无操作系统）
# 用法：cmake -B build-m3 -DCMAKE_TOOLCHAIN_FILE=arm-none-eabi.cmake
# 前提：本机装有裸机工具链（arm-none-eabi-gcc；brew / apt / 官网 xPack 均可）

# ── ① 目标系统：没有 OS，叫 Generic ──
set(CMAKE_SYSTEM_NAME Generic)        # 裸机 = Generic（不是 Linux！）
set(CMAKE_SYSTEM_PROCESSOR arm)

# ── ② 编译器 ──
set(triple arm-none-eabi)             # none=无厂商无系统，eabi=裸机 ABI
set(CMAKE_C_COMPILER   ${triple}-gcc)
set(CMAKE_CXX_COMPILER ${triple}-g++)
set(CMAKE_ASM_COMPILER ${triple}-gcc)

# 裸机链接需要启动文件/链接脚本，CMake 的"编译器自检"默认会试链接一个
# 可执行文件——必然失败。STATIC_LIBRARY 告诉它：只验证能编译，别试链接。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# ── ③ CPU 架构 flag（Makefile 里那串 -mcpu 的归宿）──
# 用 _INIT 变量：CMake 第一次探测编译器时就把它们带上，
# 之后所有 target 自动继承，不用每个 target 重复写
set(CMAKE_C_FLAGS_INIT   "-mcpu=cortex-m3 -mthumb")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m3 -mthumb")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-mcpu=cortex-m3 -mthumb --specs=nosys.specs")

# ── ④ find_* 边界（同 ARM Linux 的逻辑）──
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
