# 7.4 泄漏 → valgrind / ASan 定位

> 🔴 精读 · 内存「越用越多」，valgrind 钉死泄漏点

## 本节要点

用 7.1 的 `trader.c` 开 `BUG_LEAK` 雷：match 线程撮合成交后**摘除订单但不 free**，200 个订单对象全部泄漏。程序能正常跑完、结果也对，但内存悄悄涨——这正是「慢泄漏」的可怕之处。本节走一遍「valgrind 定性 → 报告定位 → 修复」的流程，核心工具是 Ch3 的 valgrind memcheck（以及 ASan 的 LeakSanitizer 作为快检）。

## 第一步：观察「能跑完但内存涨」

```bash
gcc -g -O0 -pthread -o trader_leak -DBUG_LEAK trader.c
./trader_leak
# total matched qty = 40100    ← 结果完全正确！程序正常退出
```

结果对了，但 200 个 `order_t`（每个约 40 字节）全泄漏了。单次跑 200 个对象、几 KB，根本看不出来——但如果这是 7×24 长跑的撮合引擎，每次撮合泄漏一个订单，几周后就是 OOM 崩溃。

> 泄漏的阴险在于：**它不影响「结果正确性」，只影响「资源可持续性」**。短命程序无所谓（进程退出 OS 全回收），长跑进程是致命的。

## 第二步：valgrind 定性 + 定位

```bash
valgrind --leak-check=full --show-leak-kinds=all ./trader_leak
```

```text
==12345== HEAP SUMMARY:
==12345==     in use at exit: 8,000 bytes in 200 blocks
==12345==   total heap usage: 200 allocs, 0 frees, 8,000 bytes allocated
==12345==
==12345== LEAK SUMMARY:
==12345==    definitely lost: 8,000 bytes in 200 blocks
==12345==    indirectly lost: 0 bytes in 0 blocks
==12345==      possibly lost: 0 bytes in 0 blocks
==12345==    still reachable: 0 bytes in 0 blocks
==12345==
==12345== 8,000 bytes in 200 blocks are definitely lost in loss record 1 of 1
==12345==    at 0x483B7F3: malloc (vg_replace_malloc.c:393)
==12345==    by 0x401234: feed_thread trader.c:20     ← 订单在哪分配
==12345==    by 0x...: start_thread
==12345==
==12345== LEAK SUMMARY:
==12345==    definitely lost: 8,000 bytes in 200 blocks
```

解读（3.1 讲过的四步）：

1. **HEAP SUMMARY**：`200 allocs, 0 frees` —— **分配了 200 次，一次都没 free**，铁证如山。
2. **definitely lost: 200 blocks** —— 200 个块没有任何指针指向，确定泄漏。
3. **分配栈**：`malloc ... by feed_thread trader.c:20` —— 订单是在 feed 线程第 20 行分配的。

> 注意：泄漏报告给的是「**在哪分配**」（feed 的 malloc），而不是「**该在哪 free 却漏了**」（match 的摘除逻辑）。因为 valgrind 只知道「谁分配的、有没有被 free」，不知道「你本来打算在哪 free」。所以定位到分配点后，要结合代码逻辑找到「谁应该 free 但没 free」。

## 第三步：从「分配点」推「应释放点」

分配在 feed（`malloc` 订单），但消费在 match（摘除订单）——**生产者分配、消费者释放**的模型。查看 match 的摘除逻辑：

```c
void *match_thread(void *arg) {
    while (g_running || g_book) {
        pthread_mutex_lock(&g_book_lock);
        order_t *o = g_book;
        if (o) {
            g_book = o->next;
#ifdef BUG_LEAK
            /* 雷3（泄漏）：成交订单摘除后不 free */   ← 就是这里漏了
#else
            free(o);                                  ← 正确版本在这 free
#endif
        }
        pthread_mutex_unlock(&g_book_lock);
        usleep(500);
    }
    return NULL;
}
```

定位结论：**match 线程摘除订单后忘了 `free(o)`**，生产者分配、消费者不释放，泄漏。

## 第四步：修复

```c
if (o) {
    g_book = o->next;
    free(o);        // 摘除即释放
}
```

修复后 valgrind 再跑：

```text
==12345== All heap blocks were freed -- no leaks are possible
```

## 用 ASan 的 LeakSanitizer 快检（备选）

开发期要快，用 ASan（3.2 讲过，内置 LeakSanitizer）：

```bash
gcc -g -O1 -pthread -fsanitize=address -o trader_leak_asan -DBUG_LEAK trader.c
./trader_leak_asan
```

```text
=================================================================
==12345==ERROR: LeakSanitizer: detected memory leaks

Direct leak of 8000 byte(s) in 200 object(s) allocated from:
    #0 0x... in malloc
    #1 0x... in feed_thread trader.c:20

SUMMARY: AddressSanitizer: 8000 byte(s) leaked in 200 allocation(s).
```

同样定位到 `feed_thread trader.c:20`。ASan 快（约 2×）、valgrind 慢（20–50×）但无需重编译，取舍见 3.2。

