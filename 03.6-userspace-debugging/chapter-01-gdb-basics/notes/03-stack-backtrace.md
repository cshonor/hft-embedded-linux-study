# 1.3 栈帧与回溯（backtrace / frame / 调用约定 / 变量查看）

> 🔴 精读 · 用户态正确性调试

## 本节要点

程序崩了之后，第一个要回答的问题是：**「我是怎么走到崩溃点这里的？」**——即调用链（谁调了谁，参数是什么，局部变量是什么）。`backtrace`（`bt`）给出整条调用栈，`frame` 让你在栈帧间上下穿梭，`print`/`info locals` 看每层帧的局部状态。这背后是**调用约定 + 栈帧布局**的硬知识：不理解栈帧，就看不懂回溯、更看不懂反汇编。

## backtrace：一眼看清调用链

回到 `orderbook` 的崩溃现场，这次直接加载 core：

```bash
gcc -g -O0 -o orderbook orderbook.c
ulimit -c unlimited
./orderbook
# Segmentation fault (core dumped)
gdb ./orderbook core
(gdb) bt
#0  0x00005555555551a7 in main () at orderbook.c:28
#1  0x00007ffff7de7d8a in __libc_start_call_main (...) at ../sysdeps/nptl/libc_start_call_main.h:58
#2  0x00007ffff7de7e45 in __libc_start_main_impl (...) at ../csu/libc-start.c:379
#3  0x00005555555550d5 in _start ()
```

`bt` 每一行是一个**栈帧**（stack frame）：

| 列 | 含义 |
|----|------|
| `#0` | 帧号，`#0` 是最内层（当前正在执行的函数），号越大越靠外 |
| `0x0000...` | 该帧的**返回地址**（`rip`/PC） |
| `in main ()` | 函数名 |
| `at orderbook.c:28` | 源码位置（靠 `.debug_line` 段） |

> 崩溃点在 `#0`，但**根因往往在外层帧**——比如 `#0` 在 `strlen` 里崩，是因为 `#3` 的某函数传了个野指针进去。`bt` 让你从内到外逐层检查「每层是不是都正常」。

### bt 的常用变体

```gdb
(gdb) bt               # 完整调用栈
(gdb) bt full          # 每帧还列出局部变量（最常用！）
(gdb) bt 3             # 只看最内层 3 帧
(gdb) bt -3            # 只看最外层 3 帧
```

## 栈帧布局：回溯为什么能工作

函数调用时，CPU 用**栈**来保存「返回地址 + 上一帧的帧指针 + 局部变量」。x86_64 上，`rbp`（frame pointer）是回溯的关键：

```
高地址
+-------------------+  ← 调用者 (caller) 的栈帧
| 调用者局部变量      |
| 调用者的 rbp        |  ← rbp 链！当前 rbp 指向这里，里面存着上一层 rbp
+-------------------+
| 返回地址 (rip)      |  ← 函数调用时 push，ret 时弹出
| 被调者的 rbp        |  ← 当前函数入口：push rbp; mov rbp, rsp
| 被调者局部变量      |
| ...               |  ← rsp（栈顶）
+-------------------+
低地址
```

回溯就是**顺着 rbp 链一路往上爬**：当前 `rbp` 指向上一个 `rbp`，每个 `rbp` 上方 8 字节是返回地址。gdb 按这条链逐帧还原调用历史。

```asm
# 典型 x86_64 函数序言（prologue）
push   rbp          # 保存调用者的 rbp
mov    rbp, rsp     # 建立自己的帧指针
sub    rsp, 0x20    # 为局部变量预留空间
```

### ⚠️ 现代编译器常常「省略帧指针」（-fomit-frame-pointer）

`-O2` 默认启用 `-fomit-frame-pointer`，此时 `rbp` 被当成通用寄存器用，**不再有 rbp 链**。回溯靠 `.eh_frame` 段（DWARF 的 CFI，Call Frame Information）里的展开规则，而不是 rbp 链。所以：

