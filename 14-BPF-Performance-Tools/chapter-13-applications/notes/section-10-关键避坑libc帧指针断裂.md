# 10. 关键避坑：libc 帧指针断裂 ⚠️

**书中强调的现实：** 多数发行版 **`libc` / `libpthread` 编译时 omit frame pointer**。

| 后果 | 表现 |
|------|------|
| `offcputime` / `ioprofile` 从内核栈 **向上 walk** | 在 **libc 层断裂** |
| 只见 | `__pwrite+79`、`read+0x…` |
| 看不见 | **应用内部** 真实调用栈 |

**对策：**

| 方案 | 说明 |
|------|------|
| **应用 `-fno-omit-frame-pointer`** | 策略二进制必须（[Ch 12](../../chapter-12-languages/)） |
| 自编译 **带 FP 的 libc** | 极端，运维成本高 |
| **ORC / LBR** 等栈 walk | 内核/perf 能力，环境相关 |
| **在应用函数上 uprobe/USDT** | 跳过 libc，直接挂业务符号 |

**HFT 构建要求：** 策略 **.so/.exe 保留 FP + debuginfo**；否则 Off-CPU 火焰图 **半盲** — 与 SysPerf 应用章同训。

→ [SysPerf Ch 5 Applications](../../../13-Systems-Performance-2nd/chapter-05-applications/)

---
