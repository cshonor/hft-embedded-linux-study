## 4.1 工具覆盖范围与「危机工具」

### 危机工具（Crisis Tools）

**Gregg 观点：** 性能危机时再装调试工具 = **为时已晚**，还可能延长 MTTR（装包、依赖、版本不匹配）。

**应提前部署的 Linux 工具包：**

| 包 / 组件 | 提供什么 |
|-----------|----------|
| **procps** | `ps`、`top`、`vmstat`、`pidstat` 等 |
| **sysstat** | `iostat`、`mpstat`、`sar`、`sadc` |
| **linux-tools-common** | `perf`（版本需匹配内核） |
| **bcc-tools** | BCC 自带脚本（biolatency、runqlat…） |
| **bpftrace** | 单行/脚本 eBPF 追踪 |

**HFT 裸机 checklist：**

```
[ ] perf 版本 = 运行中内核
[ ] bpftrace + bcc 可加载最小 BPF 程序
[ ] sar/sadc 已配置历史归档（非热路径机器也建议有）
[ ] 危机 runbook 写清：先 60 秒清单 → 再 perf/bpftrace
```

→ Ch 1 [60 秒清单](../../chapter-01-intro/)  
→ 附录 [bpftrace 单行命令](../../appendix-C-bpftrace单行命令.md)

---


---

← [本章导读](../README.md)
