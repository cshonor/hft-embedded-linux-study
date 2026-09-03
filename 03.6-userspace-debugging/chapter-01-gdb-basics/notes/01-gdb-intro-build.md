# 1.1 gdb 入门与调试信息（-g 编译 / debuginfo / 加载方式）

> 🔴 精读 · 用户态正确性调试

## 本节要点

gdb 是 GNU Debugger，Linux 用户态调试的标配。它能在程序运行时**暂停、单步、查看内存与寄存器、回溯调用栈**。但这一切的前提是：**可执行文件里带有调试信息**（源码行号、变量名、类型）。没有调试信息，gdb 只能给你一堆反汇编和裸地址，几乎没法用。本节讲清「怎么编译出可调试的二进制」和「怎么把它喂给 gdb」。

## 为什么需要调试信息

C 编译器默认不把源码信息塞进可执行文件——因为那会让二进制膨胀、还可能泄露源码结构。要让 gdb 认识你的源码，必须显式告诉编译器生成**调试信息**：

```bash
gcc -g -O0 -o orderbook orderbook.c   # -g 生成调试信息，-O0 关优化（变量不被优化掉）
```

调试信息以 **DWARF**（Debugging With Attributed Record Formats）格式编码，存进 ELF 的 `.debug_*` 段。gdb 读取这些段，把「地址 ↔ 源码行号 ↔ 变量名 ↔ 类型」对应起来。

| 编译选项 | 效果 | 调试体验 |
|----------|------|----------|
| 无 `-g` | 无调试信息 | 只有反汇编 + 裸地址，`bt` 看不到源码行 |
| `-g` | 生成 DWARF（默认 DWARF4/5） | 断点/单步/变量/行号全部可用 ✅ |
| `-g -O0` | 关优化 | 每行源码精确对应，变量值真实可读 ✅✅ |
| `-g -O2` | 开优化 | 变量可能被优化掉（`<optimized out>`），行号可能错位 ⚠️ |
| `-g3` | 额外含宏定义信息 | 可 `p MACRO` 展开宏（`-g` 默认不带宏） |
| `-ggdb` | gdb 专有扩展格式 | 与 `-g` 几乎等价，历史遗留，用 `-g` 即可 |

### 优化等级如何破坏调试

```bash
gcc -g -O2 -o orderbook_o2 orderbook.c
gdb orderbook_o2
(gdb) break print_orders
(gdb) run
(gdb) print qty
$1 = <optimized out>   # ← 变量被寄存器化/消除，读不到了
```

这是调试「性能版」二进制时的常态。**规则：调试阶段用 `-O0 -g`；出 release 前再做优化**。若必须在优化版上调试，优先看反汇编 + 寄存器（1.3 节的 `disassemble` + `info registers`）。

## 检查二进制是否带调试信息

```bash
# 方法 1：file 看有没有 debug_info
file orderbook
# orderbook: ELF 64-bit LSB ..., with debug_info, not stripped   ← 有调试信息

# 方法 2：readelf 数 .debug_* 段
readelf -S orderbook | grep debug
#   [28] .debug_info    PROGBITS   ...
#   [29] .debug_abbrev  PROGBITS   ...
#   [30] .debug_line    PROGBITS   ...   ← .debug_line 存行号映射，bt 靠它

# 方法 3：gdb 里问
(gdb) info sources
# Source files for which symbols have been read in:
# .../orderbook.c, /usr/src/debug/glibc-...
```

### 什么是 stripped（剥离）

发行版的二进制通常 `strip` 过——把 `.debug_*` 段和符号表删掉以缩小体积：

```bash
strip orderbook        # 删掉调试信息
file orderbook
# orderbook: ELF 64-bit LSB ..., stripped   ← 没了

# 想知道一个发行版库的调试信息？装 debuginfo 包
# Debian/Ubuntu: apt install libc6-dbg      （glibc 调试符号）
# Fedora:        dnf debuginfo-install glibc
```

装了 `libc6-dbg` 后，`bt` 就能穿透 glibc 内部帧，看到 `__libc_start_main` 下面的完整调用链。

## gdb 加载程序的 4 种方式

| 方式 | 命令 | 适用场景 |
|------|------|----------|
| 直接加载 | `gdb ./prog` | 从头开始调试 |
| 加载 core 文件 | `gdb ./prog core` | 程序崩了，事后回溯现场（Ch3 详解） |
| attach 运行中进程 | `gdb -p <PID>` | 程序在跑、不能重启，现场抓取 |
| 远程调试 | `gdb` → `target remote :port` | 调试板端/容器内进程（gdbserver） |

```bash
# 方式 1：直接加载
gdb ./orderbook
(gdb) run            # 运行（可带参数：run 100 200）

# 方式 2：崩溃后加载 core
ulimit -c unlimited  # 先允许生成 core 文件
./orderbook          # → Segmentation fault (core dumped)
gdb ./orderbook core # 进入后直接 bt 看崩溃点

# 方式 3：attach 到已在运行的进程（需 ptrace 权限）
gdb -p $(pidof orderbook)
# 附加后程序会暂停；detach 再 detach 放它继续跑

# 方式 4：gdbserver 远程（树莓派 5 常用）
# 板端：gdbserver :1234 ./prog
# 本地：gdb ./prog  →  (gdb) target remote 192.168.x.x:1234
```

