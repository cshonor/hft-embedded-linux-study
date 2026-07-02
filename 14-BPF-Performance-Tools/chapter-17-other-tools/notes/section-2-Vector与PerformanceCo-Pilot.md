# 2. Vector 与 Performance Co-Pilot (PCP)

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

→ PCP 传统指标与 SysPerf 方法论：[13-Systems-Performance](../../../13-Systems-Performance-2nd/)

---
