## 15.1.7 BCC vs bpftrace

| 维度 | **BCC** | **bpftrace** |
|------|---------|--------------|
| **语言** | Python/Lua + C BPF | 专用 DSL |
| **上手** | 跑预制工具快；开发慢 | 单行极快；复杂脚本中等 |
| **输出** | 成熟 CLI 格式 | 自定义 print/map |
| **维护** | 适合 **团队标准工具** | 适合 **个人诊断脚本** |
| **性能** | 优化充分 | 多数场景足够 |
| **关系** | **互补双剑** | **互补双剑** |

**Gregg 工作流：**

```
1. 生产 crisis → BCC 标准工具（runqlat、tcpretrans、biolatency…）
2. 标准工具不够 → bpftrace 即兴追 kprobe/uprobe
3. 证明重复有用 → 升格为 BCC 工具或 runbook 脚本
4. 长期产品 → 04-BPF 专书 + libbpf/CO-RE
```

**HFT runbook 示例：**

```
延迟尖刺
  → offcputime / runqlat（BCC）
  → 若 Lock → bpftrace 追 mutex
  → 若 Net → tcpretrans + ss -tiepm
  → 若 mystery stall → Ftrace hwlat（Ch 14）
```

---


---

← [本章导读](../README.md)
