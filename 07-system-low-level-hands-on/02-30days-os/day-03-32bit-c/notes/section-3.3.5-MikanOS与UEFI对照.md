## ③.5 MikanOS 与 UEFI 对照

仓库 [01-mikan-os](../../../01-mikan-os/) 对应 **《ゼロからの OS 自作入門》**（**MikanOS**，**x86-64 64 位**）。

| | **02 川合 haribote** | **01 MikanOS** |
|---|---------------------|----------------|
| **CPU 模式** | **32 位保护模式** 止步 | 内核 **64 位长模式** |
| **引导** | **BIOS** + 软盘 **`INT 0x13`** + 512 B IPL | **UEFI** + **MikanLoader.efi** + **kernel.elf** |
| **第一行你的代码** | 16 位 [ipl.asm](../code/sec-3.1-ipl-int13-disk-load/ipl.asm) | **UEFI 环境里 64 位 C/C++** |
| **汇编量** | **多** — IPL、nasmhead、asmfunc | **少很多** — UEFI 代劳；内核仍有少量 asm |
| **分页** | Day 12+ **32 位二级页表** | Ch8/Ch19 **PML4 四级页表** |

---

### MikanOS 是不是多一步「32 → 64」？

| 路径 | 有没有手写 32→64 |
|------|------------------|
| **经典裸机（Linux 早期）** | **有** — 16 → 32 → CPUID → EFER.LME → long mode |
| **MikanOS（x64 UEFI）** | **通常不用你从 16 位爬上来** — **`BOOTX64.EFI` 已在 64 位长模式** |

MikanOS **不等于**「在 haribote 后面加一段 32→64 汇编」— **引导机制换了（UEFI）**。

---

### UEFI 怎么理解？

**省掉的是你自己写的 16 位 BIOS 那套** — 不用 512 B IPL、不用 **`INT 0x13/0x10`**（对 **x64 UEFI 应用**）。

| 场景 | 你的代码要不要「先 32 再 64」 |
|------|--------------------------------|
| **x64 `BOOTX64.EFI`（MikanLoader）** | **通常不要** — 直接 **64 位 C/C++** |
| **IA32 UEFI 拉 64 位内核** | **可能要** — Loader 自己写 32→64 |
| **经典 BIOS（02 川合）** | **要** — 16 → 32（haribote 停在这里） |

```text
传统 01：你写 16 位 IPL → nasmhead 切 32 位 → HariMain
UEFI x64：固件 → BOOTX64.EFI（64 位）→ MikanLoader → kernel.elf
```

**一句话：** UEFI **把早期阶梯藏在固件里**；你 **通常跳过手写 16→32→64**，在 **64 位 + Boot Services** 上写 Loader，再 **`ExitBootServices`** 后跑内核 — **分页、IDT、APIC 仍要自己搭**。

→ [01-mikan-os Ch1 UEFI 启动流程](../../../01-mikan-os/chapter-01-hello-world/notes/section-5-UEFI启动流程.md)

**学习顺序：**

| 路线 | 建议 |
|------|------|
| **HFT 主线** | **跳过 02-30days-os**，C 达标后直接 [01-mikan-os](../../../01-mikan-os/) |
| **通用零基础** | 可选先 **01 Day 3** 建立「16 Load → 32 Run」图景，再开 MikanOS |

→ [HFT-AND-EMBEDDED-PRIORITY.md](../../../HFT-AND-EMBEDDED-PRIORITY.md)

---

← [§3.3.4 16→32→64](./section-3.3.4-16-32-64阶梯.md) · [§3.3.6 引入 C →](./section-3.3.6-引入C与嵌入式HFT.md)
