# 3.2 AddressSanitizer（ASan 快速内存错误检测）

> 🔴 精读 · 编译期插桩的「快检」——比 valgrind 快一个量级，但要重编译

## 本节要点

AddressSanitizer（ASan）是 Google 出品的编译期内存错误检测器，集成在 GCC / Clang 里，加 `-fsanitize=address` 即可。它和 valgrind 抓的是**同一类**内存错误（越界 / UAF / 泄漏 / 未初始化），但实现思路相反：**编译时把检查代码插进程序**，运行时靠「红区（redzone）+ 影子内存」高速判断。代价是必须重编译，换来约 **2× 开销**（valgrind 是 20–50×），快到可以在 CI 里每次提交都跑。本节讲原理、报错解读、与 valgrind 的取舍。

## 编译与运行

本节所有输出都是**实测**（gcc 13.3.0，Compiler Explorer 容器）。同一份病人 `code/c3_1_mem_bugs.c`，五个 case 各跑一次：

```bash
# 五个 case 都编译成 ASan 版本（用 -O0，原因见下）
gcc -g -O0 -fsanitize=address -o c3_1_asan c3_1_mem_bugs.c
./c3_1_asan 1        # 堆越界
./c3_1_asan 2        # 栈越界
./c3_1_asan 3        # use-after-free
./c3_1_asan 4        # double free
./c3_1_asan 5        # 泄漏
```

> `-g` 必须有（报错要显示源码行号）。**优化档位是本节第一个大坑**：ASan 官方推荐 `-O1`，但这套 demo 必须用 `-O0` —— 实测 `-O1` 下 **case 3 的 UAF 会被优化掉（exit 0、零报告）**，详见本节末尾「优化会改变 bug 的存在性」。
>
> 📎 **关于报告里的文件名**：Compiler Explorer 会把上传的源码落成 `/app/example.c`，所以下面的栈里显示 `example.c:38`——**行号对应我们的 `c3_1_mem_bugs.c`**（两者内容逐字节相同）。

五个 case 的实测总览：

| case | 错误类型（实测 SUMMARY 行） | 触发行 | 退出码 |
|------|---------------------------|--------|--------|
| 1 | `AddressSanitizer: heap-buffer-overflow /app/example.c:38 in bug_heap_overflow` | 38 | **1** |
| 2 | `AddressSanitizer: stack-buffer-overflow ... in __interceptor_memcpy` | 51 | **1** |
| 3 | `AddressSanitizer: heap-use-after-free /app/example.c:64 in bug_use_after_free` | 64 | **1** |
| 4 | `AddressSanitizer: double-free ...` | 76 | **1** |
| 5 | `AddressSanitizer: 40000 byte(s) leaked in 100 allocation(s).`（LSan） | 84 | **1** |

**ASan 用退出码 1 表示「检出问题」**（不是 139/136 那套信号码）——CI 里判失败直接看 `$?`。

### case 1：堆越界写（实测原文）

```text
=================================================================
==2==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000018 at pc 0x000000401295 bp 0x7ffcf92ecb80 sp 0x7ffcf92ecb78
WRITE of size 1 at 0x602000000018 thread T0
    #0 0x401294 in bug_heap_overflow /app/example.c:38
    #1 0x4015d0 in main /app/example.c:97
    #2 0x73678942a1c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: 328820b908de8ea1ef79afa8995e302e819163d7)
    #3 0x73678942a28a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: 328820b908de8ea1ef79afa8995e302e819163d7)
    #4 0x401164 in _start (/app/output.s+0x401164) (BuildId: baf7d14ac9ea8e1c22e3d3263d14e7b1e80e43f2)

0x602000000018 is located 0 bytes after 8-byte region [0x602000000010,0x602000000018)
allocated by thread T0 here:
    #0 0x73678989704f in malloc (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdc04f) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x401237 in bug_heap_overflow /app/example.c:34
    #2 0x4015d0 in main /app/example.c:97
    ...

SUMMARY: AddressSanitizer: heap-buffer-overflow /app/example.c:38 in bug_heap_overflow
Shadow bytes around the buggy address:
  0x601fffffff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x602000000000: fa fa 00[fa]fa fa fa fa fa fa fa fa fa fa fa fa
  0x602000000080: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  ...
```