## 为什么「慢泄漏」是长跑进程的头号杀手

```
短命进程（跑完就退）：
  泄漏 → 进程退出 → OS 回收全部内存 → 无影响

长跑进程（7×24 撮合引擎）：
  每次撮合泄漏一个订单（几十字节）
  → 每秒撮合 N 单，每小时泄漏 N×3600×sizeof(order)
  → 几天后内存涨到上限
  → OOM killer 杀掉进程 → 交易中断
  → 重启后又从头泄漏 → 周期性崩溃
```

**关键**：短命进程里「结果正确」的泄漏测试会漏掉 bug，因为进程退出就没事了。长跑进程必须专门做**泄漏检测**（valgrind/ASan + 长时间压测），否则上线后才暴露。

## HFT 关联

1. **7×24 进程最怕慢泄漏**：撮合引擎、行情网关「永不重启」，慢泄漏累积几周后 OOM。开发期用 valgrind 钉死分配点，比上线后盯 RSS 曲线猜强百倍。
2. **「生产者分配、消费者释放」是泄漏高发区**：feed 分配订单、match 消费订单，责任链一断（消费者忘了 free）就泄漏。这种跨线程的所有权转移，要在设计上明确「谁分配谁负责释放」。
3. **CI 挂 nightly 泄漏门禁**：valgrind 慢，挂 nightly 构建（不是每次提交），配合 `--error-exitcode=1 --errors-for-leak-kinds=definite`，专门拦截 definite 泄漏。ASan 快，可挂每次提交。
4. **单测要覆盖「释放路径」**：这个 bug 单次跑「结果正确」，容易被单测放过。所以要专门写「内存泄漏」维度的测试（valgrind/ASan 跑一遍），不能只看功能结果。

```bash
# HFT 场景：撮合引擎压测 + 泄漏检测
valgrind --leak-check=full --errors-for-leak-kinds=definite \
         --error-exitcode=1 ./matching_engine --sim big.csv
# 退出码非 0 → 有确定泄漏 → CI 判失败
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 为什么泄漏「不影响结果正确性」，却更危险？

> 因为泄漏不改变程序的计算结果——该撮合的照样撮合、`g_total` 照样是 40100——它只消耗「内存资源」。短命进程无所谓（退出时 OS 全回收），但长跑进程（撮合引擎）会持续累积，几周后内存耗尽被 OOM killer 杀掉。所以泄漏的「危险」不在正确性，而在**可持续性**：结果对了不代表程序能长期健康运行。功能测试通过 ≠ 没有泄漏。

**Q2:** valgrind 的泄漏报告给「分配点」还是「应释放点」？为什么？如何找到后者？

> 给的是**分配点**（`malloc` 发生在 `feed_thread trader.c:20`）。因为 valgrind 只能观察到「谁分配的、有没有被 free」，无法知道你「本来打算在哪 free」。要找「应释放点」，得从分配点出发、结合代码逻辑推理：谁持有这个对象、谁该负责释放、哪条路径漏了。本例是「feed 分配、match 该释放但忘了」，所以定位分配点后要顺着数据流找到消费者。

**Q3:** `200 allocs, 0 frees` 这条 HEAP SUMMARY 信息为什么是「铁证」？

> 因为它直接量化了泄漏：程序总共 malloc 了 200 次，却一次 free 都没有——说明所有分配的对象都「有去无回」。这是最直观的泄漏证据，比「内存涨了多少」更精确（不受「有没有被复用」干扰）。正常程序应该 `allocs` 和 `frees` 基本相等（`in use at exit` 趋近 0）。

**Q4:** 「生产者分配、消费者释放」为什么是泄漏高发区？

> 因为分配和释放发生在**不同的线程/模块**，所有权在两者之间转移，责任边界容易模糊：feed 觉得「我分配了，但 match 会处理」，match 觉得「订单是 feed 的，我不该 free」。两边都以为对方负责，结果谁都没释放。正确做法是在设计上明确「所有权转移规则」——比如「订单一旦塞进订单簿，所有权归订单簿，谁摘除谁释放」，并用 valgrind 验证。

**Q5:** 单次跑「结果正确」，为什么单测容易放过这个泄漏 bug？

> 因为常规单测只断言「功能结果」（`g_total == 40100`），不检查「内存是否释放」。泄漏不改变功能结果，所以功能断言全绿，bug 被放过。要抓它，必须专门加「内存维度」的测试：用 valgrind/ASan 跑一遍，断言「无 definitely lost」。这提醒我们：测试维度要和 bug 维度对齐，功能测试抓不了资源泄漏。

</details>

## 交叉引用

- [7.3 竞态 → TSan 定位](03-race-tsan.md)
- [7.5 卡住 → strace 定位](05-hang-strace.md)
- [3.1 valgrind memcheck](../../chapter-03-memory/notes/01-valgrind-memcheck.md)
- [3.2 AddressSanitizer](../../chapter-03-memory/notes/02-addresssanitizer.md)
- [Ch7 实战](../README.md)
