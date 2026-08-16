# P5 — 树莓派嵌入式 Linux 全链路

> 从裸机 UART 到飞控调度，6 个子项目（P5a–P5f）串起嵌入式 Linux 的完整链路。这是嵌入式支线的综合大作业，也是第二职业退路的敲门砖。

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
| **P5f** | 树莓派 Linux 驱动视频课（环境 → 模块 → GPIO/中断） | `07`–`09` | [P5f-pi-driver-course](./P5f-pi-driver-course/) |

> P5f 可作为**第一个项目式实践**；边界与验收见 Labs §1.5，**不替代**仓库主线。

## 必读书（项目向）

| # | 书目 | 读什么 |
|---|------|--------|
| 1 | **《嵌入式 Linux 无人机开发实战》** | 传感器 · 通信 · 实时链路 · 系统集成 |

## 从 HFT 可直接迁移的思想

| HFT 技能 | 嵌入式实战 |
|----------|------------|
| **绑核 / isolcpus** | 飞控关键线程 **CPU 隔离** |
| **无锁环 / 低延迟 IPC** | 传感器 → 融合 → 控制 **低 jitter 数据路径** |
| [SysPerf](../../06.6-systems-performance/) | **量延迟** — 控制环周期 p99 |
| [DPDK](../../13-dpdk/) 零拷贝思想 | **DMA / 共享内存** 传 IMU/图传 |
| [HFT](../../16-hft-engineering/) 日志异步 | 黑匣子 / 遥测 **移出热路径** |

## 建议实战里程碑

| 阶段 | 产出 |
|------|------|
| 1 | Buildroot 启动 + **串口 shell** |
| 2 | **字符驱动** 读 IMU · 用户态 poll |
| 3 | **DT** 描述传感器 · 驱动 auto probe |
| 4 | **多线程** 采集 + 融合 · 绑核 + 延迟统计 |
| 5 | 按书目完成 **无人机子系统** — 控制环见 [运动控制](../../10-motion-control/) |

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`07` arm-architecture](../../07-arm-architecture/) | AArch64 汇编、异常等级、MMU |
| [`08` embedded-boot-build](../../08-embedded-boot-build/) | U-Boot、内核构建、rootfs、device tree blob |
| [`09` device-drivers-dt](../../09-device-drivers-dt/) | 平台驱动、I2C/SPI 子系统、设备树 overlay |
| [P5 Labs + P5f](./RASPBERRY-PI5-LABS.md) | 板级动手清单（A→G）· 驱动视频课 · 多线程融合 · 延迟统计 |
| [`11` motion-control](../../10-motion-control/) | PID、姿态/Kalman、Linux PWM 对接 |

## 前置

[P4](../P4-kernel-module/)（内核模块开发过关）。

## 学习目标

- ARM 启动链：ROM → bootloader → kernel → userspace
- 设备树如何描述硬件、驱动如何匹配
- 内核驱动与用户态应用的协作（sysfs/ioctl/字符设备）
- 嵌入式实时性：调度延迟、p99、PREEMPT_RT 的边界
- PID 闭环在 Linux 用户态的实现约束

## 里程碑

按 P5a → P5f 顺序逐个达成，每个子项目各自有里程碑。全部完成即嵌入式支线闭环。

## 参考资源

- [07-arm-architecture/aarch64-practice/EXPERIMENT-CATALOG.md](../../07-arm-architecture/aarch64-practice/EXPERIMENT-CATALOG.md) — 《ARM64体系结构编程与实践》全书 56 个实验目录 + Pi4B→Pi5 适配清单
- [RASPBERRY-PI5-LABS.md](./RASPBERRY-PI5-LABS.md) — Pi5 板卡动手清单（A→G 执行序，对齐 `07`–`11`）
- [HFT-READING-ROADMAP §六](../../HFT-READING-ROADMAP.md) — 嵌入式支线详情
- 边界：仅 ARM-A + 嵌入式 Linux；**不学** STM32/MCU 裸机/FreeRTOS/PCB
