# 13.8 `perf stat` — 事件计数

> [章节导航](../README.md) · 上一节：[13.3–13.7 perf 事件源](./section-13.3-13.7-perf-事件源.md) · 下一节：[13.9 perf record](./section-13.9-perf-record-剖析采样.md)

## 本节讲什么

perf stat 是**计数流**的全貌。原书给的是选项表——这里补上三块机制理解：

1. 内核里 `read(fd)` 到底读了什么（`perf_event_read`，v6.6 core.c:4593）
2. **Shadow statistics**——IPC、GHz、miss 率这些"衍生指标"是 perf **用户态算的**，不是硬件直接给
3. multiplexing 下的数字可信度边界（接上节）

---

## 1. 机制：计数态和采样态用的是同一个 fd

`perf stat ./strategy` 的内核侧路径：

```
perf_event_open(attr.disabled=0)
   → PMU add/start（硬件计数器开始自增）
运行期间：硬件自由计数，内核零介入（软件事件在埋点点计数）
结束时：read(fd) → perf_event_read()（core.c:4593）
   → 读 PMC 现值 + time_enabled/time_running → 64bit 计数返回用户态
```

**要点**：

- 计数态**没有中断、没有 buffer**——硬件计数器自己加，内核只在 read 时介入一次。这是 stat 开销近零的根源（对照 record：溢出中断 + 每样本写 buffer）。
- 读回的不只是一个数，是一组：`{count, time_enabled, time_running}`（multiplexing 外推用）+ 事件 ID。`perf_event_read` 在跨核事件（CPU 迁移过的任务）上还要 IPI 到事件所在的 CPU 读现场（core.c:4593 的 task context 同步逻辑）。

## 2. 常用选项（按用途分组）

| 组 | 选项 | 作用 |
|---|---|---|
| 定时 | `-I 1000` | 每 1s 打一行——**趋势序列**（看波动/双峰） |
| 定位 | `-A` / `--no-aggr` | 每 CPU 一行——热核审计 |
| 域 | `-a` / `-p PID` / `-t TID` | 全系统 / 进程 / 线程 |
| 态 | `-u` / `-k` | 只用户态 / 只内核态（`exclude_kernel/exclude_user` attr 位） |
| 稳 | `-r 5` | 重复 5 次给均值±方差 |
| 增 | `-d` / `-dd` | 加任务时钟/IPC/前端后端（Top-down 雏形） |
| 控 | `--delay ms` / `--pre`/`--post` | 跳过 warmup；前后钩子 |

```bash
# 每 CPU 每秒 IPC 趋势（热核审计）
perf stat -e cycles,instructions -I 1000 -a -- sleep 5

# 只看用户态（策略/解码代码）
perf stat -e cycles,instructions -u -p $(pidof strategy) -- sleep 10

# 前后对比（A/B 实验纪律）
perf stat -r 5 -e cycles,instructions,LLC-load-misses ./strategy_old > old.txt
perf stat -r 5 -e cycles,instructions,LLC-load-misses ./strategy_new > new.txt
```

## 3. ⭐ Shadow Statistics——衍生指标从哪来

perf stat 输出里 `insn per cycle`、`GHz`、`% of all branches` 这几列不是硬件给的——是 perf **用户态**用同轮事件相除算出的"影子统计"：

| 衍生指标 | 计算 |
|---|---|
| IPC | `instructions / cycles` |
| GHz | `cycles / task-clock` |
| cache miss 率 | `cache-misses / cache-references` |
| branch miss 率 | `branch-misses / branches` |

**为什么重要**：① multiplexing 时两组外推值相除，**误差放大**——比单事件更不可信；② `-e` 自选事件集时影子列会消失（perf 不知道你想要衍生指标）——想要 IPC 列就别把默认事件集拆散，或手动除。

判读方法（IPC 四象限、GHz 先行纪律、组合阅读矩阵）在 [ch16.1.5](../../chapter-16-case-studies/notes/section-16.1.5-16.1.6-PMC-与软件事件.md)——本节管"数字从哪来"，那边管"数字怎么读"。

## 4. multiplexing 数字的正确姿势（接 13.3–13.7）

| 规则 | 理由 |
|---|---|
| 结论级数据 `--no-multiplex` 分轮跑 | 外推值的误差不该进报告 |
| 趋势观察（-I 序列）可用 mux 值 | 单事件自身的时间序列仍自洽 |
| 比值类指标（IPC/miss率）看占比一致性 | 两事件若不在同一轮换组，比值无意义 |

