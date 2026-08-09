# Ch2 · 姿态解算 · IMU · 卡尔曼滤波

← [23 总览](../README.md)

> **书目：** 《卡尔曼滤波与组合导航原理》（秦永元）

---

## 要点

| 主题 | 内容 |
|------|------|
| **三轴 / 向量** | 加速度 · 角速度 · 磁力计（可选） |
| **旋转表示** | 欧拉角 · 旋转矩阵 · 四元数（选一种主用） |
| **互补滤波** | 入门阶梯 — 再上 Kalman |
| **Kalman / EKF** | IMU 融合 · 姿态估计 |

## Linux 落地

- IMU 数据来自 **I2C/SPI 字符驱动**（→ [Ch4](../chapter-04-linux-drivers-integration/)）
- 解算在用户态 **固定周期线程** — 与 [Ch5 调度](../chapter-05-flight-control-scheduling/) 衔接

→ 上一章：[Ch1 PID](../chapter-01-pid-discrete-control/) · 下一章：[Ch3 电机](../chapter-03-motor-pwm-esc/)
