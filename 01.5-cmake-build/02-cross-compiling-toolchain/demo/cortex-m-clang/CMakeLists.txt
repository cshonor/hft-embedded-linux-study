# 裸机 blink 工程的 CMakeLists —— labs/00 Makefile 的 CMake 移植版
# 产物核对：与 Makefile 版同为 296 字节 .bin（见 2.3 实测）
cmake_minimum_required(VERSION 3.16)

# 工具链文件必须出现在 project() 之前（第一次配置时由命令行 -D 传入也行，
# 但写在工程里可以 `cmake -B build` 直接用；两种姿势二选一，别混）
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/arm-clang-cortex-m3.cmake")
endif()

project(blink C)   # project() 之前工具链必须就位——编译器探测在这里发生

add_executable(blink startup.c main.c)

# —— 编译期属性（挂 target 上，PRIVATE=只此 target）——
# 对照 labs/00 Makefile 的 CFLAGS：--target 已由 toolchain file 的
# CMAKE_C_COMPILER_TARGET 自动加上，这里只剩 CPU/指令集和 freestanding 一族
target_compile_options(blink PRIVATE
    -mcpu=cortex-m3 -mthumb          # STM32F103 的内核与指令集
    -ffreestanding -fno-builtin -fno-common
    -Wall -Wextra
    -ffunction-sections -fdata-sections   # 配合链接期 --gc-sections
    -fno-unwind-tables -fno-asynchronous-unwind-tables  # 裸机不要 .ARM.exidx
)
set_target_properties(blink PROPERTIES
    C_STANDARD 11 C_STANDARD_REQUIRED ON
    SUFFIX ".elf")   # Generic 系统下可执行文件默认无后缀，补回 .elf（烧录器/size 工具认它）

# —— 链接期属性 ——
# -nostdlib : 不要 crt0、不要 libc（C 运行环境四条前置条件由 startup.c 自己满足）
# -fuse-ld=lld : 让 clang 驱动改调 ld.lld（macOS 系统 ld 是 Mach-O 的，吃不了 ELF 脚本）
# -Wl,xxx : 传给底层链接器的选项统一加 -Wl, 前缀（clang 驱动只透传认识的选项）
target_link_options(blink PRIVATE
    -nostdlib
    -fuse-ld=lld
    -Wl,-T,${CMAKE_CURRENT_LIST_DIR}/linker.ld
    -Wl,--entry=Reset_Handler
    -Wl,--gc-sections
)

# —— 构建后处理：ELF → 可烧写 .bin（对照 Makefile 的 OBJCOPY 行）——
find_program(LLVM_OBJCOPY llvm-objcopy REQUIRED)
add_custom_command(TARGET blink POST_BUILD
    COMMAND ${LLVM_OBJCOPY} -O binary $<TARGET_FILE:blink> ${CMAKE_CURRENT_BINARY_DIR}/blink.bin
    COMMENT "objcopy -> blink.bin")
