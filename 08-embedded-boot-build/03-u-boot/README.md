# 03 · Bootloader：板子上电后发生了什么

> **本节讲什么：** 从掉电到内核拿到控制权之间的全过程，以及 U-Boot 在其中扮演的角色。
> **核心提醒：** **树莓派不是标准 U-Boot 启动**。Pi 的启动链是 BootROM → EEPROM → armstub/TF-A →（可选 U-Boot）→ 内核。拿通用 ARM 开发板的经验硬套 Pi 会踩坑。
> **对应动手：** [P5 · B3](../../projects/P5-raspberry-pi-embedded/RASPBERRY-PI5-LABS.md)（U-Boot / 固件启动参数 + cmdline）

---

## 启动链（通用 ARM vs 树莓派）

| 阶段 | 通用 ARM 板 | 树莓派 |
|------|------------|--------|
| 1 | SoC 内部 BootROM | 同（片内 ROM） |
| 2 | SPL → U-Boot proper（多在 SPI Flash） | **EEPROM bootloader**（板载，可 `rpi-eeprom-update`） |
| 3 | — | 读 SD 卡 `config.txt` / `cmdline.txt` + 加载 **armstub / TF-A** |
| 4 | U-Boot 加载内核 + DTB | armstub 直接跳内核（**默认不走 U-Boot**） |
| 5 | `bootz`/`booti` | `kernel=` 指定镜像 |

**结论：** Pi 上"bootloader 配置"的实际载体是 `config.txt` 和 `cmdline.txt`，不是 U-Boot 环境变量。要做 U-Boot 实验得在 `config.txt` 里显式指定 `kernel=u-boot.bin`。

---

## DTB 是怎么进内核的

```
bootloader 把 DTB 物理地址放进寄存器 / 按 arm64 boot protocol 传入
        ↓
内核早期解析 DT → 展开成 device_node 树
        ↓
platform/i2c/spi 总线按 compatible 匹配驱动
```

这是 [09 驱动 03-platform-dt](../../09-device-drivers-dt/03-platform-dt) 的前置——**驱动能 probe 成功，前提是 DTB 里有正确的节点**。

---

## 关键命令

```bash
# 运行时看内核收到的 cmdline 与 DT
cat /proc/cmdline
dtc -I fs -O dts /proc/device-tree | head -50   # 反编译运行时 DT

# Pi 固件与启动配置
vcgencmd version
cat /boot/firmware/config.txt
sudo rpi-eeprom-update
```

---

## HFT / 嵌入式关联

- 启动时间就是**故障恢复时间**。HFT 网关重启一次的成本按秒算，所以理解启动链不只是"能起来"，还要知道**每一段能不能砍**（去掉 U-Boot 交互延时、裁剪内核、跳过 initrd）。
- `cmdline` 里的 `isolcpus`、`nohz_full`、`rcu_nocbs` 是后面做绑核/隔离的入口，属于 [06-boot-to-shell](../06-boot-to-shell/) 和 [14-hft-engineering](../../14-hft-engineering) 的内容。

---

## 本节笔记

- [3.1 · U-Boot / BIOS / UEFI 对比](./3.1-uboot-bios-uefi.md)
- [1.6 · 设备树 vs UEFI](../01-orientation/1.6-device-tree-vs-uefi.md)

---

## 验收

- [ ] 能画出 Pi 5 从上电到 `start_kernel` 的完整链路
- [ ] 能解释 DTB 由谁生成、放在哪、内核怎么用
- [ ] 改过 `cmdline.txt` 并在 `/proc/cmdline` 里看到效果
- [ ] `dtc` 反编译过运行时的 `/proc/device-tree`

---

## 衔接

- **上一步：** [02-toolchain](../02-toolchain/)
- **下一步：** [04-kernel-build](../04-kernel-build/)
- **卡住查书：** MELP Ch3 · Primer Ch7
