# Ch 12 · 键盘输入

> **原书第 12 章** · HFT **⚪** · 官方源码标签 `osbook_day12`（以 [os-from-zero](https://github.com/uchan-nos/os-from-zero) 为准）  
> **键盘链路：** FADT 校准 · **USB HID** · **文本框** · **闪烁光标**

---

### 本章结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① ACPI** | **XSDT → FADT** · PM Timer 校准 | **10ms** APIC 周期（补 Ch11） |
| **② 驱动** | USB **HID 键盘** · **keycode_map** | 键码 → ASCII |
| **③ 修饰键** | **Shift** · `keycode_map_shifted` | 大写与符号 |
| **④ GUI** | **文本框** · **`\b` 退格** | 可输入界面 |
| **⑤ 光标** | **0.5s 定时器** 闪烁 | Ch11 定时器落地 |

---

## 小节笔记

| 节 | 笔记 |
|----|------|
| 1. 本章定位 | [notes/section-1-本章定位.md](./notes/section-1-本章定位.md) |
| 2. FADT 解析与定时器校准 | [notes/section-2-FADT解析与定时器校准.md](./notes/section-2-FADT解析与定时器校准.md) |
| 3. USB 键盘与键码映射 | [notes/section-3-USB键盘与键码映射.md](./notes/section-3-USB键盘与键码映射.md) |
| 4. 修改键与 Shift 映射 | [notes/section-4-修改键与Shift映射.md](./notes/section-4-修改键与Shift映射.md) |
| 5. GUI 文本框与退格 | [notes/section-5-GUI文本框与退格.md](./notes/section-5-GUI文本框与退格.md) |
| 6. 闪烁光标与小结 | [notes/section-6-闪烁光标与小结.md](./notes/section-6-闪烁光标与小结.md) |

---

## 本章小结

| 问题 | 答案 |
|------|------|
| 本章做了什么？ | **键盘输入** + **文本框** + **退格** + **闪烁 caret** |
| 与 02 川合 OS 对照？ | 01 **Day 14 键盘**；Mikan **USB HID + GUI 文本框** |
| 与 Linux / CSAPP 对照？ | HID → scancode → 输入子系统极简版 |

**本章目的：** 打通 **人机键盘交互** — 「像真 OS」的输入体验。

---

## 本章学习目标 · 自检

- [ ] 从 **RSDP → XSDT → FADT** 取 PM Timer · 校准 **10ms** tick
- [ ] 解析 **USB HID 8 字节报告** · **`keycode_map`**
- [ ] 处理 **Shift** 与 **`keycode_map_shifted`**
- [ ] 实现 **文本框** 逐字绘制与 **退格**
- [ ] 用 **0.5s 周期定时器** 实现 **光标闪烁**

---

## 相关

- 上一章：[../chapter-11-timer-acpi/](../chapter-11-timer-acpi/)
- 下一章：[../chapter-13-multitask1/](../chapter-13-multitask1/) 🔴
- 前置：[../chapter-06-mouse-pci/](../chapter-06-mouse-pci/) · [../chapter-05-console-text/](../chapter-05-console-text/)
