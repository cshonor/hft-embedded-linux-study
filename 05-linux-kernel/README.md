# Linux Kernel Development — 内核开发总目录

**文件夹 05** · LKD 第三版笔记 · [返回总清单](../READING-LIST.md#2-linux-kernel-development--robert-love)

> **书本是主线核心。** 体系化梳理内核机制，查漏补缺。  
> 内核编译/rootfs 实操 → [P3.5 BusyBox 极简 Linux](../projects/P3.5-busybox-minimal-linux/)

---

## 子目录

| 序号 | 文件夹 | 内容 |
|------|--------|------|
| 00 | [00_Book_3rd_Notes](./00_Book_3rd_Notes/) | Love · LKD 第三版 · 20 章 — **主线** |

🗺️ 源码顶层目录 ↔ LKD 章节 → [Ch2 §2.2](./00_Book_3rd_Notes/chapter-02-getting-started/notes/section-2.2-内核源码树.md)

---

## 推荐学习顺序

```
P3.5 BusyBox 极简 Linux（编译内核 + rootfs + 启动链实操）
  ↓
通读 LKD 第三版（在 00_Book_3rd_Notes 补笔记）
```

**正确节奏：** P3.5 建立感性认知 → 书本系统化串联。

---

## HFT 精读捷径（读书阶段）

```
Ch 4 调度 → Ch 7–8 中断/下半部 → Ch 9–10 同步 → Ch 11 定时器
```

内存深读 → [06-Gorman](../06-linux-mm/) · 网络栈 → [14-Rosen](../14-kernel-networking/)

完整 HFT 路线 → [HFT-READING-ROADMAP.md](../HFT-READING-ROADMAP.md)

---

## 与 C 语言笔记的交叉引用

| LKD 章节 | C 语言笔记对应 | 关联内容 |
|----------|---------------|----------|
| Ch5 系统调用 | K&R 7.1 标准输入输出 | syscall vs libc、write(fd) 底层 |
| Ch7-8 中断/下半部 | K&R 7.1 流模型与 FILE 对象 | fd → FILE* → 缓冲 → write(fd) |
| Ch12 内存管理 | Pointers on C ch11 动态内存 | malloc/brk/mmap、C vs Rust 内存安全 |
| Ch15 进程地址空间 | Pointers on C ch18 运行时环境 | ELF 段布局、VMA、栈帧 ABI |
| Ch17 设备与模块 | Pointers on C ch18.5 nm 符号表 | .ko 的 ELF 结构、符号导出、重定位 |
| Ch17 设备与模块 | Pointers on C ch18.6 readelf | .ko 的 ELF 节、重定位段分析 |
| Ch17 设备与模块 | Pointers on C ch18.7 objdump | 反汇编内核模块函数 |
| Ch18 调试 | 嵌入式 C 1.5.1-1.5.5 GDB 详解 | gdb/kgdb/kdb/crash、core dump |
| Ch18 调试 | Pointers on C ch18.8 实战案例 | nm/readelf/objdump 排错工作流 |
| Ch18 调试 | Pointers on C ch18.9 静态vs动态 | 静态分析(nm/readelf/objdump) vs 动态调试(gdb) |

> **学习建议：** C 语言笔记（`00-Linux-Kernel-DPDK-Network-C` 仓库）是内核学习的前置基础。GDB 和 ELF 工具先在用户态掌握，内核侧只需要理解差异（kgdb/crash/ftrace/eBPF）。现代内核调试工具见 [05.5-modern-kernel](../05.5-modern-kernel/)。
