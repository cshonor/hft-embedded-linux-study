## 9.5 分析方法论

> 章节导航：[9.4 硬件与软件架构](./section-9.4-硬件与软件架构.md) · 上一篇 ← · 下一篇 [9.6 观测工具](./section-9.6-观测工具.md) · [本章导读](../README.md)

**本节讲什么**：磁盘的 USE 方法检查表、工作负载特征化问题链、自上而下的全栈延迟分解树、性能优先级策略（可调优/不可调优资源）。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | USE 对**每块盘 + 控制器**各问三句 | 别漏 HBA/阵列这一级 |
| 2 | 先特征化后深挖 | 谁在什么时候打了什么 I/O |
| 3 | **自上而下**，别跳层 | cache 命中时调 scheduler 是白费 |
| 4 | 磁盘是**可调资源**，容量不可调 | 优先级：减量 > 调度 > 换介质 |
| 5 | 单指标永远不够 | util + await + PSI + 直方图交叉 |

---

### 一、USE 方法（Disk）

对**每块磁盘**及**控制器**：

| 字母 | 问什么 | 工具 | 判读 |
|------|--------|------|------|
| **U** Utilization | 设备忙的时间比 | `iostat -xz 1` `%util` | 虚拟盘会撒谎（[9.1](./section-9.1-9.3-核心概念与模型.md)），结合 await |
| **S** Saturation | 队列长度、等待 | `avgqu-sz`、`await`、PSI io | avgqu-sz > 1 持续 = 饱和；PSI some>0 = 有 stall |
| **E** Errors | 驱动/HBA/磁盘错 | `dmesg`、`smartctl`、/proc/diskstats | 重映射扇区增长、medium error、link reset |

扩展：**控制器级**（HBA/阵列卡）也要 U/S/E——`iostat -xz` 看 HBA 汇总带宽是否顶到 PCIe/RAID 卡上限（厂商 CLI）。

> 完整 USE 检查表：[附录 A](../../appendix-A-USE方法Linux.md)

### 二、工作负载特征化（先看清楚再动手）

| 问题 | 工具 | 答案用途 |
|------|------|---------|
| 哪块盘忙？ | `iostat -xz 1` | 定位目标设备 |
| 什么类型的 I/O？ | iostat 的 r/s w/s、merge | 读 vs 写、合并效率 |
| 哪个进程？ | `pidstat -d`、`biotop` | 找到发起者 |
| 什么 syscall 路径？ | `biostacks`、`biosnoop` | 区分应用/journal/kswapd/flush |
| 负载均衡吗？ | 多盘 iostat 对比、RAID CLI | 单盘热点 vs 均摊 |
| 时间分布？ | `sar -d` 历史 | 周期性（备份/日志滚动）还是常态 |

`biostacks` 的独特价值：块 I/O 的**发起内核栈**——一眼揪出「不是应用发的」I/O（ext4 journal commit、kswapd 直接回收、flush 线程 writeback）。**后台 I/O 是磁盘问题的隐形嫌疑犯**：应用明明没读写，盘却在忙——答案通常在这三类栈里（与 [ch7 direct reclaim](../../chapter-07-memory/)、[ch8 journal](../../chapter-08-file-systems/) 交叉）。

### 三、延迟分析（全栈分解树）

```
应用阻塞/变慢
  │
  ├─ syscall 慢？ → strace/perf trace 计时（read/write/fsync 各多少）
  │     ├─ VFS/FS 锁或 journal？ → Ch8 的 fileslower/ext4slower
  │     ├─ page cache miss → 变成块 I/O ↓
  │     └─ writeback 限速（脏页超阈值）→ 可感知为 write 偶尔巨慢
  │
  ├─ 块 I/O 慢？ → biolatency 直方图（-F 分读写）
  │     ├─ wait 高（avgqu-sz 大）→ 队列排队：负载/邻居/调度器
  │     └─ service 高 → 设备：GC？降级盘？Sloth？
  │
  └─ 单笔 outlier？ → biosnoop 逐次 + smartctl
        └─ >1s 且 SMART 无错 → Sloth Disk → 换盘验证
```

**原则：自上而下**——应用还在 page cache 命中时去调磁盘 scheduler 是白费力气；先确认问题真的到了块层（biolatency 有没有流量），再在块层内部分 wait/service。

**双峰直方图的分流**（与 [ch16 判读法](../../chapter-16-case-studies/) 同一套）：

