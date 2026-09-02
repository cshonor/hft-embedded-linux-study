# Ch 14 §2 实践：直觉、模拟与调优

> **Understanding the Linux Virtual Memory Manager** · Mel Gorman · **跳过 ⚪** · 收尾章

---

## 本节讲什么

原书 §2 说理论不够用时，内核开发者靠三件套：**直觉（先做什么结构）、workload 模拟（验证算法）、管理员调优（sysctl/swap/overcommit/cgroup）**。

本节把这三件套**翻译成 2026 年的可执行工具链**——每一条都配具体命令，让你读完就能在交易主机 / 嵌入式板子上动手验证，而不是停留在「要测量」的口号。

---

## 1. 直觉（intuition）→ 建立心智模型

「直觉」不是玄学，是**对因果链的心智模型**：看到某个观测值，能反推是哪条链在动。

| 观测现象 | 心智模型指向 |
|----------|--------------|
| `kswapd` CPU 高 + `allocstall` 上升 | direct reclaim 介入，进程自己回收（Ch10） |
| `si/so`（swap in/out）非零 | 匿名页被换出，延迟会暴涨（Ch11） |
| `compact_stall` 高 | 高阶分配碎片化，compaction 频繁（Ch6/Ch7） |
| dmesg 出现 `Killed process ... oom_score_adj` | 已触发 OOM，看 score 判断谁被杀（Ch13） |
| `/proc/zoneinfo` 的 `pages_scanned` 涨 | 回收扫描在加压 |

**建立直觉的唯一路径**：读源码时**带着「这段代码在什么观测值上体现」的问题**，而不是只背函数名。本仓库 Ch1 的源码阅读路线（`oom_kill.c` → `vmalloc.c` → `page_alloc.c` → `mmap.c`）就是为此设计的。

## 2. 模拟（simulation）→ 可复现的量测

「模拟」的本质是**构造一个可控 workload，隔离单变量**。现代工具链：

| 工具 | 用途 | 典型命令 |
|------|------|----------|
| `usemem` / `stress-ng` | 制造内存压力 | `stress-ng --vm 4 --vm-bytes 80% --timeout 60s` |
| `mmtests`（内核社区标准） | 可复现 benchmark 套件 | Mel Gorman 本人在用，配置化跑多轮取中位数 |
| `damon` + `damo`（v5.15+） | 数据访问模式监测 | `damo record` / `damo report`，看真实热区 |
| `perf mem` / `perf c2c` | 内存访问延迟 / cacheline 竞争 | `perf c2c record` 找伪共享 |

**HFT 可复现量测范式**（本仓库反复强调的）：

```bash
# 延迟分位数：别只看平均
sudo perf stat -e page-faults,major-faults,kmem:mm_page_alloc \
     taskset -c 2 ./trading_bench

# 缺页来源拆分：minor（新映射）vs major（磁盘/swap 换入）
grep -E '^(Min|Maj|File|Anon)' /proc/$(pgrep trading_bench)/status

# 内存布局：NUMA 是否跨 node
numastat -p $(pgrep trading_bench)
```

**原则**：结论必须**能复现**（同一 workload 跑 N 次取分位数），而不是「某次跑出来 200µs」。

## 3. 调优（tuning）→ 分层旋钮清单

管理员能拧的旋钮，按本书章节归类：

| 章 | 旋钮 | 作用 | HFT 倾向 |
|:--:|------|------|----------|
| Ch2/Ch6 | `vm.zone_reclaim_mode`、`numactl --membind` | NUMA 放置 | 绑 node，避免跨 socket |
| Ch3 | `madvise(MADV_HUGEPAGE)`、`/sys/.../transparent_hugepage` | 大页 | 显式 madvise，别全局 always |
| Ch4 | `mlock`、`mlockall` | 锁 RSS | 关键映射锁死 |
| Ch10 | `vm.swappiness`、`vm.vfs_cache_pressure` | 回收倾向 | swapiness 调低（如 1） |
| Ch11 | 关 swap（`swapoff`）/ zram | 换出 | 交易主机常关 swap |
| Ch12 | tmpfs `size=` 挂载 | 共享内存 | 明确 size 上限 |
| Ch13 | `overcommit_memory=2`、`oom_score_adj` | OOM 防护 | 严格记账 + 交易进程 -1000 |

**调优的铁律**：**一次只改一个旋钮**，改完量测，确认方向对了再改下一个。多旋钮同时拧 = 把「模拟」的可控性彻底毁掉，回到「拍脑袋」。

---

## 4. HFT / 嵌入式关联

| 维度 | 落地 |
|------|------|
| 三件套的顺序 | **先直觉（建模型）→ 再模拟（验证）→ 最后调优（落地）**，别倒着来 |
| 延迟目标 | 用 `perf stat` + `/proc/status` 把「缺页/换入/分配」拆到具体事件，而非只看总延迟 |
| 嵌入式差异 | 板子上没有 `perf` 就用 `/proc/vmstat`、`/proc/zoneinfo`、`free`、`smaps` 这些**零依赖**的 proc 接口 |

**要点**：原书的三件套放到今天，本质是**「观测驱动」**——直觉给假设，模拟给证据，调优给手段，三者闭环。HFT 里最忌讳的就是「看了一篇讲最优替换的论文，直接改 swappiness」，那是跳过模拟、只有直觉的典型反模式。

---

## 衔接

§2 讲「怎么动手」。§3 回到本书本身的定位：Mel Gorman 到底想弥合哪条鸿沟。

---

<details>
<summary>自测 5 问（点开看答案）</summary>

**Q1：原书「直觉、模拟、调优」三件套对应今天的什么？**

直觉 = 建立「观测值 → 哪条链在动」的心智模型；模拟 = 用 usemem/stress-ng/mmtests/damon 构造可控 workload 验证；调优 = 分层旋钮（swappiness/overcommit/oom_score_adj/大页）逐个试。

**Q2：看到 `kswapd` CPU 高 + `allocstall` 上升，指向哪条链？**

direct reclaim 介入（Ch10）——进程在分配路径上自己触发回收，说明后台 kswapd 跟不上分配速度。

**Q3：minor fault 和 major fault 的区别？各自对应什么场景？**

minor = 页已在内存、只需建页表映射（新映射/COW）；major = 需要从磁盘或 swap 换入，会阻塞等待 I/O，是延迟尖刺的主要来源。

**Q4：HFT 里为什么「一次只改一个调优旋钮」是铁律？**

因为调优要配合「模拟」保持可控性，多旋钮同时拧会让观测结果无法归因，退化成拍脑袋。改一个 → 量测 → 确认方向 → 再改下一个。

**Q5：嵌入式板子上没有 perf 时，用哪些零依赖接口观测内存？**

`/proc/vmstat`、`/proc/zoneinfo`、`free`、`/proc/<pid>/smaps`、`/proc/<pid>/status`——都是内核直接暴露的 proc 接口，无需额外工具。

</details>
