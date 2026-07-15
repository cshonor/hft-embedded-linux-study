## §11.9 练习题

> **Ch 11 · 浮点数据处理** · [章导读](../README.md)

---

### 练习覆盖范围（原书）

| 类型 | 对应节 | 建议 |
|------|--------|------|
| **语法 .F32** | §11.2 | 写 VADD/VMUL |
| **VCMP+VMRS+B** | §11.4 | 比较 s0,s1 分支 |
| **FZ/DN** | §11.5 | 概念 |
| **VMLA vs VFMA** | §11.7 | 口述舍入次数 |
| **二分/泰勒** | §11.8 | 读示例流程 |

---

### 自测 Checklist

- [ ] 写 **VCMP → VMRS → BGT** 三行骨架
- [ ] 解释 **为何 VADD 不能替代 VCMP 设标志**
- [ ] 对比 **VMLA 与 VFMA** 舍入
- [ ] 说明 **FPU 无位操作** 时的 workaround
- [ ] **VDIV** 可能触发的 **DZC/IXC**

---

### 浮点三章之后

→ **[Ch12 查表](../../chapter-12-tables/)**（选读）或回到精读链 **[Ch13 栈/APCS](../../chapter-13-subroutines-stacks/)**
