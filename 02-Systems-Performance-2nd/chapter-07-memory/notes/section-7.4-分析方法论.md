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


---

← [本章导读](../README.md)
