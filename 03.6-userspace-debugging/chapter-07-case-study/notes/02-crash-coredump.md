# 7.2 崩溃 → coredump 回溯定位

> 🔴 精读 · 段错误来了，先开 core，再 gdb 回溯

## 本节要点

用 7.1 的 `trader.c`，开 `BUG_CRASH` 雷，走一遍「崩溃 → 开 core → gdb 回溯 → 定位越界写 → 修复」的完整流程。核心工具是 Ch2 的 coredump + gdb。本节重点体会：**越界写往往「不立即崩」，而是潜伏到栈帧被破坏后才在别处爆**——这正是 coredump 回溯「崩溃现场」而不是「犯罪现场」的局限，要学会从崩溃点反推根因。

## 第一步：复现崩溃

```bash
gcc -g -O0 -pthread -o trader_crash -DBUG_CRASH trader.c
./trader_crash
# 可能直接 Segmentation fault (core dumped)
# 也可能跑了一会儿才崩，甚至偶尔「侥幸」跑完——越界写的典型特征
```

> 关键观察：崩溃「偶发、位置不定」。因为 `slots[o->id]` 越界写坏的是**栈上的返回地址/帧指针**，破坏程度取决于被写的值，可能当场崩、可能函数返回时崩、也可能没崩到致命处。

## 第二步：开 core dump

崩溃要有 core 文件才能事后回溯（Ch2 四道闸排查过）：

```bash
ulimit -c unlimited          # shell 级允许生成 core
./trader_crash
# Segmentation fault (core dumped)   ← 生成 core 文件
ls -l core*                   # 确认 core 文件存在
```

若没生成 core，逐项排查（详见 2.4）：`ulimit -c` 是否为 0、`cat /proc/sys/kernel/core_pattern` 是否被 systemd-coredump 接管、`fs.suid_dumpable` 是否限制。

## 第三步：gdb 加载 core 回溯

```bash
gdb ./trader_crash core
(gdb) bt
```

```text
#0  0x00007f8a3c4d5e6f in __GI___libc_write (...) at write.c:26
#1  0x00007f8a3c4a1234 in _IO_new_file_write (...) at fileops.c:1181
#2  ...
#5  0x0000559a1b2c3d40 in feed_thread (arg=0x0) at trader.c:XX
#6  0x00007f8a3c456789 in start_thread (...) at pthread_create.c:...
#7  0x00007f8a3c4dabcd in clone (...) at ...
```

**崩溃点在 `feed_thread` 里**——但注意栈顶是 `__libc_write`，说明崩溃发生时线程正在执行系统调用（`usleep` 内部的 write）。这不是巧合：

```bash
(gdb) info locals
# o = 0x... (order_t *)
# i = 200          ← 第 200 次循环
(gdb) info registers
# rip = 0x0000559a...  rsp = 0x7ffc...  rbp = 0x0000000100000001  ← rbp 被写坏！
```

**`rbp = 0x0000000100000001`**——这是个假值（不像合法栈地址）。真相大白：`slots[o->id] = 1` 越界写，把栈上的**帧指针 rbp 写成了 1**，函数返回时用损坏的 rbp 恢复栈帧，`ret` 跳到了错误位置，最终在执行某个系统调用时崩溃。

## 第四步：从「崩溃现场」反推「犯罪现场」

coredump 给的是**崩溃现场**（`feed_thread` 里 rbp 坏了），但**犯罪现场**（越界写）在哪？两步锁定：

```bash
(gdb) frame 5
(gdb) list
# 在 feed_thread 源码里找「写栈数组」的地方：
#   int slots[16];
#   slots[o->id] = 1;    ← 就是它！o->id 最大 200，写 slots[200] 越界 184 个 int

(gdb) print o->id
# $1 = 200
```

定位结论：**`o->id` 没有边界检查，直接当 `slots[16]` 的下标，id=200 时越界写，写坏了栈帧**。这与 2.6 讲的「越界写坏相邻数据」是同一类根因——只是这次坏的是栈帧而不是堆上的相邻对象。

## 第五步：修复

```c
#ifdef BUG_CRASH
        {   // 修复：加边界检查
            int slots[16];
            if (o->id >= 0 && o->id < 16)
                slots[o->id] = 1;
        }
#endif
```

重新编译（不带 BUG_CRASH）跑，`total matched qty = 40100`，恢复正常。

## 为什么「越界写不立即崩」——这是本节的灵魂

```
feed_thread 栈帧布局（简化）：
高地址  ┌──────────────┐
        │  返回地址 ret │   ← slots[200] 越界写可能落到这附近
        │  帧指针 rbp   │   ← 被写成 0x100000001（=1）
        │  o, i 等局部   │
        │  slots[15..0] │
低地址  └──────────────┘
        slots[200] 写到哪？越过 slots[15] 一路往上，可能：
        ① 写到局部变量 → 值错乱，不崩
        ② 写到 rbp → 函数返回时崩
        ③ 写到 ret → ret 跳错地址，崩在别处
        ④ 恰好写到无害区 → 侥幸不崩
```

