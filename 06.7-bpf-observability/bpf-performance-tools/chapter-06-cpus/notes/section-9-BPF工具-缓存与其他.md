# 6.3 BPF 工具（七）：缓存与其他 — llcstat / cpuwalk / cpuunclaimed / loads / vltrace

> 底本：《BPF之巅》第 6 章 CPU，6.3.16–6.3.17 节（印刷 p250–251）。

## 6.3.16 llcstat — LLC 命中率（PMC）

BCC 工具，用 PMC（性能监控计数器）按进程输出**末级缓存（LLC）命中率** — 微架构层面的 CPU 效率。

```bash
# llcstat        # 默认跑 10 秒
PID     NAME        CPU   MISS   REFERENCE  HIT%
4435    java         18   22000  200        99.09%
4116    java         38   32200  300        99.07%
```

- **第一个使用 PMC 的 BCC 工具**（Teng Qin，2016）— BPF 与硬件计数器结合的起点
- 原理：PMC **溢出采样** — 缓存命中/未命中事件每 N 次触发一次 BPF 程序，记录当前进程并累计
- 默认采样率 1/100（`-c SAMPLEPERIOD` 可调）→ 开销低
- **采样偏差提醒**：书例中出现"未命中计数 > 查找计数"的矛盾 — 两个计数器独立采样、各自近似，可能不自洽；看趋势别抠绝对值

## 6.3.17 其他工具

| 工具 | 来源 | 功能 |
|------|------|------|
| **cpuwalk** | bpftrace | 采样每个 CPU 上运行的进程名，线性直方图 — 统计 CPU 间负载均衡 |
| **cpuunclaimed** | BCC | 采样"某 CPU 有排队线程时其他 CPU 空闲"的比例 — 偶发是 CPU 亲和黏合，频发是调度器配置错误或 Bug |
| **loads** | bpftrace | 演示如何用 BPF 算负载平均值（作者提醒：这数字本身很有误导性） |
| **vltrace** | Intel | 基于 BPF 的 strace 替代品，分析消耗 CPU 的系统调用 |

cpuunclaimed 值得展开：它度量的是"**理论上能并行却没并行**"的浪费 — 队列里有活干、别的核却闲着。对绑核部署（HFT、专机专用）是直接的健康指标。

## HFT 关联

- llcstat 验证策略热数据的缓存驻留：HIT% 从 99% 掉到 95% 意味着关键路径多了数百 ns 的内存访问 — 数据结构尺寸逼近 LLC 边界的信号（配合 17-computer-architecture 的缓存理论）
- cpuunclaimed 与绑核矛盾场景：策略核排队 + 闲核并存 = 亲和设置把负载圈死了，要么扩容绑定集要么调整线程数
- 云主机注意：PMC 不透传时 llcstat 不可用（同 perf stat PMC 全 0）

## 常见陷阱

1. **llcstat 的 MISS 与 REFERENCE 不自洽就慌** — 两个 PMC 独立采样，允许小概率矛盾；关注命中率趋势
2. **提高 llcstat 采样率求精确** — 采样率越高 BPF 触发越频繁，开销上升；默认 1/100 足够看趋势
3. **loads 算出来的负载值当 KPI** — 作者原话：负载平均值本身有误导性（含 D 状态、指数衰减模糊），别当精确指标

<details>
<summary>📝 自测题（点击展开）</summary>

1. **llcstat 的工作原理是什么？为什么说它是"第一个"？**

   <details>
   <summary>参考答案</summary>

   PMC 溢出采样：设置 LLC 命中/未命中计数器每 N（默认 100）次溢出一次，每次溢出触发 BPF 程序记录当前进程并累计，最后算 HIT%。它是第一个把 PMC 硬件事件接入 BPF 的 BCC 工具，打开了 BPF 微架构观测（内存 stalls、分支预测等）的方向。
   </details>

2. **cpuunclaimed 检测什么问题？偶发和频发分别说明什么？**

   <details>
   <summary>参考答案</summary>

   检测"有 CPU 排队的同时存在空闲 CPU"。偶发：CPU 亲和/黏合度导致的正常现象（线程不愿迁移丢缓存）；频发：调度器配置错误或 Bug（负载均衡没起作用），白白浪费算力。
   </details>

</details>
