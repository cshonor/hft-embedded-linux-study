# Ch4 · Linux 驱动对接 · 飞控用户态

← [23 总览](../README.md)

> **前置：** [20 LDD](../../20-Linux-Device-Driver/) · [21 DT](../../21-Device-Tree-Study/)

---

## 要点

| 组件 | Linux 实现 |
|------|------------|
| **IMU** | I2C/SPI **字符驱动** · `read()` 原始数据 |
| **PWM / ESC** | **PWM 子系统** 或 platform 驱动 · sysfs/ioctl |
| **飞控进程** | 用户态 C/C++：**采集 → 解算 → PID → 输出** |
| **设备树** | IMU / PWM 节点 — [21 DT](../../21-Device-Tree-Study/) |

## 数据流

```
/dev/imu0  ──read──► 姿态解算 ──► PID ──► /dev/pwm_esc 或 sysfs
```

→ 书目：《嵌入式 Linux 无人机开发实战》（与 [22](../../22-Embedded-Linux-Practice/) 共用）

→ 下一章：[Ch5 调度与实时性](../chapter-05-flight-control-scheduling/)
