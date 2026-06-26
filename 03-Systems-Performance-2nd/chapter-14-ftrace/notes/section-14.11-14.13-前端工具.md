## 14.11–14.13 前端工具

### trace-cmd

**Ftrace 的命令行前端** — 配置、录制、保存、回放。

```bash
# 录制 sched 事件 5 秒
trace-cmd record -e sched -p function_graph sleep 5
trace-cmd report | head

# 保存 trace.dat 供 KernelShark 打开
trace-cmd record -o trace.dat -e net -e sched sleep 10
```

| 优点 | 说明 |
|------|------|
| 简化 tracefs 手写 | 一条命令多 event |
| **record/report** | 可归档、可分享 |

### KernelShark

**trace-cmd 的 GUI** — 时间线、过滤、关联 CPU。

- 适合：**长时间 trace** 人工浏览 — 比 `cat trace` 可读。

### perf ftrace

`perf` 内置 Ftrace 前端 — 与 Ch 13 统一入口。

```bash
perf ftrace --tracer function_graph -- sleep 5
# 或 perf trace 走 tracepoint（见 Ch 13）
```

**分工：** 日常 **perf record/trace** 够用；专精 Ftrace 特性用 **trace-cmd**。

### perf-tools（Gregg 开源脚本集）

将 **Ftrace + 部分 perf** 封装为 **单用途工具**：

| 工具类 | 例子 |
|--------|------|
| 文件 | opensnoop、execsnoop |
| 网络 | tcpconnect、tcpretrans（老版 Ftrace 实现） |
| 磁盘 | 部分 biosnoop 前身 |

**何时仍需要：**

- **老内核** — 无 BCC/bpftrace/eBPF
- **最小环境** — 不能装 bpftrace 包
- **学习** — 看脚本如何写 tracefs

**现代 HFT 裸机：** 优先 **BCC/bpftrace**（Ch 15、04-BPF）；perf-tools 作 **fallback** 或读源码学 tracepoint。

→ https://github.com/brendangregg/perf-tools

---


---

← [本章导读](../README.md)
