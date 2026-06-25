# Day 20 · API


> **原书第二十章** · **内核 ↔ 应用桥梁** — **far-CALL/RETF**、**INT 0x40**、**`cmd_app`**、**PUSHAD/POPAD**、**EDX 功能号** 路由。

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 首个 API** | 硬编码 **CALL** | **far-CALL + RETF** 回 Console |
| **② INT 0x40** | API 挂 **IDT** | **地址无关** · 更小指令 |
| **③ cmd_app** | 未知命令 → **`.hrb`** | 任意文件名运行 |
| **④ 寄存器保护** | **PUSHAD/POPAD** | API 不破坏 app 循环 |
| **⑤ EDX 路由** | **功能号 1/2/3…** | 一中断 **多 API** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 第一个 API | [notes/section-20.1-第一个-API.md](./notes/section-20.1-第一个-API.md) |
| INT 0x40 | [notes/section-20.2-INT-0x40.md](./notes/section-20.2-INT-0x40.md) |
| 自由运行任意应用程序 | [notes/section-20.3-自由运行任意应用程序.md](./notes/section-20.3-自由运行任意应用程序.md) |
| 寄存器保护 | [notes/section-20.4-寄存器保护.md](./notes/section-20.4-寄存器保护.md) |
| 扩展 API | [notes/section-20.5-扩展-API.md](./notes/section-20.5-扩展-API.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 首个 API 干嘛？ | **Console 出字符** |
| 如何回到 Shell？ | OS **far-CALL** · app **RETF** |
| 为何 INT？ | **OS 改版 app 不碎** · 指令更短 |
| 向量号？ | **0x40**（IDT 空闲槽） |
| 任意程序？ | **`cmd_app`** → **`xxx.hrb`** |
| 只显示一字？ | C 改寄存器 → **PUSHAD/POPAD** |
| 多 API？ | **`EDX` = 功能号** 分发 |

**里程碑：** **可扩展、稳定的用户态接口** — 内核与 **.hrb 生态** 正式分层。

---

---

## 本日学习目标 · 自检

- [ ] 对比 **硬编码 CALL vs INT 0x40**
- [ ] 说清 **far-CALL / RETF** 与 Day 19 **farjmp** 差别
- [ ] 描述 **`cmd_app`** 与内置命令分支
- [ ] 解释 **PUSHAD/POPAD** 与 ECX bug
- [ ] 列举 **EDX=1/2/3** 并理解 BIOS 式路由

---

← [Day 19](./day-19-应用程序.md) · [01 导读](../README.md) · [Day 21](./day-21-保护操作系统.md)

---

## 相关

- 上一日：[../day-19-apps/](../day-19-apps/)
- 下一日：[../day-21-protection/](../day-21-protection/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
