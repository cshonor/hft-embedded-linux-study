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


### 常见陷阱

1. 危机时才装工具——出事时才发现 perf/BPF 没装或版本不匹配，应该预装并验证
2. 危机工具不熟——出事时现学 man page 太慢，runbook 应预设好第一反应命令
3. 工具和内核版本不匹配——perf 需匹配 linux-tools-$(uname -r)，BCC 需匹配内核 headers

<details>
<summary>自测题（点击展开）</summary>

1. 什么是「危机工具包」？为什么需要预装？
   <details><summary>答</summary>出事时第一时间需要的工具（vmstat/mpstat/perf/ss/iostat）——出事时才装可能网络不通或版本不对</details>
2. perf 工具的版本匹配要求是什么？
   <details><summary>答</summary>perf 需匹配 linux-tools-$(uname -r)，不匹配会导致事件不可用或数据错误</details>
3. HFT runbook 中危机工具应该怎么组织？
   <details><summary>答</summary>预设好第一反应命令（如 vmstat 1; mpstat -P ALL 1; ss -tiepm），复制粘贴即可跑</details>

</details>


---

← [本章导读](../README.md)
