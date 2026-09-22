# 移动平均库：一个 target，自己的源文件自己管
add_library(ma STATIC ma.c)

# PUBLIC 的含义：①编译 ma.c 时要有这个 include 路径；
# ②任何链接 ma 的 target（如 app）也自动获得这个路径——
# ma.h 是 ma 的"对外接口"，用库的人必须能 include 到
target_include_directories(ma PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(ma PRIVATE -Wall -Wextra)
set_target_properties(ma PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