和 valgrind 报告同构（错误类型 → 读写方栈 → 越界程度 → 分配方栈），但 `SUMMARY` 前面多了最后一段 **shadow bytes 图**，它把出问题地址附近的影子内存状态画出来，精确定位「越界了几个字节」。

**读这张影子图**（这是 ASan 最独有的信息）：`0x602000000000` 那一行，每个影子字节代表 8 字节应用内存，带方括号的那个是「出错地址所在的影子字节」：

```text
=>0x602000000000: fa fa 00[fa]fa fa fa fa ...
                   ↑  ↑  ↑  ↑
                   │  │  │  └─ 第 24~31 字节：堆红区（0xfa）← 我们写到了这里
                   │  │  └──── 第 16~23 字节：堆红区（0xfa）
                   │  └─────── 第 8~15 字节：可访问（0x00）← malloc(8) 的有效区间
                   └────────── 第 0~7 字节：堆左侧红区（0xfa）
```

`malloc(8)` 给了 8 字节可用（影子 `00`），前后各铺红区（`fa`）。`buf[8]` 越界到第 9 个字节，落在**第一个 `fa` 里**——方括号标出的正是它。**「越界 1 字节」这件事被画出来了**，比报错文字更直观。

### case 2：栈越界写（实测原文）

```text
=================================================================
==2==ERROR: AddressSanitizer: stack-buffer-overflow on address 0x7d6127200028 at pc 0x7d612989474f bp 0x7ffe6a79b0f0 sp 0x7ffe6a79a8b0
WRITE of size 32 at 0x7d6127200028 thread T0
    #0 0x7d612989474e in __interceptor_memcpy (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0x7174e) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x401370 in bug_stack_overflow /app/example.c:51
    #2 0x4015da in main /app/example.c:98
    ...

Address 0x7d6127200028 is located in stack of thread T0 at offset 40 in frame
    #0 0x4012d2 in bug_stack_overflow /app/example.c:46

  This frame has 2 object(s):
    [32, 40) 'dst' (line 48)
    [64, 96) 'src' (line 47) <== Memory access at offset 40 partially underflows this variable
HINT: this may be a false positive if your program uses some custom stack unwind mechanism, swapcontext or vfork
      (longjmp and C++ exceptions *are* supported)
SUMMARY: AddressSanitizer: stack-buffer-overflow (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0x7174e) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d) in __interceptor_memcpy
Shadow bytes around the buggy address:
  0x7d61271fff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x7d6127200000: f1 f1 f1 f1 00[f2]f2 f2 00 00 00 00 f3 f3 f3 f3
  0x7d6127200080: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
  ...
```

这段报告含金量最高，逐条读：

1. **`This frame has 2 object(s)`** —— ASan 把**栈帧里两个局部变量的地址区间**都列出来了：`dst` 在 `[32,40)`（8 字节，第 48 行声明），`src` 在 `[64,96)`（32 字节，第 47 行）。**valgrind 给不出这个**。
2. **`Memory access at offset 40 partially underflows this variable`** —— 出事的偏移 40 正好是 `dst` 的**末尾之外第一个字节**，同时落进 `src` 的区间里。所以 ASan 的措辞是「部分越过了这个变量」。
3. **影子图读法**：`f1 f1 f1 f1 00[f2]f2 f2 00 00 00 00 f3 f3 f3 f3`
   - `f1`×4 = 栈**左**红区（函数序言铺的）
   - `00` = `dst` 的 8 字节有效区
   - `[f2]f2 f2` = 栈**中**红区（两个对象之间的守卫）← **方括号就落在这里**，`memcpy` 的 32 字节写穿了 `dst`、冲进了中间红区
   - `00 00 00 00` = `src` 的 32 字节有效区
   - `f3`×4 = 栈**右**红区
4. **`WRITE of size 32`** —— 一次 `memcpy` 写 32 字节，所以是 size 32（不是逐字节的 size 1）。

> **这就是「ASan 能抓栈越界、valgrind 抓不到」的确切形态**：ASan 在编译期给每个栈对象两侧都插了红区（`f1`/`f2`/`f3`），所以越界一字节也躲不过；而且它能把栈帧布局打印出来。见 [3.1 局限第 3 条](01-valgrind-memcheck.md)。

