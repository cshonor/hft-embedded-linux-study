## 1. 本章定位

> **《从零自制操作系统》Ch 12 键盘输入**

---

### 一、本章核心目标

| Ch 11 | **Ch 12** |
|-------|-----------|
| 定时器 · ACPI **RSDP 入口** | **补全 FADT** · **10ms 校准** |
| 鼠标输入完整 | **键盘输入** + **GUI 文本框** |

**体验目标：** 手指敲击 → **字符出现在文本框** · **光标闪烁** — 交互 **质变**。

---

### 二、本章讲什么

| 主题 | 要点 |
|------|------|
| **FADT / XSDT** | Ch11 RSDP **续** · PM Timer 端口 |
| **USB HID 键盘** | 同鼠标 **xHCI** 栈 |
| **keycode_map** | 键码 → **ASCII** |
| **Modifier** | 报告 **byte0** · **Shift 表** |
| **TextBox** | 逐字绘制 · **`\b` 退格** |
| **Caret blink** | **0.5s** `kTimerTimeout` |

---

### 三、输入链路总览

```
USB 键按下 → HID 报告 → keycode → (Shift?) → ASCII
    → Message → 主循环 → TextBox 绘制
Timer 0.5s → 切换 caret 可见性
```

→ [Ch6 USB/xHCI](../chapter-06-mouse-pci/) · [Ch7 Message](../chapter-07-interrupt-fifo/)

---

← [Ch 12 导读](../README.md) · 下一节 [2. FADT](./section-2-FADT解析与定时器校准.md)
