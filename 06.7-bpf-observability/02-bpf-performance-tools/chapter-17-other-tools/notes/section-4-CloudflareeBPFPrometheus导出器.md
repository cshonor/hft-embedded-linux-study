# 4. Cloudflare eBPF Prometheus Exporter（配合 Grafana）（17.3）

> 底本：《BPF之巅》第 17 章，17.3 节（印刷 p750–752）

## 为什么是 Prometheus

**Prometheus** 已成为非常流行的指标收集、存储和查询工具：

- 提供简单、众所周知的**协议**——任何语言的集成都十分容易，有大量语言绑定
- 支持**报警**功能，与动态环境（如 **Kubernetes**）继承得很好
- 本身仅提供基本 UI，但其上构建了许多绘图工具（包括 **Grafana**），提供一致的仪表盘体验

在 Prometheus 中，收集和公布指标的工具被称为**导出器（exporter）**。官方和第三方导出器可收集 Linux 主机统计、Java 应用的 JMX 导出器，以及 Web 服务器、存储层、硬件、数据库服务等。**Cloudflare 开源了针对 BPF 指标的导出器**，允许通过 Prometheus 向 Grafana 公开和可视化这些指标。

## 17.3.1 构建并运行 ebpf_exporter

使用 Docker 构建：

```bash
$ git clone https://github.com/cloudflare/ebpf_exporter.git
$ cd ebpf_exporter
$ make
$ sudo ./release/ebpf_exporter-*/ebpf_exporter \
    --config.file=./examples/runqlat.yaml
2019/04/10 17:42:19 Starting with 1 programs found in the config
2019/04/10 17:42:19 Listening on :9435
```

配置文件（如 runqlat.yaml）声明 BPF 程序与导出方式，默认监听 **9435 端口**等待 Prometheus 抓取。

## 17.3.2 配置 Prometheus 监控 ebpf_exporter 实例

假设实例在端口 9435 上运行，Kubernetes 环境的示例目标配置（利用 node 发现 + relabel 把 10250 换成 9435）：

```yaml
$ kubectl edit configmap -n monitoring prometheus-core
- job_name: kubernetes-nodes-ebpf-exporter
  scheme: http
  kubernetes_sd_configs:
  - role: node
  relabel_configs:
  - source_labels: [__address__]
    regex: (.*):10250
    replacement: ${1}:9435
    target_label: __address__
```

## 17.3.3 在 Grafana 中设置查询

ebpf_exporter 运行后马上产生指标。Grafana 查询示例（图 17-14）：

```
query         : rate(ebpf_exporter_run_queue_latency_seconds_bucket[20s])
legend format : {{le}}
axis unit     : seconds
```

- `rate(...bucket[20s])`：对直方图**桶计数**取每秒速率（20 秒窗口）
- `{{le}}` 图例：按"小于等于"边界分列——多个桶曲线叠起来就是热图效果

图 17-14 展示了 **schbench 在线程数多于内核数时**的运行队列延迟尖峰。

## 17.3.4 进一步阅读

Grafana/Prometheus 信息见原书链接 9；Cloudflare eBPF 导出器见链接 10。

## 与 Vector/PCP 路线的对比

| 维度 | Vector/PCP | ebpf_exporter + Prometheus |
|---|---|---|
| 模型 | 无状态、浏览器持状态、不观察零开销 | Prometheus 主动**轮询并持久存储** |
| 生态 | PCP/PMWEBD 专用 | Prometheus 协议生态（告警、K8s 服务发现、多语言绑定） |
| 定制 | bcc.conf 启用 BCC 模块 | **自定义 YAML 配置任意 BPF 程序** |

## HFT 关联

- 交易公司若已有 Prometheus/Grafana 基础设施，ebpf_exporter 是把 runqlat/biolatency 直方图纳入统一监控与告警的最短路径
- 抓取间隔（scrape interval）与 rate 窗口要在"分辨率 vs 轮询开销"间权衡；tick 路径节点建议仅暴露低开销计数类指标，直方图用于排障节点

<details>
<summary>自测题</summary>

1. Prometheus 中"exporter"是什么？Cloudflare 贡献了什么？
2. ebpf_exporter 默认监听哪个端口？K8s 中如何通过 relabel 把节点指标端口指向它？
3. `rate(ebpf_exporter_run_queue_latency_seconds_bucket[20s])` + `{{le}}` 图例各是什么含义？

</details>