### case 3：use-after-free（实测原文）

```text
=================================================================
==2==ERROR: AddressSanitizer: heap-use-after-free on address 0x602000000010 at pc 0x0000004014a9 bp 0x7ffe302ec8a0 sp 0x7ffe302ec898
WRITE of size 4 at 0x602000000010 thread T0
    #0 0x4014a8 in bug_use_after_free /app/example.c:64
    #1 0x4015e1 in main /app/example.c:99
    ...

0x602000000010 is located 0 bytes inside of 16-byte region [0x602000000010,0x602000000020)
freed by thread T0 here:
    #0 0x72dadca25d18  (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdad18) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x401471 in bug_use_after_free /app/example.c:63
    #2 0x4015e1 in main /app/example.c:99
    ...

previously allocated by thread T0 here:
    #0 0x72dadca2704f in malloc (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdc04f) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x401415 in bug_use_after_free /app/example.c:59
    #2 0x4015e1 in main /app/example.c:99
    ...

SUMMARY: AddressSanitizer: heap-use-after-free /app/example.c:64 in bug_use_after_free
Shadow bytes around the buggy address:
  0x601fffffff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x602000000000: fa fa[fd]fd fa fa fa fa fa fa fa fa fa fa fa fa
  0x602000000080: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa
  ...
```

**三段栈 = UAF 的完整故事**：`freed by ... example.c:63`（谁释放的）→ `previously allocated by ... example.c:59`（谁最初申请的）→ 报错点 `example.c:64`（谁在释放后又用了）。**第 64 行写了第 63 行刚 free 的东西**。

影子图 `fa fa[fd]fd fa fa ...` 里的 `fd` 就是**「已释放的堆块」编码**——free 之后这块 16 字节（两个影子字节）被标成 `fd`，隔离区里等着被复用。第 64 行的写落进 `[fd]`，报错。

### case 4：double free（实测原文）

```text
=================================================================
==2==ERROR: AddressSanitizer: attempting double-free on 0x602000000010 in thread T0:
    #0 0x77269dd91d18  (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdad18) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x401502 in bug_double_free /app/example.c:76
    #2 0x4015e8 in main /app/example.c:100
    ...

0x602000000010 is located 0 bytes inside of 16-byte region [0x602000000010,0x602000000020)
freed by thread T0 here:
    #0 0x77269dd91d18  (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdad18) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x4014f6 in bug_double_free /app/example.c:75
    ...

previously allocated by thread T0 here:
    #0 0x77269dd9304f in malloc (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdc04f) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x4014df in bug_double_free /app/example.c:72
    ...

SUMMARY: AddressSanitizer: double-free (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdad18) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
==2==ABORTING
```

注意 `double-free` 这条**没有 shadow bytes 段**——因为缺的不是「哪块内存不可访问」，而是「这个 `free` 调用本身非法」。它的三段栈是 `:76`（第二次 free，报错点）/ `:75`（第一次 free）/ `:72`（最初 malloc）。**`==2==ABORTING`** 表示 ASan 直接终止进程（double free 会破坏 allocator 内部结构，没有安全继续的可能）。

### case 5：内存泄漏（LSan，实测原文）

```text
=================================================================
==2==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 40000 byte(s) in 100 object(s) allocated from:
    #0 0x76239fc7504f in malloc (/opt/compiler-explorer/gcc-13.3.0/lib64/libasan.so.8+0xdc04f) (BuildId: 95d2dfcd183a2b9107b34fd25c40252c37aaa56d)
    #1 0x40152c in bug_leak /app/example.c:84
    #2 0x4015ef in main /app/example.c:101
    #3 0x76239f82a1c9  (/lib/x86_64-linux-gnu/libc.so.6+0x2a1c9) (BuildId: 328820b908de8ea1ef79afa8995e302e819163d7)
    #4 0x76239f82a28a in __libc_start_main (/lib/x86_64-linux-gnu/libc.so.6+0x2a28a) (BuildId: 328820b908de8ea1ef79afa8995e302e819163d7)
    #5 0x401164 in _start (/app/output.s+0x401164) (BuildId: baf7d14ac9ea8e1c22e3d3263d14e7b1e80e43f2)

SUMMARY: AddressSanitizer: 40000 byte(s) leaked in 100 allocation(s).
```

