# toolchain file：ARM Linux（如树莓派、ARM 工控板）交叉编译
# 用法：cmake -B build-arm -DCMAKE_TOOLCHAIN_FILE=arm-linux-gnueabihf.cmake
# 前提：本机装有交叉工具链（Ubuntu: apt install gcc-arm-linux-gnueabihf；
#       macOS: brew install arm-linux-gnueabihf-binutils 或自建 crosstool-NG）

# ── ① 告诉 CMake"产出物给谁跑"（触发交叉模式，CMAKE_CROSSCOMPILING=ON）──
set(CMAKE_SYSTEM_NAME Linux)          # 目标系统：跑 Linux
set(CMAKE_SYSTEM_PROCESSOR arm)       # 目标 CPU 架构：ARM 32 位

# ── ② 告诉 CMake"编译器换谁" ──
# 三元组前缀 = <架构>-<厂商>-<系统>[-<ABI>]，工具链里每个工具都带这个前缀
set(triple arm-linux-gnueabihf)
set(CMAKE_C_COMPILER   ${triple}-gcc)
set(CMAKE_CXX_COMPILER ${triple}-g++)   # 用不到 C++ 也可写上，省得以后漏

# ── ③ 告诉 CMake"头文件/库去哪找" ──
# sysroot = 目标系统的"/"在本机的镜像：usr/include、lib 全在里面。
# 没装 sysroot 时注释掉这行也能编纯用户态小程序（编译器自带头文件够用）。
#set(CMAKE_SYSROOT /opt/sysroots/arm-linux-gnueabihf)

# ── ④ find_* 命令的搜索边界（交叉编译最容易翻车的地方）──
# 找程序（如 protoc）：在"本机"找——生成代码的工具要能在本机跑
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# 找库、找头文件、找 package：只在"目标 sysroot"找——绝不能链到本机 x86 的库
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