## 5. `perf stat` 输出精读模板

```
 Performance counter stats for process id '4242':

      12,345,678,901      cycles                    #    3.201 GHz
      15,678,901,234      instructions              #    1.27  insn per cycle
                                                    #    0.65  stalled cycles per insn
         1,234,567,890      cache-references
           123,456,789      cache-misses              #   10.00 % of all cache refs
       ...                    (68.23%)
             3.856223456 seconds time elapsed
```

| 位置 | 判读 |
|---|---|
| `(68.23%)` | 该事件被 mux 轮换，实际计数时间 68%——数字是外推值 |
| `insn per cycle` | 影子统计；对照 [IPC 四象限](../../chapter-16-case-studies/notes/section-16.1.5-16.1.6-PMC-与软件事件.md)：低 IPC + 高 cache-miss → 内存墙；低 IPC + 低 miss → 停顿/依赖链 |
| `stalled cycles per insn` | x86 老事件；新平台用 `-dd` 看 Top-down 四分类 |
| `seconds time elapsed` | 与 task-clock 差异大 → 时间花在 off-CPU（转 [offcputime 路线](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md)） |

---

## HFT / 嵌入式关联

| 场景 | 用法 |
|---|---|
| **验收基线** | 绑核/调优/换内核参数前后，固定事件集 + `-r 5` 存档 diff——[ch16.1.4 配置审计](../../chapter-16-case-studies/notes/section-16.1.3-16.1.4-统计数据与静态配置.md)的"前后对照"环节 |
| 热核审计 | `-A -I 1000`：隔离核上的 IPC 应显著高于普通核；突然拉平 = 隔离失效或迁核 |
| 用户/内核分账 | `-u`/`-k` 各跑一轮：策略慢（用户态）还是协议栈慢（内核态）一刀分开 |
| 生产常驻 | stat 是唯一可长跑的深检——[ch16 S0-S4 演练](../../chapter-16-case-studies/notes/section-16.9-HFT-版Unexplained-Win演练模板.md)的 S2 阶段主力 |
| Pi5 | 无 PEBS/SPE，但计数流完整——eBPF 实验前先 stat 建立基线 |

---

## 衔接

- 上一节：[事件源与 multiplexing](./section-13.3-13.7-perf-事件源.md)
- 下一节：[13.9 perf record——采样流与 ring buffer](./section-13.9-perf-record-剖析采样.md)
- 数字判读：[ch16.1.5 PMC 精读](../../chapter-16-case-studies/notes/section-16.1.5-16.1.6-PMC-与软件事件.md)
- 缺页计数延伸：[Ch 7 内存](../../chapter-07-memory/) · [06-linux-mm](../../../06-linux-mm/)

---

## 代码自测

<details><summary>Q1：perf stat 为什么开销近零？和 record 的本质区别？</summary>

计数态没有中断和 buffer：硬件 PMC 自增，内核只在结束时 read(fd) 介入一次。record 是采样态——PMC 溢出中断每样本触发，内核写 ring buffer，用户态定期收割。stat 是"读表"，record 是"装监控"。
</details>

<details><summary>Q2：IPC 这一列是谁算的？multiplexing 下它还可信吗？</summary>

perf 用户态影子统计：instructions/cycles 相除。mux 下两事件若在不同轮换组，各自外推误差再相除放大——结论级数据应 `--no-multiplex` 分轮采集。
</details>

<details><summary>Q3：<code>-I 1000</code> 和 <code>-r 5</code> 各解决什么问题？</summary>

`-I 1000` 给时间序列（看波动/双峰/周期性），`-r 5` 给重复样本（看方差/置信度）。前者发现"什么时候异常"，后者确认"差异是否显著"。
</details>

<details><summary>Q4：elapsed 和 task-clock 差很多说明什么？下一步查什么？</summary>

进程大量时间不在 CPU 上（等锁/IO/调度）。on-CPU 工具到此为止——转 off-CPU 路线：offcputime/eBPF（[ch16.1.7](../../chapter-16-case-studies/notes/section-16.1.7-16.1.8-动态追踪与结论.md)）。
</details>

<details><summary>Q5：为什么任务迁移过的进程 read 计数要 IPI？</summary>

事件绑定在"打开时的上下文"（task context），迁移后计数现场在别的 CPU。`perf_event_read`（core.c:4593）需 IPI 到事件当前所在 CPU 读取 PMC 现场保证一致性——这也是 `cpu-migrations` 软件事件存在的原因：迁移本身有观测成本。
</details>