100 次 `malloc(400)` = 40000 字节，一次没 `free`。**注意 `#1 ... example.c:84`** —— 泄漏点被精确指到第 84 行那一句 `malloc(400)`，这就是 LSan 的价值：不用你猜「哪块内存漏了」。

## 优化会改变 bug 的「存在性」（实测，重要）

ASan 官方推荐 `-O1`，本节却坚持 `-O0`。原因不是「-O1 报告啰嗦」，而是一个更硬的事实：**优化会把某些内存错误直接从程序里删掉**。实测 `-O1 -fsanitize=address` 跑同一份 `c3_1_mem_bugs.c`：

| case | `-O0 -fsanitize=address` | `-O1 -fsanitize=address` | 为什么 |
|------|--------------------------|--------------------------|--------|
| 1 堆越界 | ✅ 检出（exit 1） | ✅ 检出 | `free(buf)` 让 `malloc` 无法被消除 |
| 2 栈越界 | ✅ 检出 | ✅ 检出（但**栈帧归到 `main`、写方变成 `memset`**） | `bug_stack_overflow` 被内联进 `main`，且 `memset(src)+memcpy(dst,src)` 被合并成一次 `memset(dst)` |
| 3 UAF | ✅ 检出（exit 1） | ❌ **检不出（exit 0、零报告）** | `p[0] = 7` 与紧随的 `g_sink = p[0]` 一起成了死代码，被整段删除 |
| 4 double free | ✅ 检出 | ✅ 检出 | `free()` 调用有副作用，不能被消除 |
| 5 泄漏 | ✅ 检出 | ✅ 检出 | `malloc` 结果必须产生，否则后面的 `free` 无对象 |

**case 3 那一行是整个模块最该记住的反直觉结论之一**：

> **`-O1` 下 UAF「不存在」了——不是 ASan 变瞎了，而是这段代码真的被编译器删掉了。** 于是同一份源码在 `-O0` 下是「有 UAF 的程序」，在 `-O1` 下是「没有 UAF 的程序」。你在 `-O2` release 构建里看到「内存错误消失了」，很可能只是**测试用例被优化没了**。

case 2 那一行则展示了另一半：优化不仅改变「有没有 bug」，还改变**报告的措辞**——栈帧名从 `bug_stack_overflow` 变成 `main`，写方从 `__interceptor_memcpy` 变成 `memset`。**读 ASan 报告时要意识到「报告描述的是优化后的代码」**，别拿源码逐字对。

```bash
# 所以验证内存 bug 的标准姿势是两段式：
# 1) -O0 把病因看清（五个 case 全部现形）
gcc -g -O0 -fsanitize=address -o prog_asan_O0 prog.c && ./prog_asan_O0
# 2) 再用接近生产的 -O2 复跑一遍（看哪些 bug 在 release 下"隐身"了）
gcc -g -O2 -fsanitize=address -o prog_asan_O2 prog.c && ./prog_asan_O2
```

> ⚠️ 还有一条实测的对照：**不带 sanitizer 时，`-O0` 和 `-O1`/`-O2` 都不会告诉你任何事**。case 1 在 `gcc -g -O0 -Wall -Wextra`、`-O1`、`-O2` 三种档位下**都是 exit 0**，只留下编译期警告（`-O0` 有 27 条、`-O1` 20 条、`-O2` 28 条）。**警告数量和检出能力不成正比**——别以为警告多就查得全。

## 原理：红区 + 影子内存

ASan 的核心是**编译期插桩**：编译器在**每一次**内存访问（load/store）指令前后，插入一段「检查目标地址是否中毒」的代码。而「中毒」由 malloc 时布下的**红区**标记。

```
+---------+  redzone (0xfa，中毒)  ← 写到这里 → 报错
|  应用块  |  8 字节，正常（0x00）
+---------+  redzone (0xfa，中毒)  ← 越界到右边也是
```

1. **红区（redzone）**：malloc 返回给你的 8 字节前后，ASan 各铺一段「毒化」内存。程序任何越界访问（哪怕只越界 1 字节）都会踩进红区。
2. **影子内存（shadow memory）**：把每 8 字节应用内存映射到 1 字节 shadow，用编码记录「这 8 字节里哪些可访问」。检查时只需一条位运算 + 一次内存读，所以快。
3. **隔离区（quarantine）**：free 后的块不立刻还给 OS，而是放进 quarantine 延迟复用。这样 use-after-free 在「复用之前」就会被抓到——因为那段内存还被标着「已 free、中毒」。

