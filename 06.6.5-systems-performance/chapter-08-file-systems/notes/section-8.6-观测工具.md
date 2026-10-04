## 8.6 观测工具

> 章节导航：[8.5 分析方法论](./section-8.5-分析方法论.md) · 上一篇 ← · 下一篇 [8.7–8.8 实验与调优](./section-8.7-8.8-实验与调优.md) · [本章导读](../README.md)

**本节讲什么**：FS 观测的两层工具（传统统计 / BCC 专项）、cachestat 与 ext4slower 的输出精读、双峰直方图的判读。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | `cachestat` 回答「**到底有没有读盘**」 | 命中率第一手证据 |
| 2 | `ext4dist` 的**双峰**是结构信息 | 快 cache / 慢 disk |
| 3 | `ext4slower` 抓**超阈值现场** | 慢操作逐次+进程+文件 |
| 4 | `opensnoop` 揪意外文件访问 | 配置/权限/安全 |
| 5 | `filetop` 是 FS 层的 **top** | 哪个文件在狂读写 |

---

### 一、传统统计层

| 工具 | 用途 | 关键 |
|------|------|------|
| **`mount`** | 挂载选项审计 | `noatime`、`data=` 模式 |
| **`free` / `top` / `vmstat`** | cache 占用 | 与 [ch7](../../chapter-07-memory/) 联动 |
| **`sar -v`** | dentry/inode cache 统计 | 增长趋势（泄漏/洪泛） |
| **`slabtop`** | dentry/inode slab 占用 | 谁吃掉了内存 |

`sar -v` 的 dentunusd（未用 dentry）持续涨 = 文件创建删除频繁（或泄漏）；`slabtop` 里 dentry/inode 占用大 = 元数据工作集大——两者都指向「小文件风暴」型负载。

### 二、BPF / BCC 工具集

| 工具 | 作用 | HFT 场景 |
|------|------|----------|
| **`opensnoop`** | 谁 open 了什么文件 | 找意外读配置、权限问题 |
| **`filetop`** | 按文件 I/O 吞吐排序 | 哪份日志/数据文件在狂读写 |
| **`cachestat`** | page cache **命中率** | 区分 cache 命中 vs 真读盘 |
| **`ext4dist` / `xfsdist`** | FS 操作延迟直方图 | 看双峰 |
| **`ext4slower` / `xfsslower`** | 超阈值慢操作 | 抓 fsync、journal 尖刺 |
| `fileslower` | VFS 层慢操作（跨 FS） | 不依赖具体 FS |
| bpftrace VFS 单行 | `vfs_read` 等定点 | [附录 C](../../appendix-C-bpftrace单行命令.md) |

**⭐ cachestat 精读**：

```
    HITS   MISSES  DIRTIES HITRATIO   BUFFERS_MB   CACHES_MB
   12345        3       12    99.98%          32      10240
```

- `HITRATIO 99.98%`：读几乎全命中——读慢与盘无关（[8.5 读路径分解](./section-8.5-分析方法论.md)）
- `MISSES`：每秒真读盘次数——**这是连接 FS 层与 ch9 的桥**（miss 数 ≈ iostat 的 r/s）
- `DIRTIES`：每秒脏页数——writeback 压力的前瞻指标
- 命中率跌 + miss 涨 → 查内存回收/工作集（ch7）；miss 稳定但延迟涨 → 查盘（ch9）

**⭐ ext4slower 精读**：

```
Tracing ext4 operations slower than 10 ms
TIME     COMM           PID    T BYTES   OFF_KB   LAT(ms) FILENAME
14:03:22 strategy       4321   W 4096    1048572  12.5     trade-20260831.log
14:03:22 jbd2/nvme0n1-8 305    W 131072  -        15.2     -   ← journal 线程！
```

- 每行 = 一次超阈值操作：谁、读/写、多大、哪个文件、多慢
- `jbd2` 出现 = journal commit 在慢操作现场——写尖刺与 journal 争用的直接证据（[8.4 journal](./section-8.4-文件系统架构与特性.md)）
- `trade-*.log` 频繁出现 = 日志盘/策略路径的 I/O 冲突实锤

**ext4dist 双峰判读**：

```
     usecs           : count    distribution
         0 -> 1      : 45201    |**************      ← cache 命中（µs 级）
         2 -> 7      : 3012     |*
       ...
      8192 -> 16383  : 892      |                     ← 盘 I/O（ms 级）第二峰
```

