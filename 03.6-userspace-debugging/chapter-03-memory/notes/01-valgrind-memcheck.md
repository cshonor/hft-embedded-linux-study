# 3.1 valgrind memcheck（泄漏 / 越界 / UAF 精确定位）

> 🔴 精读 · 内存错误的「金标准」——慢，但准，且无需重编译

## 本节要点

valgrind 是 Linux 下最权威的动态内存分析工具，其默认子工具 **memcheck** 能在程序运行时抓到四类内存错误：**越界读写、use-after-free、内存泄漏、未初始化值**。它最独特的地方是**不需要重新编译**——直接 `valgrind ./prog` 就能对现有二进制做全量检查。代价是慢（约 20–50×），所以定位在「开发期定性」而不是「生产期测速」。本节讲清它的原理、命令、报告解读，并用贯穿示例 `mem_bugs.c` 逐条对照。

## 先看贯穿示例

下面这个文件埋了四类内存雷（越界写 / UAF / 泄漏 / 未初始化读）+ 两个 UB 雷（溢出 / 移位，留给 3.3）。Ch3 三节都用它：

```c
// mem_bugs.c —— Ch3 贯穿示例：内存错误 + 未定义行为
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 雷 1：堆越界写（heap buffer overflow）
void bug_heap_overflow(void) {
    char *buf = malloc(8);          // 只分配 8 字节
    strcpy(buf, "0123456789ABCDEF"); // 16 字节写进去 → 越界 8 字节
    printf("buf=%s\n", buf);
    free(buf);
}

// 雷 2：use-after-free（free 后再读）
void bug_uaf(void) {
    char *p = malloc(16);
    strcpy(p, "order-12345");
    free(p);                        // 释放
    printf("after free: %s\n", p);  // UAF：free 后再解引用
}

// 雷 3：内存泄漏（分配后从不 free）
void bug_leak(void) {
    for (int i = 0; i < 100; i++) {
        int *x = malloc(sizeof(int)); // 从不 free，100 次 → 慢泄漏
        *x = i;
    }
}

// 雷 4：未初始化读（读栈上未初始化的局部变量）
void bug_uninit(void) {
    int x;               // 未初始化
    if (x > 0)           // 读未初始化值 → 结果取决于垃圾值
        printf("positive\n");
}

// 雷 5：有符号整数溢出（UB，valgrind 不报，3.3 UBSan 报）
void bug_signed_overflow(void) {
    int x = 2147483647;  // INT_MAX
    int y = x + 1;       // 有符号溢出 → 未定义行为
    printf("y=%d\n", y);
}

// 雷 6：移位越界（UB，3.3 UBSan 报）
void bug_shift(void) {
    int x = 1 << 40;     // 移位数 >= 位宽(32) → 未定义行为
    printf("x=%d\n", x);
}

int main(void) {
    bug_heap_overflow();
    bug_uaf();
    bug_leak();
    bug_uninit();
    bug_signed_overflow();
    bug_shift();
    return 0;
}
```

普通编译运行，前四个雷**要么不崩、要么崩得毫无线索**：

```bash
gcc -g -O0 -o mem_bugs mem_bugs.c
./mem_bugs
# buf=0123456789ABCDEF        ← 越界写「成功」了，没崩
# after free: order-12345     ← UAF 读了已释放内存，没崩
# y=-2147483648               ← 溢出悄悄回绕成负数，没崩
# x=0                         ← 移位 UB 悄悄给了 0，没崩
# (可能某次运行在这里崩，也可能不崩——这就是内存 bug 的阴险)
```

**注意：什么都没崩**，但程序里埋着 6 个雷。这就是内存 bug 的可怕——它「潜伏」，等你上线几周后才在别处爆。valgrind 登场：

```bash
valgrind ./mem_bugs
```

下面逐段解读它的报告。

## 原理：影子内存（shadow memory）

valgrind 不是「插桩编译」而是「动态二进制翻译」（Dynamic Binary Instrumentation, DBI）：它把程序的机器码切成基本块，翻译成等价但**额外带检查**的代码，在一个「虚拟 CPU」上执行。程序自己完全不知道自己跑在 valgrind 里。

memcheck 的核心是**影子内存**——为程序的每个 bit 维护两份元数据：

