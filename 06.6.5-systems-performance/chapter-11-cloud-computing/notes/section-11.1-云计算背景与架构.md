## 11.1 云计算背景与架构

> 章节导航：[本章导读](../README.md) · 下一篇 [11.2 硬件虚拟化](./section-11.2-硬件虚拟化Hardware-Virtualization.md)

**本节讲什么**：云的性能模型——水平/垂直扩展的权衡、容量规划与动态缩放的陷阱（bursting cliff）、多租户与 K8s 编排的性能代价，以及 HFT 各组件的云/裸机选型原则。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | 云的经济学 = **资源共享** | 共享 = noisy neighbor 的根源 |
| 2 | 水平扩展只对**无状态**友好 | 有状态热路径靠垂直扩展 |
| 3 | **Bursting 是性能 cliff** 不是平滑加成 | credits 耗尽瞬间跌落 |
| 4 | K8s 的每层抽象都有**延迟价格** | overlay 网络 +1 hop |
| 5 | HFT 的正确拆分 | 热路径裸机、周边上云 |

---

### 一、水平扩展 vs 垂直扩展

| 模式 | 做法 | 典型组件 | 适用 |
|------|------|----------|------|
| **垂直扩展** | 单机更大 CPU/内存 | 传统大型机、裸机 scale-up | 有状态、低延迟、难拆分 |
| **水平扩展** | 更多实例分担负载 | LB + 无状态 app 集群 + 分片存储 | 无状态、高吞吐、可复制 |

```
                    Load Balancer
                   /      |      \
              Web/App   Web/App   Web/App      ← 无状态层：随便复制
                   \      |      /
                  Sharded / Cloud-native DB     ← 有状态层：分片是难点
```

**水平扩展的前提是状态可拆**：会话无粘性（或集中存储）、数据可分片、结果可合并。云原生技术栈（LB/分片存储/auto scaling）全部围绕这个前提构建。

**HFT 对比**：

| 组件 | 扩展方式 | 原因 |
|------|---------|------|
| **tick 热路径** | 垂直扩展 + **绑核裸机** | 策略状态（order book、持仓）强一致，拆分代价 > 复制代价；微秒级延迟不容许任何间接层 |
| 行情 fan-out | 水平 | 发布订阅天然无状态 |
| 回测 worker | 水平 | embarrassingly parallel |
| 监控/研究 notebook | 水平 | 非关键路径 |

### 二、容量规划与动态缩放的三个陷阱

| 机制 | 好处 | 风险 | 表现特征 |
|------|------|------|---------|
| **按需计费** | 用多少付多少 | 忘记关机 → 账单 | 成本侧 |
| **Auto Scaling** | 负载涨 → 加实例 | 缩放滞后、冷实例 | 流量突增的前几分钟延迟尖刺 |
| **Bursting** | 短时超配 CPU credits | credits 耗尽 → **性能 cliff** | 突然跌到 baseline 的 10-20% |

**⭐ Bursting cliff 的机制**（t 系列 CPU credits 模型）：

```
CPU 性能
  ▲ 突发区（花 credits）
  │ ────────────────
  │                 ╲  credits 耗尽
  │                  ╲ ← cliff：不是渐降，是瞬间跌到 baseline
  │  baseline ────────┴────────
  └──────────────────────────▶ 时间
```

对延迟敏感服务这是灾难：前 30 分钟 P99 正常，credits 耗尽后延迟直接翻 5-10×，且告警往往只盯 CPU%（还是满的）——**监控要看 throttle/credits 余量**，不只看 CPU%。

**容量监控的正确口径**：CPU% 之外，加 P99 延迟、throttle 指标、成本/请求——防止 overprovisioning（为峰值 1% 付全天费用）和 underprovisioning（cliff）。

### 三、多租户与 Kubernetes 的性能价格

