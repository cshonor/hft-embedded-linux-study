# 3.1 valgrind memcheck（泄漏 / 越界 / UAF 精确定位）

> 🔴 精读 · 内存错误的「金标准」——慢，但准，且无需重编译

## 本节要点

valgrind 是 Linux 下最权威的动态内存分析工具，其默认子工具 **memcheck** 能在程序运行时抓到四类内存错误：**越界读写、use-after-free、内存泄漏、未初始化值**。它最独特的地方是**不需要重新编译**——直接 `valgrind ./prog` 就能对现有二进制做全量检查。代价是慢（约 20–50×），所以定位在「开发期定性」而不是「生产期测速」。本节讲清它的原理、命令、报告解读，并用贯穿示例 `c3_1_mem_bugs.c` 逐条对照。

## 先看贯穿示例

本节和 3.2 共用同一份「病人」：`code/c3_1_mem_bugs.c`。它埋了五类内存雷，用 argv 选哪一颗 —— 这样「一个 bug、多种工具」的对照才有意义（同一份源码，valgrind 与 ASan 两套报告可以逐字段比）：

```c
/* code/c3_1_mem_bugs.c —— 五个内存错误，一个程序，用 argv 选 */
static volatile int g_sink;         /* 让「读」这件事真发生，别被优化掉 */

/* ---- case 1：堆越界写 ---- */
static void bug_heap_overflow(void)
{
    char *buf = malloc(8);          /* 申请 8 字节 */
    if (!buf) return;
    memset(buf, 'A', 8);
    buf[8] = 'X';                   /* ← 第 9 个字节：踩进 malloc 块右侧的红区 */
    g_sink = buf[0];
    printf("case 1: 写了 buf[8]（块只有 8 字节），程序继续跑\n");
    free(buf);
}

/* ---- case 2：栈越界写 ---- */
static void bug_stack_overflow(void)
{
    char src[32];
    char dst[8];

    memset(src, 'B', sizeof(src));
    memcpy(dst, src, sizeof(src));  /* ← 32 字节塞进 8 字节的栈数组 */
    g_sink = dst[0];
    printf("case 2: 把 32 字节 memcpy 进 char dst[8]，程序继续跑\n");
}

/* ---- case 3：use-after-free ---- */
static void bug_use_after_free(void)
{
    int *p = malloc(4 * sizeof(int));
    if (!p) return;
    p[0] = 42;
    free(p);
    p[0] = 7;                       /* ← 释放后再写：踩进已中毒的隔离区 */
    g_sink = p[0];
    printf("case 3: free 后又写了 p[0]，程序继续跑\n");
}
```

（`case 4` = double free、`case 5` = 100 × 400 字节泄漏，完整源码见 `code/c3_1_mem_bugs.c`。）

⚠️ **本节的报告是「格式示意」，不是实测输出。** 原因说清楚：本仓库的验证环境（Windows 本地 + Compiler Explorer 容器）**都装不了 valgrind** —— 本地没有 Linux 环境，CE 只提供编译器和执行，不提供 valgrind 这类重型动态插桩工具。所以下面 valgrind 报告的**字段布局、措辞、行号体系依据 valgrind 官方手册与真实使用经验给出**，报告里的**地址、PID、行号是我按 `c3_1_mem_bugs.c` 的真实行号对齐的**，方便你上 Linux 机器时逐字段对上。

> **同一份病人在 ASan 下的报告是逐字实测的**（见 [3.2](02-addresssanitizer.md)）。两套报告结构同构（错误类型 → 读/写方栈 → 越界程度 → 分配方栈），所以「先读格式、再看真实样本」的组合阅读是成立的——**别把这里的地址当成可以复现的常量**。

**第一件事：裸编译先把「不崩」看一遍。** 加 `-Wall -Wextra` 编译运行：

```bash
gcc -g -O0 -Wall -Wextra -o c3_1_mem_bugs c3_1_mem_bugs.c
./c3_1_mem_bugs 1        # 堆越界
```

实测输出（退出码 **0**）：

```text
case 1: 写了 buf[8]（块只有 8 字节），程序继续跑
case 1 跑完了 main（没有崩，也没有任何提示 —— 这正是内存 bug 阴险的地方）
```

**什么都没崩。** 但编译器其实已经警告了——这就是为什么 `-Wall -Wextra` 是零成本的第一道防线（实测诊断 27 条，关键三条）：

