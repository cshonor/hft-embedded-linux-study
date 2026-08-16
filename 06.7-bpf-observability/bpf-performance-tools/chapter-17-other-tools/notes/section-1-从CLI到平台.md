# 1. 导览：从 CLI 到 GUI（章首）

> 底本：《BPF之巅》第 17 章 其他 BPF 性能工具，章首（印刷 p738）

本章介绍基于 BPF 的**其他可观察性工具**——全部开源、可网上免费获得（感谢 Netflix 性能工程组的 Jason Koch，他完成了本章大部分内容）。

## 为什么需要 GUI

尽管本书包含数十种命令行 BPF 工具，但预计**大部分人会通过图形界面使用 BPF 跟踪**。对于由**成千上万个实例组成的云计算环境**尤其如此——这些通常必须通过 GUI 进行管理。

学习前面章节的 BPF 工具，对使用和理解这些基于 BPF 的 GUI 有帮助：**这些 GUI 仅仅是同样工具的前端**。

## 本章介绍的工具

| 工具 | 用途 |
|---|---|
| **Vector + Performance Co-Pilot (PCP)** | 远程 BPF 监控 |
| **Grafana + PCP** | 远程 BPF 监控 |
| **eBPF Exporter（Cloudflare）** | BPF 与 Prometheus/Grafana 集成 |
| **kubectl-trace** | 跟踪 Kubernetes 的 pods 和 nodes |

本章目的是通过示例展示基于 BPF 的 GUI 和自动化工具的可能性，每个工具一节：功能、内部工作原理、用法、更多参考。注意：撰写时这些工具的大量开发工作正在进行，功能可能会增强。

## 章节结构

- 17.1 Vector 和 PCP（17.1.1–17.1.10：可视化折线图/热图/表格、BCC 指标、内部实现、安装、连接、配置、改进）
- 17.2 Grafana 和 PCP（17.2.1–17.2.4）
- 17.3 Cloudflare eBPF Prometheus Exporter（17.3.1–17.3.4）
- 17.4 kubectl-trace（17.4.1–17.4.3）
- 17.5 其他工具
- 17.6 小结

## HFT 关联

交易公司监控数十个行情/策略/风控节点时，命令行逐台 SSH 不可扩展；runqlat/biolatency 直方图以**热图**形式集中呈现是尾延迟治理的基础看板（对应本仓库 16-hft-engineering 的可观测性需求）。

<details>
<summary>自测题</summary>

1. 为什么云计算环境必须通过 GUI 使用 BPF 跟踪？
2. 本章介绍的四个工具分别解决什么场景？

</details>
