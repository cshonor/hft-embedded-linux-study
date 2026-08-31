## 8.5 分析方法论

> 章节导航：[8.4 文件系统架构与特性](./section-8.4-文件系统架构与特性.md) · 上一篇 ← · 下一篇 [8.6 观测工具](./section-8.6-观测工具.md) · [本章导读](../README.md)

**本节讲什么**：文件系统的延迟分层测量法、读/写两条路径的分解树、工作负载特征化五维度、事务成本判定阈值、WSS 陷阱。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | **四层延迟测量**自上而下 | 应用→syscall→VFS→FS |
| 2 | 读写路径**分开分解** | 读卡在 cache miss；写卡在 writeback/fsync |
| 3 | 事务成本 **<1% 排除 FS** | >10% 深挖 |
| 4 | 读 I/O 的第一嫌疑是 **cache miss** | 写 I/O 的第一嫌疑是 **fsync/journal** |
| 5 | WSS << RAM 的 fio 测的是 **cache** | — |

---

### 一、延迟分析：四层测量

```
① 应用事务计时（端到端）       —— 策略一个主循环多少 µs
② syscall 层                  —— read/write/fsync 各多少（perf trace / BPF）
③ VFS 层                      —— vfs_read/write 追踪
④ FS 层                       —— ext4_file_read_iter / xfs_* 延迟直方图（ext4dist）
⑤ 块层（Ch 9）                —— biolatency/biosnoop
```

**层间差值即线索**：

| 现象 | 结论 |
|------|------|
| ①慢 ②快 | 问题不在 FS（锁/调度/计算） |
| ②慢 ③④快 | syscall 路径开销（copy、锁竞争） |
| ③慢 ④快 | cache/路径查找问题 |
| ④慢 ⑤快 | FS 内部：journal、分配、锁 |
| ⑤慢 | 盘/队列（[ch9 方法论](../../chapter-09-disks/notes/section-9.5-分析方法论.md)） |

### 二、读路径分解树

```
read() 慢
  ├─ page cache 命中？ → cachestat 命中率
  │     ├─ 命中且仍慢 → copy/锁/syscall 开销（②③层）
  │     └─ miss 高 → 谁把 cache 挤了？
  │           ├─ 内存压力回收（ch7 PSI memory）
  │           ├─ 工作集真大于 RAM（WSS 问题）
  │           └─ 读模式随机（预读无效）
  └─ 变成块 I/O → ch9 分解（wait vs service）
```

**读问题的第一嫌疑永远是 cache miss**——命中率从 99% 掉到 90%，盘 I/O 涨 10×（线性放大效应）。`cachestat`（[8.6](./section-8.6-观测工具.md)）是第一手证据。

### 三、写路径分解树

```
write() 慢 / fsync() 慢
  ├─ write() 慢？
  │     ├─ 脏页超阈值 → writeback 限速（throttle，可感知为偶发巨慢）
  │     └─ dirty_ratio 太小 / 刷盘跟不上
  ├─ fsync() 慢？
  │     ├─ journal commit 争用（jbd2 在 commit 中，等下一个周期）
  │     ├─ 脏页量大（攒太久一次刷）
  │     └─ 盘 GC/排队（ch9）
  └─ ext4slower/xfsslower 抓现场 + biostacks 看 jbd2
```

**写问题的第一嫌疑是 fsync/journal**：异步写把成本推迟到 fsync 时刻集中爆发——「平时快、偶尔 ms 级尖刺」的典型来源。`data=ordered` + 大 commit 间隔 + 攒批写 是标准缓解。

### 四、事务成本（Transaction Cost）

```
事务成本 = 事务总时间中阻塞在 FS I/O 上的比例
```

| 比例 | 判定 |
|------|------|
| < 1% | FS 不是瓶颈——去查 CPU/锁/调度（[ch6](../../chapter-06-cpus/)） |
| 1–10% | 边缘——看趋势 |
| > 10% | 深挖：cache 命中、慢 fsync、元数据风暴 |

这是**快速排除法**——比逐层排查省时间：先量比例，小就直接换方向。

### 五、工作负载特征化（五维度）

