# Ch 3 §3.6 MikanLoader 加载 kernel.elf — 完整六步

> **MikanOS** · 原书第 3 章

把 [§3.2 执行视图](./section-3-2-ELF三大结构与链接执行双视图.md) 的 Program Header 落成代码，并放在 **加载器 · ELF · 内核** 全链路里理解（见 [§3.1 误区与三者关系](./section-3-1-kernel.elf基础定义与核心作用.md#三加载器--elf--内核--三者关系mikanos)）。

---

## 三者各是什么

| | 是什么 | 格式 | 能否自己跑 |
|---|--------|------|------------|
| **磁盘上的 `kernel.elf`** | 操作系统内核 **本体打包** | ELF | ❌ 只是文件 |
| **`bootx64.efi` / MikanLoader** | **操作系统加载器** | PE / `.efi` | ✅ 固件直接执行 |
| **跳转到 `e_entry` 之后** | **内核在内存中运行** | 已是机器码 | ✅ 这才是 OS 启动 |

> **纠正：** 「加载 ELF」≠「OS 已在运行」— 必须 **搬段 + 跳入口**（通常还有 **ExitBootServices**）。

---

## 加载器完整六步（贴合你写的代码）

```
① 读文件
   gBS / EFI_FILE_PROTOCOL
   把磁盘上 kernel.elf 读入内存缓冲区

② 解析 ELF
   校验魔数 7F 45 4C 46
   读 ELF Header（e_entry、Program Header 表位置）
   遍历 Program Header（PT_LOAD）

③ 按段搬运
   对每个 PT_LOAD：
     AllocatePages → EfiConventionalMemory（见 Ch2 memmap）
     memcpy(文件 p_offset → 内存 p_paddr / p_vaddr)

④ 清空 .bss
   若 p_memsz > p_filesz：
     多出来的字节清零（未初始化全局变量）

⑤ ExitBootServices
   释放 UEFI Boot Services 占用的临时资源
   （全书后续会细讲；Ch3 可能部分简化，但逻辑上属于「交给内核前」）

⑥ 跳转内核入口
   函数指针 ← e_entry
   传入 KernelMain 所需参数（GOP 帧缓冲等 — §4）
   不再返回 EfiMain
```

| 步 | 失败时常见现象 |
|----|----------------|
| ① | 找不到 `kernel.elf`、FAT 路径错 |
| ② | 魔数错、不是 **EXEC** 类型 ELF |
| ③ | **AllocatePages** 失败、地址与 [Ch2](../../chapter-02-edk2-memmap/) 冲突 |
| ④ | 变量乱值、莫名崩溃 |
| ⑤ | 未 Exit 就跳内核 → 后续章节才稳定 |
| ⑥ | **`e_entry` 错** → RIP 乱飞、黑屏（用 QEMU `info registers` 查） |

---

## 代码层对应

| ELF 概念 | MikanLoader |
|----------|-------------|
| **ELF Header** | 魔数、`e_entry` |
| **PT_LOAD** | AllocatePages + memcpy |
| **p_memsz > p_filesz** | memset 清零 BSS |
| **Section Header** | **本阶段不读** — 仅 `readelf`/gdb 用 |

**UEFI 固件不会做 ②–⑥** — 它只会 **直接跑 `.efi`**。解析 ELF 是 **Loader 你的责任**。

---

## 与 02 川合 OS 对照

| | **01 Day 4–5** | **MikanOS Ch 3** |
|---|----------------|------------------|
| 内核格式 | flat / 自定义 | **ELF** |
| 加载者 | 引导扇区 | **MikanLoader (.efi)** |
| 首屏 | VGA 文本 | **GOP 帧缓冲** |

---

## 架构图

```
┌─────────────────────────────────────┐
│  MikanLoader (bootx64.efi / PE)     │
│  固件可直接执行                       │
│  · 读 kernel.elf（ELF — 固件不自动跑）│
│  · 解析 PHT · 搬段 · 清 BSS          │
│  · ExitBootServices（交棒前）        │
│  · jump(e_entry) + KernelMain 参数   │
└──────────────┬──────────────────────┘
               │
               ▼
┌─────────────────────────────────────┐
│  内存中的内核（来自 kernel.elf）      │
│  KernelMain · 后期 GOP 绘图          │
└─────────────────────────────────────┘
```

→ [§0 完整流程（五步法）](./section-1-完整流程Miniload-GOP-kernel.md) · [Ch2 §2.3 MikanLoader 定位](../../chapter-02-edk2-memmap/notes/section-2-3-MikanLoader是什么.md)

---

## 自检

- [ ] 能说出 **ELF 通用格式** vs **`kernel.elf` 只是其一**
- [ ] 能说出 **磁盘 ELF ≠ OS 在跑**
- [ ] 能复述 **六步**，并解释 **为何固件不能代替你解析 ELF**
- [ ] 能对照 **`readelf -l`** 与 Loader 搬了哪几段

---

← [3.5 常见问题](./section-3-5-与vmlinux对比及常见问题.md) · [§3 索引](./section-3-第一个内核与ELF加载.md) · 下一节 [§4 GOP](./section-4-GOP与帧缓冲区.md)