所以越界写是「**犯罪和命案分离**」：写坏内存（犯罪）和崩溃（命案）之间隔着「什么时候用到被写坏的东西」。这解释了为什么 coredump 回溯只能看到「命案现场」，要结合源码推理才能找到「犯罪现场」。

## HFT 关联

1. **线上 core 是「第一现场」**：交易进程崩溃后，第一个动作永远是拿 core + `bt` 看崩溃点，而不是盲目重启（重启会掩盖问题，且丢掉现场）。
2. **保留带符号的构建产物**：`bt` 能定位到源码行，前提是二进制带 `-g`。HFT 团队必须归档每个 release 的调试符号（或未 strip 产物），否则线上 core 拿回来只能看反汇编（见 2.1）。
3. **越界写是「偶发崩溃」的经典根因**：偶发、位置漂移、压力下才出现——这些特征都指向越界/UAF/竞态（1.1 分类学）。崩溃点每次不同，是因为「被写坏的东西」每次不同。
4. **边界检查是下单路径的基本纪律**：订单 id、价格档位索引、数组下标，凡是从外部（行情/网络）来的值，都必须先校验再当索引用。这个 bug 的本质就是「信任了未校验的 id」。

```bash
# HFT 场景：崩溃后第一时间回溯
gdb ./matching_engine core
(gdb) bt full          # 完整栈 + 局部变量
(gdb) info registers   # 看 rbp/rsp 是否被写坏（判断是否栈破坏）
(gdb) frame N          # 逐帧查看
(gdb) info locals      # 看崩溃点局部变量
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么越界写「不立即崩溃」，而是「偶发、位置不定」？

> 因为越界写只是「写坏了一块内存」（犯罪），是否崩溃取决于「之后有没有用到被写坏的东西」（命案）。写坏的可能是局部变量（值错但不崩）、帧指针 rbp（函数返回时崩）、返回地址 ret（跳错地址崩在别处），也可能恰好写到无害区（侥幸不崩）。所以崩溃与否、崩溃在哪，取决于「被写坏的东西是什么、何时被用到」——这正是越界写偶发难复现的根源。

**Q2:** coredump 给的是「崩溃现场」还是「犯罪现场」？如何从前者反推后者？

> coredump 给的是**崩溃现场**（程序崩在哪一行、当时寄存器/栈什么样）。**犯罪现场**（越界写发生在哪）需要结合源码推理：从崩溃点的异常（如 `rbp` 是假值、局部变量被污染）判断「栈被写坏了」，再去源码里找「哪个写操作可能越界」，最后用 `print o->id` 等确认越界下标。coredump 是线索，不是答案——它告诉你「死在哪」，你要推理「谁杀的他」。

**Q3:** `info registers` 里 `rbp` 是假值（如 `0x100000001`）说明什么？

> 说明栈帧被写坏了。`rbp`（帧指针）正常情况下是当前栈帧的基址，应指向栈区（接近 `rsp` 的合法地址）。`0x100000001` 这种不像地址的值，说明有越界写把它覆盖了——本例正是 `slots[o->id] = 1` 写坏了帧指针。所以看到 `rbp`/`rsp`/返回地址异常，第一反应就是「栈溢出/越界写」。

**Q4:** 崩溃点栈顶是 `__libc_write`（系统调用），但真正的 bug 在 `feed_thread`，这说明什么？

> 说明崩溃点是「被波及」的，不是根因。栈帧被写坏后，程序还能「带伤运行」一段时间，直到某次函数返回/系统调用用到损坏的 rbp/ret 才崩。所以栈顶的系统调用只是「压死骆驼的最后一根稻草」，真正的病根在上面的 `feed_thread` 越界写。教训：`bt` 要看全栈、看局部变量、看寄存器，不能只盯着栈顶那一帧下结论。

**Q5:** 这个 bug 的正确修法是什么？体现什么工程原则？

> 修法是加边界检查：`if (o->id >= 0 && o->id < 16)` 再下标访问。体现的工程原则：**外部输入（订单 id）必须校验后才能当数组下标/索引**。订单 id 来自「外部」，范围不可信，信任它就会越界。这在 HFT 里是下单路径的基本纪律——任何从行情/网络来的值，先校验再使用。

</details>

## 交叉引用

- [7.1 程序结构](01-program-structure.md)
- [7.3 竞态 → TSan 定位](03-race-tsan.md)
- [2.4 core 文件生成配置](../../chapter-02-crash/notes/04-core-dump-config.md)
- [2.5 加载 core 回溯](../../chapter-02-crash/notes/05-load-core-backtrace.md)
- [Ch7 实战](../README.md)
