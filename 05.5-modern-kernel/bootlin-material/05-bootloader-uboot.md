# Bootlin: Bootloader (U-Boot)

> **来源:** [Bootlin Embedded Linux Training](https://bootlin.com/docs/)
> **主题:** U-Boot 引导加载器
> **对标旧书:** ULK3 无覆盖 / 嵌入式C Ch10 (启动链)

---

## 讲义要点

### 树莓派 5 启动链

```
ROM Bootloader → start4.elf (GPU 固件) → config.txt → U-Boot → Linux Kernel
```

| 阶段 | 代码位置 | 功能 |
|------|---------|------|
| **ROM BL** | SoC 内部 ROM | 加载 start4.elf |
| **start4.elf** | GPU 固件 | 读 config.txt、加载 kernel8.img 或 U-Boot |
| **U-Boot** | SD/eMMC | 初始化 DDR、加载 kernel + dtb 到内存、跳转内核 |
| **Linux** | kernel8.img | 解压、初始化设备、挂载 rootfs |

### U-Boot 环境变量

```bash
# U-Boot 命令行 (串口)
=> printenv           # 查看所有环境变量
=> setenv bootargs 'console=serial0,115200 root=/dev/mmcblk0p2 rootwait'
=> setenv kernel_addr_r 0x08000000
=> setenv fdt_addr_r 0x10000000
=> bootz ${kernel_addr_r} - ${fdt_addr_r}

# 常用命令
=> help               # 帮助
=> ls mmc 0:1         # 列出 SD 卡第一分区文件
=> load mmc 0:1 ${kernel_addr_r} Image
=> load mmc 0:1 ${fdt_addr_r} bcm2712-rpi-5-b.dtb
=> fdt addr ${fdt_addr_r}  # 设置 FDT 地址
=> booti ${kernel_addr_r} - ${fdt_addr_r}
```

### bootcmd 配置

```bash
# 自动启动命令
setenv bootcmd 'load mmc 0:1 ${kernel_addr_r} Image; load mmc 0:1 ${fdt_addr_r} bcm2712-rpi-5-b.dtb; booti ${kernel_addr_r} - ${fdt_addr_r}'
saveenv  # 保存到环境变量存储区
```

### U-Boot 网络启动 (TFTP)

```bash
setenv serverip 192.168.1.100
setenv ipaddr 192.168.1.50
setenv bootfile Image
tftpboot ${kernel_addr_r} ${bootfile}
tftpboot ${fdt_addr_r} bcm2712-rpi-5-b.dtb
booti ${kernel_addr_r} - ${fdt_addr_r}
```

### U-Boot 设备树处理

```
# U-Boot 传递 FDT (Flattened Device Tree) 给内核
# 内核启动参数:
#   r0 = 0, r1 = machine type (ARM32), r2 = FDT 地址 (ARM64)
# ARM64: x0 = FDT 地址, 其余寄存器为 0

# U-Boot 可以修改 FDT (fdt 命令)
=> fdt addr ${fdt_addr_r}
=> fdt set /chosen bootargs 'console=serial0,115200 root=/dev/mmcblk0p2'
```

---

## 动手实验

```bash
# 1. 编译 U-Boot for 树莓派 5
git clone https://github.com/u-boot/u-boot.git
cd u-boot
make rpi_5_defconfig
make CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc)
# 生成 u-boot.bin

# 2. 替换树莓派 5 启动文件
cp u-boot.bin /boot/firmware/u-boot.bin
# 在 config.txt 中添加:
# kernel=u-boot.bin

# 3. 串口连接树莓派 5
# GPIO 14 (TXD) / GPIO 15 (RXD) / GND
# 用 USB-TTL 串口线连接
sudo screen /dev/ttyUSB0 115200

# 4. 在 U-Boot 中设置环境
=> setenv bootdelay 3    # 启动延迟 3 秒
=> saveenv

# 5. 使用 fw_printenv 在 Linux 中读取 U-Boot 环境变量
fw_printenv
fw_setenv bootdelay 5
```

---

## 与旧书差异

| ULK3 | Bootlin 讲义 |
|------|-------------|
| 不覆盖 bootloader | U-Boot 是嵌入式启动核心 |
| 无设备树传递 | FDT 是 U-Boot → 内核的标准参数传递方式 |
| 无网络启动 | TFTP 启动是开发标配 |

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** U-Boot 传递给 Linux 内核的三个关键信息是什么？

> 1. 内核镜像地址（kernel_addr_r）2. 设备树地址（fdt_addr_r）3. 命令行参数（bootargs，通过 FDT 的 /chosen 节点传递）。ARM64 通过寄存器 x0 传递 FDT 地址。

**Q2:** 树莓派 5 的启动链中 start4.elf 的作用是什么？

> start4.elf 是 GPU 固件，在 ARM CPU 启动前运行。它读取 SD 卡上的 config.txt 配置，初始化 DDR 内存，然后根据配置加载 kernel8.img（Linux 内核）或 U-Boot 到内存，最后唤醒 ARM CPU 开始执行。

**Q3:** `booti` 和 `bootz` 命令的区别？

> `bootz` 启动 zImage（ARM32 压缩内核），`booti` 启动 Image（ARM64 未压缩或自解压内核）。树莓派 5 (ARM64) 使用 `booti`。传递参数格式相同：`booti <kernel_addr> - <fdt_addr>`（中间的 `-` 表示无 ramdisk）。

</details>
