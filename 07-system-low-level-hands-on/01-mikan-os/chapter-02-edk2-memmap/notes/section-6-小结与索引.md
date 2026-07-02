## 6. 小结与索引

---

### 一、本章总结

| 成果 | 说明 |
|------|------|
| **EDK II 工程化** | `<Uefi.h>` — Hello World → **MikanLoader** |
| **物理内存摸底** | **Memory Map** — 空闲 vs 占用 |
| **memmap CSV** | `gBS->GetMemoryMap()` + **EFI_FILE_PROTOCOL** |
| **指针能力** | `->` / `**` — 读懂 UEFI 协议 |

```
MikanLoader
    ├── 仍输出 Hello（验证 EDK II 路径）
    └── 导出 memmap → 后续内核的「物理 RAM 账本」
```

---

### 二、与 Ch 1 / Ch 3 衔接

| 章 | 关系 |
|----|------|
| **Ch 1** | 裸 **EfiMain** 与工具链 — Ch 2 **库化 + _loader 命名_** |
| **Ch 3** | 屏幕显示实践、引导加载器扩展 — MikanLoader **继续演进** |

---

### 三、交叉阅读

| 主题 | 路径 |
|------|------|
| Ch 1 Hello / PE | [chapter-01-hello-world](../chapter-01-hello-world/) |
| EDK II 文件 | [appendix-C-edk2-files](../../appendix-C-edk2-files/) |
| 01 内存相关 | [01 Day 12+ 分页](../../02-30days-os/)（更晚才分页） |
| CSAPP 虚拟内存 | [CSAPP Ch9](../../../01-CSAPP-3rd/chapter-09-virtual-memory/) |
| Ch 8 内存管理 | [chapter-08-memory](../chapter-08-memory/) 🔴 |

---

### 四、后续索引

| Ch2 主题 | 继续读 |
|----------|--------|
| 显示 / Loader | [chapter-03-bootloader-display](../chapter-03-bootloader-display/) 🟡 |
| 物理内存管理 | [chapter-08-memory](../chapter-08-memory/) 🔴 |
| 分页 | [chapter-19-paging](../chapter-19-paging/) 🔴 |

---

← [5. 指针基础](./section-5-C指针基础.md) · [Ch 1](../chapter-01-hello-world/) · [Ch 2 导读](../README.md)