| 元数据 | 含义 | 追踪什么 |
|--------|------|----------|
| **A bit（addressability）** | 这个字节「可不可访问」 | 是否已 malloc / 已 free / 越界 |
| **V bit（validity）** | 这个 bit 的值「有没有被初始化」 | 未初始化值（垃圾值）传播 |

程序每读一个字节，memcheck 检查 A bit：不可访问 → 报 `Invalid read`；每读一个未初始化的 bit（V=0）→ 记下来，一旦这个值**影响了控制流或系统调用**就报 `Conditional jump depends on uninitialised value`。

```
应用内存     [0x40][0x41][0x42][0x43] ...   ← 你 malloc 的 8 字节
A bit       [  1 ][  1 ][  1 ][  1 ][ 0 ][ 0 ]  ← 第 8 字节之后 = 不可访问
V bit       [  1 ][  1 ][  0 ][  0 ] ...        ← 0 = 未初始化
                                          ↑ 写越界到 0x48 会被 A bit 抓
```

**为什么越界写「不崩」也被抓到**：因为 memcheck 不看 CPU 会不会 SIGSEGV，它看的是 A bit——只要写到一个 A=0 的地址，立刻报错，哪怕这块地址物理上恰好可写。这就是它比「等程序崩溃」强得多的原因：**它抓「未遂犯罪」，而不是等「命案发生」**。

## 四类报告逐条解读

### ① 越界写：`Invalid write`

```text
==1234== Invalid write of size 1
==1234==    at 0x48C7B3E: bug_heap_overflow (mem_bugs.c:11)
==1234==    by 0x48C7B52: main (mem_bugs.c:42)
==1234==  Address 0x4a4a048 is 8 bytes after a block of size 8 alloc'd
==1234==    at 0x483B7F3: malloc (vg_replace_malloc.c:393)
==1234==    by 0x48C7B2E: bug_heap_overflow (mem_bugs.c:10)
```

读报告的四步法：

1. **错误类型 + 大小**：`Invalid write of size 1` —— 往不可访问地址写了 1 字节（`strcpy` 逐字节拷，所以报 size 1 而不是 size 16）。
2. **发生位置**（写方）：`at bug_heap_overflow (mem_bugs.c:11)` —— 越界发生在第 11 行。
3. **越界程度**：`8 bytes after a block of size 8` —— 写到 malloc(8) 块**之后** 8 字节处，即恰好写满了一倍。
4. **分配位置**（块归属）：`malloc ... by bug_heap_overflow (mem_bugs.c:10)` —— 这块内存是第 10 行 malloc 的。

> **定位口诀**：`at` 是「谁写的」，`alloc'd` 是「谁分配的」，两者夹击就能看出「某函数分配了 N 字节，却写了 N+k 字节」。

### ② use-after-free：`Invalid read` + `Address is on heap`

```text
==1234== Invalid read of size 1
==1234==    at 0x48C7C8E: bug_uaf (mem_bugs.c:19)
==1234==    by 0x48C7B57: main (mem_bugs.c:43)
==1234==  Address 0x4a4a0a0 is 0 bytes inside a block of size 16 free'd
==1234==    at 0x483C9F4: free (vg_replace_malloc.c:540)
==1234==    by 0x48C7C80: bug_uaf (mem_bugs.c:18)
```

关键句 `0 bytes inside a block of size 16 free'd`：读的地址在**一个已 free 的 16 字节块内部**（偏移 0）。最后一段 `free ... by bug_uaf (mem_bugs.c:18)` 告诉你这个块是第 18 行 free 的。**第 19 行读了第 18 行刚 free 的东西**——UAF 实锤。

### ③ 泄漏：`LEAK SUMMARY`

```text
==1234== HEAP SUMMARY:
==1234==     in use at exit: 408 bytes in 101 blocks
==1234==   total heap usage: 106 allocs, 5 frees, 8,704 bytes allocated
==1234==
==1234== LEAK SUMMARY:
==1234==    definitely lost: 400 bytes in 100 blocks
==1234==    indirectly lost: 0 bytes in 0 blocks
==1234==      possibly lost: 0 bytes in 0 blocks
==1234==    still reachable: 8 bytes in 1 blocks
==1234==         suppressed: 0 bytes in 0 blocks
==1234== Rerun with --leak-check=full to see details of leaked memory
```

泄漏的四种定性（**必须分清**，否则会误判）：

