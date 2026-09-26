# demo/Makefile —— 同一份源文件，换工具链 / 换优化级别地编译与调试
#
# 用法：
#   make                  # 默认：clang 带调试信息编译
#   make CC=gcc           # 换编译器
#   make o2               # 同一份源码在 -O2 下（UB 的行为可能完全不同，见 3.2）
#   make matrix           # 四组「编译器 × 优化级别」跑一遍，对照 bug 表现
#   make asan             # AddressSanitizer —— 越界读会被抓现行
#   make lldb             # macOS：非交互下一次断点会话
#   make gdb              # Linux：非交互下一次断点会话
#   make clean
#
# 本模块要证明的核心结论：
#   MSVC / clang / gcc 的**概念完全相同**，只是拼写不同；
#   但同一份 UB 代码的**实际后果**由「编译器 + 版本 + 优化级别 + 平台」共同决定。
#   macOS（Apple clang 21 / LLVM clang 23）与 Linux/Pi（gcc 14 / clang 19）均已实测。

UNAME_S := $(shell uname -s)

# ⚠️ 坑：GNU make **内置**就有 CC=cc、CFLAGS 等变量，所以 `CC ?= clang` 永远不生效
#    （实测：Linux 上会悄悄用 cc 编译）。想给默认值必须先用 origin 判断变量来源。
#    命令行 `make CC=gcc` 时 origin 为 "command line"，这里就尊重它。
ifeq ($(origin CC),default)
CC := clang
endif
ifeq ($(origin CFLAGS),default)
CFLAGS := -g -O0 -Wall -Wextra -std=c11
endif

TARGET   = ptrbug
SRC      = ptrbug.c
# sum_n() 里 `s += a[i];` 所在的行号（改了源码要跟着改）
LINE     = 34

.PHONY: all o2 matrix asan lldb gdb clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

# ---- 同一份源码在 -O2 下重编：UB 在这个级别才可能露出獠牙 ----
o2: clean
	$(CC) -O2 -Wall -std=c11 $(SRC) -o $(TARGET)_o2
	@echo "== $(CC) -O2 输出 =="; ./$(TARGET)_o2 || echo "（非零退出）"

# ---- 矩阵：把四种组合各跑一遍，亲眼看 UB 的不确定性 ----
matrix: clean
	@for opt in -O0 -O2; do \
	  ext=$${opt#-}; \
	  echo "===== $(CC) $$opt ====="; \
	  $(CC) $$opt -Wall -std=c11 $(SRC) -o $(TARGET)_$$ext 2>/dev/null; \
	  ./$(TARGET)_$$ext || echo "（非零退出 / 可能被卡住后强杀）"; \
	done

asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer -O1
asan: clean $(TARGET)
	@echo "== 跑 ASan 版 =="; ./$(TARGET) 2>&1 | head -20 || true

# ---- macOS：一次完整 lldb 会话（批处理，不等你输任何东西） ----
lldb: $(TARGET)
	lldb --batch \
		-o "breakpoint set --file $(SRC) --line $(LINE)" \
		-o "run" \
		-o "frame variable" \
		-o "bt" \
		-o "continue" \
		-- ./$(TARGET)

# ---- Linux：一次完整 gdb 会话（-ex + --args，注意和 lldb 的 -o + -- 不一样） ----
gdb: $(TARGET)
	gdb -batch -q \
		-ex "break $(SRC):$(LINE)" \
		-ex "run" \
		-ex "info locals" \
		-ex "backtrace" \
		-ex "continue" \
		--args ./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET)_O0 $(TARGET)_O2 $(TARGET)_o2 $(TARGET)_asan
	rm -rf $(TARGET).dSYM
