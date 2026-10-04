# 3. Grafana 和 Performance Co-Pilot（17.2）

> 底本：《BPF之巅》第 17 章，17.2 节（印刷 p747–750）

**Grafana** 是流行的开源绘图和可视化工具，支持连接并展示后端数据源的数据。以 **PCP 作为数据源**，可以可视化 PCP 公布的任何指标（PCP 见 17.1 节）。

## 17.2.1 安装和配置：两种数据源

| 选项 | 插件 | 特点 | 适用 |
|---|---|---|---|
| **实时数据源** | grafana-pcp-**live** | 轮询 PCP 实例获取最新指标，短暂历史（几分钟）保存在**浏览器**中，不长期持久化；**不观察时对被监控系统零开销** | 深入查看主机实时指标 |
| **归档数据源** | grafana-pcp-**redis** | 从 PCP **pmseries** 数据存储读取并整理到 **Redis**；依赖配置过的 pmseries，意味着 PCP 将轮询并存储数据 | 收集并查看**较大跨多主机的时间序列数据** |

假定已执行 17.1 节的 PCP 配置步骤。两个项目都在大量变更中，最好按 17.2.4 链接的插件安装说明操作。

## 17.2.2 连接并查看数据

grafana-pcp-live 正在大量开发中。连接后端的方法取决于 PCP 客户端变量设置——因为**没有任何存储**，仪表盘可被动态重置以连接多个不同主机。三个变量：

| 变量 | 示例值 |
|---|---|
| `_proto` | http |
| `_host` | 目标主机 |
| `_port` | 7402 |

创建新仪表盘 → 仪表盘设置 → 创建变量并设为所需配置值（图 17-10）。完成后添加新图表，选择一个 PCP 指标——**`bcc.runq.latency`** 是好的开始（图 17-11）。

**可视化配置**（图 17-12/17-13）：选择 **Heatmap** 可视化 →

- Data format: **Time series buckets**
- Unit: **microseconds (µs)**
- Bucket bound: **Upper**（桶边界取上界）

配置后仪表盘可同时显示标准 PCP 指标（上下文切换/秒、可运行线程数）与 runqlat BCC 指标的延迟热图（图 17-12）。

## 17.2.3–17.2.4 改进与阅读

Grafana+PCP 与整套 bcc-tools 的集成仍需大量工作；希望在以后更新中提供**可视化自定义 bpftrace 程序**的支持；grafana-pcp-live 插件还需要大量额外工作才足够可靠。进一步阅读：grafana-pcp-live 数据源（链接 7）、grafana-pcp-redis 数据源（链接 8）——项目成熟中，链接可能变化。

## HFT 关联

- live 插件"**不观察零开销**"的特性适合生产交易节点：平时只保留归档型低频指标，排障时临时打开 live 仪表盘直连
- redis 归档路线适合事后复盘（如 T+1 复盘昨晚尾延迟事件），但注意 pmseries 持续轮询本身有成本，生产 tick 节点慎用

<details>
<summary>自测题</summary>

1. grafana-pcp-live 与 grafana-pcp-redis 的核心区别是什么？各自适用场景？
2. live 插件靠什么实现仪表盘动态切换多个主机？
3. 在 Grafana 中把 bcc.runq.latency 画成热图需要哪三项关键配置？

</details>

<details><summary>参考答案</summary>

1. **live**：实时轮询 PCP，几分钟的短暂历史存在**浏览器**里、不持久化——不观察时零开销，适合深入实时排查；**redis**：从 PCP pmseries 数据存储读入 Redis，PCP 会持续轮询并存储——适合**跨多主机的长期时间序列**（回溯、趋势），代价是被监控侧常驻成本。
2. 靠**客户端变量**（`_proto`/`_host`/`_port`，如 http/目标主机/7402）。live 没有任何服务端存储，所以仪表盘可以随时"重指"另一台主机——数据源本身就是可变的连接参数。
3. ① 可视化选 **Heatmap**；② Data format 选 **Time series buckets**；③ Unit 选 **microseconds (µs)** 且 Bucket bound 选 **Upper**（桶边界取上界）。

</details>
