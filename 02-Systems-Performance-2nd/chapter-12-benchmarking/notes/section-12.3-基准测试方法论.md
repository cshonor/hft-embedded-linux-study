## 12.3 基准测试方法论

### 被动 vs 主动基准测试

| 类型 | 做法 |
|------|------|
| **Passive** | 观察 **生产** 或 idle 系统 — 不施加人工负载 |
| **Active** | **施加** 负载（fio、压测客户端） |

**HFT：** 生产 **passive**（P99、BPF）为主；**active** 在 **staging / 裸机验收** — 勿在生产 peak 乱 fio。

### 结合观测（必做）

压测时 **同时**：

| 方法 | 确认什么 |
|------|----------|
| **USE** | 资源是否如预期饱和（Ch 2） |
| **CPU Profiling** | 热点是否在声称路径 |
| **Workload characterization** | IOPS/块大小/并发是否匹配设计 |

```
例：fio 报 500k IOPS，但 CPU 火焰图 80% 在 fio 自身
  → 测的是 fio 能力，不是「你的日志进程写盘能力」
```

→ Ch 4–6 工具 · Ch 13 perf

### 自定义基准（Custom Benchmarks）

当 fio/iperf **无法代表** 业务时 — **自己写**：

- 解码真实 FIX/SBE 报文 → 更新 book → 发单 stub
- 固定 seed、固定输入文件 — **可复现**

**HFT 最佳实践：** Micro 工具 baseline + **自定义 replay harness** 报 **端到端 span**。

→ [11-HFT ch10](../../../11-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测/)

### 阶梯式施加负载（Ramping Load）

```
并发 1 → 2 → 4 → 8 → … → 直到
  - 延迟急剧恶化（拐点）
  - 错误率上升
  - 吞吐平台
```

**找：** 可扩展性上限、排队开始的位置 — 与 Ch 2 排队论、Ch 10 backlog 呼应。

**HFT：** 行情 pps 阶梯 + 记录 **P99 tick latency** — 找 **单核/单 NUMA 上限**。

### 合理性检查（Sanity Check）

| 检查 | 做法 |
|------|------|
| 数量级 | 10G 网卡不应测出 1Tbps |
| 交叉工具 | fio vs dd vs 应用写 — 趋势一致 |
| 系统计数器 | `iostat`/`ip -s` 与 fio 报告 |
| 瓶颈位置 | profile 是否在预期层 |
| 可重复 | 连跑 3 次，方差可接受 |

### 统计分析（Statistical Analysis）

→ Ch 2 [统计与可视化](../../chapter-02-methodologies/)

| 要点 | 做法 |
|------|------|
| 别只报 mean | **P50/P99/P999**、直方图 |
| 离群值 | 单独分析，不默默删掉 |
| 样本量 | 足够长（覆盖 GC、cache 稳态） |
| 置信 | 多次运行、固定环境 |

### 基准测试检查清单（Benchmarking Checklist）

**测试前：**

- [ ] **目标** 一句话：测什么、成功标准是什么
- [ ] **Workload** 描述：读/写比、大小、并发、时长
- [ ] **环境** 文档：硬件、内核、驱动、邻居、挂载选项
- [ ] **生产相关性** 说明：为何此负载代表生产
- [ ] 观测工具就绪：perf、sar、sadc、BPF
- [ ] 基线：未调优 vs 调优后 **分开测**

**测试中：**

- [ ] **预热**（JIT、cache、连接）— 区分冷/热
- [ ] 同时 **USE + profile**
- [ ] 记录 **系统计数器**（非仅工具 stdout）
- [ ] **Ramping** 或至少多档并发

**测试后：**

- [ ] **Sanity check** 通过
- [ ] 报告 **分布** + 均值
- [ ] 结论 **限定范围** — 不外推
- [ ] 存档：命令行、配置、fio job 文件、内核版本

---


---

← [本章导读](../README.md)