```text
<source>:64:10: warning: pointer 'p' used after 'free' [-Wuse-after-free]          ← case 3
<source>:65:15: warning: pointer 'p' used after 'free' [-Wuse-after-free]          ← case 3
<source>:76:5:  warning: pointer 'p' used after 'free' [-Wuse-after-free]          ← case 4
<source>:51:5:  warning: 'memcpy' writing 32 bytes into a region of size 8
                overflows the destination [-Wstringop-overflow=]                  ← case 2（栈越界）
```

注意编译器**抓到了 3 颗雷（case 2/3/4）却抓不到 case 1（堆越界写 `buf[8]='X'`）和 case 5（泄漏）**——原因很实在：`buf[8]` 越界写、`malloc` 后不 `free`，都不是「编译期可判定的静态事实」，需要**运行时**才知道（堆块边界、生命周期）。**编译器看得懂的交给编译器，看不懂的才需要 valgrind/ASan** —— 这就是动态工具不可替代的位置。

下面逐段解读 valgrind 的报告（格式示意）。

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
==1234==    at 0x48C7B3E: bug_heap_overflow (c3_1_mem_bugs.c:38)
==1234==    by 0x48C7B52: main (c3_1_mem_bugs.c:97)
==1234==  Address 0x4a4a048 is 0 bytes after a block of size 8 alloc'd
==1234==    at 0x483B7F3: malloc (vg_replace_malloc.c:393)
==1234==    by 0x48C7B2E: bug_heap_overflow (c3_1_mem_bugs.c:34)
```

读报告的四步法：

1. **错误类型 + 大小**：`Invalid write of size 1` —— 往不可访问地址写了 1 字节。case 1 写的是 `buf[8] = 'X'`（一个 char 赋值），所以是 size 1 而不是 size 16。
2. **发生位置**（写方）：`at bug_heap_overflow (c3_1_mem_bugs.c:38)` —— 越界发生在第 38 行。
3. **越界程度**：`0 bytes after a block of size 8` —— 地址正好落在 `malloc(8)` 块的**末尾之外第一个字节**。**只越界了 1 字节**——所以裸编译时「什么都没发生」（对比 case 2 一次越界 24 字节，往往当场就出事）。
4. **分配位置**（块归属）：`malloc ... by bug_heap_overflow (c3_1_mem_bugs.c:34)` —— 这块内存是第 34 行 `malloc(8)` 的。

> **定位口诀**：`at` 是「谁写的」，`alloc'd` 是「谁分配的」，两者夹击就能看出「某函数分配了 N 字节，却写了 N+k 字节」。

> 📌 **和 3.2 的实测逐字对照**：ASan 对**同一行**（`c3_1_mem_bugs.c:38`）的措辞是 `WRITE of size 1` + `0 bytes after 8-byte region [0x602000000010,0x602000000018)`——两套工具连「0 bytes」的计数口径都一致（都指「块末尾之后第一个字节」），只有前缀格式不同（`Invalid write of size 1` vs `WRITE of size 1`）。

### ② use-after-free：`Invalid write` + `Address is ... inside a block ... free'd`

```text
==1234== Invalid write of size 4
==1234==    at 0x48C7C8E: bug_use_after_free (c3_1_mem_bugs.c:64)
==1234==    by 0x48C7B57: main (c3_1_mem_bugs.c:99)
==1234==  Address 0x4a4a0a0 is 0 bytes inside a block of size 16 free'd
==1234==    at 0x483C9F4: free (vg_replace_malloc.c:540)
==1234==    by 0x48C7C80: bug_use_after_free (c3_1_mem_bugs.c:63)
```

关键句 `0 bytes inside a block of size 16 free'd`：访问的地址在**一个已 free 的 16 字节块内部**（偏移 0）。最后一段 `free ... by bug_use_after_free (c3_1_mem_bugs.c:63)` 告诉你这个块是第 63 行释放的。**第 64 行写了第 63 行刚 free 的东西**——UAF 实锤。

> 注意 case 3 是**写**（`p[0] = 7`）不是读，所以报 `Invalid write of size 4`（`int` 4 字节）。ASan 对同一行的措辞同样是 `WRITE of size 4`。**「释放后再写」比「释放后再读」更危险**——写会污染可能已被别人复用的内存，正是 HFT 里「偶发错单」的典型形态。

### ③ 泄漏：`LEAK SUMMARY`

