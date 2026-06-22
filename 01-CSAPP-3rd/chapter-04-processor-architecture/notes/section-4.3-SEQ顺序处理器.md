## 4.3 Y86-64 的顺序实现 SEQ（4.3.1–4.3.4）

### 4.3.1 处理阶段

经典 **五阶段**（与 PIPE 同名，但 SEQ 每周期只推进一条指令走完）：

```
F  Fetch      — 读指令字节，算 valP（下一条 PC）
D  Decode     — 译码，读寄存器
E  Execute    — ALU、算有效地址
M  Memory     — 读/写内存
W  Write back — 写寄存器
```

### 4.3.2 SEQ 硬件结构

- **PC 更新：** `call`/`jXX`/`ret` 等特殊路径
- **寄存器文件：** 双读端口 + 单写（同周期读写规则）
- **ALU** — 算地址与 `OPq`
- **Stat** — 异常汇总

### 4.3.3 时序

- **单周期：** 一条指令在一个 **极长时钟周期** 内完成五阶段（教学模型）
- 真实 x86：**多周期、微码、乱序** — SEQ 是清晰起点

### 4.3.4 各阶段实现要点

| 阶段 | 关键 |
|------|------|
| F | `icode:ifun`、`rA:rB`、`valC`、`valP` |
| D | `srcA/srcB`、`dstE/dstM` |
| E | `valE = ALU(valA, valB)` |
| M | `valM = M[valE]` 或写 |
| W | 选 `valE` 或 `valM` 写回 |

**HFT：** SEQ 的「一条指令一气呵成」帮助理解 **latency**；高吞吐靠 PIPE/超标量，不是 magic。

---

← [本章导读](../README.md)
