## 7.4 分析方法论

> 章节导航：[本章导读](../README.md) · 下一篇 [7.5 观测工具](./section-7.5-观测工具.md)

**本节讲什么**：内存的 USE 检查（saturation 三信号）、泄漏 vs 正常增长的判定法、缺页/WSS/direct reclaim 三个深挖方向、内存压力的传导链。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | free 低 ≠ 有问题 | cache 可回收，**saturation 信号**才算数 |
| 2 | swap / direct reclaim / PSI = **三个红灯** | 任一非零即压力 |
| 3 | 判泄漏看**形状**不看绝对值 | 单调涨不回落 vs 涨后平台 |
| 4 | minor fault 快、**major fault 是盘 I/O** | 分开计数分开归因 |
| 5 | WSS > 可用内存 = 结构性 swap | 加内存或减工作集 |

---

### 一、USE 方法（Memory）

| 字母 | 问什么 | 怎么量 | 判读 |
|------|--------|--------|------|
| **U** Utilization | 物理/虚拟内存使用 | `free -h`、`/proc/meminfo`、RSS/PSS | **free 低本身不报警**——available 才有意义 |
| **S** Saturation | 扫描、Swap、direct reclaim、OOM | `vmstat si/so`、`sar -B`、**PSI memory**、`dmesg` OOM | 见下方三红灯 |
| **E** Errors | 分配失败、ECC | `dmesg`、EDAC、应用 ENOMEM | OOM kill 计数 |

**为什么 U 的判读特殊**：Linux 拿空闲内存做 page cache（[ch8](../../chapter-08-file-systems/)）——free 低是常态而非故障。真正要看的：

```bash
free -h
#              total   used   free   shared  buff/cache  available
# Mem:          125G    40G    1.2G     2.1G        84G        85G   ← available 才是可用量
```

**Saturation 三红灯**：

| 信号 | 工具 | 含义 | HFT 判定 |
|------|------|------|---------|
| **swap si/so** | `vmstat 1` | 匿名页换入换出 | 持续非零 = 灾难（ms 级停顿） |
| **direct reclaim** | `sar -B` 的 pgscan/direct；BPF drsnoop | 分配时同步回收（kswapd 跟不上） | 任何出现都是尖刺源 |
| **PSI memory** | `/proc/pressure/memory` | 线程因内存 stall 的时间占比 | some > 0 即告警 |

> 完整检查表：[附录 A](../../appendix-A-USE方法Linux.md) · PSI 概念见 [Ch6](../../chapter-06-cpus/)

### 二、内存压力的传导链

理解信号之间的关系（避免只见症状不知病因）：

```
内存水位逼近 low watermark
  → kswapd 后台回收（异步，尚无直接延迟）
    → 回收速度 < 分配速度
      → 水位触 min watermark
        → ★ direct reclaim：分配者自己同步回收（页延迟从 ns 变 ms 级）
          → 可回收不够 → 直接扫描 → 换出匿名页
            → swap out（盘写）→ 后续访问 swap in（盘读，ms 级停顿）
              → 还不够 → OOM kill
```

**drsnoop 的价值**：direct reclaim 发生在分配路径上（malloc → 缺页 → 回收）——受害者进程能被逐次抓到（谁、等了多久）；这是「偶发的分配变慢」的头号嫌疑。回收机制细节见 [06-linux-mm ch04 fault 路径](../../../06-linux-mm/chapter-04-process-address-space/)。

### 三、内存泄漏 vs 正常增长

| 现象 | 可能原因 | 验证 |
|------|----------|------|
| RSS 单调涨、从不回落 | **Leak**——alloc 无 free | Valgrind/ASan（测试）；生产 BPF uprobe malloc/free 差值 |
| 启动后涨然后平台 | 预热 cache、加载合约字典 | 预期行为（画曲线确认平台） |
| PSS 涨、多进程共享库 | 映射增多 | `pmap -X` 分项 |
| 阶梯涨（每天一阶） | 日志缓冲/日报表累积 | 对齐业务周期 |
| RSS 稳但 available 降 | 别的进程/内核 slab 在涨 | slabtop + 全进程扫描 |

**HFT**：7×24 行情服务画 **RSS/PSS 日曲线**——斜率异常先查 leak（工具见 [7.5](./section-7.5-观测工具.md)），再查 order book 是否无界增长（业务对象泄漏：过期合约/断线会话没清）。**内核侧**同样会泄漏：slabtop 里 dentry/skbuf 持续涨是内核对象泄漏的形态。

### 四、缺页与 WSS 剖析

