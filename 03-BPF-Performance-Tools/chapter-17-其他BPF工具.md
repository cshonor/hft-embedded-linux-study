# Ch 17 其他 BPF 性能工具 · Other BPF Performance Tools

> **BPF Performance Tools** · Brendan Gregg · **选读 🟡**

> 本章定位：**BPF 之上的可观测性生态** — Ch 4–16 以 **BCC/bpftrace CLI** 为主；本章介绍 **Vector/PCP、Grafana、Prometheus、kubectl-trace** 及 Cilium/Sysdig 等，把 BPF 指标 **GUI 化、持久化、集群化**。  
> **HFT：** **NOC/运维大屏** 可用 **热力图** 看 `runqlat`/`biolatency` 长尾；**交易机本身** 仍 **短窗口 CLI** 为主，长期 exporter 需 **限指标、限频率**。K8s 辅助服务用 **kubectl-trace**。  
> **上一章：** [chapter-16-虚拟机管理程序.md](./chapter-16-虚拟机管理程序.md) · **下一章：** [chapter-18-技巧与常见问题.md](./chapter-18-技巧与常见问题.md)

---

## 1. 从 CLI 到平台

| 层次 | 代表 | 适用 |
|------|------|------|
| **CLI 诊断** | BCC、bpftrace（Ch 4–5） | **Incident 深潜、短窗口** |
| **主机 GUI** | **Vector + PCP** | 单机实时热力图 |
| **企业面板** | **Grafana + PCP/Prometheus** | 历史趋势、告警 |
| **K8s 集群** | **kubectl-trace**、Cilium | 节点/Pod 级追踪 |

```
bpftrace/bcc（点查）
        ↓ 集成
PCP / ebpf_exporter → Grafana / Prometheus
        ↓
告警 · 热力图 · 集群 kubectl-trace
```

---

## 2. Vector 与 Performance Co-Pilot (PCP)

### Netflix Vector

| 属性 | 说明 |
|------|------|
| **类型** | 开源 **Web** 主机性能监控 |
| **特点** | **近实时**、高分辨率指标 |
| **底层** | **PCP (Performance Co-Pilot)** 收集框架 |

### BCC PMDA

通过 **BCC PMDA**（Performance Metrics Domain Agent）插件，PCP 在目标主机上 **执行 BCC BPF 程序** 并暴露为指标 — Vector 读取展示。

### 可视化方式

| 形式 | 适合 BPF 输出 | 示例指标 |
|------|---------------|----------|
| **热力图 (Heat Maps)** | **延迟直方图** 随时间 | `biolatency`、`runqlat` |
| **表格 (Tabular)** | **单次事件** 日志 | `execsnoop`、`tcplife` 会话行 |

**相对 CLI 优势：** 直方图 **长尾随时间** 一眼可见 — 比终端滚动 `runqlat` 更适合 **「何时开始抖」**。

```text
# 概念路径（部署因环境而异）
Host: bcc PMDA → pmcd → Vector Web UI
```

**HFT：** 共置机 **监控跳板** 或 **非 tick 管理机** 上跑 Vector；**勿在最低延迟核机器常开全量 PMDA**。

→ PCP 传统指标与 SysPerf 方法论：[02-SysPerf](../02-Systems-Performance-2nd/)

---

## 3. Grafana 与 PCP

**Grafana** — 主流开源 **Dashboard** 工具。

### 两种 PCP 数据源模式

| 模式 | 插件/路径 | 特点 |
|------|-----------|------|
| **实时 (live)** | **grafana-pcp-live** | 轮询 PCP **最新** 指标；浏览器保留短历史 |
| **历史 (redis)** | **grafana-pcp-redis** | PCP → **Redis** 持久化 — 长期趋势 |

| live | redis |
|------|-------|
| **无人看时开销低** | 适合 **容量规划、回归对比** |
| 适合 **当前主机 deep dive** | 存历史 baseline |

### 面板示例

Grafana **Heatmap** 面板展示 **`bcc.runq.latency`** 等 — 与 Vector 热力图同族。

**HFT：** Grafana 存 **runqlat P99 桶、tcpretrans 计数** 等 **少量 SLI** — 非全量 BCC 工具集。

---

## 4. Cloudflare eBPF Prometheus 导出器

### Prometheus 生态

| 组件 | 角色 |
|------|------|
| **Prometheus** | 拉取 (pull) 指标、存储、告警 |
| **Grafana** | 查询 Prometheus 画图 |
| **K8s** | ServiceMonitor 等原生集成 |

### `ebpf_exporter`

