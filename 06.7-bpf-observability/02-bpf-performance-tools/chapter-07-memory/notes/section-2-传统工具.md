# 7.2 传统工具

> 底本：《BPF之巅》第 7 章 内存，7.2 节（印刷 p263–269）。三类：内核日志 / 内核统计 / 硬件统计和采样（表 7-2）。传统工具提供"容量"视角（用了多少），跟踪视角（谁、哪条路径）要靠 BPF。

## 7.2.1 内核日志

**dmesg** — 深入内存分析前**必查** OOM 记录：

```
Out of memory: Kill process 23409 (perl) score 329 or sacrifice child
Killed process 23409 (perl) total-vm:5370580kB, anon-rss:5224980kB
```

日志含全系统内存状态（active/inactive anon、slab、free…）、每进程表（total_vm/rss/oom_score_adj）、被杀目标与原因。

## 7.2.2 内核统计信息（/proc 数据源，开销≈0）

| 工具 | 用途 | 关键点 |
|------|------|--------|
| `swapon` | swap 配置与用量 | 无输出 = 未配 swap（现代生产常态） |
| `free -m` | 全系统内存 | **看 available 不看 free**（available 含可回收缓存）；`-w` 分开 buff/cache |
| `ps aux` | 每进程 | `%MEM`/VSZ（虚拟）/RSS（物理）三列；书例 java 占 95.7% 物理内存 |
| `pmap -x PID` | 按地址段 | 找库和映射文件占了多少；`-x` 加 Dirty 列（改过未回写的页） |
| `vmstat 1` | 时间序列 | 第一行是开机以来均值（memory 列除外）；**si/so** = 换入/换出；free/buff/cache |
| `sar -B 1` | 缺页与扫描 | fault/s、majflt/s、**pgscank/s、pgscand/s**（页扫描）、pgsteal/s、%vmeff |

sar -B 实例对比：

- 空闲生产机：fault/s < 300，pgscan 全 0 → 无内存压力
- 软件编译中：fault/s **>100 万**（短命进程不断首访新地址空间），pgscand 高 → 内存压力出现（kswapd 直接扫描）

## 7.2.3 硬件统计和硬件采样（PMC）

PMC 观测的是 CPU↔主存之间的 I/O（途经 CPU 缓存）。

**累计模式**（开销≈0）：

```bash
perf stat -e LLC-loads,LLC-load-misses -a -I 1000
# LLC-load-misses 3,610,704  42.97% of all LL-cache hits
```

perf 自动算出未命中百分比；**LLC 未命中 ≈ 主存读流量**（LLC 不命中就去主存）。

**采样模式**（定位到指令/函数）：

```bash
perf record -e L1-dcache-load-misses -c 100000 -a   # 每 10 万次事件采 1 样本
perf report -n --stdio
```

- `-c 100000` 很大因为 L1 未命中极其频繁 — **不确定频率时先用 perf stat 累计模式估算，再定采样率**
- **PEBS**：事件名加 `:p`/`:pp`（更好）/`:ppp`（最佳）后缀启用，指令指针更精确

## 传统工具的局限（引出 BPF）

- 传统工具都是"容量统计"；想知道**每次分配来自哪条代码路径**，要么跟踪分配库（BPF），要么用 Valgrind 类虚拟机技术（性能大降不可用于生产）
- BPF：额外消耗更低、可生产环境直接分析

## HFT 关联

- 交易机巡检三件套：`free -m`（available）、`vmstat 1`（si/so 必须为 0）、`sar -B 1`（pgscand/s 为 0，fault/s 平稳）
- `pmap -x <策略pid>` 存档：RSS 增长时对比哪些段在涨（匿名堆 vs 文件映射 vs 巨页）
- LLC 未命中率高 = 每次访存多 ~100ns — perf stat -e LLC-* 进基线，与第 6 章 llcstat 呼应

## 常见陷阱

1. **free 列焦虑** — Linux 用空闲内存做缓存是设计使然，看 available
2. **vmstat 第一行当实时值** — 除 memory 列外是开机均值，分析要从第二行起
3. **fault/s 高就慌** — 编译/大量短命进程场景 fault/s 上百万是正常现象，要结合 majflt（major fault，真的走了磁盘）判断
4. **PMC 采样率拍脑袋** — 先 perf stat 累计看事件频率，再算合理 -c，否则影响被测软件

<details>
<summary>📝 自测题（点击展开）</summary>

1. **sar -B 里哪些列说明内存压力？**

   <details>
   <summary>参考答案</summary>

   pgscank/s（kswapd 后台扫描）与 pgscand/s（直接回收扫描，更严重）：非零说明内核在找可释放页。fault/s 高只说明缺页频繁（可能正常），配合扫描列才能定性。%vmeff = pgsteal/pgscan，回收效率。
   </details>

2. **LLC-load-misses 为什么能近似代表主存 I/O？**

   <details>
   <summary>参考答案</summary>

   缓存层级 L1→L2→LLC→主存。LLC 是最后一级缓存，未命中 LLC 的加载请求必然落到主存。所以 LLC 未命中率 × 加载量 ≈ 主存读流量，是内存带宽压力的代理指标。
   </details>

</details>
