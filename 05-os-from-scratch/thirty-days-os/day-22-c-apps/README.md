# Day 22 · 用 C 语言编写应用程序


> **原书第二十二章** · **保护验收 + .hrb 头 + GUI API** — 特权指令 **#GP**、**0x0c 栈异常**、**Shift+F1 强杀**、**36B Hari 头**、窗口句柄与绘图。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 特权指令** | **CLI/HLT/IN/OUT** 攻击 | **INT 0x0d** 拦截 |
| **② 异常调试** | **0x0c 栈异常** · **.map/.lst** | **Shift+F1** 强结束 |
| **③ .hrb 头** | **36B「Hari」+ 数据段大小** | C **字符串/数据段** 正确装载 |
| **④ GUI API** | 窗口句柄 · 字符 · 方块 | **C 写图形 app** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 抵御特权指令攻击 | [notes/section-22.1-抵御特权指令攻击.md](./notes/section-22.1-抵御特权指令攻击.md) |
| 异常找 Bug | [notes/section-22.2-异常找-Bug.md](./notes/section-22.2-异常找-Bug.md) |
| .hrb 文件头 | [notes/section-22.3-hrb-文件头.md](./notes/section-22.3-hrb-文件头.md) |
| GUI 编程 | [notes/section-22.4-GUI-编程.md](./notes/section-22.4-GUI-编程.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| CLI/HLT/IN/OUT？ | app 低权限 → **0x0d** |
| 数组越界？ | 常 **0x0c 栈异常** + **EIP/.map** |
| 死循环？ | **Shift+F1** 强杀 |
| C 字符串为何不显示？ | 未装 **数据段** → **36B .hrb 头** |
| GUI 怎么给 app？ | **窗口句柄 API** + 字符/方块 |
| 里程碑？ | **C + 图形 app** 正式可行 |

---

---

## 本日学习目标 · 自检

- [ ] 列举 **三条** 被 0x0d 挡住的特权行为
- [ ] 会用 **EIP + .map/.lst** 描述定位崩溃
- [ ] 说清 **.hrb 36 字节头** 与 datasize
- [ ] 理解 **句柄** 把 app 与 **SHEET** 关联
- [ ] 串起 Day 20 API → 21 保护 → 22 loader+GUI

---

← [Day 21](./day-21-保护操作系统.md) · [01 导读](../README.md) · [Day 23](./day-23-图形处理相关.md)

---

## 相关

- 上一日：[../day-21-protection/](../day-21-protection/)
- 下一日：[../day-23-graphics/](../day-23-graphics/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
