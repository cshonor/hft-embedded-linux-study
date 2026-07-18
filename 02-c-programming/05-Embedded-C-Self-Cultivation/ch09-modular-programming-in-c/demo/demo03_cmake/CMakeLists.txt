cmake_minimum_required(VERSION 3.10)
project(ch09_demo03 C)

set(CMAKE_C_STANDARD 11)
set(COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../common)

add_library(mod_common ${COMMON_DIR}/err.c)
target_include_directories(mod_common PUBLIC ${COMMON_DIR})

add_library(mod_utils utils/crc.c)
target_include_directories(mod_utils PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils ${COMMON_DIR})

add_library(mod_uart driver/uart.c)
target_include_directories(mod_uart PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/driver ${COMMON_DIR})
target_link_libraries(mod_uart mod_common)

add_executable(demo03_cmake app/main.c)
target_include_directories(demo03_cmake PRIVATE ${COMMON_DIR})
target_link_libraries(demo03_cmake mod_uart mod_utils mod_common)
