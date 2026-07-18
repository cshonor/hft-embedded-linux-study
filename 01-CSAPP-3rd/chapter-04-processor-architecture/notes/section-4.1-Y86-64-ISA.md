## 4.1 Y86-64 指令集体系结构（4.1.1–4.1.6）

> **和全书的关系：** CSAPP **生产级例子 / Ch3 汇编** 是真 **x86-64**；本章 **Y86-64** 是为讲流水线而造的 **简化教学 ISA**（命名、`movq` 风格故意贴近 x86-64），**不是** 另一套你要在 Linux 上跑的 ABI。  
> 真机数据格式与寻址 → [Ch3 §3.3](../../chapter-03-machine-level-programs/notes/section-3.3-数据格式.md) · [§3.4.1](../../chapter-03-machine-level-programs/notes/section-3.4.1-操作数指示符.md)

### 4.1.1 程序员可见的状态

| 组件 | 说明 |
|------|------|
| **程序计数器 PC** | 下条指令地址 |
| **寄存器文件** | `%r0–%r14`（%r0 恒 0），8 字节 |
| **条件码 CC** | ZF、SF、OF |
| **Stat** | 程序状态：正常 / halt / 异常 |
| **内存** | 字节数组，小端 |

简化版 x86-64：**无特权级、无浮点、无向量** — 只为讲清数据通路。真机有 **16 GPR + 16 XMM** 等 → [Ch3 寄存器统计](../../chapter-03-machine-level-programs/notes/section-3.3-数据格式.md#x86-64-寄存器分类统计)

### 4.1.2–4.1.3 指令与编码

**指令类：** `irmovq`、`OPq`、`jXX`、`cmovXX`、`mrmovq`、`rmmovq`、`pushq`、`popq`、`call`、`ret`、`halt`、`nop`

- **变长编码** — 类似 x86：1 字节码 + 可选寄存器字节 + 可选 8 字节常数
- `fn` 字段区分 `add/sub/and/xor` 等

### 4.1.4 异常

- 非法指令、地址错、越界 — 置 `Stat` 并停止（真 CPU 更复杂：#GP、#PF…）

### 4.1.5–4.1.6 程序与指令细节

- `irmovq $imm, %r` — 立即数到寄存器
- `mrmovq` / `rmmovq` — load/store
- `jXX` / `cmovXX` — 控制与条件传送（与 [Ch 3](../chapter-03-machine-level-programs/notes/section-3.6-控制流.md) 对照）

**HFT：** 真 x86-64 指令集庞大得多；本章学 **「指令 → 控制信号 + ALU + 访存」** 的映射关系，读 `objdump` 时知道每条指令在干什么类操作。

---

← [本章导读](../README.md)
