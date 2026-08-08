# P5b — U-Boot → kernel → rootfs 启动到 shell

> 让树莓派 5 从上电一路启动到能登录的 Linux shell，亲手走完 boot chain 每一环。

## 项目目标

打通嵌入式 Linux 的"启动三件套"：bootloader（U-Boot）加载内核、内核挂载根文件系统、init 起用户态。每一步都能解释发生了什么，而不是烧现成镜像。

## 交付物

- [ ] U-Boot 编译（AArch64 交叉工具链），SD 卡分区布局
- [ ] U-Boot 环境变量：`bootcmd`/`bootargs`，加载 kernel + dtb
- [ ] Linux 内核编译（`defconfig` + 树莓派配置）
- [ ] rootfs：BusyBox 或 debootstrap 最小根文件系统
- [ ] 设备树 blob（dtb），串口 + SD + USB 基本可用
- [ ] 上电 → U-Boot → kernel → `/#` shell

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`11` embedded-boot-build](../../../11-embedded-boot-build/) | Primer 启动全貌、MELP 构建/Yocto、U-Boot、内核构建 |

## 前置

[P5a](../P5a-qemu-uart-hello/)（裸机启动直觉）。

## 学习目标

- ARM Linux 启动协议（内核入口、dtb 传递、ATAGS→FDT）
- U-Boot 的 bootcmd/bootargs 与环境变量持久化
- 内核配置裁剪、模块 vs 内建、initramfs
- rootfs 最小组成（/dev、/bin、/lib、init）
- bootargs `console=` `root=` `rw` 的含义

## 里程碑

1. **M1** U-Boot 串口交互，能 `printenv`/`boot`
2. **M2** U-Boot 加载内核 + dtb，内核开始打印日志
3. **M3** rootfs 挂载，init 起来，登录 shell

## 参考模块

- [11-embedded-boot-build/](../../../11-embedded-boot-build/) — Embedded Linux Primer、Mastering Embedded Linux Programming
