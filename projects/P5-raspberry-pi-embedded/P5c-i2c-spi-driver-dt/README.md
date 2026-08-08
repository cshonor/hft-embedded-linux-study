# P5c — I2C/SPI 传感器驱动 + 设备树

> 给一个真实传感器写内核驱动，用设备树描述硬件，让用户态能读到温度/加速度。

## 项目目标

把"驱动 + 设备树"这对嵌入式 Linux 核心机制跑通：设备树描述"硬件在哪"，驱动匹配"怎么访问"，用户态通过 sysfs/字符设备读数据。

## 交付物

- [ ] 选一个传感器（如 MPU6050 I2C 加速度计 / BMP280 SPI 气压计）
- [ ] 设备树 overlay：节点 + reg + compatible + interrupt
- [ ] 内核驱动：I2C 或 SPI 子系统 `probe`/`remove`，注册 `iio` 或字符设备
- [ ] 寄存器读写封装（`i2c_transfer` / `spi_sync`）
- [ ] 用户态读取：`cat /sys/bus/iio/...` 或 read 字符设备
- [ ] 中断驱动版本（传感器 DRDY 引脚接 GPIO）

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`12` device-drivers-dt](../../../12-device-drivers-dt/) | 平台驱动、I2C/SPI 子系统、设备树、LDD3 原理 |

## 前置

[P5b](../P5b-uboot-kernel-rootfs/)（能改内核 + 设备树并启动）。

## 学习目标

- 设备树语法、overlay、`compatible` 匹配机制
- 平台驱动模型：`platform_driver` + `probe` 生命周期
- I2C/SPI 子系统的总线适配器与客户端
- 内核中断注册（`request_irq`、threaded irq）
- iio 子系统 vs 字符设备的取舍

## 里程碑

1. **M1** 设备树加节点，`/proc/device-tree` 能看到
2. **M2** 驱动 probe 成功，dmesg 打印芯片 ID
3. **M3** 用户态读到一次传感器数据
4. **M4** 中断驱动 + 缓冲，连续采样

## 参考模块

- [12-device-drivers-dt/](../../../12-device-drivers-dt/) — Madieu 驱动开发、LDD3、设备树
