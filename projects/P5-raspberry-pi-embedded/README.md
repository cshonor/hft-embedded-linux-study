# P5 — 树莓派嵌入式 Linux 全链路

> 从裸机 UART 到飞控调度，5 个子项目串起嵌入式 Linux 的完整链路。这是嵌入式支线的综合大作业，也是第二职业退路的敲门砖。

## 项目目标

在树莓派 5（Cortex-A76, AArch64, Linux 6.1）上走完"裸机 → 启动 → 驱动 → 应用 → 控制算法"全链路，每一步都有可运行交付物。

## 子项目（按顺序）

| 子项目 | 交付 | 模块 | 脚手架 |
|:------:|------|:----:|--------|
| **P5a** | QEMU 裸机 UART Hello World | `07` | [P5a-qemu-uart-hello](./P5a-qemu-uart-hello/) |
| **P5b** | U-Boot → kernel → rootfs 启动到 shell | `08` | [P5b-uboot-kernel-rootfs](./P5b-uboot-kernel-rootfs/) |
| **P5c** | I2C/SPI 传感器驱动 + 设备树 | `09` | [P5c-i2c-spi-driver-dt](./P5c-i2c-spi-driver-dt/) |
| **P5d** | 多线程传感器融合 + 延迟 p99 统计 | `10` | [P5d-sensor-fusion-latency](./P5d-sensor-fusion-latency/) |
| **P5e** | PID 姿态控制（可选） | `11` | [P5e-pid-attitude-control](./P5e-pid-attitude-control/) |

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`07` arm-architecture](../../07-arm-architecture/) | AArch64 汇编、异常等级、MMU |
| [`08` embedded-boot-build](../../08-embedded-boot-build/) | U-Boot、内核构建、rootfs、device tree blob |
| [`09` device-drivers-dt](../../09-device-drivers-dt/) | 平台驱动、I2C/SPI 子系统、设备树 overlay |
| [`10` embedded-projects](../../10-embedded-projects/) | 板级项目、多线程融合、延迟统计 |
| [`11` motion-control](../../11-motion-control/) | PID、姿态/Kalman、Linux PWM 对接 |

## 前置

[P4](../P4-kernel-module/)（内核模块开发过关）。

## 学习目标

- ARM 启动链：ROM → bootloader → kernel → userspace
- 设备树如何描述硬件、驱动如何匹配
- 内核驱动与用户态应用的协作（sysfs/ioctl/字符设备）
- 嵌入式实时性：调度延迟、p99、PREEMPT_RT 的边界
- PID 闭环在 Linux 用户态的实现约束

## 里程碑

按 P5a → P5e 顺序逐个达成，每个子项目各自有里程碑。全部完成即嵌入式支线闭环。

## 参考资源

- [07-arm-architecture/aarch64-practice/EXPERIMENT-CATALOG.md](../../07-arm-architecture/aarch64-practice/EXPERIMENT-CATALOG.md) — 《ARM64体系结构编程与实践》全书 56 个实验目录 + Pi4B→Pi5 适配清单
- [10-embedded-projects/RASPBERRY-PI5-LABS.md](../../10-embedded-projects/RASPBERRY-PI5-LABS.md) — Pi5 板卡动手清单（A→G 执行序）
- [HFT-READING-ROADMAP §六](../../HFT-READING-ROADMAP.md) — 嵌入式支线详情
- 边界：仅 ARM-A + 嵌入式 Linux；**不学** STM32/MCU 裸机/FreeRTOS/PCB
