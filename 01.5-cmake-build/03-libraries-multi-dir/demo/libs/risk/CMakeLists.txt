# 风控库：依赖 ma（clamp 前先算均值）——库之间也可以链接
add_library(risk STATIC risk.c)

# risk 的头文件对外可见（PUBLIC）；
# 链接 ma：risk.c 里调了 ma_compute，且 risk.h 没暴露 ma 的类型 →
# 链接依赖是 PRIVATE 的（用 risk 的人不需要知道 ma 的存在）
target_include_directories(risk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(risk PRIVATE ma)

target_compile_options(risk PRIVATE -Wall -Wextra)
set_target_properties(risk PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
