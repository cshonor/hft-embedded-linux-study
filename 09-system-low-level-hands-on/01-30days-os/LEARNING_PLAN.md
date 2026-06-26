# 《30 天自制操作系统》· 完整标准学习方案

> **原著：** 川合秀实（KAWAKAMI Hideaki）· **Osliver**  
> **本仓库：** 与 [OUTLINE.md](./OUTLINE.md)（🔴/🟡/⚪ 裁剪）和 `day-XX-slug/` 笔记配套使用

---

## 前置结论

**完全不需要外接光驱。** 全书配套 **tolset** 工具链、每日案例源码与编译脚本；获取原书配套资源包后解压即可在 **Windows + QEMU** 上完成全部演练。

环境部署见 **[SETUP.md](./SETUP.md)**（Day 0）。

---

## 一、先核对自身前置基础

本书混合三套核心技术，基础储备决定上手顺畅度：

| 技能 | 用途 | 对应笔记 |
|------|------|----------|
| **x86 16 位实模式汇编（NASM 语法）** | Day 1–2 引导扇区、Day 3 前 IPL | [day-01-boot-asm/](./day-01-boot-asm/) · [day-02-asm-makefile/](./day-02-asm-makefile/) |
| **标准 C 语言** | Day 3 起内核主体；汇编只管引导与底层 | [day-03-32bit-c/](./day-03-32bit-c/) 起 |
| **基础 Makefile** | 多文件编译、链接、镜像打包 | [day-02-asm-makefile/](./day-02-asm-makefile/) |

**若你已有 C++/体系结构背景：** C 与内存布局通常不是门槛，主要补齐 **16 位汇编 + NASM + make** 即可。可与 [01-CSAPP Ch3](../../01-CSAPP-3rd/chapter-03-machine-level-programs/) 并行。

**工具链：** 全程 **NASM + GCC + Make**（不用原书魔改 **nask/bcc**）— 见 [TOOLCHAIN.md](./TOOLCHAIN.md)。

---

## 二、三阶段划分（30 天循序渐进）

### 第一周期 · Day 0–7 · 地基建设（重中之重）

| 阶段 | 内容 | 笔记 |
|------|------|------|
| **Day 0** | NASM/GCC/Make/QEMU 部署、路径规范、能 boot 映像 | [SETUP.md](./SETUP.md) · [TOOLCHAIN.md](./TOOLCHAIN.md) |
| **Day 1–2** | 512 B 引导扇区、`0x7C00`、软盘结构、**NASM** + Makefile → `.img` | [day-01](./day-01-boot-asm/) · [day-02](./day-02-asm-makefile/) |
| **Day 3** | **分水岭**：实模式 → **32 位保护模式**，正式引入 C | [day-03](./day-03-32bit-c/) |
| **Day 4–7** | 显存绘图、GDT/IDT、键鼠中断、FIFO 环形缓冲 | [day-04](./day-04-c-graphics/) … [day-07](./day-07-fifo-mouse/) |

**周期末收获：** 可中断、可输入、有图形输出的 **最小内核骨架**。

### 第二周期 · Day 8–14 · 内核功能完善

| 主题 | 笔记 |
|------|------|
| 32 位切换收尾、内存管理、图层叠加 | [day-08](./day-08-mouse-32bit/) · [day-09](./day-09-memory/) · [day-10](./day-10-layers/) |
| 窗口、PIT 定时器、高分辨率与键盘 | [day-11](./day-11-window/) · [day-12](./day-12-timer1/) · [day-13](./day-13-timer2/) · [day-14](./day-14-keyboard/) |

**周期末收获：** 带定时与图层的多任务 **调度雏形**。

### 第三周期 · Day 15–30 · 完整 OS 生态

