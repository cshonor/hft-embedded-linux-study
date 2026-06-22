## 3.11 浮点代码

> x86-64 浮点走 **SSE/AVX 寄存器 `%xmm0–%xmm15`**（及扩展 `%ymm`），与整数 ABI 分开。

### 3.11.1 浮点传送与转换

- `movss` / `movsd` — float / double
- `cvtsi2sd` 等 — 整数↔浮点转换（舍入模式影响结果）

### 3.11.2 过程中的浮点

**System V AMD64 浮点参数：**

- 前 8 个 `float`/`double` → `%xmm0` … `%xmm7`
- 返回值 → `%xmm0`（float 也在 xmm0）

与整数寄存器 **独立计数** — 混参时注意编译器分配。

### 3.11.3–3.11.4 运算与常数

- `addsd` `mulsd` `divsd` — 标量 double
- 浮点常数编译进 **.rodata** 或 **RIP 相对寻址**

### 3.11.5 位级操作

- 用 `mov` 把 float 位模式当整数处理 — 快速 `abs`、比较、NaN 检测（`perf` 里少见手写，库函数多）

### 3.11.6 浮点比较

- `ucomisd` + 条件跳转 — NaN 无序比较走特殊路径

### 3.11.7 观察结论

- 浮点路径 **与整数热路径分离** — HFT 核心定价常用 **整数 tick**；指标模块可用 double
- `-mfpmath=sse`、`-ffast-math` 改变语义 — 回测与生产 flags 一致

### 3.12 小结（原书）

机器级程序 = **数据格式 + 传送/算术 + 控制 + 栈上过程 + 复合类型布局**；安全与性能都要求在 C 抽象之下还能推理 **内存与指令**。

→ 下一章 CPU 如何执行这些指令：[Ch 4](../../chapter-04-processor-architecture/)

---

← [本章导读](../README.md)