```text
==1234== HEAP SUMMARY:
==1234==     in use at exit: 40,000 bytes in 100 blocks
==1234==   total heap usage: 100 allocs, 0 frees, 40,000 bytes allocated
==1234==
==1234== LEAK SUMMARY:
==1234==    definitely lost: 40,000 bytes in 100 blocks
==1234==    indirectly lost: 0 bytes in 0 blocks
==1234==      possibly lost: 0 bytes in 0 blocks
==1234==    still reachable: 0 bytes in 0 blocks
==1234==         suppressed: 0 bytes in 0 blocks
==1234== Rerun with --leak-check=full to see details of leaked memory
```

（这是 `./c3_1_mem_bugs 5` 单跑 case 5 的样子：`100 allocs, 0 frees`——**贷了 100 笔，一笔没还**，正是 case 5 代码里那个循环。ASan/LSan 对同一份代码的实测汇总完全对得上：`SUMMARY: AddressSanitizer: 40000 byte(s) leaked in 100 allocation(s).`）

泄漏的四种定性（**必须分清**，否则会误判）：

| 分类 | 含义 | 是否真泄漏 |
|------|------|-----------|
| **definitely lost** | 没有任何指针指向它，无法再访问 | ✅ 真泄漏，必须修 |
| **indirectly lost** | 指针被 lost 块「间接」引用（如链表头丢了，节点也丢了） | ✅ 真泄漏（跟着 definitely lost 一起丢的） |
| **possibly lost** | 有指针指向块「内部」而非开头（如 `p = &arr[3]`） | ⚠️ 可能泄漏，需人工判断 |
| **still reachable** | 程序退出时仍有有效指针指向 | ❌ 通常不是泄漏（全局缓存/单例），但长跑进程也要看 |

加 `--leak-check=full --show-leak-kinds=all` 看每个泄漏块的分配栈：

```text
==1234== 40,000 bytes in 100 blocks are definitely lost in loss record 1 of 1
==1234==    at 0x483B7F3: malloc (vg_replace_malloc.c:393)
==1234==    by 0x48C7D1E: bug_leak (c3_1_mem_bugs.c:84)   ← 泄漏点：第 84 行 malloc(400)
==1234==    by 0x48C7D30: main (c3_1_mem_bugs.c:101)
```

> **对照实测**：ASan/LSan 给出的分配栈是 `#1 0x40152c in bug_leak /app/example.c:84` / `#2 0x4015ef in main /app/example.c:101` —— **行号 84/101 与 valgrind 格式示意里的完全一致**（因为就是同一份源码的同一行），只是地址与调用栈符号化风格不同。这说明两套报告在「定位到哪一行」这件事上是等价的，差别只在措辞。

### ④ 未初始化读：`Conditional jump depends on uninitialised value`

这一条用独立 demo `code/c3_2_uninit_read.c`（未初始化读要从「内存错误」里单独拎出来讲——它和越界/UAF 是**不同维度**的问题）：

```c
/* code/c3_2_uninit_read.c 节选 */
struct order { long id; long qty; long price; };

static volatile long g_sink;

int main(void)
{
    int stack_var;                       /* 未初始化：值 = 栈上的残留 */
    struct order *o = malloc(sizeof *o); /* 未初始化：值 = 堆上的残留 */
    if (!o) return 1;

    o->id = 1;
    /* 注意：qty / price 故意不赋值 */

    printf("stack_var = %d\n", stack_var);
    printf("o->qty = %ld, o->price = %ld\n", o->qty, o->price);

    g_sink = stack_var + o->qty;

    if (o->qty > 0)                      /* 用垃圾值做分支：行为不可预测 */
        printf("qty > 0 分支被走到（这完全取决于栈/堆里剩了什么）\n");
    else
        printf("qty > 0 分支没走到（同上）\n");

    free(o);
    return 0;
}
```

valgrind 报告格式示意（用 `--track-origins=yes` 才能追到源头）：

```text
==1234== Conditional jump depends on uninitialised value(s)
==1234==    at 0x48C7D90: main (c3_2_uninit_read.c:40)
==1234==  Uninitialised value was created by a heap allocation
==1234==    at 0x483B7F3: malloc (vg_replace_malloc.c:393)
==1234==    by 0x48C7D45: main (c3_2_uninit_read.c:28)   ← 源头：第 28 行的 malloc 块从未写全
```

这里只告诉你「第 40 行的 `if (o->qty > 0)` 依赖了未初始化值」，但**没告诉你是谁污染了这个值**。要追溯来源，必须加 `--track-origins=yes`（代价是更慢）——加了之后报告末尾会多一段 `Uninitialised value was created by ...`，指出第 28 行的 `malloc` 是源头（`qty` 从没被赋值过）。

