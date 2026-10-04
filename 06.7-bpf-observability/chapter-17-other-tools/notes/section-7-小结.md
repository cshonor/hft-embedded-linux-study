# 7. 小结（17.6）

> 底本：《BPF之巅》第 17 章，17.6 节（印刷 p755）

BPF 工具空间正在迅速增长，并将开发出更多工具和功能。本章介绍了当前基于 BPF 的 4 种可用工具：

- **Vector/PCP、Grafana、Cloudflare eBPF 导出器**——图形工具，可对大量复杂数据提供可视化，包括时间序列 BPF 输出
- **kubectl-trace**——可对 Kubernetes 集群直接执行 bpftrace 脚本

此外还提供了其他 BPF 工具的简短列表（17.5 节）。

## 本章工具选型速查

| 需求 | 推荐 | 关键点 |
|---|---|---|
| 单主机深入排查、不观察零开销 | Vector / grafana-pcp-**live** | 浏览器持状态；热图看每秒直方图 |
| 跨多主机长期时间序列 | grafana-pcp-**redis** | 依赖 pmseries + Redis 持久化 |
| 已有 Prometheus/Grafana 基础设施 + 告警 | Cloudflare **ebpf_exporter** | 自定义 YAML 任意 BPF 程序，9435 端口 |
| Kubernetes 节点/pod 临时 bpftrace | **kubectl-trace** | `$container_pid` 过滤聚焦单 pod |
| 嵌入式/依赖极简环境 | **ply** | 无 LLVM 依赖的 BPF 跟踪器 |
| K8s 网络数据面/安全 | **Cilium** | BPF 取代 iptables |

## 贯穿本章的主线

1. **GUI 只是前端**：底层都是第 4–16 章的 BCC/bpftrace 工具——学好命令行工具是使用 GUI 的前提
2. **规模决定形态**：单机用 CLI，云规模（成千上万实例）必须 GUI + 自动化
3. **两种持久化哲学**：PCP 无状态（不观察零开销）vs Prometheus 持续轮询存储（可告警、可回溯）——按场景取舍

## HFT 关联

交易公司的分层监控建议：tick 路径节点用 ebpf_exporter 暴露低开销计数指标 + Prometheus 告警；排障时用 Vector/Grafana live 直连热图取证；K8s 侧备 kubectl-trace 做 pod 级临时插桩。

<details>
<summary>自测题</summary>

1. 本章四种工具中哪三个是图形工具？各自依托的数据链路是什么？
2. "无状态观察"与"持续轮询存储"两种模式各适合什么场景？

</details>

<details><summary>参考答案</summary>

1. ① Vector：浏览器 → pmwebd(REST) → pmcd → BCC PMDA → BPF/Perf 缓冲区（状态在浏览器）；② Grafana+PCP：同一 pmcd 链路，live 插件（浏览器存短暂历史）或 redis 插件（pmseries→Redis 持久化）；③ ebpf_exporter：BPF 程序 → 9435 端口 HTTP 暴露 → Prometheus 周期抓取存储 → Grafana 查询。kubectl-trace 不是图形工具，是 K8s 的 bpftrace 命令行前端。
2. 无状态观察：**不看不花**——适合生产节点平时零开销、出事时临时接入深入排查；持续轮询存储：一直花但换来**历史回溯、跨主机聚合、告警**——适合容量规划、事后复盘（T+1 分析昨晚的延迟事件）与需要自动告警的场景。

</details>
