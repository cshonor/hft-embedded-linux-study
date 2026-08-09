# P3.5 BusyBox 极简 Linux — 从零搭一个能启动的系统

> **定位：** P3（用户态并发）→ **P3.5**（系统启动链全景）→ P4（内核模块）
> **目标：** 用 BusyBox + 自己编译的内核，在 QEMU 里搭一个能启动、能跑 shell 的最小 Linux
> **时间：** Phase 1 约 1-2 小时（核心），Phase 2-3 各 1 小时，Phase 4 选做

---

## 为什么做这个

P4 写内核模块时你需要：编译内核、创建 rootfs、理解启动链。与其到 P4 时手忙脚乱，不如先用最小代价（BusyBox 半小时）把这些概念全过一遍。

| 这个项目教的 | P4/P5 哪里用到 |
|-------------|---------------|
| 内核编译（menuconfig → make → bzImage） | P4 加载模块需要匹配内核版本 |
| rootfs 制作（BusyBox + initramfs） | P5b 树莓派 rootfs、P4 测试环境 |
| 启动链（bootloader → kernel → init → userspace） | P5a 裸机启动、P5b U-Boot 启动链 |
| 内核裁剪（关掉不需要的子系统） | HFT 内核调优、P5 嵌入式最小系统 |

---

## 架构概览

```
QEMU (x86_64)
├── -kernel bzImage              ← 你编译的内核镜像
├── -initrd initramfs.cpio.gz    ← BusyBox rootfs 打包成 initramfs
└── -append "console=ttyS0"      ← 内核命令行参数

启动流程：
QEMU 加载 bzImage → 内核初始化 → 解压 initramfs → 执行 /init → BusyBox shell
```

BusyBox 是一个单二进制程序，通过符号链接模拟上百个命令（ls/cp/cat/mount…），是嵌入式 Linux 的标准 rootfs 方案。

---

## Phase 1：内核 + BusyBox → QEMU 启动到 shell（核心）

### 1.1 编译内核

```bash
# 下载内核源码（用 LTS 版本）
wget https://cdn.kernel.org/pub/linux/kernel/v6.x/linux-6.6.tar.xz
tar xf linux-6.6.tar.xz
cd linux-6.6

# 默认配置（x86_64）
make defconfig

# 编译（-j 用满核）
make -j$(nproc)

# 产物：arch/x86/boot/bzImage
ls -lh arch/x86/boot/bzImage
# 大约 10-15MB
```

> **WSL 用户注意：** 内核编译在 WSL Ubuntu 里做。编译产物 `bzImage` 在 Windows 里也能访问（`\\wsl$\Ubuntu\...`）。

### 1.2 编译 BusyBox

```bash
# 下载 BusyBox
wget https://busybox.net/downloads/busybox-1.36.1.tar.bz2
tar xf busybox-1.36.1.tar.bz2
cd busybox-1.36.1

# 默认配置
make defconfig

# 关键：启用静态链接（不依赖 glibc 动态库）
make menuconfig
# → Settings → Build Options → Build static binary (no shared libs)  → 选中

# 编译
make -j$(nproc)

# 产物：busybox（单二进制，约 2MB）
ls -lh busybox
```

> **为什么静态链接？** initramfs 里没有 `/lib` 目录，BusyBox 必须自带所有依赖。嵌入式系统也常这样做。

### 1.3 制作 initramfs

```bash
mkdir rootfs
cd rootfs

# 安装 BusyBox（生成 bin/sbin/usr 等目录 + 符号链接）
make -C ../busybox-1.36.1 install CONFIG_PREFIX=$(pwd)

# 创建必需目录
mkdir -p proc sys dev etc

# 写 init 脚本（内核启动后执行的第一个用户态程序）
cat > init << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

echo "================================"
echo "  Hello from BusyBox minimal Linux!"
echo "  Kernel: $(uname -r)"
echo "================================"

exec /bin/sh
EOF
chmod +x init

cd ..

# 打包成 initramfs（cpio 格式 + gzip）
cd rootfs
find . | cpio -H newc -o | gzip > ../initramfs.cpio.gz
cd ..
```

### 1.4 QEMU 启动

```bash
# 安装 QEMU（WSL 里）
sudo apt install qemu-system-x86

# 启动！
qemu-system-x86_64 \
  -kernel linux-6.6/arch/x86/boot/bzImage \
  -initrd initramfs.cpio.gz \
  -append "console=ttyS0" \
  -nographic \
  -m 512M

# 你应该看到内核启动日志，最后进入 BusyBox shell
# 输入 ls / 看看 rootfs 结构
# 输入 uname -a 确认内核版本
# Ctrl+A 然后按 X 退出 QEMU
```