| 主题 | 笔记 |
|------|------|
| 多任务、命令行、FAT/文件、应用程序 | [day-15](./day-15-multitask1/) … [day-19](./day-19-apps/) |
| INT 0x40 API、保护环、用户态 C 程序 | [day-20](./day-20-api/) … [day-22](./day-22-c-apps/) |
| GUI、窗口管理、LDT/库、压缩与成品应用 | [day-23](./day-23-graphics/) … [day-30](./day-30-advanced-apps/) |

**周期末收获：** 可独立启动的 **Haribote OS** — 图形界面、进程、外设、简易 Shell 与 API。

---

## 三、正确学习方法（避坑）

1. **严格按天迭代，不要跳章**  
   每天在前一天源码上增量修改；跳过任意一天易导致 Makefile / 链接布局断裂。建议各 `day-XX-slug/code/` 保留每日可编译快照。

2. **先跑通，再深挖原理**  
   作者路线是「先看见效果，再拆 CPU/BIOS/保护模式」。与笔记里的 **HFT 对照** 可第二遍精读时做。

3. **全程 QEMU，不要 U 盘真机引导**  
   原书软盘/U 盘启动在现代学习环境非必要；QEMU 加载 `.img` 重启快、可脚本化。

4. **工具链用 NASM + GCC 通用链**  
   原书 **nask** 是作者基于 NASM 风格的魔改汇编器；**直接用 NASM** 即可，既能生成与 Day 1 手工 hex 一致的机器码，又能与 **GCC、Makefile** 配合，从引导扇区一直写到带 C 的内核，后续学 Linux 底层也能复用。tolset 仅作对照可选。

5. **路径禁中文与空格**  
   tolset 与部分批处理对非 ASCII 路径敏感；工程根目录用纯英文，例如 `C:\dev\haribote\`。

---

## 四、与本仓库学习链的衔接

```
01 自制 OS（作者视角：引导、中断、页表、任务切换）
    ↓
05 LKD / 07 Gorman（Linux 真实实现对照）
    ↓
08 TLPI（用户态 syscall 边界）
    ↓
15 HFT（绑核、热路径、少 syscall）
```

| 原书机制 | 仓库对照 |
|----------|----------|
| GDT/IDT/中断 | [05-LKD Ch7–8](../../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-07-interrupts/) |
| 调度/上下文切换 | [05-LKD Ch4](../../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-04-process-scheduling/) |
| INT 0x40 API | [05-LKD Ch5 syscall](../../05-Linux-Kernel-Development/00_Book_3rd_Notes/chapter-05-system-calls/) · [08-TLPI](../../08-The-Linux-Programming-Interface/) |

**后续可选：** 吃透本书后读 Linux 内核源码；或将内核逻辑用 **Rust** 重写，对接 [15-Rust](../../15-Rust/) 路线。

---

## 五、现阶段第一步行动

- [ ] **零工具链体感（推荐）：** 按 [day-01 section 1.1](./day-01-boot-asm/notes/section-1.1-先动手操作.md) 用 HxD + QEMU 手工做第一个 `helloos.img`
- [ ] 按 [1.3 安装 NASM](./day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#安装-nasm替代-nask) + [SETUP.md](./SETUP.md) 完成 **Day 0**（QEMU；GCC/Make 可 Day 3 前再装）
- [ ] 阅读 [day-01-boot-asm/](./day-01-boot-asm/) 笔记并对照源码
- [ ] 在 [day-01-boot-asm/code/](./day-01-boot-asm/code/) 对照映像；`make run` 验证与 HxD 版一致

---

## 相关文档

| 文档 | 用途 |
|------|------|
| [README.md](./README.md) | 模块导读与 Day 索引 |
| [OUTLINE.md](./OUTLINE.md) | 每日要点速览 + 🔴/🟡/⚪ |
| [SETUP.md](./SETUP.md) | Windows + NASM/GCC/QEMU Day 0 部署 |
| [TOOLCHAIN.md](./TOOLCHAIN.md) | NASM vs nask · GCC · Make |