| 分类 | 含义 | 是否真泄漏 |
|------|------|-----------|
| **definitely lost** | 没有任何指针指向它，无法再访问 | ✅ 真泄漏，必须修 |
| **indirectly lost** | 指针被 lost 块「间接」引用（如链表头丢了，节点也丢了） | ✅ 真泄漏（跟着 definitely lost 一起丢的） |
| **possibly lost** | 有指针指向块「内部」而非开头（如 `p = &arr[3]`） | ⚠️ 可能泄漏，需人工判断 |
| **still reachable** | 程序退出时仍有有效指针指向 | ❌ 通常不是泄漏（全局缓存/单例），但长跑进程也要看 |

加 `--leak-check=full --show-leak-kinds=all` 看每个泄漏块的分配栈：

```bash
valgrind --leak-check=full --show-leak-kinds=all ./mem_bugs
# ==1234== 400 bytes in 100 blocks are definitely lost in loss record 1 of 2
# ==1234==    at 0x483B7F3: malloc (vg_replace_malloc.c:393)
# ==1234==    by 0x48C7D1E: bug_leak (mem_bugs.c:26)   ← 泄漏点：第 26 行 malloc
# ==1234==    by 0x48C7D30: main (mem_bugs.c:44)
```

### ④ 未初始化读：`Conditional jump depends on uninitialised value`

```text
==1234== Conditional jump depends on uninitialised value(s)
==1234==    at 0x48C7D90: bug_uninit (mem_bugs.c:33)
==1234==    by 0x48C7D45: main (mem_bugs.c:45)
```

这里只告诉你「第 33 行的 if 依赖了未初始化值」，但**没告诉你是谁污染了这个值**。要追溯来源，加 `--track-origins=yes`（代价是更慢）：

```bash
valgrind --track-origins=yes ./mem_bugs
# ==1234== Conditional jump depends on uninitialised value(s)
# ==1234==    at 0x...: bug_uninit (mem_bugs.c:33)
# ==1234==  Uninitialised value was created by a stack allocation
# ==1234==    at 0x...: bug_uninit (mem_bugs.c:30)   ← 第 30 行 int x; 是源头
```

## 常用命令选项

| 选项 | 作用 |
|------|------|
| `--leak-check=full` | 退出时详细报告每个泄漏块的分配栈（默认 summary 只给汇总） |
| `--show-leak-kinds=all` | 显示所有泄漏类型（默认只显示 definite + possible） |
| `--track-origins=yes` | 追溯未初始化值的来源（更慢） |
| `--error-exitcode=1` | 发现错误时让 valgrind 以退出码 1 结束（用于 CI 判失败） |
| `--errors-for-leak-kinds=definite` | 只有 definite 泄漏才算「错误」（CI 常用，忽略 still reachable） |
| `--num-callers=20` | 加大回溯栈深度（默认 12，深调用链会被截断） |
| `--child-silent-after-fork=yes` | fork 后子进程不重复输出 |
| `--log-file=v.log` | 报告写文件而非 stderr |
| `--xml=yes --xml-file=v.xml` | XML 输出（CI 解析用） |

CI 里跑泄漏检查的推荐组合：

```bash
valgrind --leak-check=full --errors-for-leak-kinds=definite \
         --error-exitcode=1 ./mem_bugs
# 只有「确定泄漏」才导致 CI 失败，still reachable 不拦
```

## valgrind 的局限（必须诚实评估）

1. **慢**：20–50× 开销，程序里跑 1 秒的路径，valgrind 下要 30 秒。**绝不能拿它测延迟/吞吐**（测出来的数字毫无意义，见 Ch6 性能类）。
2. **只认「可执行指令」**：它是 DBI，只能看到实际执行的代码路径。不执行的 `if` 分支里的 bug 它看不见（所以要用测试用例覆盖到）。
3. **对栈越界盲**：memcheck 抓**堆**越界很准，但对**栈**数组越界（`char buf[8]; strcpy(buf, "...")`）历史上抓不到，因为它没给栈变量做 redzone（ASan 能抓，见 3.2）。
4. **不查数据竞争**：内存错误和并发竞态是两个维度，valgrind 另有 `helgrind` / `drd` 子工具（Ch4 并发类），memcheck 不管。
5. **对 UB 无感**：有符号溢出、移位越界这些「未定义行为但不算内存错误」，memcheck 完全没反应（3.3 UBSan 的活）。

