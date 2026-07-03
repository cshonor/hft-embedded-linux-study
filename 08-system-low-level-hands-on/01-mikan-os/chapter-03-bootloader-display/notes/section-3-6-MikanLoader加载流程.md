# Ch 3 §3.6 MikanLoader 加载 kernel.elf 流程

> **MikanOS** · 原书第 3 章

把 [§3.2 执行视图](./section-3-2-ELF三大结构与链接执行双视图.md) 的 **Program Header** 落成 **MikanLoader 代码**。

---

## 与 02 川合 OS 对照

| | **01 约 Day 4–5** | **MikanOS Ch 3** |
|---|-------------------|------------------|
| 内核格式 | 二进制 flat / 自定义 | **ELF** |
| 加载者 | 引导扇区 / asm | **UEFI MikanLoader** |
| 首屏 | VGA 文本 | **GOP 像素帧缓冲** |

---

## Loader 加载流程

```
1. EFI_FILE_PROTOCOL 读取 U 盘上 kernel.elf 到缓冲
2. 解析 ELF Header — 魔数 7F 45 4C 46、架构 x86-64
3. 遍历 Program Headers — 对每个 PT_LOAD：
      gBS->AllocatePages（须在 Conventional 等可用区）
      拷贝段内容到 p_paddr / p_vaddr
      若 p_memsz > p_filesz → 清零 BSS 部分
4. 取 e_entry → 函数指针
5. 准备 KernelMain 参数（GOP 帧缓冲等 — 见 §4）
6. 跳转 — 不再返回 EfiMain
```

| 步骤 | 失败点 |
|------|--------|
| 读文件 | 路径错误、FAT 无 `kernel.elf` |
| 分配页 | **内存不足** — `EFI_STATUS`（§5） |
| 解析 ELF | 魔数错、无 **PT_LOAD** |
| 跳转 | **`e_entry` / 链接脚本** 错 → RIP 乱飞 |

---

## 代码层对应关系

| ELF 概念 | MikanLoader 动作 |
|----------|------------------|
| **ELF Header** | 校验魔数、读 **`e_entry`** |
| **Program Header `PT_LOAD`** | **AllocatePages + memcpy** |
| **Section Header** | **本阶段不读** — 调试用 `readelf` 即可 |
| **`KernelMain` 参数** | GOP 查询结果 **按调用约定** 传入 |

---

## 架构图（本章总览）

```
┌─────────────────────────────────────┐
│  MikanLoader (UEFI .efi / PE)       │
│  · GetMemoryMap（Ch2）               │
│  · GOP → 帧缓冲信息（§4）            │
│  · 读 kernel.elf · 解析 PHT         │
│  · KernelMain(fb_base, fb_size, …)  │
└──────────────┬──────────────────────┘
               │ jump(e_entry)
               ▼
┌─────────────────────────────────────┐
│  kernel.elf (ELF)                   │
│  · KernelMain — 后期像素绘图         │
│  · 初期：hlt 死循环                  │
└─────────────────────────────────────┘
```

→ 衔接 [Ch2 §2.3 MikanLoader](../../chapter-02-edk2-memmap/notes/section-2-3-MikanLoader是什么.md)

---

## 自检

- [ ] 能对照 **`readelf -l`** 输出解释 Loader **加载了哪几段**
- [ ] 能说出 **为何 Loader 不读 Section Header**
- [ ] 知道加载地址要避开 [Ch2 固件/MMIO 区](../../chapter-02-edk2-memmap/notes/section-3-2-RAM四层占用.md)

---

← [3.5 常见问题](./section-3-5-与vmlinux对比及常见问题.md) · [§3 索引](./section-3-第一个内核与ELF加载.md) · 下一节 [§4 GOP](./section-4-GOP与帧缓冲区.md)
