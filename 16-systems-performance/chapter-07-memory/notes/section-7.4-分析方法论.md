## 7.4 分析方法论

### USE 方法（Memory）

| 字母 | 问什么 | 怎么量 |
|------|--------|--------|
| **U** Utilization | 物理/虚拟内存使用 | `free -h`、`/proc/meminfo`、RSS/PSS |
| **S** Saturation | 扫描、Swap、direct reclaim、OOM | `vmstat si/so`、`sar -B`、**PSI memory**、`dmesg` OOM |
| **E** Errors | 分配失败、ECC | `dmesg`、EDAC、应用 ENOMEM |

**PSI memory：**

```bash
cat /proc/pressure/memory
# some/full — 线程因等内存而 stall 的时间占比
```

→ [附录 A](../../appendix-A-USE方法Linux.md) · Ch 6 [PSI 概念](../../chapter-06-cpus/)

### 内存泄漏 vs 正常增长

| 现象 | 可能原因 | 验证 |
|------|----------|------|
| RSS 单调涨、从不回落 | **Leak** — alloc 无 free | Valgrind/ASan（测试）；生产 BPF uprobe malloc |
| 启动后涨然后平台 | 预热 cache、加载合约字典 | 预期行为 |
| PSS 涨、多进程共享库 | 映射增多 | `pmap -X` 分项 |

**HFT：** 7×24 运行的行情服务 — 画 **RSS/PSS 日曲线**；斜率异常先查 leak，再查 order book 是否无界增长。

### 缺页与 WSS 剖析

| 方法 | 工具 | 产出 |
|------|------|------|
| **Page fault profiling** | `perf record -e page-faults` | **缺页火焰图** — 谁在 touch 新页 |
| **Direct reclaim 延迟** | BPF `drsnoop` | 哪进程在等回收 |
| **WSS 估算** | BPF `wss`（实验） | 容量规划 |

---


### 常见陷阱

1. USE 只查 free——Saturation（swap/direct reclaim/PSI memory）才是关键
2. RSS 当真实占用——RSS 包含共享库整页，PSS（按比例分摊）才反映真实占用
3. leak 只看 RSS——RSS 涨可能是 cache/映射增多，要用 pmap -X 分项确认是 heap leak

<details>
<summary>自测题（点击展开）</summary>

1. 内存的 USE 方法中 Saturation 看什么？
   <details><summary>答</summary>swap si/so、direct reclaim、PSI memory stall——任一非零说明内存压力</details>
2. RSS 和 PSS 的区别？
   <details><summary>答</summary>RSS 把共享库整页算给每个进程（重复计算），PSS 按进程数分摊共享页（更真实）</details>
3. 如何区分内存泄漏和正常增长？
   <details><summary>答</summary>RSS 单调涨不回落 = leak；涨后平台 = 预热 cache——用 pmap -X 分项确认是 heap 还是映射</details>

</details>


---

← [本章导读](../README.md)