- 调试版（`-O0`）：有 rbp 链，`bt` 直接可靠；
- 优化版（`-O2`）：`bt` 依赖 `.eh_frame`，若二进制被 strip 掉该段，回溯会断。

### AArch64（树莓派 5）的对应关系

| 概念 | x86_64 | AArch64 |
|------|--------|---------|
| 栈指针 | `rsp` | `sp` |
| 帧指针 | `rbp` | `x29`（`fp`） |
| 返回地址寄存器 | 栈上 push 的 `rip` | `x30`（`lr`，link register） |
| 程序计数器 | `rip` | `pc` |
| 函数序言 | `push rbp; mov rbp,rsp` | `stp x29,x30,[sp,#-16]!` + `mov x29,sp` |
| 返回 | `ret`（弹栈到 rip） | `ret`（`br x30`，跳回 lr） |

AArch64 用 `x29`（帧指针）+ `x30`（链接寄存器）同样能回溯，机制一致：`x29` 链 + 每个帧里保存的 `lr`。

## frame：在栈帧间穿梭

```gdb
(gdb) frame 3          # 跳到 3 号帧（外层）
(gdb) up               # 往外走一帧（更早的调用者）
(gdb) down             # 往内走一帧（更晚的被调者）
(gdb) frame 0          # 回最内层

(gdb) info frame       # 当前帧的详细信息（地址、寄存器、源码位置）
(gdb) info locals      # 当前帧的局部变量
(gdb) info args        # 当前帧的函数参数
```

```gdb
(gdb) frame 0
(gdb) info frame
Stack level 0, frame at 0x7fffffffe100:
 rip = 0x5555555551a7 in main (orderbook.c:28)
 rbp = 0x7fffffffe0f0, rsp = 0x7fffffffe0d0
(gdb) info locals
p = 0x0          # ← 崩溃点局部变量，p 是 NULL
head = 0x5555555592e0
(gdb) info args
No arguments.   # main 无参数
```

## 查看变量与类型

| 命令 | 作用 |
|------|------|
| `print <expr>`（`p`） | 求值表达式并打印（可调用函数、解引用指针） |
| `print/x <expr>` | 十六进制输出 |
| `display <expr>` | 每次停下自动打印该表达式（持续监控） |
| `ptype <var>` | 打印变量/表达式的**类型** |
| `whatis <var>` | 打印变量声明 |
| `info registers` | 看所有寄存器 |
| `x/NFU <addr>` | 查看内存（见下方） |

```gdb
(gdb) print head->price
$1 = 99.5
(gdb) print *head
$2 = {id = 3, price = 99.5, qty = -1, next = 0x5555555592a0}
(gdb) ptype head
type = struct order {
    int id;
    double price;
    long qty;
    struct order *next;
} *
(gdb) whatis head
type = order_t *
```

### x 命令查看内存

```gdb
(gdb) x/8x head          # 以十六进制看 head 指向的 8 个「单位」
# 0x5555555592e0: 0x00000003  0x00000000  0x00000000  0x00000000
# 0x5555555592f0: 0x00000000  0x00005840  ...

(gdb) x/s (char*)head    # 以字符串解释
(gdb) x/i main           # 反汇编 main 入口的一条指令
```

`x/NFU` 格式：`N`=数量、`F`=格式（`x`十六进制 / `d`十进制 / `s`字符串 / `i`指令）、`U`=单位（`b`字节 / `w`四字节 / `g`八字节）。

## disassemble：看崩溃点的机器码

当源码行不足以定位（优化版 / 无调试信息），反汇编是最后的真相来源：