两个峰天然分开（内存 vs 盘的数量级差）——**峰高比例就是 cache 命中率的另一种表达**；第二峰变宽/右移 = 盘路径恶化。

### 三、工具选型速查

| 问题 | 第一工具 | 深挖 |
|------|---------|------|
| 到底有没有读盘？ | `cachestat` | ext4dist 双峰 |
| 哪个操作慢？ | `ext4slower`/`fileslower` | biostacks（ch9） |
| 哪个文件在狂读写？ | `filetop` | biosnoop |
| 谁在 open 奇怪文件？ | `opensnoop` | auditd（合规级） |
| 元数据缓存健康？ | `sar -v` | `slabtop` |
| 挂载选项对不对？ | `mount` | /etc/fstab 审计 |

### 四、与 ch9 的分工

| 层 | 本章 | ch9 |
|----|------|-----|
| 问题形态 | 「应用读写慢」「fsync 尖刺」 | 「盘忙」「I/O 延迟高」 |
| 工具 | cachestat/ext4slower/filetop | biolatency/biosnoop/iostat |
| 桥接 | cachestat 的 MISSES | ≈ iostat 的 r/s |
| 典型结论 | cache miss / journal 争用 | 队列 / GC / Sloth |

自上而下：先用本章工具确认「FS 层真有问题且到了块层」，再换 ch9 工具在块层归因。

### HFT / 嵌入式关联

- **巡检常驻**：cachestat 命中率（热路径读应恒 100%）——跌破即告警，早于任何延迟指标。
- **事件触发**：ext4slower 抓写尖刺现场 + jbd2 出现与否（journal 争用 vs 盘问题分流的第一个证据）。
- **opensnoop 安全审计**：热路径进程的文件访问白名单化——意外 open（配置热加载、locale 文件、/etc 查询）是启动延迟和隐蔽 I/O 的来源。
- **嵌入式**：BCC 不可用时，`/proc/meminfo` 的 Cached + vmstat 的 bi/bo 是 cachestat 的退化替代（精度低但有总账）。

### 衔接

- 上一节：[8.5 分析方法论](./section-8.5-分析方法论.md)
- 下一节：[8.7–8.8 实验与调优](./section-8.7-8.8-实验与调优.md)
- 关联：[ch15 BPF](../../chapter-15-bpf/)、[ch9 磁盘观测](../../chapter-09-disks/notes/section-9.6-观测工具.md)、[ch7 内存](../../chapter-07-memory/)、[06.7-BPF](../../../06.7-bpf-observability/)

---

### 常见陷阱

1. **读慢不查 cachestat**——命中率是第一手证据；miss 数直接桥接 ch9。
2. **ext4slower 只看应用进程行**——jbd2 行才是 journal 争用的实锤。
3. **filetop 忽略读方向**——读热点文件（配置/字典反复读）与写热点（日志）是不同问题。
4. **slabtop 里 dentry 大不管**——元数据工作集大说明文件数量失控（容器场景常见）。

<details>
<summary>自测题（点击展开）</summary>

1. cachestat 的 MISSES 字段为什么是连接 ch8/ch9 的桥？
   <details><summary>答</summary>miss 即真读盘次数——近似等于 iostat 的 r/s：FS 层的 miss 计数与块层的读计数对上，问题就从「FS 层」推进到「块层」。</details>
2. ext4slower 里出现 jbd2 行说明什么？
   <details><summary>答</summary>ext4 journal 线程的 commit 操作超阈值——写尖刺时刻 journal 在现场，争用/串行化的直接证据。</details>
3. ext4dist 双峰的成因与判读？
   <details><summary>答</summary>cache 命中（µs）与盘 I/O（ms）天然分离；峰高比≈命中率，第二峰右移/变宽=盘路径恶化。</details>
4. 热路径进程被 opensnoop 发现读意外文件，可能是？
   <details><summary>答</summary>配置热加载轮询、locale/zoneinfo 惰性读取、动态链接库缺失后的搜索路径——启动延迟与隐蔽 I/O 的来源。</details>
5. dentry/inode slab 占用大说明什么负载？
   <details><summary>答</summary>文件数量庞大的元数据工作集（海量小文件/容器镜像层）——查创建删除频率与目录结构。</details>

</details>


---

← [本章导读](../README.md)
