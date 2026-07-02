## ④ 中断处理程序 · 栈 · `IRETD`

#### IRQ → INT 向量（原书映射）

| 设备 | 典型映射 |
|------|----------|
| **键盘** | **IRQ1** → **INT 0x21** |
| **鼠标** | **IRQ12** → **INT 0x2c** |

IDT 里登记 **0x21 / 0x2c** 的处理入口。

#### 难题：C 不能 `IRETD`

| 要求 | C 函数默认 |
|------|------------|
| 中断返回 | **`IRETD`**（恢复 CS/EIP/EFLAGS 等） |
| 普通 `return` | 仅 **`RET`** — **不够** |

**解法：** 汇编 **stub** + C **逻辑**：

```
_asm_inthandler21:          ; 汇编入口（IDT 指向这里）
    PUSH 寄存器…            ; 保存现场
    CALL  inthandler21_c     ; C 里写业务（读扫描码、打印等）
    POP 寄存器…             ; 恢复现场
    IRETD                   ; 中断返回
```

| 层 | 职责 |
|----|------|
| **汇编 `_asm_inthandlerXX`** | 保存/恢复寄存器、**`IRETD`** |
| **C `inthandler21` 等** | 读端口、**sprintf/putstr** 提示 |

#### 栈（Stack）

**PUSH / POP** 依赖 **栈** — **后进先出（LIFO / FILO）**：

```
中断发生 → 硬件自动压栈部分状态
         → 处理程序 PUSH 更多寄存器
         → … CALL C …
         → POP 逆序恢复
         → IRETD
```

→ [01-CSAPP Ch8 异常控制流](../../../../01-CSAPP-3rd/chapter-08-exceptional-control-flow/) · 中断与过程调用都深依赖栈

#### Day 6 里程碑

**按下键盘 A** → 屏幕 **打印提示** — 证明 **硬件中断 → PIC → CPU → IDT → ISR → C** 全链打通。

Day 7 再 **真正读键盘/鼠标数据** 并驱动光标。

---
