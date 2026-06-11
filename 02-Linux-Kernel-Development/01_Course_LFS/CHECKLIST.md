# LFS 跟做清单与避坑要点

> [01_Course_LFS](./README.md) · 配合 [OUTLINE.md](./OUTLINE.md) 分集笔记使用

---

## 环境准备（开做前一次性）

- [ ] 虚拟机分配 ≥ 20GB 磁盘、≥ 4GB RAM（编译内核建议 8GB）
- [ ] 宿主安装：gcc、make、wget、bc、bison、flex 等 LFS 前置包
- [ ] 单独分区或虚拟磁盘给 LFS 构建，**不要**在主系统根分区直接 chroot 实验
- [ ] 克隆/下载 课程脚本仓库 + 对照 [官方 LFS stable](https://www.linuxfromscratch.org/lfs/view/stable/) 版本

---

## 按阶段操作清单

### p0 概述 — 先建立心智模型

- [ ] 画一遍启动链：**BIOS → MBR/GPT → Bootloader → vmlinuz → init**
- [ ] 理解 LFS 目标：最小可启动 Linux，而非发行版安装
- [ ] 笔记记录：硬盘分区、`/boot`、内核镜像、`init` 进程各是什么

### p1–p3 准备工作

- [ ] 检查脚本权限与路径（`$LFS`、`$LFS_TGT` 等变量是否一致）
- [ ] 源码包校验和/版本与手册一致，避免混版本
- [ ] 编译环境版本：宿主 glibc/gcc 不能过旧（手册有最低要求）

**避坑：** 脚本里硬编码路径与本地目录不一致 → 全局搜 `$LFS` 再跑。

### p4–p5 工具链 + chroot

- [ ] p4：交叉/临时工具链编译失败时先看 `config.log`，常见缺 dev 包
- [ ] p5：`chroot` 前确认挂载 `proc`、`sysfs`、`dev`（`mount -t proc ...`）
- [ ] 进入 chroot 后 `echo $LFS` 与根目录内容是否预期

**避坑：** 忘记 mount proc/sys/dev → chroot 内编译或设备节点异常。

### p6–p9 系统构建

- [ ] 按手册顺序装包，**不要跳步**（后面包依赖前面）
- [ ] 每装完一个大包做简单 smoke test（如 `gcc --version`）
- [ ] p9 调试：保存完整构建日志，便于回溯

**避坑：** 磁盘满 — 编译内核/ glibc 前 `df -h`；tmpfs 不足时调大或改用磁盘目录。

### p10–p14 完善（内核 + 引导 · HFT 最相关）

- [ ] p10：时区、locale、网络配置（若需下载）
- [ ] p11–p12：**内核编译** — `make menuconfig` 至少认识 Processor / Networking / File systems
- [ ] 内核安装：`make modules_install` + `make install` 或手动复制 `vmlinuz` 到 `/boot`
- [ ] p13：GRUB 安装到正确磁盘（`grub-install` 目标设备别选错）
- [ ] p14：重启进新系统，验证 `uname -a`、基本命令

**避坑：**

| 问题 | 处理 |
|------|------|
| 内核 panic 找不到 root | 检查 `root=` 内核参数、initramfs、分区 UUID |
| GRUB 进不去 | Live CD  chroot 回去修 GRUB |
| 模块缺失 | 是否执行 `depmod`、模块路径是否在正确内核版本下 |

### p15 BusyBox 快速演示（选做）

- [ ] 理解 BusyBox 单二进制多命令 vs 完整 GNU 工具链
- [ ] 对比完整 LFS 与极简路径的体积/功能差异

---

## 与 LKD 第三版的衔接检查

跟完 p10–p14 后，读 Love 书应能直接对应：

- [ ] Ch 2 内核入门：`menuconfig`、编译、安装不再陌生
- [ ] Ch 1 简介：用户态/内核态、系统调用有整机构建直觉
- [ ] Ch 5 系统调用：知道 glibc 如何包装 syscall

→ [LEARNING-PATH.md § LFS](../LEARNING-PATH.md#1-lfs--编译系统上下文用户态内核态)

---

## HFT 延伸（可选）

- [ ] 内核配置：关闭不需要的子系统、模块变内置（减少启动与 lookup 开销）
- [ ] 记录当前 `.config` 与 HFT 裸机常用选项差异，供 [10-HFT ch05](../../10-HFT-Low-Latency-Practice/chapter-05-操作系统内核极致调优.md) 对照
