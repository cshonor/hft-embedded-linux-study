# demo/Makefile —— 同一个源文件，换三套工具链编译
#
# 用法：
#   make            # 本机默认（macOS: clang）带调试信息编译
#   make CC=gcc     # 换编译器
#   make asan       # 打开 AddressSanitizer —— 越界读会被抓现行
#   make lldb       # 命令行下一次断点跑一遍（非交互）
#   make clean
#
# 本模块要证明的核心结论：
#   MSVC / clang / gcc 的**概念完全相同**，只是拼写不同。
#   这条 Makefile 在 macOS 实测过；Windows 侧见 02 章的命令对照。

CC      ?= clang
CFLAGS  ?= -g -O0 -Wall -Wextra -std=c11
TARGET   = ptrbug
SRC      = ptrbug.c

.PHONY: all asan lldb clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean $(TARGET)

# 命令行下的一次完整调试会话：下断点 → 跑 → 看变量 → 看调用栈 → 继续
lldb: $(TARGET)
	lldb --batch \
		-o "breakpoint set --file $(SRC) --line 34" \
		-o "run" \
		-o "frame variable" \
		-o "bt" \
		-o "continue" \
		-- $(TARGET)

clean:
	rm -f $(TARGET)
