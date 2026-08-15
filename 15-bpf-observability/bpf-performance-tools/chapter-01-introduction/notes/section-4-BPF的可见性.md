# 4. BPF 的可见性 (Visibility)

| 传统工具局限 | BPF 能做什么 |
|--------------|--------------|
| 固定统计项、盲区多 | **可编程** — 按需 hook 任意内核/用户路径 |
| 改配置常需重启或特殊模式 | **生产在线** attach/detach，验证器保证安全 |
| 用户态只见 syscall 入口 | **内核栈、TCP 内部、块层、调度器** 同一工具链 |

Gregg 的比喻：**X 射线** — 穿透整栈，而非只看 `/proc` 或应用日志。

**HFT：** 策略热路径在 user space，但 **run queue、重传、direct reclaim、off-CPU** 都在内核 — BPF 把「延迟在栈外哪一段」钉死。


### 常见陷阱

1. **以为 BPF 能看到一切** — BPF 受 verifier 约束：不能随意解引用指针、不能无限循环、有栈深度限制；某些内核路径（如 NMI 上下文）的 probe 行为受限
2. **混淆「可见」和「可安全访问」** — kprobe 能 attach 到几乎任意内核函数，但函数内部数据结构无稳定 ABI，升级后 offset 可能变；BTF + CO-RE 部分缓解但不完全消除
3. **忽视 BPF 程序的资源限制** — BPF Map 有大小上限、栈空间有限（通常 512 字节）、指令数有验证器上限；复杂聚合逻辑可能被 verifier 拒绝

<details>
<summary>📝 自测题（点击展开）</summary>

1. **BPF 的「可见性」相比传统工具有什么本质提升？**

   <details>
   <summary>参考答案</summary>

   传统工具只能看预定义的 /proc、sysfs、perf 事件；BPF 可在几乎任意内核函数（kprobe）和用户态函数（uprobe）插桩，实时按需定义观测点，无需重新编译内核或应用。相当于从「固定摄像头」升级到「可编程探针」。

   </details>

2. **verifier 对 BPF 可见性有哪些限制？**

   <details>
   <summary>参考答案</summary>

   (1) 指针解引用必须经过边界检查；(2) 不能无限循环（有指令数上限）；(3) 栈空间有限（~512 字节）；(4) 某些上下文（如 NMI）限制 probe 类型；(5) Map 有大小上限。这些限制保证安全但约束了复杂逻辑。

   </details>

3. **HFT 场景中 BPF 可见性的盲区在哪？**

   <details>
   <summary>参考答案</summary>

   (1) DPDK 用户态 PMD 轮询路径——BPF 主要看内核栈，用户态 PMD 需 uprobe；(2) 硬件级别（网卡 ASIC、交换机）——BPF 看不到；(3) 极高频路径（每包 probe）——verifier 允许但开销不可接受。

   </details>

</details>

---