```gdb
(gdb) disassemble /m main     # 反汇编 main，并交错源码行（需调试信息）
Dump of assembler code for function main:
27          int *p = NULL;
   0x00005555555551a0 <+21>:  movq   $0x0,-0x8(%rbp)
28          *p = 42;
   0x00005555555551a8 <+29>:  mov    -0x8(%rbp),%rax
   0x00005555555551ac <+33>:  movl   $0x2a,(%rax)   # ← 把 42 写进 rax 指向的地址；rax=0 → SIGSEGV
29          return 0;
```

崩溃指令 `movl $0x2a,(%rax)`：把 42 写进 `rax` 指向的内存，而 `rax = -0x8(%rbp) = NULL`，于是写 0 地址触发段错误。这就是「为什么崩」的机器级铁证。

```gdb
(gdb) info registers rax
rax            0x0                 0
```

## HFT 关联

1. **`bt full` 是崩溃分析第一命令**：core 加载后 `bt full` 一次拿到「完整调用链 + 每层局部变量」，多数段错误/断言一眼定位，比逐行 `next` 高效一个量级。
2. **穿透库边界**：交易进程崩在 glibc/libstdc++ 内部时，`bt` 若断在库边界，说明缺 debuginfo 包——装 `libc6-dbg` 后才能看到是**你的哪行代码**把坏参数传进了库里。
3. **`info frame` 读寄存器佐证**：优化版上源码行错位时，用 `disassemble /m` + `info registers` 还原「到底执行到哪条指令」，是性能版崩溃定位的兜底手段。
4. **帧指针策略**：交易系统若追求极致性能用 `-O2 -fomit-frame-pointer`，务必**保留 `.eh_frame`**（别过度 strip），否则线上 core 的 `bt` 回溯会残缺。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么 `bt` 能从崩溃点还原出整条调用链？

> 每个函数调用都会在栈上保存「返回地址」和「上一帧的帧指针」。x86_64 用 `rbp` 链：当前 `rbp` 指向的位置存着上一层 `rbp`，其上 8 字节是返回地址。gdb 顺这条链逐层回溯，配合 `.debug_line` 把返回地址映射回源码行号，就得到了调用链。

**Q2:** `-fomit-frame-pointer` 之后 `bt` 还能工作吗？靠什么？

> 能，但机制变了。没有 `rbp` 链后，回溯依赖 `.eh_frame` 段里的 DWARF CFI（Call Frame Information）展开规则，它记录了「每条指令处如何从当前栈/寄存器还原上一层」。前提是二进制没 strip 掉 `.eh_frame`。所以优化版崩溃回溯，务必保留该段。

**Q3:** `bt full` 和 `bt` 的区别？什么时候必须用 full？

> `bt` 只列每帧的函数名、地址、源码位置；`bt full` 额外打印每帧的**局部变量**。定位「某个变量在哪一层被设成了坏值」时必须用 full——它一次给出整条链上所有局部状态，省去逐帧 `frame N` + `info locals` 的手动切换。

**Q4:** AArch64 上返回地址存在 `x30`（lr），那函数调用更深一层时，旧的 lr 怎么办？

> 被调函数入口序言 `stp x29,x30,[sp,#-16]!` 会把调用者的 `x29`（帧指针）和 `x30`（返回地址）一起压栈保存，然后 `mov x29,sp` 建立新帧。这样每层函数的 lr 都被压在栈上，形成与 x86 类似的回溯链。叶子函数若不再调用别人，可能不压栈（直接用 lr 返回），此时回溯到该帧要依赖 CFI 展开规则。

**Q5:** `print` 一个 `struct order *` 得到什么？想看结构体字段用什么？

> `print` 指针得到指针值（一个十六进制地址）。要看它指向的结构体内容，用 `print *head` 解引用，会展开所有字段；想只看单个字段用 `print head->price`。类型信息用 `ptype head`（看类型定义）或 `whatis head`（看声明）。

</details>

## 交叉引用

- [1.1 gdb 入门与调试信息](01-gdb-intro-build.md)
- [1.2 断点与观察点](02-breakpoints.md)
- [03.6 模块导读](../../README.md)
