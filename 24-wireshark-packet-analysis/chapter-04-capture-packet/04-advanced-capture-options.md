# 4.4 高级捕获选项

> 本章：[chapter-summary.md](./chapter-summary.md) · 全书：[../README.md](../README.md) · 混杂模式：[§2.1](../chapter-02-traffic-monitor/01-promiscuous-mode.md)

**核心主旨**：`Capture Options` 三标签页——在抓包源头控制接口、分文件输出与性能。

## 核心知识点

入口：`Capture` → `Options`（或 Interfaces 旁齿轮）

### 1. 输入（Input）

| 功能 | 说明 |
|------|------|
| 接口列表 | 选择网卡；可看**实时流量条** |
| **Promiscuous mode** | 混杂模式开关（与第 2 章一致） |
| 多接口 | 可同时选多块网卡（生成多链路 pcapng） |

### 2. 输出（Output）与多文件

| 功能 | 说明 |
|------|------|
| **File** | 输出路径与文件名模板 |
| **File Sets** | 按**时间**（如每分钟）或**大小**（如每 1MB）自动切分多个文件 |
| **Ring Buffer（环状缓冲）** | 限制**文件个数**；写满最后一个后 **FIFO 覆盖第一个** |

**环状缓冲结论**

```text
持续抓最新流量 + 磁盘上限可控 → 长跑排障首选
```

| 场景 | 建议 |
|------|------|
| 间歇故障、需保留很久 | File Sets 足够大 + 足够磁盘 |
| 7×24 探针、磁盘有限 | Ring Buffer + 合理单文件大小 |

### 3. 选项（Options）

| 选项 | 建议 |
|------|------|
| **Update list of packets in real-time** | 边抓边刷新 GUI；**大量流量时 CPU 暴涨** |
| 排障默认 | **关闭**实时更新；抓完再打开文件分析 |
| 捕获过滤器 | 见 [§4.5](./05-filter-basics.md) |
| 其他 | Snaplen、Buffer、Monitor mode（无线）等按场景查官方文档 |

**高危警告**：高 PPS 环境勾选「实时更新」可能导致 Wireshark **卡顿、丢包**（内核缓冲溢出），除非必须盯着 live 列表。

## 抓包/实操记录

| 练习 | 配置 |
|------|------|
| 环状缓冲 | 5 个文件 × 10MB，观察覆盖行为 |
| 性能对比 | 同一流量：开/关 real-time update，观察 CPU |
| 长跑 | File Sets 每 1h 一个文件 + 捕获过滤器 `host 目标` |

## 疑问与总结

- 分文件 ≠ 自动合并；事后用 [§4.1 Merge](./01-use-capture-files.md)。
- 镜像口过载丢包时，选项再优也救不了**物理瓶颈**（见第 2 章 SPAN 过载）。
