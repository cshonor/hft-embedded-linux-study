# 3.2 AddressSanitizer（ASan 快速内存错误检测）

> 🔴 精读 · 编译期插桩的「快检」——比 valgrind 快一个量级，但要重编译

## 本节要点

AddressSanitizer（ASan）是 Google 出品的编译期内存错误检测器，集成在 GCC / Clang 里，加 `-fsanitize=address` 即可。它和 valgrind 抓的是**同一类**内存错误（越界 / UAF / 泄漏 / 未初始化），但实现思路相反：**编译时把检查代码插进程序**，运行时靠「红区（redzone）+ 影子内存」高速判断。代价是必须重编译，换来约 **2× 开销**（valgrind 是 20–50×），快到可以在 CI 里每次提交都跑。本节讲原理、报错解读、与 valgrind 的取舍。

## 编译与运行

```bash
gcc -g -O1 -fsanitize=address -o mem_bugs_asan mem_bugs.c
./mem_bugs_asan
```

> 注意：`-g` 必须有（报错要显示源码行号），优化建议 `-O1` 而非 `-O0`（ASan 官方推荐 `-O1`，`-O0` 下插桩代码和检查逻辑混在一起反而更慢、报告更啰嗦）。

跑 3.1 那个 `mem_bugs.c`，第一个雷（堆越界写）的报错：

```text
=================================================================
==5678==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x602000000018 at pc 0x55c1a2b4c3e4 bp 0x7fff1234 sp 0x7fff122c
WRITE of size 8 at 0x602000000018 thread T0
    #0 0x55c1a2b4c3e3 in bug_heap_overflow mem_bugs.c:11
    #1 0x55c1a2b4c4f2 in main mem_bugs.c:42
    #2 0x7f2a3b4c5d6e in __libc_start_main

0x602000000018 is located 8 bytes to the right of 8-byte region [0x602000000010,0x602000000018)
allocated by thread T0 here:
    #0 0x55c1a2b4a1a0 in malloc
    #1 0x55c1a2b4c3c0 in bug_heap_overflow mem_bugs.c:10

SUMMARY: AddressSanitizer: heap-buffer-overflow mem_bugs.c:11 in bug_heap_overflow
```

和 valgrind 报告同构（错误类型 → 读写方栈 → 越界程度 → 分配方栈），但多了最后一段 **shadow bytes 图**（篇幅略），它把出问题地址附近的影子内存状态画出来，精确定位「越界了几个字节」。

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
shadow 字节编码（部分）：
0x00 = 8 字节全可访问
0x01..0x07 = 前 N 字节可访问（部分）
0xfa = 堆 redzone（中毒）
0xfd = 已 free 的堆块（中毒，隔离区）
0xf1 = 栈 redzone
```

**为什么 ASan 能抓栈越界而 valgrind 不行**：编译器给**栈上每个数组**也插入了 redzone（`0xf1`），所以 `char buf[8]; strcpy(buf, "长字符串")` 这种栈溢出 ASan 精确抓到，这正是它对 valgrind 的关键补强。

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

跑 `mem_bugs.c` 会依次报 `heap-buffer-overflow`（雷1）、`heap-use-after-free`（雷2）、`detected memory leaks`（雷3）。**注意雷 4 未初始化读 ASan 默认不报**——那是 MemorySanitizer（MSan）的活，ASan 只管「地址合法性」不管「值合法性」。

## LeakSanitizer（LSan）集成

ASan 内置 LeakSanitizer，程序退出时自动查泄漏：

```text
=================================================================
==5678==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 400 byte(s) in 100 object(s) allocated from:
    #0 0x... in malloc
    #1 0x... in bug_leak mem_bugs.c:26

SUMMARY: AddressSanitizer: 400 byte(s) leaked in 100 allocation(s).
```

只报 **direct leak**（和 valgrind 的 definitely lost 对应），且默认不逐个列 100 个对象，只汇总 + 给一个代表栈。要更细可设环境变量。

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
ASAN_OPTIONS=abort_on_error=1 gdb ./mem_bugs_asan
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
# 任一内存错误 → 非零退出 → CI 判失败
```

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

</details>

## 交叉引用

- [3.1 valgrind memcheck](01-valgrind-memcheck.md)
- [3.3 UndefinedBehaviorSanitizer](03-undefinedbehaviorsanitizer.md)
- [1.2 症状 → 工具决策树](../../chapter-01-methodology/notes/02-symptom-to-tool.md)
- [Ch3 内存类](../README.md)
