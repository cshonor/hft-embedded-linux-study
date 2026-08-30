# 7. BPF 工具：qdisc 排队（10.3.24–10.3.25）

> 底本：《BPF之巅》第 10 章 网络，10.3 节（印刷 p510–515）

覆盖：qdisc-latency（qdisc-fq）与 qdisc-* 变体家族。

## 7.1 qdisc-fq —— fq 队列延迟

```
$qdisc_latency:
     (us)
      0 -> 1    : 12   | |
      ...
```

- 跟踪 `fq_enqueue`（入队打点）与 `fq_dequeue`（出队算差），**sk_buff 地址为键**配对。
- 前提：`modprobe sch_fq` 且 qdisc 为 fq —— 未加载则无事件（工具无输出时先 `tc qdisc show` 确认）。

## 7.2 qdisc-* 变体家族（表 10-4 模式）

同一模板替换 `Qdisc_ops` 结构体的 **enqueue/dequeue 成员函数**即可适配不同 qdisc：

| 变体 | enqueue / dequeue | 适用场景 |
|---|---|---|
| qdisc-cbq | cbq_enqueue / cbq_dequeue | class-based 遗留 |
| qdisc-cbs | cbs_enqueue / ... | 时间敏感网络 TSN |
| qdisc-codel | codel_enqueue / ... | 抗 bufferbloat |
| qdisc-fq_codel | fq_codel_enqueue / ... | 默认派 |
| qdisc-red | red_enqueue / ... | 随机早期检测 |
| qdisc-tbf | tbf_enqueue / ... | 令牌桶限速 |

- **通用方法论**：`crash`/pahole/BTF 查 `struct Qdisc_ops` 成员名 → 套模板 → 编译即得新 qdisc 观测工具。

## 7.3 解读

- 延迟尖刺 = 队列堆积（shaping/bottleneck）；
- 恒为 0 = qdisc 空转（瓶颈在别处：驱动/物理链路）；
- HFT 出口整形（限速日志上传）时用于验证 tbf 令牌桶是否引入突发延迟。

## HFT 关联

- 共置机出口带宽受限时 tbf/fq 整形是常见做法——qdisc-tbf/qdisc-fq 直接量化"排队等待"这截延迟，避免日志/风控流量挤占行情回传。
- fq（含 EDT 模型，Linux 4.20+）是低延迟出口首选；观测工具与调度器配套选型。

<details>
<summary>自测题</summary>

1. qdisc-fq 用什么键配对入队/出队？前提条件是什么？
2. 如何给一个书中没有的新 qdisc 写观测工具？
3. qdisc 延迟恒为 0 说明什么？
</details>
