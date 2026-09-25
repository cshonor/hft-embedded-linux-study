# arm-clang-cortex-m3.cmake —— 交叉编译工具链文件（Cortex-M3 + LLVM 工具链）
#
# 用法：cmake -B build -DCMAKE_TOOLCHAIN_FILE=arm-clang-cortex-m3.cmake
# 原则：toolchain file 必须在**第一次配置**时给全——它的内容会被写进
#       CMakeCache.txt，之后改它不删 build/ 是不生效的（2.4 的坑实录 #4）。

# ① 我要构建的系统不是"这台 Mac"。Generic = 没有宿主 OS 的裸机/交叉目标。
#    不设它，CMake 按 macOS 探测，编译器探测会带出 Mach-O / 框架那套假设。
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)            # ② 目标 CPU 架构，写进 CMakeCache 供工程读

# ③ 编译器。clang 本体装在宿主上（cdev 环境），但"编给谁"由 ④ 决定——
#    这就是 LLVM 交叉编译和 GNU 工具链最大的不同：一套 clang，--target 换目标。
set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET armv7m-none-eabi)  # ④ 三元组 → CMake 自动给每次编译加 --target

# ⑤ ★ 裸机交叉编译的救命行：
#    CMake 配置期会做编译器探测（try_compile），默认试图**编译并链接一个
#    可执行文件**——裸机上根本链接不出宿主可跑的程序，直接报错
#    "the C compiler is not able to compile a simple test program"。
#    改成只编静态库（不链接），探测就能过。实测踩过，见 2.4。
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
