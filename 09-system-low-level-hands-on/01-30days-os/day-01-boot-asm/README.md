# Day 1 · 从计算机结构到汇编入门


> **原书第一章** · 目标：打破「写 OS 很难」的恐惧 — **黑屏白字** 先跑起来，再从机器码跨入汇编。

---

### 本节四段结构

| 段 | 做什么 | 带走什么 |
|----|--------|----------|
| **① 先动手** | 二进制编辑器 → `helloos.img` | 1.44MB 软盘映像能 **boot 出 hello, world** |
| **② 理解底层** | CPU 只认 0/1 电信号 | 手工输入的十六进制 = **机器语言指令** |
| **③ 汇编入门** | 装 **NASM** + `helloos.nas` 编译 | 同一份 `.img`；下载教程在 [1.3](./notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask) |
| **④ 润色 + 术语** | `TIMES` / `$` / `DW 0xAA55`；512 B 嵌入 1.44 MB | **IPL**、**Boot**、引导扇区 vs 整盘 |

---

## 小节笔记

| 段 | 笔记 |
|----|------|
| 先动手操作 | [notes/section-1.1-先动手操作.md](./notes/section-1.1-先动手操作.md)（导读） |
| └ 1.1.1 准备工具 · HxD | [notes/section-1.1.1-准备工具-HxD.md](./notes/section-1.1.1-准备工具-HxD.md) |
| └ 1.1.2 HxD 界面与新建映像 | [notes/section-1.1.2-HxD界面与新建映像.md](./notes/section-1.1.2-HxD界面与新建映像.md) |
| └ 1.1.3 写入引导扇区机器码 | [notes/section-1.1.3-写入引导扇区机器码.md](./notes/section-1.1.3-写入引导扇区机器码.md) |
| └ 1.1.4 启动签名与自检 | [notes/section-1.1.4-启动签名与自检.md](./notes/section-1.1.4-启动签名与自检.md) |
| └ 1.1.5 QEMU 安装与运行 | [notes/section-1.1.5-QEMU安装与运行.md](./notes/section-1.1.5-QEMU安装与运行.md) |
| └ 1.1.6 启动链路与排错 | [notes/section-1.1.6-启动链路与排错.md](./notes/section-1.1.6-启动链路与排错.md) |
| **实验文件** | [code/](./code/) — `helloos.img` · hex 对照 |
| 究竟做了些什么 | [notes/section-1.2-究竟做了些什么.md](./notes/section-1.2-究竟做了些什么.md) |
| 初次体验汇编程序（含 **NASM 安装**） | [notes/section-1.3-初次体验汇编程序.md](./notes/section-1.3-初次体验汇编程序.md) |
| 加工润色 | [notes/section-1.4-加工润色.md](./notes/section-1.4-加工润色.md) |
| 关键术语 | [notes/section-1.5-关键术语.md](./notes/section-1.5-关键术语.md) |

---

## 本日小结

| 问题 | 答案 |
|------|------|
| 第一天做出了什么？ | **1.44MB 软盘映像**，启动后 **hello, world** |
| 为什么先玩二进制编辑器？ | 证明 **OS 底层就是字节**；消除神秘感 |
| 为什么马上上汇编？ | 机器码 **不可规模维护**；**NASM** 生成 **相同 img** |
| NASM 和 HxD 文件大小？ | **`nasm` → 512 B `ipl.bin`**；**1.44 MB** 整盘要再把引导扇区 **贴到映像偏移 0** — 见 [1.4](./notes/section-1.4-加工润色.md) |
| 「加工润色」省什么？ | **`TIMES` 填 0** · **`$`/`$$` 算偏移** · **`DW 0xAA55` 写魔数** — 不用手算 hex |

**本章目的：** 用 **可见成果** 激发兴趣，从 **机器语言** 跨入 **汇编** — 为后续保护模式、GDT、中断打地基。

---

---

## 本日学习目标 · 自检

- [ ] **（推荐先做）** 用 HxD 手工做 `helloos.img` 并在 QEMU 看到 `hello, world` — [section 1.1](./notes/section-1.1-先动手操作.md)
- [ ] 按 [1.3 安装 NASM](./notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask) 并完成 `nasm -v`
- [ ] 能用 **`nasm -f bin helloos.nas -o ipl.bin`** 得到 **512 B** 引导扇区，并与 HxD 版 hex 对照
- [ ] 读 [1.4 加工润色](./notes/section-1.4-加工润色.md)：说清 **`TIMES` / `$` / `$$` / `0xAA55`** 与 **512 B vs 1.44 MB**
- [ ] 说清 **1440KB（1,474,560 B）** 与 **512 字节引导扇区** 的关系
- [ ] 解释 **IPL** 与 **boot** 的含义
- [ ] 知道 **`55 AA`** 在引导扇区 **偏移 `0x1FE`**（第 511、512 字节）

---

---

## 相关

- 下一日：[../day-02-asm-makefile/](../day-02-asm-makefile/)
- 模块导读：[../../README.md](../../README.md) · [../../OUTLINE.md](../../OUTLINE.md)
