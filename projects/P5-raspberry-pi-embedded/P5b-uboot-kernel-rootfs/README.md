# P5b — U-Boot → kernel → rootfs 启动到 shell

> 让树莓派 5 从上电一路启动到能登录的 Linux shell，亲手走完 boot chain 每一环。
> **做法：项目驱动，[`08`](../../../08-embedded-boot-build/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [MELP ch03 bootloader](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-03-bootloader/) | U-Boot 是什么、bootcmd 怎么写 |
| [MELP ch04 kernel](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-04-configuring-building-kernel/) | 内核配置/编译流程 |
| [MELP ch05 rootfs](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-05-building-root-filesystem/) | rootfs 最小组成 |
| [MELP ch13 booting](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-13-booting-init/) | init 进程、启动序列 |

---

## 项目目标

打通嵌入式 Linux 的"启动三件套"：bootloader 加载内核、内核挂载根文件系统、init 起用户态。每一步都能解释发生了什么，而不是烧现成镜像。

## Phase 1：U-Boot 编译 + SD 卡分区（1-2 小时）

### 做什么

编译 U-Boot for AArch64，分区 SD 卡，让 U-Boot 能从串口交互。

### 分步实现

1. **装交叉工具链**：`sudo apt install gcc-aarch64-linux-gnu bc bison flex libssl-dev`
2. **下载 U-Boot**：`git clone https://source.denx.de/u-boot/u-boot.git`
3. **配置**：
   ```bash
   cd u-boot
   make rpi_5_defconfig   # 或 rpi_4_64_defconfig（取决于板子）
   make CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
   ```
4. **SD 卡分区**：
   ```bash
   # /dev/sdX = 你的 SD 卡
   sudo fdisk /dev/sdX
   # 分区 1: 256MB FAT32 (boot)
   # 分区 2: 剩余 ext4 (rootfs)
   sudo mkfs.vfat /dev/sdX1
   sudo mkfs.ext4 /dev/sdX2
   ```
5. **拷贝 U-Boot**：`sudo cp u-boot.bin /mnt/boot/`
6. **config.txt**（树莓派引导 U-Boot）：
   ```
   kernel=u-boot.bin
   arm_64bit=1
   ```
7. **上电**：串口看到 U-Boot 提示符 `=>`

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| defconfig 名字不对 | 编译失败 | 查 `ls configs/ | grep rpi` |
| SD 卡分区类型错 | 树莓派不引导 | boot 分区必须 FAT32，MBR 分区表 |
| 串口看不到输出 | 黑屏 | USB-TTL 接线：TX/RX/GND，波特率 115200 |
| U-Boot 版本太老 | 不支持 Pi5 | 用最新 mainline 或 rpi 分支 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| U-Boot bootcmd | [MELP ch03](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-03-bootloader/) |
| SD 卡布局 | [MELP ch09 storage](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-09-storage-strategy/) |

---

## Phase 2：Linux 内核编译 + 设备树（2-3 小时）

### 做什么

编译 Linux 内核 for 树莓派，生成 kernel Image + dtb。

### 分步实现

1. **下载内核**：
   ```bash
   git clone --depth=1 https://github.com/raspberrypi/linux.git
   cd linux
   ```
2. **配置**：
   ```bash
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- bcm2712_defconfig  # Pi5
   # 或 bcm2711_defconfig (Pi4)
   ```
3. **编译**：
   ```bash
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image dtbs modules
   ```
4. **拷贝到 SD 卡**：
   ```bash
   sudo cp arch/arm64/boot/Image /mnt/boot/kernel8.img
   sudo cp arch/arm64/boot/dts/broadcom/bcm2712-rpi-5-b.dtb /mnt/boot/
   ```
5. **U-Boot 引导内核**（boot.cmd）：
   ```
   fatload mmc 0:1 ${kernel_addr_r} kernel8.img
   fatload mmc 0:1 ${fdt_addr_r} bcm2712-rpi-5-b.dtb
   setenv bootargs console=serial0,115200 root=/dev/mmcblk0p2 rw rootwait
   booti ${kernel_addr_r} - ${fdt_addr_r}
   ```
6. **编译 boot.scr**：`mkimage -C none -A arm64 -T script -d boot.cmd boot.scr`
7. **上电** → U-Boot 加载内核 → 内核开始打印日志

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| defconfig 选错 | 内核不启动 | Pi5= bcm2712, Pi4=bcm2711, Pi3=bcm2837 |
| bootargs 缺 root= | kernel panic: VFS | 必须告诉内核 rootfs 在哪 |
| dtb 不匹配 | 外设不工作 | dtb 必须和板子型号一致 |
| boot.scr 没编译 | U-Boot 不执行 | `mkimage` 把 cmd 编译成 scr |
| 内核太大放不进 FAT | 文件损坏 | FAT32 单文件 < 4GB，通常够用 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 内核配置 | [MELP ch04](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-04-configuring-building-kernel/) |
| 设备树 | [MELP ch11](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-11-device-drivers-interaction/) |
| bootargs 参数 | [MELP ch13](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-13-booting-init/) |

---

## Phase 3：rootfs + 启动到 shell（2 小时）

### 做什么

用 BusyBox 构建最小 rootfs，让内核挂载后能登录。

### 分步实现

1. **编译 BusyBox**：
   ```bash
   git clone https://git.busybox.net/busybox
   cd busybox
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
   # 开启静态链接：make menuconfig → Settings → Build static binary
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- install
   ```
2. **构建 rootfs 目录**：
   ```bash
   mkdir -p rootfs/{bin,sbin,etc,proc,sys,dev,tmp,root,usr/bin,usr/sbin}
   cp -a _install/* rootfs/
   ```
3. **init 脚本**（rootfs/etc/inittab）：
   ```
   ::sysinit:/bin/mount -t proc proc /proc
   ::sysinit:/bin/mount -t sysfs sys /sys
   ::sysinit:/bin/mount -t devtmpfs dev /dev
   ::sysinit:/bin/hostname mypi
   ttyS0::respawn:/bin/sh
   ::shutdown:/bin/umount -a
   ```
4. **拷贝到 SD 卡**：`sudo cp -a rootfs/* /mnt/rootfs/`
5. **上电** → U-Boot → kernel → BusyBox init → `/#` shell

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| kernel panic: no init | 找不到 /init 或 /sbin/init | 检查 rootfs 里有没有 /sbin/init |
| 串口没 shell | 登录不了 | inittab 里 ttyS0::respawn:/bin/sh |
| /dev 没设备节点 | 无法操作设备 | 挂载 devtmpfs 或手动 mknod |
| BusyBox 动态链接 | 缺 libc | 开静态编译或把 libc 也拷进去 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| rootfs 组成 | [MELP ch05](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-05-building-root-filesystem/) |
| init 流程 | [MELP ch13](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-13-booting-init/) |
| 构建系统对比 | [MELP ch06](../../../08-embedded-boot-build/build-toolchain-yocto/chapter-06-choosing-build-system/) |

---

## 测试验证

```bash
# 上电后串口应该看到：
# 1. 树莓派 GPU 固件输出
# 2. U-Boot 提示符 =>
# 3. U-Boot 执行 boot.scr
# 4. Linux 内核日志 (一大串 [    0.000000] ...)
# 5. BusyBox 启动
# 6. /# 提示符

# 在 shell 里验证：
ls /dev       # 设备节点
cat /proc/cpuinfo  # CPU 信息
uname -a      # 内核版本
```

## 状态

⬜ 未开始 → 建议先装交叉工具链，确认 `aarch64-linux-gnu-gcc --version` 能跑。

← [P5 索引](../README.md) · [11 模块](../../../08-embedded-boot-build/)
