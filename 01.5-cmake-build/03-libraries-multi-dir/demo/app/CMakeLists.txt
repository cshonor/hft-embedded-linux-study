# 应用层：只声明"我用谁"，不关心库的内部
add_executable(feed_handler main.c)

# 链接 risk 就够了：ma 会被 PRIVATE 依赖自动带入链接命令；
# risk 的 PUBLIC include 路径也自动传过来——不用手写任何 -I
target_link_libraries(feed_handler PRIVATE risk)

target_compile_options(feed_handler PRIVATE -Wall -Wextra)
set_target_properties(feed_handler PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED ON)
