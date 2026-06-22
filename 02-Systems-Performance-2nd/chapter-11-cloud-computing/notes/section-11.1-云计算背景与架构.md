## 11.1 云计算背景与架构

### 水平扩展（Horizontal Scalability）

| 模式 | 做法 | 典型组件 |
|------|------|----------|
| **垂直扩展** | 单机更大 CPU/内存 | 传统大型机、裸机 scale-up |
| **水平扩展** | 更多实例分担负载 | LB + 无状态 app 集群 + 分片存储 |

```
                    Load Balancer
                   /      |      \
              Web/App   Web/App   Web/App
                   \      |      /
                  Sharded / Cloud-native DB
```

**HFT 对比：**

- **tick 热路径**：垂直扩展 + **绑核裸机** — 不靠水平复制同一策略实例（状态难拆）。
- **可水平部分**：行情 fan-out、回测 worker、监控、研究 notebook — 适合云。

### 容量规划与动态缩放

| 机制 | 好处 | 风险 |
|------|------|------|
| **按需计费** | 用多少付多少 | 忘记关机 → 账单 |
| **Auto Scaling** | 负载涨 → 加实例 | 缩放滞后、冷实例 |
| **Bursting** | 短时超配 CPU credits | credits 耗尽 → **性能 cliff** |

**监控：** 不仅看 CPU%，还要看 **P99 延迟、throttle、成本/请求** — 防 overprovisioning。

### 多租户与编排（Kubernetes）

| 概念 | 性能影响 |
|------|----------|
| **Multi-tenancy** | 共享物理机 → **noisy neighbor** |
| **Kubernetes Pod** | 调度、CNI 网络、overlay 增 hop |
| **CNI** | VXLAN/iptables/eBPF — 吞吐与延迟特征不同 |

**HFT：** 生产策略 **避免** 与未知负载同节点；若 K8s 跑 **非热路径**（/grafana/批任务），需 **Dedicated node pool + tolerations**。

→ Ch 10 [网络](../../chapter-10-network/) · Ch 6/7 [cgroups](../../chapter-06-cpus/)

---


---

← [本章导读](../README.md)