[Cloudflare ebpf_exporter](https://github.com/cloudflare/ebpf_exporter) — 将 **eBPF 程序** 采集的数据转为 **Prometheus 格式** 暴露 `/metrics`。

| 优点 | 注意 |
|------|------|
| 标准 **PromQL**、Alertmanager | Exporter 本身也跑 BPF — **需控频率** |
| 与 K8s/Grafana 栈统一 | 指标设计要 **聚合**，非 per-event |

**HFT 示例指标（概念）：**

- `runqlat` 直方图 bucket → histogram metric  
- `tcpretrans` counter  
- 与 **应用 tick-to-trade histogram** 同屏 — 区分 **内核 vs 策略**

---

## 5. kubectl-trace

| 问题 | 解决 |
|------|------|
| BPF 习惯 **SSH 单机** | K8s 多节点 **不便** |
| **kubectl-trace** | 在 **集群节点/Pod/Container** 上跑 **bpftrace 脚本** |

```bash
# 示意 — 以项目文档为准
kubectl trace run node/<node-ip> -f vfsstat.bt
kubectl trace run pod/<pod-name> -f tcpretrans.bt
```

| 目标 | 用途 |
|------|------|
| **Node** | 宿主机级（同 Ch 15 Host 视角） |
| **Pod/Container** | 针对 workload — 仍受 **权限/namespace** 约束 |

**HFT：** **K8s 上的风控/网关 Pod** incident — 无需 SSH 逐节点；**策略裸金属** 不用。

→ [Ch 15 容器](./chapter-15-容器.md) · [Ch 5 bpftrace](./chapter-05-bpftrace.md)

---

## 6. 其他知名 BPF 生态项目

| 项目 | 定位 | HFT 关联 |
|------|------|----------|
| **Cilium** | K8s **BPF 网络 + 安全策略** | 容器集群 CNI；非裸金属 tick 路径 |
| **Sysdig** | 容器 **可观测性**（BPF 扩展） | 商业/开源混合 — 与 CLI BCC 互补 |
| **Android eBPF** | 移动端网络监控 | 本书外 |
| **osquery + eBPF** | 主机分析、kprobe 监控 | 安全/资产 — 偏 [Ch 11](./chapter-11-安全.md) |
| **ply** | 类 bpftrace，**轻依赖**（无 LLVM/Clang） | **嵌入式** — 资源受限设备 |

### Cilium（略深）

- **eBPF 数据平面** — 替代 iptables 部分路径  
- **NetworkPolicy** — L3/L4/L7  
- **HFT：** 若交易 **不在 K8s Cilium 数据面**，仅作 **共置服务** 网络策略参考

### ply

| vs bpftrace | ply |
|-------------|-----|
| 依赖 LLVM/Clang | **更小依赖** |
| 功能/生态 | 较新、较轻 |

---

## 7. 架构选型速查

| 需求 | 推荐栈 |
|------|--------|
| Incident 5 分钟点查 | **bcc/bpftrace CLI**（Ch 3–16） |
| 单机实时热力图 | **Vector + PCP + BCC PMDA** |
| 团队 Dashboard + 历史 | **Grafana + PCP-redis** 或 **Prometheus + ebpf_exporter** |
| K8s 节点跑 bpftrace | **kubectl-trace** |
| K8s 网络策略 | **Cilium** |
| 嵌入式 | **ply** |

---

## 8. HFT 读者 Takeaway

1. **CLI 仍是 incident 第一选择** — 平台层 **不能替代** `runqlat`/`tcpretrans` 手跑。
2. **热力图价值** — `runqlat`/`biolatency` **长尾随时间** — 适合 **开盘/roll** 窗口回顾，非 tick 核常驻。
3. **Prometheus/Grafana** — 存 **少量 SLI**（重传率、runq P99、steal%）；避免 **全量 biosnoop** 式 exporter。
4. **ebpf_exporter** — 与现有 **K8s 监控栈** 统一；Cloudflare 实践可借鉴 metric 设计。
5. **kubectl-trace** — 仅 **容器化辅助服务**；裸金属 SSH + bcc 更简单。
6. **Cilium/Sysdig** — 了解即可；**14-DPDK** 热路径不在此栈。
7. 避坑与开销 → **下一章 Ch 18** 压轴。

---

## 相关章节

- 上一章：[chapter-16-虚拟机管理程序.md](./chapter-16-虚拟机管理程序.md)
- 下一章：[chapter-18-技巧与常见问题.md](./chapter-18-技巧与常见问题.md)
- BCC 工具源：[chapter-04-BCC.md](./chapter-04-BCC.md)
- bpftrace：[chapter-05-bpftrace.md](./chapter-05-bpftrace.md)
- 容器/K8s：[chapter-15-容器.md](./chapter-15-容器.md)
- SysPerf 监控方法论：[chapter-02-methodologies](../02-Systems-Performance-2nd/chapter-02-methodologies/)
- HFT 工程监控：[chapter-10-延迟测量与基准压测](../15-HFT-Low-Latency-Practice/chapter-10-延迟测量与基准压测.md)