## HFT 关联

1. **7×24 长跑进程的慢泄漏是头号杀手**：撮合引擎、行情网关这种「永不重启」的进程，每次处理订单泄漏几十字节，几周后内存涨到 OOM 被杀。valgrind 在**开发期**就能把泄漏点钉到具体 malloc 行号，比上线后盯着 RSS 曲线猜强一百倍。
2. **「不崩」的越界比崩溃更危险**：越界写可能写坏的是**相邻的订单对象**（价格字段被覆盖、数量被篡改），导致的是「错单」而非崩溃——这类问题 valgrind 的 A bit 检查能抓到，而等它自然崩溃可能永远等不到。
3. **定位口诀服务 HFT 场景**：`at`（谁写）+ `alloc'd`（谁分配）夹击，能快速定位「订单对象 16 字节却拷贝了 20 字节」这类结构体大小算错的 bug。
4. **CI 门禁**：把 valgrind 挂到 nightly 构建（不用挂每次提交，太慢），配合 `--error-exitcode=1 --errors-for-leak-kinds=definite`，专门拦截 definite 泄漏。

```bash
# HFT 场景：对撮合引擎跑一轮仿真行情，抓泄漏
valgrind --leak-check=full --show-leak-kinds=definite \
         --log-file=engine_leak.log ./matching_engine --sim data.csv
grep "definitely lost" engine_leak.log   # 非 0 就有人泄漏
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** valgrind 为什么「不需要重新编译」就能检查？和 ASan 的本质区别是什么？

> valgrind 用**动态二进制翻译（DBI）**：运行时不改你的二进制，而是把它翻译成带检查的等价代码在虚拟 CPU 上跑，所以对任意现成二进制都能查。ASan 是**编译期插桩**，要在编译时加 `-fsanitize=address` 把检查代码编进程序，所以必须重编译。代价：valgrind 通用但慢（20–50×），ASan 快（约 2×）但要重编译。

**Q2:** `definitely lost` 和 `still reachable` 的区别？哪个才是必须修的泄漏？

> `definitely lost`：没有任何指针指向它，程序再也无法访问这块内存——这是**真泄漏**，必须修。`still reachable`：程序退出时仍有有效指针指向它（如全局缓存、单例对象），通常**不是泄漏**，是「刻意持有到程序结束」。对短命进程两者都无害（进程退出时 OS 全回收），但对 7×24 长跑进程，只有 definitely lost 会真正累积。

**Q3:** `Conditional jump depends on uninitialised value` 报告为什么只给「读的位置」不给「污染来源」？怎么拿到来源？

> 因为 memcheck 默认只记录「哪个值未初始化、在哪被读」，不记录「这个未初始化值从哪来」（记录来源需要额外 shadow 开销）。加 `--track-origins=yes` 后，memcheck 会额外追踪未初始化值的起源，报告末尾多一行 `Uninitialised value was created by ...` 指出分配点。代价是更慢。

**Q4:** 为什么 memcheck 能抓到「没崩溃的越界写」？

> 因为它不看 CPU 是否 SIGSEGV，而看**影子内存的 A bit（addressability）**。只要程序写到一个 A=0 的地址（malloc 块之外、或已 free 的块），立即报 `Invalid write`，哪怕那块地址物理上恰好可写、程序根本没崩。它抓的是「越界行为」本身，不是「越界后果」。这正是「未遂犯罪」和「命案发生」的区别。

**Q5:** valgrind 对栈数组越界为什么相对盲？这类问题交给谁？

> memcheck 主要给**堆**分配做红区标记，对**栈**上的局部数组（`char buf[8]`）默认不做细粒度 redzone，所以 `strcpy(buf, "长字符串")` 这种栈溢出它常抓不到。这类问题交给 **ASan**（3.2），ASan 在编译期给栈变量也插入了 redzone，能精确抓到栈越界。

</details>

## 交叉引用

- [3.2 AddressSanitizer](02-addresssanitizer.md)
- [3.3 UndefinedBehaviorSanitizer](03-undefinedbehaviorsanitizer.md)
- [2.6 深入内存分析](../../chapter-02-crash/notes/06-analyze-corrupted-memory.md)
- [Ch3 内存类](../README.md)
