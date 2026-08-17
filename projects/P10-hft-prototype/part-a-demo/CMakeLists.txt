cmake_minimum_required(VERSION 3.16)
project(hft_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(hft_demo src/main.cpp)
target_include_directories(hft_demo PRIVATE src)

if (UNIX)
  find_package(Threads REQUIRED)
  target_link_libraries(hft_demo PRIVATE Threads::Threads)
endif()