| 维度 | 问什么 | 工具 |
|------|--------|------|
| **IOPS** | 每秒多少次 I/O | iostat、`filetop` |
| **吞吐量** | MB/s | `filetop`、sar |
| **I/O 大小** | 4K vs 1M | `biosnoop`、fio |
| **读/写比** | 读多还是写多 | sar、BPF |
| **随机/顺序** | 预取是否有效 | fio `--rw=randread` vs `read` |

特征化的价值：**负载形状决定调优方向**——顺序读调预读（read_ahead_kb）、随机读调缓存大小、写多查 journal 策略、元数据多（小文件创建删除）查分配器与 dentry。

### 六、微基准的 WSS 陷阱

| 测试集大小 | 实际测到的是 |
|------------|--------------|
| **WSS << RAM** | **page cache 性能**——极快，误导 |
| **WSS >> RAM** | 磁盘 + FS 真实路径 |
| **O_DIRECT** | 绕过 cache，测磁盘/FS 直连 |

```bash
# 清空 page cache（仅测试环境！生产禁止——会引发 I/O 风暴）
echo 3 | sudo tee /proc/sys/vm/drop_caches
```

与 [ch12 的失败模式①](../../chapter-12-benchmarking/notes/section-12.1-基准测试的背景与挑战.md) 完全同源：测试集必须 >> RAM 或 direct=1，否则报告的是内存带宽。

### 七、60 秒 FS 检查

```bash
mount | grep -v proc          # 挂载选项（noatime 有没有）
free -m                       # cache 占用（可回收）
sar -v 1 3                    # dentry/inode 增长
sudo cachestat-bpfcc 5        # 命中率
sudo ext4slower-bpfcc 10      # >10ms 的慢操作
```

### HFT / 嵌入式关联

- **tick 循环的事务成本应 < 0.1%**：热路径读到的一切（配置、合约表）启动时加载并常驻——任何常态 FS I/O 都是架构问题。
- **日志的 fsync 策略是显式决策**：每条 fsync（安全、慢）vs 攒批周期刷（快、可丢最后 N 秒）——合规要求决定，不是性能决定；写进 runbook。
- **嵌入式 flash**：小文件高频创建删除（日志分片）在 eMMC 上触发元数据 I/O 风暴——环形缓冲单文件 + 定长记录绕开元数据。

### 衔接

- 上一节：[8.4 架构与特性](./section-8.4-文件系统架构与特性.md)
- 下一节：[8.6 观测工具](./section-8.6-观测工具.md)
- 关联：[ch2 延迟分解](../../chapter-02-methodologies/)、[ch7 内存回收](../../chapter-07-memory/)、[ch9 磁盘方法论](../../chapter-09-disks/notes/section-9.5-分析方法论.md)、[ch12 基准测试](../../chapter-12-benchmarking/)

---

### 常见陷阱

1. **读慢直接查盘**——先看 cachestat：命中率跌 9% 就能让盘 I/O 涨 10×，问题在内存不在盘。
2. **写慢只看 write()**——异步写把成本搬到 fsync；两条路径分开分解。
3. **事务成本没量就深挖**——<1% 时全部 FS 调优都是白费，先排除再动手。
4. **fio 测试集小于 RAM**——测的是 page cache（[ch12 同款陷阱](../../chapter-12-benchmarking/)）。

<details>
<summary>自测题（点击展开）</summary>

1. syscall 慢但 VFS/FS 层快，说明什么？
   <details><summary>答</summary>开销在 syscall 路径本身——copy_to/from_user、锁竞争、路径查找，不是 I/O。</details>
2. cache 命中率从 99% 掉到 90% 意味着什么？
   <details><summary>答</summary>盘 I/O 涨 10 倍（线性放大）——先查谁把 cache 挤掉（内存回收/工作集增长），不是查盘。</details>
3. 「平时快、偶尔 ms 级写尖刺」的第一嫌疑？
   <details><summary>答</summary>fsync 集中爆发或 writeback 限速——异步写攒的账一次清；ext4slower 抓现场。</details>
4. 事务成本阈值怎么用？
   <details><summary>答</summary><1% 直接排除 FS 换方向；>10% 深挖（cache/fsync/元数据）；中间看趋势。</details>
5. 随机读负载该调什么？
   <details><summary>答</summary>调小预读（read_ahead_kb，防污染 cache）+ 保证缓存容量；预读对随机读无效。</details>

</details>


---

← [本章导读](../README.md)
