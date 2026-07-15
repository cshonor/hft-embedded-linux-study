## §8.6 练习题

> **Ch 8 · 分支与循环** · [章导读](../README.md)

---

### 练习覆盖范围（原书）

| 类型 | 对应节 | 建议 |
|------|--------|------|
| **B/BL/BX** | §8.2 | 写调用/返回骨架 |
| **CBZ vs CMP+B** | §8.2 | 何时可替换 |
| **While/For/Do-While** | §8.3 | 各写一版汇编 |
| **向下计数** | §8.3 | `SUBS`+`BNE` |
| **IT 掩码** | §8.4 | 解析 `ITTE LT` |
| **展开** | §8.5 | 4 次迭代手写展开 |

---

### 自测 Checklist

- [ ] 解释 **分支对流水线** 的影响
- [ ] **`BL` 与 `B` 区别**；LR 谁写
- [ ] 用 **SUBS+BNE** 写 For 向下循环
- [ ] 写 **ARM 条件后缀** 与 **IT 块** 各一段 if-else
- [ ] 说明 **CBZ 限制**（r0–r7、无 IT、短距）
- [ ] 口述 **循环展开** 利弊

---

### 与下一章

→ **[Ch9 浮点简介](../../chapter-09-floating-point-basics/)** — **跳过**（嵌入式 Linux 主线）  
→ 或直进 **[Ch13 子程序与堆栈](../../chapter-13-subroutines-stacks/)** / **[Ch16 MMIO](../../chapter-16-memory-mapped-peripherals/)** 按 [OUTLINE](../../OUTLINE.md) 精读顺序
