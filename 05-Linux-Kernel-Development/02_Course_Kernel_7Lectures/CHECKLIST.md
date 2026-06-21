# 内核编程课 · 环境搭建与避坑

> [02_Course_Kernel_7Lectures](./README.md) · B站 **内核编程视频教程** · 配合 [OUTLINE.md](./OUTLINE.md)

---

## 一次性环境准备

- [ ] **QEMU** + 磁盘镜像（推荐，避免本机直接引导实验内核）
- [ ] 构建依赖（按发行版二选一或对照安装）：

| 工具/包 | Arch (`pacman`) | Debian/Ubuntu (`apt`) |
|---------|-----------------|------------------------|
| Git | `sudo pacman -S git` | `sudo apt install git` |
| 编译链 | `base-devel` | `build-essential` |
| 内核头/源码 | `linux` / 自行下载 tarball | `linux-source` 或 kernel.org |
| QEMU | `qemu-full` | `qemu-system-x86` |

- [ ] 内核源码版本与模块编译 **KERNELRELEASE** 一致（`uname -r` vs 源码树）
- [ ] 预留 ≥ 20GB 磁盘（源码 + 构建树 + QEMU 镜像）

---

## 按集 checklist

### e1 编译与启动 🔴

- [ ] 安装 Git 与构建依赖（理解 pacman vs apt 差异即可，用自己发行版命令）
- [ ] 下载/解压内核源码，`make menuconfig` 或 `make defconfig`
- [ ] `make -j$(nproc)` 编译；记录耗时与常见缺失包
- [ ] QEMU 或备用方式启动新内核，确认 `uname -a`

**避坑：**

| 问题 | 处理 |
|------|------|
| 缺 bc/bison/flex/openssl | 按编译报错补 dev 包 |
| 模块版本 magic 不匹配 | 模块必须用**同一**内核树编译 |
| 启动 panic | 检查 rootfs、initramfs、QEMU 内核/磁盘参数 |

→ 衔接 LKD [Ch 2 内核入门](../00_Book_3rd_Notes/chapter-02-内核入门.md)

### e2 Linux 内核模块 🔴

- [ ] 最小 `hello.c` + Makefile（`obj-m`）
- [ ] `make` → `insmod` → `dmesg` → `rmmod`
- [ ] 理解模块许可证、`MODULE_*` 宏、初始化/退出函数

**避坑：** 在非匹配内核上 insmod → **Invalid module format**；Secure Boot 下模块签名。

→ LKD [Ch 17 设备与模块](../00_Book_3rd_Notes/chapter-17-设备与模块.md)

### e3 前期准备 🟡

- [ ] 浏览内核源码顶层目录：`arch/` `kernel/` `drivers/` `include/`
- [ ] 配置 `.config`、out-of-tree 模块构建路径

### e4 Linux 驱动程序 🟡

- [ ] 字符设备注册、`file_operations` 骨架
- [ ] 用户态 `open/read/write` 测试（可选）

→ HFT 热路径通常不走自定义驱动，但 **LKM 加载/符号** 概念通用

### e5 BusyBox 极简系统 ⚪

- [ ] 与 [LFS p15](../01_Course_LFS/episode-p15-BusyBox快速演示.md) 对照：BusyBox 单二进制 vs 完整 GNU

### e6 内核调试 🔴

- [ ] QEMU + GDB：`qemu-system-x86_64 -s -S` + `gdb vmlinux`
- [ ] 触发 Oops，读 call trace；了解 KGDB 概念

**避坑：** 调试符号需 `CONFIG_DEBUG_INFO=y`；QEMU 端口 1234 冲突时换端口。

→ LKD [Ch 18 调试](../00_Book_3rd_Notes/chapter-18-调试.md)

---

## 与 LKD / HFT 衔接

| 本课 | 书本 | HFT |
|------|------|-----|
| e1 编译启动 | Ch 2 | 内核裁剪、`menuconfig` 直觉 |
| e2 模块 | Ch 17 | 内核扩展、排障 |
| e6 调试 | Ch 18 | 生产 panic/Oops 分析 |
| e2–e4 上下文 | Ch 7–8 | 中断/下半部（后续架构课深化） |

→ [LEARNING-PATH.md § 内核实操](../LEARNING-PATH.md#2-内核编程视频--6-集--模块进程中段)