```text
shadow 字节编码（录自上面 case 1/2/3 报告的 "Shadow byte legend" 原文）：
0x00        = Addressable（8 字节全可访问）
0x01..0x07  = Partially addressable（前 N 字节可访问）
0xfa        = Heap left redzone（堆红区）
0xfd        = Freed heap region（已 free 的堆块 = 隔离区）
0xf1        = Stack left redzone（栈左红区）
0xf2        = Stack mid redzone（栈中红区 ← 两对象之间的守卫）
0xf3        = Stack right redzone（栈右红区）
0xf5        = Stack after return
0xf8        = Stack use after scope
0xf9        = Global redzone
0xbb        = Intra object redzone
（完整表见报告末尾的 legend，共 15 项）
```

**为什么 ASan 能抓栈越界而 valgrind 不行**：编译器给**栈上每个对象**都插了红区，而且是**分段**的——`f1`（左）、`f2`（对象之间）、`f3`（右）。所以 `char dst[8]` 被 `memcpy` 灌 32 字节时，ASan 不但报错，还能画出「越界正好冲进 `dst` 与 `src` 之间的 `f2` 中红区」（见上面 case 2 的影子图）。这正是它对 valgrind 的关键补强。

## 报错类型一览

ASan 报错格式统一为 `ERROR: AddressSanitizer: <类型> on address ...`：

| 报错类型 | 对应问题 | 触发 |
|----------|----------|------|
| `heap-buffer-overflow` | 堆越界 | 写/读 malloc 块外 |
| `stack-buffer-overflow` | 栈越界 | 写/读栈数组外（valgrind 盲区） |
| `global-buffer-overflow` | 全局越界 | 写/读全局数组外 |
| `heap-use-after-free` | 堆 UAF | free 后再读写 |
| `stack-use-after-return` | 栈 UAF（返回后） | 用了已返回函数的栈变量地址 |
| `use-after-scope` | 作用域外使用 | 用了已出作用域的变量 |
| `double-free` | 重复释放 | 同一指针 free 两次 |
| `allocation-size-too-big` | 分配过大 | malloc 超限（往往是算错的负数转 size_t） |
| `detected memory leaks` | 泄漏 | 退出时 LSan 报告（见下） |

**表里前 9 项里有 5 项已在上面被实测打中**（`heap-buffer-overflow` / `stack-buffer-overflow` / `heap-use-after-free` / `double-free` / `detected memory leaks`）——对应 `c3_1_mem_bugs.c` 的 case 1~5。

> ⚠️ **「未初始化读」不在上表里，因为 ASan 抓不到它**：ASan 只检查「地址是否可访问」（addressability），不检查「值是否被初始化」（validity）。`c3_2_uninit_read.c` 在 `-fsanitize=address` 下实测**退出码 0、零报告**。未初始化读是 **MemorySanitizer（MSan，`-fsanitize=memory`）** 的职责，valgrind memcheck 也能抓（V bit 机制）。三者分工：ASan 管地址、MSan 管值、TSan 管并发。

## LeakSanitizer（LSan）集成

ASan **内置** LeakSanitizer，程序退出时自动查泄漏——不用额外开关（`-fsanitize=address` 就带上了）。实测输出见上面 **case 5**，结构是「`detected memory leaks` → `Direct leak of N byte(s) in M object(s) allocated from:` → 分配栈 → `SUMMARY`」。

只报 **direct leak**（和 valgrind 的 `definitely lost` 对应），且默认不逐个列 100 个对象，只汇总 + 给一个代表栈。要更细可设环境变量：

```bash
# 关掉泄漏检查，只查越界/UAF（避免泄漏报告淹没主要问题）
ASAN_OPTIONS=detect_leaks=0 ./c3_1_asan 1
# 保留隔离区细节（每条泄漏都给完整分配栈）
ASAN_OPTIONS=detect_leaks=1:fast_unwind_on_malloc=0 ./c3_1_asan 5
```

