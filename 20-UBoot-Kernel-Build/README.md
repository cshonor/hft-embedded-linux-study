# U-Boot · 内核裁剪 · 根文件系统构建

**文件夹 20** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线19–24)

> **定位：** 嵌入式 **自己编译** 引导链 + 内核 + rootfs — 区别于服务器 **成品内核**。  
> **前置：** [19 ARM 汇编](../19-ARM64-Architecture/) · [04 LKD](../04-Linux-Kernel-Development/) 调度/中断概念  
> **书目原则：** **全外文** — 无国产《嵌入式 Linux 开发实战》等。

---

## 必读书（2 本 · 按路线图顺序）

| 序 | 书目 | 读什么 |
|----|------|--------|
| **1** | ***Mastering Embedded Linux Programming***, 2nd ed — Chris Simmonds | **系统编译实操** — Yocto/Buildroot、工具链、rootfs、应用部署 |
| **2** | ***Embedded Linux Primer***, 2nd ed — Christopher Hallinan | **启动底层原理** — Bootloader、内核启动流程、嵌入式 Linux 全貌 |

**建议读法：** 以 **Simmonds 动手搭系统** 为主线；遇「为什么这样启动」时 **回查 Hallinan** 补原理。与 MikanOS **UEFI → Loader → kernel** 链可对照（x86 UEFI vs ARM U-Boot）。

---

## 复用（HFT 链直接搬）

| 模块 | 复用什么 |
|------|----------|
| [04 LKD](../04-Linux-Kernel-Development/) | Kconfig、模块、启动流程 |
| [05 ULK](../05-Understanding-Linux-Kernel/) | 启动初期内存、中断初始化 |
| [06 Gorman](../06-Linux-Virtual-Memory-Manager/) | 页表、ZONE — 裁剪内核时懂删什么 |
| [07 TLPI](../07-The-Linux-Programming-Interface/) | rootfs 里用户态程序仍走 syscall |
| [08 MikanOS](../08-system-low-level-hands-on/01-mikan-os/) | GetMemoryMap / GOP / ELF 加载 — **概念同源** |

**差异一句话：** 服务器用 **发行版内核**；嵌入式用 **板级 defconfig + 设备树 + Buildroot/Yocto**。

---

## 典型构建链

```
ROM/SPL → U-Boot → 加载 zImage/Image + DTB
                        ↓
                   Linux 内核（裁剪）
                        ↓
                   rootfs（Buildroot / Yocto）
```

---

## 验收

- [ ] 能说明 **U-Boot → 内核 → DTB → rootfs** 启动顺序  
- [ ] 会用 **menuconfig/defconfig** 裁剪无关子系统  
- [ ] 能产出可启动的 **最小 rootfs**  

**上一章：** [19 ARM 汇编](../19-ARM64-Architecture/) · **下一章：** [21 Linux 驱动](../21-Linux-Device-Driver/)
