# 4. 动态插桩：kprobes 与 uprobes

### kprobes（内核）

| 要点 | 说明 |
|------|------|
| **机制** | 在 **几乎任意内核指令** 动态插桩（x86_64 常用 `int3` 断点） |
| **触发** | 命中时跑 BPF 程序 — 可读上下文、写 map |
| **能力** | 深度透视 **未导出** 的内核路径 |
| **风险** | 内核内部函数 **无稳定 ABI** — 升级可能断；高频 probe 有开销 |

### uprobes（用户态）

| 要点 | 说明 |
|------|------|
| **机制** | 在用户二进制/共享库指令上插桩 — 类似 kprobes |
| **用途** | 追 `malloc`、自定义 SO 函数、语言 runtime |
| **警告** | **极高频** 函数（如每次 `malloc`）attach 可 **显著拖慢** — HFT 热路径慎用 per-hit 逻辑 |

**原则：** 能 **Tracepoint/USDT** 就不用 kprobe/uprobe；动态 probe **未 attach 时零开销**。


### 常见陷阱

1. **在极高频函数上 attach uprobe** — 如对 malloc/recv 每 hit 跑 BPF 程序，可能导致目标进程减速数倍；HFT 热路径绝对禁止 per-hit uprobe
2. **依赖 kprobe 追踪的内核函数名** — 内核内部函数无 ABI 保证，升级后可能重命名（如 do_sys_open → do_sys_openat2）；应优先用 tracepoint
3. **忽视 kprobe 的 instrument 限制** — 某些内核函数（如 __schedule 中的部分路径）不适合 kprobe，可能递归或死锁；内核有 `NOKPROBE_SYMBOL` 标记禁止插桩的函数

<details>
<summary>📝 自测题（点击展开）</summary>

1. **kprobe 和 uprobe 的插桩机制有什么共同点和区别？**

   <details>
   <summary>参考答案</summary>

   共同点：都是动态在指令地址插入断点（x86 用 int3），命中时执行 BPF 程序。区别：kprobe 在内核函数插桩，uprobe 在用户态二进制/库插桩；kprobe 受内核 NOKPROBE 限制，uprobe 受文件映射和权限限制。

   </details>

2. **为什么 HFT 热路径上绝对禁止 per-hit uprobe？**

   <details>
   <summary>参考答案</summary>

   每次 uprobe 命中需要：trap → 上下文切换 → BPF 程序执行 → 返回，这个开销在微秒级。HFT 策略循环中每次 recv/send 都触发 probe，会把延迟从微秒级推到毫秒级。如需观测，用低频采样或 Map 聚合。

   </details>

3. **kprobe 未 attach 时有开销吗？attach 后呢？**

   <details>
   <summary>参考答案</summary>

   未 attach 时零开销——原始指令不受影响。Attach 后每次命中需要 int3 trap + BPF 程序执行，开销取决于 BPF 程序复杂度和命中频率。原则：用完即撤，不长期挂载。

   </details>

</details>

---