## ASAN_OPTIONS 常用项

ASan 运行时行为通过环境变量 `ASAN_OPTIONS` 调（用冒号分隔）：

| 选项 | 作用 |
|------|------|
| `abort_on_error=1` | 首个错误就 abort，配合 gdb 抓现场（默认报完继续跑，可能被后续错误淹没） |
| `halt_on_error=1` | 同 abort_on_error，报错即停 |
| `detect_leaks=0` | 关闭泄漏检查（只想查越界/UAF 时） |
| `symbolize=0` | 关闭符号化（栈只给地址，不转函数名，CI 里日志更小） |
| `log_path=/tmp/asan` | 报告写文件 `/tmp/asan.<pid>` |
| `quarantine_size_mb=256` | 调大隔离区，提高 UAF 检出率（默认 256MB） |
| `handle_segv=0` | 关掉 ASan 对 SIGSEGV 的接管（让程序自然崩，不用 ASan 报） |

```bash
# 调试场景：报错即停，交给 gdb 看现场
ASAN_OPTIONS=abort_on_error=1 gdb ./c3_1_asan
# 运行到第一个雷就 abort，gdb 停在崩溃点，可 bt 看完整栈
```

## valgrind vs ASan 取舍

| 维度 | valgrind memcheck | ASan |
|------|-------------------|------|
| 检测原理 | 动态二进制翻译（DBI） | 编译期插桩 + 运行时红区 |
| 是否需重编译 | ❌ 不需要 | ✅ 必须 `-fsanitize=address` |
| 性能开销 | 20–50× | 约 2× |
| 栈越界 | 弱（基本抓不到） | ✅ 强（栈红区） |
| 未初始化读 | ✅ 强（V bit） | ❌ 默认不查（交给 MSan） |
| 数据竞争 | 另有 helgrind/drd | ❌（交给 TSan） |
| 适用场景 | 现有二进制快速定性 | 开发期每次回归、CI 常驻 |
| 对第三方库 | 直接可查 | 需库也用 ASan 编译才查得深 |

> **经验法则**：开发期默认用 ASan（快，可进 CI）；拿到一个**没有源码/不方便重编译**的二进制时，用 valgrind 兜底定性。两者不是二选一，是「快检 ASan + 兜底 valgrind」互补。

## HFT 关联

1. **ASan 进 CI 是「内存安全的最后一道门」**：撮合引擎每次提交后，用 `-fsanitize=address` 编译跑一轮单元测试 + 仿真行情，约 2× 开销完全可接受，能在合入前拦截越界和 UAF。这是 HFT 团队性价比最高的内存防线。
2. **栈越界是 C 字符串操作的常见雷**：`char sym[8]; sprintf(sym, "%s", ticker)` 这种在行情解析里很常见，valgrind 抓不到，ASan 的栈红区能精确钉死——**这是选 ASan 而非 valgrind 的关键理由之一**。
3. **UAF 与「订单生命周期」强相关**：订单对象 free 后，别的线程/回调还持有旧指针去写，是高频场景里的偶发错单根因。ASan 的 quarantine 让 UAF 在复用前就现形。
4. **release 构建绝不能带 ASan**：ASan 有内存开销（约 2×）和崩溃时打印大段报告的行为，线上二进制**必须去掉** `-fsanitize=address` 重新编译。可以维护「debug-asan」和「release」两套构建目标。

```bash
# HFT 场景：CI 里对撮合引擎跑 ASan 回归
gcc -g -O1 -fsanitize=address -o engine_asan matching_engine.c ...
ASAN_OPTIONS=abort_on_error=1:detect_leaks=1 ./engine_asan --sim data.csv
# 任一内存错误 → 非零退出（ASan 检出即 exit 1）→ CI 判失败

# 更完整的门禁：加一档 -O0，保证「写完就死」的 bug 不被优化掉
gcc -g -O0 -fsanitize=address -o engine_asan_O0 matching_engine.c ...
ASAN_OPTIONS=abort_on_error=1:detect_leaks=1 ./engine_asan_O0 --sim data.csv
```

> ⚠️ **别只挂 `-O1`/`-O2` 一档**：上面实测过，`-O1` 会把「写完就死」的 UAF 整段优化掉，于是 CI 全绿、线上偶发。**CI 用 `-O0` 保证检出率，生产档位再单独复跑一档作对照**——两档都跑才是完整门禁。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** ASan 为什么比 valgrind 快一个量级？「快」的根本原因是什么？