| 概念 | 性能影响 | 机制 |
|------|----------|------|
| **Multi-tenancy** | 共享物理机 → **noisy neighbor** | 争 CPU（steal）、LLC、内存带宽、磁盘队列（[ch11.2](./section-11.2-硬件虚拟化Hardware-Virtualization.md)） |
| **K8s Pod 调度** | pod 可能被驱逐/迁移 | 调度器不懂你的延迟 SLA |
| **CNI overlay** | VXLAN/IPIP 封装 +1 hop | 每包额外封装/解封装 + MTU 缩水 |
| CNI 实现差异 | VXLAN/iptables/eBPF 各不同 | iptables 规则链随 service 数线性增长；eBPF 最短路径 |
| **requests/limits** | limit 触发 **CPU throttle** | cgroup quota 机制（[11.3](./section-11.3-操作系统虚拟化-容器.md)） |

**K8s 网络的延迟叠加**（同节点 pod 间通信为例）：

```
直接裸机：app A → loopback/veth → app B        ~ 1-5µs
K8s 同节点：A → veth pair → bridge → veth → B   ~ 10-50µs
跨节点 + VXLAN：+ 封装 → 物理网 → 解封装         ~ 50-200µs + 抖动
```

对 HFT 的意义：跨节点 overlay 的抖动是结构性不可控的——**热路径组件要么同节点要么裸机直连**。

### 四、HFT 的云/裸机选型决策表

| 组件 | 部署 | 理由 |
|------|------|------|
| tick 引擎 + 发单 | **裸金属 / dedicated host** | 微秒 SLA + 物理隔离 |
| 行情落盘/回放 | 裸机（本地 NVMe） | 盘 I/O 直控 |
| 回测集群 | 云 spot/preemptible | 批处理，中断可容忍 |
| 监控/面板 | 云 | 非关键路径 |
| 灾备/研发 | 云 | 弹性 + 成本 |
| 若必须云上跑延迟敏感组件 | **裸金属实例 + SR-IOV + 增强网络** | 见 [11.2](./section-11.2-硬件虚拟化Hardware-Virtualization.md) |

**若用 K8s 跑非热路径**：dedicated node pool + tolerations，把监控/批任务和策略周边隔开——不是防延迟，是防资源争抢和调度驱逐。

### 衔接

- 下一节：[11.2 硬件虚拟化](./section-11.2-硬件虚拟化Hardware-Virtualization.md)（VM-EXIT/EPT/直通机制）
- 关联：[ch10 网络](../../chapter-10-network/)（overlay 的网络侧）、[ch6 cgroups](../../chapter-06-cpus/)（throttle 机制）、[ch12](../../chapter-12-benchmarking/)（云上基准的邻居噪声）

---

### 常见陷阱

1. **以为 bursting 是平滑的弹性**——credits 模型是 cliff 不是斜坡，耗尽瞬间跌到 baseline。
2. **K8s requests/limits 不看 throttle**——CPU% 满不等于没被限流，`nr_throttled` 才是证据。
3. **延迟敏感服务放 overlay 网络**——每包 +1 hop 封装，抖动结构性不可控。
4. **有状态热路径硬拆水平**——策略状态的强一致拆分代价远超垂直 scale-up。

<details>
<summary>自测题（点击展开）</summary>

1. 水平扩展的前提是什么？
   <details><summary>答</summary>状态可拆：会话无粘性、数据可分片、结果可合并。云原生栈全部围绕这个前提。</details>
2. bursting 的性能 cliff 为什么监控 CPU% 发现不了？
   <details><summary>答</summary>credits 耗尽后 vCPU 仍显示高占用（在 baseline 频率上跑满）——要看 credits 余量和实际吞吐/P99。</details>
3. K8s overlay 网络的延迟代价从哪来？
   <details><summary>答</summary>VXLAN/IPIP 封装解封装 + 额外网桥 hop + MTU 缩水；iptables 型 CNI 还有规则链增长问题。</details>
4. HFT 哪些组件适合上云？
   <details><summary>答</summary>回测 worker、监控面板、研究环境、灾备——无状态、可中断、非微秒敏感的周边。</details>

</details>


---

← [本章导读](../README.md)