| 方法 | 工具 | 产出 |
|------|------|------|
| **Page fault profiling** | `perf record -e page-faults -g` | **缺页火焰图**——谁在 touch 新页 |
| **Major fault 精查** | `perf record -e major-faults -g` | 谁的缺页在读盘 |
| **Direct reclaim 延迟** | BPF `drsnoop` | 哪进程在等回收、等多久 |
| **WSS 估算** | BPF `wss`（referenced 位法） | 容量规划——真实热集多大 |

**minor vs major fault 的性能差**：

| 类型 | 处理 | 延迟量级 |
|------|------|---------|
| minor fault | 内核分配物理页 + 建映射（可能 zero page） | ~1µs |
| major fault | 从盘读回（swap 或文件） | **ms 级** |

热路径的纪律：**稳态零 major fault、minor fault 也要平稳**——启动后数据常驻（mlock/预 touch），任何稳态缺页都是工作集在漂移的信号。缺页四路分发机制（匿名/COW/文件/swap）见 [06-linux-mm ch04](../../../06-linux-mm/chapter-04-process-address-space/)。

**WSS 的用途**：`wss` 用页表 referenced 位采样统计「真实被摸过的页」——比 RSS 准（RSS 含冷页）。容量规划的口径：**WSS + 余量 < available**，且 WSS 的增长曲线要有上限。

### 五、60 秒内存检查

```bash
free -m                          # available
vmstat 1 5                       # si/so、扫描
cat /proc/pressure/memory        # PSI
slabtop -o | head -15            # 内核 slab top
dmesg -T | grep -i oom | tail    # OOM 历史
numastat -M                      # NUMA 均衡（[ch7 其他节]）
```

### HFT / 嵌入式关联

- **热路径内存 mlock**：order book/行情缓冲锁在物理内存——杜绝 major fault（swap-in）与直接回收导致的 µs→ms 尖刺；mlock 配合 [ch6 隔离栈](../../chapter-06-cpus/) 是低延迟双保险。
- **hugepage 减 TLB miss**：见 [06-linux-mm THP](../../../06-linux-mm/chapter-03-page-table-management/notes/note-透明大页THP.md)——数据库式显式 hugetlbfs 或 THP always。
- **RSS 曲线进监控**：斜率告警（%/天）比阈值告警早发现泄漏。
- **嵌入式**：无 swap 的板子 swap 信号恒零——PSI + direct reclaim 计数是唯一压力信号；内存不足直接 OOM（更暴力）。

### 衔接

- 下一节：[7.5 观测工具](./section-7.5-观测工具.md)（本方法论的武器）
- 关联：[ch6 PSI](../../chapter-06-cpus/)、[ch8 page cache](../../chapter-08-file-systems/)、[ch9 swap-in 的盘侧](../../chapter-09-disks/)、[06-linux-mm ch04 fault 机制](../../../06-linux-mm/chapter-04-process-address-space/)、[06-linux-mm ch06 回收](../../../06-linux-mm/chapter-06-physical-page-allocation/)

---

### 常见陷阱

1. **USE 只查 free**——saturation（swap/direct reclaim/PSI）才是关键；free 低是 Linux 正常形态。
2. **RSS 当真实占用**——共享库整页重复计算；PSS 才公平。
3. **leak 只看 RSS**——可能是 cache/映射增多；pmap -X 分项确认 heap。
4. **忽略内核侧泄漏**——slab（dentry/skbuf）涨挤占用户内存，进程视角不可见。
5. **稳态 minor fault 不当回事**——工作集在漂移的早期信号，等 major fault 才查就晚了。

<details>
<summary>自测题（点击展开）</summary>

1. 内存饱和的三个红灯？
   <details><summary>答</summary>swap si/so（换页在发生）、direct reclaim（分配者同步回收）、PSI memory stall——任一非零即有压力。</details>
2. direct reclaim 为什么造成分配尖刺？
   <details><summary>答</summary>水位触 min 后分配路径同步回收——malloc/缺页从 ns 级变 ms 级；kswapd 异步回收跟不上时发生。</details>
3. 如何区分泄漏和正常预热？
   <details><summary>答</summary>形状：单调涨不回落 = leak；涨后平台 = 预热。pmap -X 分项确认 heap 还是映射；内核侧查 slabtop。</details>
4. minor 和 major fault 的延迟差？
   <details><summary>答</summary>minor ~1µs（内核分配页+建映射）；major ms 级（盘读回）——稳态出现 major fault 是热路径事故。</details>
5. WSS 和 RSS 的区别与用途？
   <details><summary>答</summary>WSS 是真实被访问的热页集（referenced 位采样），RSS 含冷页——容量规划用 WSS+余量，比 RSS 准。</details>

</details>


---

← [本章导读](../README.md)