> 因为检查时机不同。ASan 是**编译期插桩**：编译器把「检查地址是否中毒」的代码直接编进程序，运行时执行的是原生机器码，检查只需「算 shadow 地址 + 读 1 字节 + 判断」几条指令。valgrind 是**运行时二进制翻译**：每条指令都要翻译成带检查的等价代码在虚拟 CPU 上跑，相当于多了一层解释，所以慢 20–50×。ASan 用「多花一点编译时间 + 重编译的麻烦」换来运行时近原生速度。

**Q2:** 红区（redzone）和隔离区（quarantine）分别抓什么？

> **红区**抓「越界」：malloc 块前后铺毒化内存，任何越界访问（哪怕 1 字节）都踩进红区报错。**隔离区**抓「use-after-free」：free 的块不立即还给 OS，而是留在 quarantine 里保持「已 free、中毒」状态一段时间，这样 free 后再访问就会命中中毒块被抓住，而不是「块被复用了、越界访问写坏了别人」。两者配合覆盖了内存错误的两大类。

**Q3:** ASan 能抓「未初始化读」吗？不能的话交给谁？

> 默认**不能**。ASan 只检查「地址是否可访问」（addressability），不检查「值是否已初始化」（validity）。`int x; if (x>0)` 这种读垃圾值，ASan 无感。未初始化读是 **MemorySanitizer（MSan，`-fsanitize=memory`）** 的职责，MSan 维护 V bit 追踪每个 bit 的初始化状态。valgrind memcheck 也能抓（V bit 机制）。三者分工：ASan 管地址、MSan 管值、TSan 管并发。

**Q4:** 为什么 release 构建绝不能带 ASan？

> 三个原因：①性能——ASan 有约 2× 时间和内存开销，线上会拖慢撮合延迟、加大内存占用；②行为——报错时会打印大段报告并可能 abort，不适合生产；③安全——ASan 的 shadow 内存和插桩代码会改变程序内存布局，且 `ASAN_OPTIONS` 环境变量可被利用。所以 ASan 只用于 debug/测试构建，release 必须去掉 `-fsanitize=address` 单独编译。

**Q5:** 一个「偶发、只在压力测试下出现」的段错误，你会先上 valgrind 还是 ASan？为什么？

> 先上 **ASan**。因为偶发段错误大概率是越界或 UAF（压力下分配/释放更频繁、边界条件更容易触发），ASan 快（约 2×），能在压力场景接近真实负载下复现；valgrind 慢 20–50×，压力测试根本跑不动。如果 ASan 复现不了（比如问题出在没有源码的第三方库里），再退回 valgrind 对现成二进制定性。若怀疑是并发竞态（偶发 + 多线程），则直接上 TSan（Ch4）。

**Q6:** 实测里 `-O1 -fsanitize=address` 跑 case 3（UAF）得到 exit 0、零报告。是 ASan 失灵了吗？这对 CI 配置意味着什么？

> **不是 ASan 失灵，是那段代码真的不存在了**。`-O1` 下 `p[0] = 7` 与紧随其后的 `g_sink = p[0]` 构成一对「写了没人用」的死代码，被编译器整段删除——程序里已经没有 UAF 可检。对照实测：同一次 `-O1` 下 case 1/2/4/5 都能检出（`free` 和 `malloc` 有副作用，删不掉），只有 case 3 消失。对 CI 的意义：**只挂 `-O1`/`-O2` 一档的 ASan 门禁会漏检这类 bug**，必须加一档 `-O0` 保证检出率。更一般的教训是「优化会改变 bug 的存在性，不只是改变报告的措辞」——`-O2` release 里「内存错误消失了」，往往只是测试用例被优化没了。

</details>

## 交叉引用

- [3.1 valgrind memcheck](01-valgrind-memcheck.md)
- [3.3 UndefinedBehaviorSanitizer](03-undefinedbehaviorsanitizer.md)
- [1.2 症状 → 工具决策树](../../chapter-01-methodology/notes/02-symptom-to-tool.md)
- [Ch3 内存类](../README.md)
