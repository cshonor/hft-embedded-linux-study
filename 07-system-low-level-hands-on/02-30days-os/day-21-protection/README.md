# Day 21 · 保护操作系统


> **原书第二十一章** · **安全加固** — 段基址 **`0xfe8`**、**C + `_api_*`**、**1003/1004 段隔离**、**INT 0x0d GPE**、**DPL 0x60**。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 字符串 API** | app **DS 基址 @ 0xfe8** | 指针算对物理地址 |
| **② C 应用程序** | **`_api_putchar` + INT 0x40** | **CALL 0x1b + RETF** 引导 |
| **③ 段隔离** | **1003 代码 / 1004 数据** | **DS/SS** 限 app 空间 |
| **④ 异常拦截** | **`inthandler0d`** · **属性 +0x60** | 越权 **CPU 击落** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 攻克字符串 API | [notes/section-21.1-攻克字符串-API.md](./notes/section-21.1-攻克字符串-API.md) |
| 用 C 语言编写应用程序 | [notes/section-21.2-用-C-语言编写应用程序.md](./notes/section-21.2-用-C-语言编写应用程序.md) |
| 隔离内存段 | [notes/section-21.3-隔离内存段.md](./notes/section-21.3-隔离内存段.md) |
| INT 0x0d | [notes/section-21.4-INT-0x0d.md](./notes/section-21.4-INT-0x0d.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 字符串 API 为何错？ | **DS 不对** → **`0xfe8` 存 app 数据基址** |
| C app 怎么调 API？ | **`_api_*` → INT 0x40** |
| C 入口怎么对齐？ | **CALL 0x1b + RETF 桩** |
| 如何防 crack1？ | **1003/1004 段** · **DS/SS 限定** |
| 越权怎么办？ | **INT 0x0d** · **`inthandler0d` 杀 app** |
| 防 MOV DS 偷段？ | 段属性 **低 DPL（+0x60）** |
| 唯一出路？ | **`INT 0x40` API** |

---

---

## 本日学习目标 · 自检

- [ ] 解释 **API 读 app 字符串为何要知 DS 基址**
- [ ] 描述 **C app 的 asm 桩 + _api_ 包装**
- [ ] 说清 **1003/1004 与 crack1 对照实验**
- [ ] 列举 **触发 0x0d** 的两种行为
- [ ] 理解 ** syscall 网关 vs 任意内存写** 的安全模型

---

← [Day 20](./day-20-API.md) · [01 导读](../README.md) · [Day 22](./day-22-用C语言编写应用程序.md)

---

## 相关

- 上一日：[../day-20-api/](../day-20-api/)
- 下一日：[../day-22-c-apps/](../day-22-c-apps/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