### Phase 1 常见坑

| 坑 | 原因 | 解决 |
|----|------|------|
| `Kernel panic - not syncing: No init found` | init 脚本没有执行权限 | `chmod +x init` |
| BusyBox 报 `applet not found` | 没跑 `make install` 生成符号链接 | 确认 `CONFIG_PREFIX` 路径正确 |
| 启动卡在 `Booting the kernel` | QEMU 内存太小 | `-m 512M` 或更大 |
| `mount: proc not found` | 忘了 `mount -t proc` | init 脚本里必须有 mount |
| 编译内核报 `flex: not found` | 缺编译依赖 | `sudo apt install build-essential libncurses-dev bison flex libssl-dev` |

### Phase 1 卡住翻哪篇笔记

| 问题 | 翻哪篇 |
|------|--------|
| 内核编译流程 | `07-linux-kernel/00_Book_3rd_Notes/chapter-02-getting-started/notes/section-2.3-编译和安装内核.md` |
| 启动链概念 | `11-embedded-boot-build/primer-system-overview/` |
| CSAPP 异常控制流 | `02-computer-systems/chapter-08-exceptional-control-flow/` |
| initramfs 原理 | [内核文档 initramfs.txt](https://docs.kernel.org/admin-guide/initrd.html) |

---

## Phase 2：内核裁剪 — 理解 menuconfig 每一项在干什么

### 2.1 打开 menuconfig

```bash
cd linux-6.6
make menuconfig
```

### 2.2 关掉不需要的子系统

目标是把内核从 15MB 缩到 5MB 以下，同时理解每个子系统的作用：

| menuconfig 路径 | 关掉什么 | 为什么可以关 |
|-----------------|---------|-------------|
| Device Drivers → Sound card support | 声卡 | 服务器/嵌入式不需要 |
| Device Drivers → GPU/DRM | 显卡驱动 | 纯 console 模式不需要 |
| Device Drivers → USB support | USB | QEMU 不接 USB |
| Filesystems → Network File Systems | NFS/SMB | 单机不需要 |
| Networking → Wireless | WiFi | QEMU 纯有线 |
| Device Drivers → Bluetooth | 蓝牙 | 不需要 |

**不要关的：**

| 必须保留 | 为什么 |
|---------|--------|
| Processor type → 你的 CPU | 关了内核不启动 |
| Block devices → RAM block device | initramfs 依赖 |
| Filesystems → proc / sysfs / tmpfs | 系统运转必需 |
| Character devices → Virtual terminal | console 输出 |
| Executable formats → ELF | 运行 BusyBox 必需 |

### 2.3 重新编译验证

```bash
make -j$(nproc)
ls -lh arch/x86/boot/bzImage
# 看体积变化

# 重新启动测试
qemu-system-x86_64 \
  -kernel arch/x86/boot/bzImage \
  -initrd ../initramfs.cpio.gz \
  -append "console=ttyS0" \
  -nographic -m 512M
```

### Phase 2 常见坑

| 坑 | 原因 | 解决 |
|----|------|------|
| 关完之后内核不启动 | 关了必需项 | `make defconfig` 重来，一次只关一组 |
| menuconfig 选项是灰色 | 有依赖项没关 | 先关依赖项 |
| bzImage 没变小 | 编译没干净 | `make clean` 再 `make` |

---

## Phase 3：自定义 init + 持久磁盘 + 网络

### 3.1 写一个有意义的 init

```bash
cat > rootfs/init << 'INIT_EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev

# 设置 hostname
hostname mybox

# 基本网络（QEMU user networking）
ip link set lo up
ip link set eth0 up
udhcpc -i eth0 2>/dev/null

echo ""
echo "============================"
echo "  $(uname -sr)"
echo "  hostname: $(hostname)"
echo "  uptime: $(cat /proc/uptime)"
echo "  eth0: $(ip addr show eth0 2>/dev/null | grep 'inet ')"
echo "============================"
echo ""

# 自动执行用户脚本（如果有）
if [ -f /etc/rc.local ]; then
    /etc/rc.local
fi

exec /bin/sh
INIT_EOF
chmod +x rootfs/init
```

### 3.2 加持久磁盘（ext4 镜像）

```bash
# 创建一个 100MB 磁盘镜像
dd if=/dev/zero of=disk.img bs=1M count=100
mkfs.ext4 disk.img

# QEMU 启动时挂载
qemu-system-x86_64 \
  -kernel linux-6.6/arch/x86/boot/bzImage \
  -initrd initramfs.cpio.gz \
  -append "console=ttyS0" \
  -nographic -m 512M \
  -drive file=disk.img,format=raw,if=virtio

# 在 shell 里手动挂载
mount /dev/vda /mnt
echo "hello" > /mnt/test.txt
# 重启后 /mnt/test.txt 还在
```

### 3.3 网络测试

QEMU 默认用 `-netdev user`，Guest 通过 NAT 访问外网：

```bash
# 在 BusyBox shell 里（需要 BusyBox 编译时启用网络工具）
ping 10.0.2.2     # QEMU 网关 = 宿主
wget http://10.0.2.2:8000/index.html  # 访问宿主 8000 端口
```

### Phase 3 常见坑

| 坑 | 原因 | 解决 |
|----|------|------|
| `/dev/vda` 不存在 | 内核没开 virtio | menuconfig → Device Drivers → Virtio drivers |
| `udhcpc: not found` | BusyBox 没编网络工具 | `make menuconfig` → Networking Utilities 全选 |
| ping 不通 | QEMU 网络模式 | 默认 user 模式只能 Guest→Host，不能 Host→Guest |
| 磁盘写入后重启丢失 | 没用 `-drive` 挂载持久镜像 | initramfs 是内存文件系统，重启即丢 |

---

## Phase 4（选做）：ARM64 交叉编译 — 桥接 P5

这一步把你 x86 的 BusyBox 系统搬到 ARM64，直接对接 P5a/P5b：

```bash
# 安装 ARM64 交叉工具链
sudo apt install gcc-aarch64-linux-gnu

# 交叉编译 BusyBox
cd busybox-1.36.1
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- menuconfig
# → Settings → Build Options → Build static binary
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)

# 交叉编译内核（ARM64 defconfig）
cd linux-6.6
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- defconfig
make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
# 产物：arch/arm64/boot/Image

# QEMU 启动 ARM64
qemu-system-aarch64 \
  -M virt -cpu cortex-a72 -m 512M \
  -kernel linux-6.6/arch/arm64/boot/Image \
  -initrd initramfs_arm64.cpio.gz \
  -append "console=ttyAMA0" \
  -nographic
```

> **这就是 P5b 的 x86 版预演** — 到 P5b 时你要用 U-Boot 替代 QEMU 直接加载内核，rootfs 放 SD 卡而不是 initramfs，但核心流程一样。

### Phase 4 卡住翻哪篇笔记

| 问题 | 翻哪篇 |
|------|--------|
| 交叉编译工具链 | `10-arm-architecture/aarch64-practice/` |
| QEMU ARM64 启动 | `11-embedded-boot-build/` |
| ARM64 体系结构 | `10-arm-architecture/` |

---

## 前置与后续

| | |
|---|---|
| **前置** | [P3](../P3-http-server/)（用户态系统编程过关） |
| **后续** | [P4](../P4-kernel-module/)（内核模块开发，需要本项目的内核编译+rootfs技能） |
| **环境** | WSL Ubuntu / Linux VM，安装 `qemu-system-x86` + `build-essential` |

## 覆盖模块

| 模块 | 用到什么 |
|------|---------|
| `02` CSAPP | Ch8 异常控制流（启动链 = 异常/中断的宏观体现） |
| `07` LKD | Ch2 内核入门（编译、menuconfig、源码树） |
| `11` Embedded Boot | 启动链全景（bootloader → kernel → init） |

## 交付物 Checklist

- [ ] `bzImage`（x86_64 内核镜像，能 QEMU 启动）
- [ ] `initramfs.cpio.gz`（BusyBox rootfs，含 init 脚本）
- [ ] QEMU 启动到 BusyBox shell，能跑 `ls / uname -a`
- [ ] Phase 2：裁剪后的 `bzImage`，体积 < 原始的 50%
- [ ] Phase 3：持久磁盘 + init 脚本 + 网络
- [ ] Phase 4（选做）：ARM64 交叉编译版 `Image` + `initramfs`

## 参考资源

- [BusyBox 官方](https://busybox.net/) — 下载 + 文档
- [内核 initramfs 文档](https://docs.kernel.org/admin-guide/initrd.html)
- [LFS Book](https://www.linuxfromscratch.org/lfs/view/stable/) — 深入理解系统构建（参考，不需要跟做）