> ⚠️ **这一条也是本模块里唯一没能拿到实测输出的** —— 如实说明：
> - **ASan 抓不到它**（实测：`gcc -g -O0 -fsanitize=address` 跑 `c3_2_uninit_read.c`，**退出码 0、零报告**）。ASan 只查「地址是否可访问」，不查「值是否已初始化」——这正是本节和 3.2 的分工边界。
> - **MSan 才是它的对口工具**（`clang -fsanitize=memory`），但 MSan 要求**整条依赖链**（包括 libc）都用 MSan 重编，且需要 `-fsanitize-memory-track-origins` 才有「来源追溯」。我在 Compiler Explorer 的容器里实测 MSan 跑这份程序得到的是 `Program terminated with signal SIGSEGV (11)` 而**不是** MSan 的 `use-of-uninitialized-value` 报告——判断是该容器的 C++/libc 没做 MSan 插桩导致的假失败，**因此我不把 MSan 的输出写进本节**。你在自己的 Linux 机器上按上面命令跑能得到真实报告。
> - **可确认的部分**：源码第 35/36 行会直接打印出未初始化值，实测（ASan 构建）打出的是
>   ```text
>   stack_var = 0
>   o->qty = -4702111234474983746, o->price = -4702111234474983746
>   qty > 0 分支没走到（同上）
>   ```
>   这串 `-4702111234474983746`（十六进制 `0xBEBEBEBEBEBEBEBE`）就是栈/堆里的残留垃圾——**「值不对」这件事被实测坐实了，只是抓不到它的工具不在本环境**。

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
         --error-exitcode=1 ./c3_1_mem_bugs 5
# 只有「确定泄漏」才导致 CI 失败，still reachable 不拦
```

## valgrind 的局限（必须诚实评估）

1. **慢**：20–50× 开销，程序里跑 1 秒的路径，valgrind 下要 30 秒。**绝不能拿它测延迟/吞吐**（测出来的数字毫无意义，见 Ch6 性能类）。
2. **只认「可执行指令」**：它是 DBI，只能看到实际执行的代码路径。不执行的 `if` 分支里的 bug 它看不见（所以要用测试用例覆盖到）。
3. **对栈越界盲**：memcheck 抓**堆**越界很准，但对**栈**数组越界（`char dst[8]; memcpy(dst, src, 32)`）历史上抓不到，因为它没给栈变量做 redzone。**这条有实测对照**：同一条 `c3_1_mem_bugs.c` 的 case 2（`char dst[8]` 被 `memcpy` 灌 32 字节），ASan 精确报出
   ```text
   ERROR: AddressSanitizer: stack-buffer-overflow ... WRITE of size 32
       #1 bug_stack_overflow (c3_1_mem_bugs.c:51)
   Address ... is located in stack of thread T0 at offset 40 in frame
       #0 bug_stack_overflow (c3_1_mem_bugs.c:46)
     This frame has 2 object(s):
       [32, 40) 'dst' (line 48)
       [64, 96) 'src' (line 47) <== Memory access at offset 40 partially underflows this variable
   ```
   连「栈帧里两个对象各占哪段地址、越界落在哪个对象的边界」都画出来了——**这是 ASan 存在的核心理由之一**（见 3.2）。
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

**Q6:** 本节里哪些内容是「实测」，哪些是「格式示意」？为什么要区分？

> **实测**：裸编译 `-Wall -Wextra` 的诊断 27 条、case 1 退出 0 且打印「程序继续跑」、ASan/LSan 对 case 1/2/3/4/5 的报告、`c3_2_uninit_read.c` 在 ASan 下退出 0（零报告）及打印出的垃圾值。**格式示意**：所有 `==1234==` 开头的 valgrind 报告（本环境装不了 valgrind，行号按真实源码对齐、措辞按官方手册）。区分的理由是：**「工具能抓到什么」是知识（可引用手册），「在你这台机器上抓到什么」是证据（必须真跑）**。把示意当实测，就会在别人机器上等一份永远不会出现的报告——这正是 1.4 讲的「信数据」的反面。

</details>

## 交叉引用

- [3.2 AddressSanitizer](02-addresssanitizer.md)
- [3.3 UndefinedBehaviorSanitizer](03-undefinedbehaviorsanitizer.md)
- [2.6 深入内存分析](../../chapter-02-crash/notes/06-analyze-corrupted-memory.md)
- [Ch3 内存类](../README.md)