### 最小命令流

```gdb
(gdb) run              # r，开始运行
(gdb) quit             # q，退出
(gdb) Ctrl-C           # 打断正在跑的程序，回到 gdb 提示符
(gdb) list             # l，显示当前行附近源码
```

## 一个贯穿本章的崩溃示例

下面这个迷你订单簿故意埋了两个雷：`qty` 传了负数、末尾解引用 `NULL` 指针。本章三节都用它：

```c
// orderbook.c —— 迷你订单簿，埋雷
#include <stdio.h>
#include <stdlib.h>

typedef struct order {
    int    id;
    double price;
    long   qty;
    struct order *next;
} order_t;

order_t *head = NULL;

void add_order(int id, double price, long qty) {
    order_t *o = malloc(sizeof(order_t));
    o->id = id; o->price = price; o->qty = qty;
    o->next = head; head = o;
}

void print_orders(void) {
    for (order_t *p = head; p; p = p->next)
        printf("id=%d price=%.2f qty=%ld\n", p->id, p->price, p->qty);
}

int main(void) {
    add_order(1, 100.5, 100);
    add_order(2, 101.0, 200);
    add_order(3,  99.5, -1);   // 雷 1：qty 负数
    print_orders();
    int *p = NULL;
    *p = 42;                    // 雷 2：段错误
    return 0;
}
```

编译并跑：

```bash
gcc -g -O0 -o orderbook orderbook.c
./orderbook
# id=3 price=99.50 qty=-1
# id=2 price=101.00 qty=200
# id=1 price=100.50 qty=100
# Segmentation fault (core dumped)   ← 崩了，gdb 登场
```

## HFT 关联

交易进程崩溃时，你往往只有一句 `Segmentation fault` 和一堆日志。gdb 加载 core 文件 + `bt` 是**第一现场**：

1. **发行版编译规范**：调试阶段 `-O0 -g` 全量生成；发布前单独出 `-O2` 优化版，但**必须保留一份带符号的副本**（`objcopy --only-keep-debug` 或直接存未 strip 的构建产物），否则线上 core 拿回来没法回溯。
2. **attach 能力**：交易进程通常 7×24 运行、不能随便重启，`gdb -p <PID>` 现场 attach 看死循环/阻塞卡在哪，比重启复现高效得多。
3. **debuginfo 包**：`bt` 要穿透 glibc / libstdc++，务必在开发机装 `libc6-dbg`、`libstdc++6-...-dbg`，否则栈回溯断在库边界。
4. **gdbserver 远程**：树莓派 5 上的用户态程序（配合 eBPF/驱动），用 gdbserver 暴露端口、本地 gdb 远程连，免去在板端装全套工具链。

```bash
# HFT 常用：崩溃后立刻回溯
gdb ./matching_engine core
(gdb) bt          # 立刻定位崩溃函数与行号
(gdb) info locals # 看崩溃点局部变量
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** `-g` 和 `-O0` 分别管什么？为什么调试时要一起给？

> `-g` 让编译器生成 DWARF 调试信息（行号/变量名/类型），是 gdb 认识源码的前提；`-O0` 关优化，避免变量被优化成寄存器或直接消除（否则 `print` 报 `<optimized out>`）。两者缺一不可：只有 `-g` 无 `-O0`，优化版上变量难读；只有 `-O0` 无 `-g`，gdb 根本看不到源码。

**Q2:** `stripped` 二进制和带调试信息的二进制，本质区别是什么？

> stripped 删掉了 `.debug_*` 段和符号表（`.symtab`），只剩动态链接必需的 `.dynsym`。gdb 加载 stripped 二进制时只能反汇编 + 看动态符号，`bt` 拿不到源码行号。带调试信息的二进制保留完整 DWARF，gdb 能精确映射地址↔行号↔变量。

**Q3:** 线上程序崩了，但没有保留带符号的构建产物，core 文件还有用吗？

> 还有有限价值：能反汇编看崩溃地址附近的指令、看寄存器、看内存内容，但很难还原到源码行。所以工程上务必在 CI 里**归档每个 release 的调试符号**（`objcopy --only-keep-debug prog prog.debug` + `objcopy --add-gnu-debuglink`），或直接保留未 strip 产物。这是 HFT 团队的基本纪律。

**Q4:** `gdb -p <PID>` attach 有什么前置条件？失败常见原因？

> 需要 ptrace 权限：同用户、且进程没被 `PR_SET_DUMPABLE=0` 屏蔽；新版内核受 `kernel.yama.ptrace_scope` 限制（默认 1 只允许父子关系 attach，root 可绕过）。attach 会向目标发 SIGSTOP 暂停它，调试完 `detach` 才能继续跑。容器内还要注意 `--cap-add=SYS_PTRACE`。

**Q5:** 为什么调试信息会「泄露源码结构」？HFT 为何在意？

> DWARF 里存了源文件路径、函数名、变量名、类型布局，相当于把源码骨架打包进了二进制。对闭源交易系统，这会暴露内部模块命名与数据结构的轮廓，利于逆向。所以发布版要 strip、调试符号内部归档不外发。

</details>

## 交叉引用

- [1.2 断点与观察点](02-breakpoints.md)
- [1.3 栈帧与回溯](03-stack-backtrace.md)
- [03.6 模块导读](../../README.md)
