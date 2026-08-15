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


### 常见陷阱

1. BCC 和 bpftrace 二选一——Gregg 强调互补双剑：生产 crisis 用 BCC 标准工具，不够再上 bpftrace
2. bpftrace 脚本不升格——重复有用的 bpftrace 脚本应升格为 BCC 工具或 runbook，不是每次重写
3. BCC 工具不记 runbook——出事才 man page，runbook 应预设好第一反应 BCC 命令

<details>
<summary>自测题（点击展开）</summary>

1. Gregg 的 BCC/bpftrace 工作流是什么？
   <details><summary>答</summary>1) 生产 crisis → BCC 标准工具 2) 不够 → bpftrace 即兴追 3) 证明有用 → 升格 BCC/runbook</details>
2. 为什么 bpftrace 脚本应该升格？
   <details><summary>答</summary>重复有用的脚本应升格为 BCC 工具或 runbook——避免每次出事重写，且可团队共享</details>
3. HFT runbook 中 BCC 工具应该怎么用？
   <details><summary>答</summary>预设好第一反应命令（延迟尖刺→offcputime/runqlat），复制粘贴即可跑</details>

</details>


---

← [本章导读](../README.md)