| 形状 | 含义 | 下一步 |
|------|------|--------|
| 单峰右偏 | 正常排队 | 无事 |
| **双峰**（µs 峰 + ms 峰） | 混合负载或 GC | 分读写/分进程看（-F / biotop） |
| 长尾拖到 1s+ | Sloth / 降级 | biosnoop 定位 + smartctl |

### 四、性能优先级策略

磁盘是**可调资源**（调度、队列、优先级），容量与介质是**不可调资源**（要花钱）——优先做免费的：

| 优先级 | 手段 | 成本 |
|--------|------|------|
| 1 | **减 I/O**：缓存、合并写、异步化 | 应用改造 |
| 2 | **错峰/降级**：ionice、cgroup io.max、限速 | 配置 |
| 3 | 调度/队列参数 | 配置（收益有限） |
| 4 | 换介质/加盘/RAID | 硬件预算 |

典型顺序错误：盘饱和 → 先买盘。正确顺序：盘饱和 → 先看 biotop 谁在打 I/O → 常是备份任务/journal/swap → ionice + 错峰解决 → 不用买盘。

### 五、60 秒磁盘检查（ch1 清单的磁盘部分）

```bash
iostat -xz 1            # util/await/avgqu-sz（虚拟盘陷阱在心）
pidstat -d 1            # 哪个进程在读盘
cat /proc/pressure/io   # PSI stall 证据
dmesg | tail -30        # 存储相关错误（link reset、medium error）
swapon --show && vmstat 1   # swap 活动（盘忙的隐形原因）
```

### HFT / 嵌入式关联

- **tick 尖刺与 I/O 对齐验证**：尖刺时刻的 `biolatency`/`biosnoop` 是否有对应 outlier——有则追 I/O 来源（biostacks 揪 journal/kswapd），无则排除盘、查 CPU/网络方向。
- **swap 是纪律红线**：热路径机器 swap 应为 0——一次 page-in 就是 ms 级停顿（内存策略见 [ch7](../../chapter-07-memory/)，mlock 热路径内存）。
- **写入模式审计**：日志顺序写 + 定长记录（对 FTL/GC 友好）+ 异步批量 flush——这三条让 NVMe 的 P99 写延迟稳定。

### 衔接

- 上一节：[9.4 硬件与软件架构](./section-9.4-硬件与软件架构.md)
- 下一节：[9.6 观测工具](./section-9.6-观测工具.md)（本方法论的武器）
- 关联：[ch2 USE 方法](../../chapter-02-methodologies/)、[ch8 文件系统](../../chapter-08-file-systems/)、[ch7 内存回收](../../chapter-07-memory/)（kswapd 的 I/O 面）

---

### 常见陷阱

1. **跳层归因**——应用层 fsync 慢（journal 频率问题）被当成「盘慢」换盘；先全栈分解再动作。
2. **忘了后台 I/O**——journal/kswapd/flush 是盘忙的三大隐形来源，biostacks 一眼定罪。
3. **忘了控制器层**——HBA 饱和时每块盘的 util 都不高但 await 齐涨；看 PCIe/RAID 卡级带宽。
4. **饱和就买盘**——先减量错峰（免费）后扩容（花钱）。

<details>
<summary>自测题（点击展开）</summary>

1. USE 方法对磁盘多出来的一个检查对象是什么？
   <details><summary>答</summary>控制器/HBA/阵列卡——盘没问题但控制器饱和（PCIe/RAID 卡带宽顶格）时每块盘 await 齐涨而 util 都不高。</details>
2. 应用没做 I/O 但盘在忙，怎么查？
   <details><summary>答</summary>biostacks 看块 I/O 发起栈——ext4 journal commit、kswapd 直接回收、flush 线程 writeback 三大后台来源。</details>
3. biolatency 出现 µs 峰 + ms 峰的双峰，说明什么？
   <details><summary>答</summary>混合负载（不同类型 I/O 叠加）或 SSD GC 干扰——用 -F 分读写、biotop 分进程拆开看。</details>
4. 盘饱和的正确处理顺序？
   <details><summary>答</summary>减 I/O（缓存/异步）→ 错峰降级（ionice/cgroup 限额）→ 调度参数（收益小）→ 换介质/扩容（花钱）——先免费后付费。</details>
5. 为什么 swap 活动要在磁盘排查中检查？
   <details><summary>答</summary>page-in 是 ms 级 I/O 且来源是内存压力不是存储需求——vmstat si/so 有值时问题在内存侧（ch7），调盘没用。</details>

</details>


---

← [本章导读](../README.md)
