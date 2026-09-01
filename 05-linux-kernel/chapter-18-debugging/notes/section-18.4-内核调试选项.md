## ③ 内核调试选项 · Kernel Hacking

`make menuconfig` → **Kernel Hacking**（依赖 **`CONFIG_DEBUG_KERNEL`**）

| 功能示例 | 作用 |
|----------|------|
| **sleep-inside-spinlock 检测** | 在 **原子上下文**（持 spinlock / 关抢占）**非法睡眠** → 抓 **死锁元凶** |
| `CONFIG_DEBUG_SPINLOCK` | 抓**未初始化/重复解锁**等锁 API 误用 |
| `CONFIG_DEBUG_ATOMIC_SLEEP` | 原子上下文 `might_sleep()` 立即告警（对应上一行） |
| `CONFIG_DEBUG_INFO` | vmlinux 带 DWARF 行号——Oops→源码行的前提（见 [18.3](./section-18.3-Oops.md)） |
| `CONFIG_DEBUG_OBJECTS` | 对象生命周期误用检测（init/destroy 配对） |

→ **Ch 9–10** 自旋锁 vs mutex 上下文规则

#### 现代三巨头：LOCKDEP / KASAN / KFENCE（+KCSAN）

书中只点了一句"方向一致"，实际已是内核调试的常规武器：

| 工具 | 机理 | 抓什么 | 代价 | 适用 |
|------|------|--------|------|------|
| **LOCKDEP**（`CONFIG_PROVE_LOCKING`） | 运行时记录**每把锁的获取顺序**，构建锁依赖图 | **ABBA 死锁隐患**（尚未发生就报警）、中断上下文用错锁类型 | 开销大（锁操作慢数倍） | 开发/复现环境 |
| **KASAN**（Address Sanitizer） | 每次内存访问查**影子内存**（1/8 额外内存记录每字节状态） | 越界读写、use-after-free（**释放后立刻**用才抓得到） | 慢 2~3 倍 + 内存 1.25 倍 | 测试负载跑真实压力 |
| **KFENCE** | 低开销采样式：**少量**对象单独放隔离页（guard page 包围） | 与 KASAN 同类错误，但**只覆盖被采样命中的分配** | **近乎零**（<1%）——可常驻生产 | 生产/长跑环境 |
| KCSAN | 时钟采样检测**数据竞争**（两次读中间见写） | 无锁共享的 racy 读 | 低 | 有并发嫌疑的子系统 |

> 选型读法：**KASAN 是"全量精确"、KFENCE 是"抽样廉价"**——同一类 bug 的两种经济学。生产 HFT 机器常驻 KFENCE 没问题；KASAN/LOCKDEP 只进 lab。
> 静态对应物是**sparse**（`make C=1`，抓 `__user` 地址误用，见 [5.4](../../chapter-05-system-calls/notes/section-5.4-实现与参数验证.md)）。

#### 调试内核 ≠ 生产内核

| 维度 | 调试内核 | 生产内核 |
|------|----------|----------|
| LOCKDEP/KASAN | 开 | **关**（慢 2~3 倍） |
| KFENCE | 开 | 可开（近零开销） |
| DEBUG_INFO | 开 | 开（符号可分离存 debuginfo 包，不占运行内存） |
| panic_on_oops | 无所谓 | **常设 1**（见 [18.3](./section-18.3-Oops.md)） |



<details>
<summary>自测题（点击展开）</summary>

**Q1.** CONFIG_DEBUG_INFO 有什么用？为什么 HFT 内核应该开启？

<details><summary>答案</summary>

CONFIG_DEBUG_INFO 在编译时保留调试符号（DWARF），vmlinux 包含完整的函数/变量/行号信息。开启后：1) Oops 的 addr2line 能精确定位源码行；2) gdb 调试内核（kgdb）有符号信息；3) crash 工具分析 vmcore。代价：vmlinux 体积增大 ~10x（但运行时无影响）。HFT 生产内核应开启（debuginfo 分离存储，不影响运行时性能）。

</details>

**Q2.** KFENCE 与 KASAN 抓同一类 bug，为什么内核两个都要？各自的经济模型是什么？

<details><summary>答案</summary>

KASAN：**全量精确**——每次访存查影子内存，捕获率高但慢 2~3 倍、内存 +25%，只能进测试环境跑压力负载。KFENCE：**低频抽样**——只有被采样选中的少量分配进入隔离页（guard page 包围），覆盖面小但开销 <1%，可以**常驻生产**，抓长跑才暴露的偶发越界/UAF。两者是同一类错误的两种经济学：测试期买"覆盖率"，生产期买"在线时间"。真中招时 KFENCE 的报告同样带完整栈与分配/释放历史。

</details>

**Q3.** 为什么 sleep-inside-spinlock 检测抓的是"死锁元凶"而不是死锁本身？

<details><summary>答案</summary>

死锁（ABBA）真正发生时系统已卡死，事后无从下手。该检测在**非法睡眠发生的那一刻**（还在持锁/原子上下文）就告警——此时还没死锁，只是"如果此时调度切走，持着的锁将无法释放"这个**必要条件**成立。它把"不可复现的死锁事后分析"转化为"可观测的规则违反即时报告"。LOCKDEP 同理更早一步：锁**顺序图**里出现环就报警，连非法睡眠都还没发生。

</details>

</details>
---
