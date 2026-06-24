# Day 19 · 应用程序


> **原书第十九章** · **外部程序时代** — **`type`/cat**、FAT 解压与大文件、**window/console/file.c**、**`hlt.hrb` + GDT + farjmp**。

---

### 本节五段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① type** | 簇号 → 地址公式 | **≤512B** 先读通 |
| **② 改良 type** | **`\n` `\r` `\t`** | 文本排版正确 |
| **③ 完整 FAT** | **3B 压 2 扇区** 展开 | 大文件 · **碎片链** |
| **④ 代码整理** | 拆 **`window/console/file.c`** | bootpack 瘦身 |
| **⑤ hlt.hrb** | **`file_loadfile` + GDT 段** | **farjmp 跑 .hrb** |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| type 命令 | [notes/section-19.1-type-命令.md](./notes/section-19.1-type-命令.md) |
| 改良 type | [notes/section-19.2-改良-type.md](./notes/section-19.2-改良-type.md) |
| 完全支持 FAT | [notes/section-19.3-完全支持-FAT.md](./notes/section-19.3-完全支持-FAT.md) |
| 代码整理 | [notes/section-19.4-代码整理.md](./notes/section-19.4-代码整理.md) |
| 第一个应用程序 | [notes/section-19.5-第一个应用程序.md](./notes/section-19.5-第一个应用程序.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| type 首版？ | **clustno×512+0x003e00** · ≤512B |
| 文本排版？ | 处理 **0x0a/0x0d/0x09** |
| 大/碎文件？ | **FAT12 3B→2 簇** 展开 · 链到 **0xFFF** |
| 代码结构？ | **window / console / file.c** |
| 首个 app？ | **`hlt.hrb`** · **GDT 段 + farjmp** |
| 里程碑？ | **任意文件可读 + 外部程序可执行** |

---

---

## 本日学习目标 · 自检

- [ ] 会写 **clustno → 映像地址** 公式
- [ ] 说清 **FAT 链式读** 与 **0xFFF 结束**
- [ ] 知道 **2 簇/3 字节** FAT12 需解压
- [ ] 描述 **loadfile → GDT → farjmp** 三步
- [ ] 串起 Day 18 dir → Day 19 type → .hrb

---

← [Day 18](./day-18-dir命令.md) · [08-1 导读](../README.md) · [Day 20](./day-20-API.md)

---

## 相关

- 上一日：[../day-18-dir/](../day-18-dir/)
- 下一日：[../day-20-api/](../day-20-api/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
