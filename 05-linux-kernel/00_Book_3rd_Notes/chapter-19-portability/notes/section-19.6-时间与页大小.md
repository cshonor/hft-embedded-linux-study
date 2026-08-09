## ⑥ 时间与页大小

#### jiffies 与 HZ

| 事实 | **`HZ`** 因架构/配置而异 |
|------|--------------------------|
| 错误 | `jiffies + 300` 当「3 秒」而不看 HZ |
| 正确 | **`msecs_to_jiffies(ms)`** · **`jiffies_to_msecs`** |

→ **Ch 11** `HZ` · `time_after`

#### PAGE_SIZE

| 事实 | **不固定** — x86-32 常 4KB，其他或 **8/16/64KB** |
|------|--------------------------------------------------|
| 必须用 | **`PAGE_SIZE`** · **`PAGE_SHIFT`** |

```c
order = get_order(size);
alloc_pages(GFP_KERNEL, order);
```

→ **Ch 12** 页分配 · **Ch 15** 映射



<details>
<summary>自测题（点击展开）</summary>

**Q1.** jiffies 和 HZ 的关系？为什么 HFT 不能用 jiffies 做精确计时？

<details><summary>答案</summary>

jiffies = 自启动以来时钟中断次数。HZ = 每秒时钟中断次数（默认 250 或 1000）。1 jiffy = 1/HZ 秒 = 1ms~4ms。HFT 需要纳秒级计时，jiffies 精度太低。用 `ktime_get_ns()` 或 `rdtsc()` 获取纳秒时间戳。TSC（Time Stamp Counter）是 CPU 硬件计数器，每个时钟周期递增，`rdtsc` ~10ns 精度。

</details>

**Q2.** PAGE_SIZE 在不同架构上有什么不同？

<details><summary>答案</summary>

x86/ARM64 默认 4KB。ARM64 支持 16KB/64KB（CONFIG_ARM64_64K_PAGES）。某些架构支持多种页大小。HFT 应避免硬编码 4096，用 PAGE_SIZE 宏。Huge Page 大小也随架构变化：x86 = 2MB/1GB，ARM64 = 2MB/32MB/512MB/1GB。跨架构移植时页大小差异影响 mmap 粒度和 TLB 效率。

</details>

</details>
---
