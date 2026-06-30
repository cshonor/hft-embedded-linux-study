# U-Boot · 内核裁剪 · 根文件系统构建

**文件夹 19** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线18–23)

> **定位：** 嵌入式 **自己编译** 引导链 + 内核 + rootfs — 区别于服务器 **成品内核**。  
> **前置：** [18 ARM64](../18-ARM64-Architecture/) · [05 LKD](../05-Linux-Kernel-Development/) 调度/中断概念

---

## 必读书（2 本 · 精简）

| # | 书目 | 读什么 |
|---|------|--------|
| 1 | **《嵌入式 Linux 开发实战：U-Boot、内核、根文件系统》** | U-Boot · **defconfig** · 内核裁剪 · 最小 rootfs |
| 2 | **《Buildroot 实战指南》** | **一键构建** rootfs — 工业/无人机常用 |

---

## 复用（HFT 链直接搬）

| 模块 | 复用什么 |
|------|----------|
| [05 LKD](../05-Linux-Kernel-Development/) | Kconfig、模块、启动流程 |
| [06 ULK](../06-Understanding-Linux-Kernel/) | 启动初期内存、中断初始化 |
| [07 Gorman](../07-Linux-Virtual-Memory-Manager/) | 页表、ZONE — 裁剪内核时懂删什么 |
| [08 TLPI](../08-The-Linux-Programming-Interface/) | rootfs 里用户态程序仍走 syscall |

**差异一句话：** 服务器用 **发行版内核**；嵌入式用 **板级 defconfig + 设备树 + Buildroot/Yocto**。

---

## 典型构建链

```
ROM/SPL → U-Boot → 加载 zImage/Image + DTB
                        ↓
                   Linux 内核（裁剪）
                        ↓
                   rootfs（Buildroot）
```

---

## 验收

- [ ] 能说明 **U-Boot → 内核 → DTB → rootfs** 启动顺序  
- [ ] 会用 **menuconfig/defconfig** 裁剪无关子系统（如不需要的 FS/驱动）  
- [ ] 能用 **Buildroot** 产出可启动的最小 rootfs  

**上一章：** [18 ARM64](../18-ARM64-Architecture/) · **下一章：** [20 Linux 驱动](../20-Linux-Device-Driver/)
