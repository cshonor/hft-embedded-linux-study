# 02 · MikanOS · ゼロからの OS 自作入門

**09-system-low-level-hands-on** 子模块 · [返回 09 总览](../README.md)

> **来源：** 内田公太（uchan）· [ゼロからの OS 自作入門](https://book.mynavi.jp/ec/products/detail/id=121220) · 官方 [zero.osdev.jp](http://zero.osdev.jp/) · 源码 [uchan-nos/os-from-zero](https://github.com/uchan-nos/os-from-zero)  
> **定位：** **现代 64 位 UEFI OS** — 与 [01 30 天 OS](../01-30days-os/)（实模式 BIOS 软盘）**互补**，不替代 Linux 主线。

---

## 与 01 的分工

| | **01 川合 30 天** | **02 MikanOS** |
|---|---------------------|------------------|
| 启动 | BIOS · 软盘 · 实模式 → 保护模式 | **UEFI** · GPT · **长模式** |
| 语言 | C + 汇编（nask） | **C++** + EDK II |
| 内存 | 分段/GDT · 后期分页 | **内存 map** · **页表** · 进程地址空间 |
| 中断 | PIC · IDT | **APIC** · ACPI |
| 价值 | 「上电后第一条指令」体感 | **现代 PC 启动链** + 规范分层内核 |

**推荐顺序：** 至少完成 **01 Day 1–15**（引导、GDT/IDT、中断、多任务雏形）→ 再开 MikanOS；或 **01 通读后** 整本 MikanOS 作「现代版重制」。

**交叉：** [01-CSAPP](../01-CSAPP-3rd/) Ch 9 虚拟内存 · [04-Hennessy](../04-Computer-Architecture-6th/) · [07-TLPI](../07-The-Linux-Programming-Interface/) 进程/内存 API 对照。

---

## 文档

| 文件 | 说明 |
|------|------|
| [OUTLINE.md](./OUTLINE.md) | 第 0–31 章 + 附录索引 |
| [LEARNING_PLAN.md](./LEARNING_PLAN.md) | 阶段划分 · 与 01 衔接 · 避坑 |
| [SETUP.md](./SETUP.md) | WSL2 / EDK II / QEMU(OVMF) 环境 |

---

## 目录约定（与 01 对齐）

```
02-mikan-os/
├── README.md · OUTLINE.md · LEARNING_PLAN.md · SETUP.md
├── assets/                    # 截图
├── chapter-XX-slug/           # 按书章（例 chapter-19-paging）
│   ├── README.md              # 章导读 + 官方 osbook_dayXX 标签
│   ├── notes/                 # section 笔记
│   └── code/                  # 本书快照 / diff（非完整上游 clone）
└── code/                      # 可选：链到 os-from-zero  tag
```

> **代码：** 完整工程以官方 GitHub 为准；本仓库 `chapter-XX/code/` 只存 **对照用快照** 与笔记链接，不 fork 全书 744 页全部二进制。

---

## 进度

- [ ] 环境 [SETUP.md](./SETUP.md)
- [ ] Ch 0–2 UEFI + 内存 map
- [ ] Ch 7–8 中断 + 内存管理
- [ ] Ch 13–14 多任务
- [ ] Ch 19–20 **分页 + 系统调用**（与 CSAPP / LKD 强相关）
- [ ] Ch 29 应用间通信（→ 远期 IPC 模块对照）
