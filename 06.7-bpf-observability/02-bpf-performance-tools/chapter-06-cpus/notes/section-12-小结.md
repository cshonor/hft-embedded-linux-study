# 6.6 小结

> 底本：《BPF之巅》第 6 章 CPU，6.6 节（印刷 p254）。

## 原书小结

本章介绍了 CPU 资源使用分析：

- **传统工具**三类手段：统计数据、性能分析器（剖析器）、跟踪器
- **BPF 工具**解决的具体问题：
  - **短期进程**发现（execsnoop / exitsnoop）
  - **运行队列延迟**详细分析（runqlat / runqlen / runqslower）
  - **CPU 使用效率**剖析（cpudist / cpufreq / profile / offcputime）
  - **函数调用频率**统计（funccount / syscount / argdist / trace）
  - **软中断 / 硬中断** CPU 用量（softirqs / hardirqs / smpcalls）
  - **缓存命中率**（llcstat，PMC × BPF）

## 本章工具 × 问题速查

| 问题 | 工具 | 开销 |
|------|------|------|
| 短命进程在哪 | execsnoop / exitsnoop | 可忽略 |
| CPU 排队多严重 | runqlat（量化）/ runqlen（常驻） | 事件跟踪 vs 99Hz 采样 |
| 谁受害了 | runqslower | 事件跟踪，短期 |
| 在核上跑什么 | profile + 火焰图 | 可忽略（采样） |
| 离核在等什么 | offcputime + off-CPU 火焰图 | 可能 >10%，短期 |
| 哪个系统调用高频 | syscount | ~0.1% |
| 函数慢还是调用勤 | funccount | 与频率成正比 |
| 中断吃掉多少 | softirqs / hardirqs | 与频率相关 |
| 核间打断 | smpcalls | kprobe |
| 缓存效率 | llcstat | 1% 采样 |

## 开销原则（贯穿全章的纪律）

- **定时采样**（profile / runqlen / cpufreq / llcstat）≈ 零开销 → 可常驻
- **事件跟踪**（runqlat / cpudist / offcputime / funccount）与事件频率成正比 → 繁忙系统短期运行、-p 限定范围
- 开销估算公式：额外开销 ≈ 每事件处理耗时 × 事件频率 ÷ CPU 数（书例：1µs × 100 万次/s ÷ 10 CPU = 10%）

公式的工作示例（把"纪律"变成"算术"）——一台 8 核交易机，评估三个候选工具：

| 工具 | 每事件成本 | 事件频率 | 总开销（÷8 核） | 结论 |
|------|-----------|---------|----------------|------|
| runqlat（sched_wakeup+switch） | ~1µs | 切换 2 万次/s | 0.25% | 可放心跑半天 |
| offcputime（同上+栈抓取） | ~5µs | 2 万次/s | 1.25% | 排障期短跑 |
| memleak 类逐 malloc | ~1µs | 200 万次/s | 25% | 绝不生产（ch7） |

数字本身不用背——要背的是**先算再挂**的习惯：任何工具上线前过一遍这三个变量。

## 60 秒 CPU 检查（小结的操作化）

```
uptime                        # 1. 负载 vs 核数（粗筛）
mpstat -P ALL 1 5             # 2. 单核饱和？idle 分布
runqlat.bt（60s 自动退出）     # 3. 排队延迟分布——右尾是重点
profile -af 60 > out.svg      # 4. on-CPU 火焰图（可疑时）
```

第 3 步右尾异常才进第 4 步——顺序本身就是开销纪律：便宜的先跑。

## HFT 收尾 Checklist

- [ ] runqlat：策略核延迟分布近全在 0–15µs 桶
- [ ] profile + offcputime：on/off-CPU 火焰图成对留档，事故可对比
- [ ] execsnoop：交易机无脚本风暴（无高频 cron/shell 循环）
- [ ] hardirqs：网卡中断分布与 IRQ affinity 设置一致，不落策略核
- [ ] smpcalls：无高频 do_flush_tlb_all / 无用 /proc 监控读取
- [ ] cpufreq 无输出（= performance governor 频率恒定）
- [ ] 事件跟踪类工具（runqlat/offcputime/funccount）绝不 7×24 常驻

## 相关章节

- 调度器原理深入：[chapter-05-linux-kernel](../../../../05-linux-kernel/)（调度子系统）
- 缓存/微架构理论：[15-computer-architecture](../../../../15-computer-architecture/) · [02-CSAPP Ch6](../../../../02-computer-systems/chapter-06-memory-hierarchy/)
- SysPerf 对照：[06.6-systems-performance/chapter-06-cpus](../../../../06.6-systems-performance/chapter-06-cpus/)
- 下一章内存：[chapter-07-memory](../../chapter-07-memory/)
