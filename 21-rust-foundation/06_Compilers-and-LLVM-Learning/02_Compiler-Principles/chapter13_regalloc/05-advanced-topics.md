# 第 13 章 · 寄存器分配 · §5 高级话题

← [本章目录](./README.md) · 上一节：[04-graph-coloring.md](./04-graph-coloring.md) · 下一节：---

## 编码机器限制

真实 ISA 并非「任意 k 个 preg 等价」：

| 限制 | 冲突图编码 |
|------|------------|
| **固定寄存器指令** | `mul` 必须用 `rax` → 相关 live range 与 rax **预着色**（precolor） |
| **仅部分寄存器可寻址** | 某些 op 只允许子集 → 缩小可用色 |
| **Register pairs** | 旧 x86 `dx:ax` 等 |
| **Callee-saved** | 跨 call 需保存 → 插入 save/restore，影响 LIVE |

**预着色节点**：必须固定某色 — 与邻居冲突更严，spill 压力↑。

**LLVM**：`PhysReg` 约束在 **RegisterClass** + **Operand` flags`** 中描述。

---

## 变形与困难问题

| 话题 | 说明 |
|------|------|
| **Live range splitting** | 一范围太长 → 拆成两段，中间 spill，减冲突 |
| **Rematerialization** | 值可 cheap 重算 → spill 变「再算」而非 load |
| **Phase ordering（再访）** | RA 后是否再调度 · SSA 退出后的 copy |
| **线性扫描** | O(n) 替代方案 — JIT 常用；与图着色 trade-off |
| **PBQP / ILP** | 研究/部分工业 — 更优但更贵 |

**边界难题**：**不规则架构**、**可变长度指令**、**极端寄存器压力**（如 x87 栈）。

---

## Part IV 与全书收束

```text
ch11  Instruction Selection   IR → 机器指令（vreg）
ch12  Instruction Scheduling  重排 hide stall
ch13  Register Allocation     vreg → preg / stack
        ↓
   目标文件 / 链接
```

| 部分 | 章 | 完成 |
|------|:--:|:----:|
| 前端 | 2～4 | ✓ |
| 基础结构 | 5～7 | ✓ |
| 优化 | 8～10 | ✓ |
| **代码生成** | **11～13** | **✓** |

→ 附录 A/B · 或 [04 LLVM](../../../04_Learn-LLVM-17/README.md) 实验深化。

---

## HFT / Rust

- **`#[inline]` + LTO`**：更大函数 → 更大冲突图 → regalloc 更关键。
- **读汇编**：循环内 spill = 红旗；`-C target-cpu=native` 影响可用寄存器与指令选择。
- **`rustc`**：MIR 有自己的寄存器分配；LLVM 层再做 Greedy RA。

---

## 自测

- [ ] 预着色节点是什么
- [ ] Live range splitting 解决什么问题
- [ ] 图着色 vs 线性扫描 各适合什么场景
