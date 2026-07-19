## 4.5 Y86-64 流水线 PIPE（4.5.1–4.5.10）

> **Ch4 最重要一节（HFT）：** 五级流水线 + **数据 / 控制 / 结构** 三类冒险。  
> 无分支、`inline`、拆依赖 — 本质都是少 stall / 少冲刷。章导读 → [README](../README.md)

### 4.5.1–4.5.3 SEQ+ 与流水线寄存器

- **SEQ+：** 把 PC 计算挪到 E 段，为预测留结构
- **PIPE：** 在 F/D/E/M/W 之间加 **流水线寄存器**（`F_D`, `D_E`, `E_M`, `M_W`）
- 信号 **重命名/重排** — 每段读上一段锁存值

### 4.5.4 预测下一个 PC

- **默认：** `valP`（顺序下一条）
- **分支：** 预测 **跳转 taken 与否** — Y86 用简单策略（书中常 **预测跳转 taken** 到 `valC`）
- **预测错** → **恢复 PC**，冲刷错误路径上的指令（bubble）

**HFT ↔ 真 CPU：** Intel **分支预测器**（BTB、历史表）— `perf` 的 `branch-misses` 就是预测失败代价；热路径 **可预测分支**（如固定轮询方向）更友好。

### 4.5.5 流水线冒险

| 类型 | 原因 | 对策 |
|------|------|------|
| **结构 (structural)** | 硬件资源冲突（如同周期读写同一端口） | 停顿、双端口 |
| **数据 (data)** | RAW 真相关 | **转发 (forwarding)** + 必要时 **stall** |
| **控制 (control)** | 分支/ret 改 PC | **预测** + 错则 bubble |

**load-use 冒险：** `mrmovq` 后立即用 `valM` — 无法转发，**必须 stall 1 拍**（经典考点）

```asm
mrmovq 8(%rax), %rbx
addq %rbx, %rcx    # 需停顿
```

**HFT 编码：** 热循环里避免 **load 下一条立刻依赖**；可插 `nop` 或重排指令（编译器 `-O` 常做）。

### 4.5.6 异常处理

- 异常指令 **标记 Stat**，后续指令 **冲刷 (bubble)**，不写回
- 乱序 CPU 更复杂（精确异常）— Y86 简化

### 4.5.7–4.5.8 各阶段与控制逻辑

- **stall 条件：** load-use、ret 等
- **bubble：** 预测失败、异常 — 插入 `nop` 等价气泡
- **控制逻辑表** — `stall_C`、`bubble_F` 等（作业核心）

### 4.5.9 性能分析

- **CPI** = 1 + 停顿周期占比
- 分支预测准确率、load-use 频率直接拉低 IPC

**HFT：** `perf stat` 看 `instructions`, `cycles`, `branches`, `branch-misses` — 算有效 IPC；与 Ch 5 profile 结合改热循环。

### 4.5.10 未完成的工作

- 超标量、乱序执行、向量、多核 — Hennessy / 真芯片文档

---

## 4.6 小结与模拟器

- **SEQ** — 正确性参考；**PIPE** — 性能与冒险
- 配套工具（课程常用）：**`csim`**（cache）、**`ssim`**（SEQ）、**`psim`**（PIPE）— 可选跑 Y86 程序加深印象

→ 下一章在 **软件侧** 压榨 CPU：[Ch 5 优化程序性能](../../chapter-05-optimizing-performance/)  
→ 真实流水线 / 乱序：[02-Hennessy](../../../03-Computer-Architecture-6th/)

---

← [本章导读](../README.md)
