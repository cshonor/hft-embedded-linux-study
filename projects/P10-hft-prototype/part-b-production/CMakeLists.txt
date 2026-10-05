cmake_minimum_required(VERSION 3.20)
project(hft_production CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ---- 编译选项：告警当错误，热路径友好 ----
add_compile_options(
    -Wall -Wextra -Wpedantic -Werror
    -fno-exceptions      # 热路径禁用异常：错误一律返回值
    -fno-rtti            # 禁用 RTTI：无 dynamic_cast/typeid 开销
)

# 本机微架构优化（Pi 5 / 服务器各自本地编译时打开；交叉编译勿开）
option(HFT_NATIVE "Build with -march=native" OFF)
if(HFT_NATIVE)
    add_compile_options(-march=native -mtune=native)
endif()

# 头文件-only 库（Phase 1 全部组件都是 header-only，链接零成本）
add_library(hft INTERFACE)
target_include_directories(hft INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ---- 自测 ----
enable_testing()
add_executable(test_protocol tests/test_protocol.cpp)
target_link_libraries(test_protocol PRIVATE hft)
add_test(NAME protocol COMMAND test_protocol)

add_executable(test_book tests/test_book.cpp)
target_link_libraries(test_book PRIVATE hft)
add_test(NAME book COMMAND test_book)

add_executable(test_feed tests/test_feed.cpp)
target_link_libraries(test_feed PRIVATE hft)
add_test(NAME feed COMMAND test_feed)

# Release 默认 -O3（生产延迟敏感代码的底线）
foreach(t test_protocol test_book test_feed)
    target_compile_options(${t} PRIVATE $<$<CONFIG:Release>:-O3>)
endforeach()
