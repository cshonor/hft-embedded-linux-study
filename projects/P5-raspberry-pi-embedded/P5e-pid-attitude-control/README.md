# P5e — PID 姿态控制（可选）

> 在 Linux 用户态实现 PID 闭环姿态控制，让"控制算法"和"Linux 对接"都落地。

## 项目目标

把 PID 离散化、姿态解算（Kalman/互补滤波）、电机 PWM 输出在 Linux 用户态串成闭环。验证飞控算法的工程实现路径（注意：本项目只做算法 + Linux 对接，不做 PCB/STM32 裸机）。

## 交付物

- [ ] 离散 PID（位置式 + 增量式，含抗积分饱和）
- [ ] IMU 数据读取（复用 P5c 驱动 + P5d 融合）
- [ ] 姿态解算：互补滤波 / Kalman
- [ ] PWM 输出（Linux sysfs pwm 或内核驱动）
- [ ] 闭环控制环（固定周期，如 1kHz）
- [ ] 仿真验证（先在仿真里稳定再上硬件）

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`14` motion-control](../../../14-motion-control/) | PID 离散闭环、姿态/Kalman、电机/ESC、Linux PWM 对接、飞控调度 |

## 前置

[P5d](../P5d-sensor-fusion-latency/)（传感器融合 + 延迟统计过关）。

## 学习目标

- 离散 PID 的实现细节（采样周期、抗饱和、微分滤波）
- 姿态表示：欧拉角 vs 四元数，奇异性
- 互补滤波 vs Kalman 的工程取舍
- Linux 用户态做硬实时控制的边界（调度抖动）
- ESC 协议（PWM/OneShot/DShot）理论

## 里程碑

1. **M1** PID 离散化 + 仿真（Python/C 离线）
2. **M2** IMU 姿态解算跑通
3. **M3** PWM 输出 + 闭环（仿真验证稳定）
4. **M4**（可选）上硬件，悬停/姿态保持

## 参考模块

- [14-motion-control/](../../../14-motion-control/) — PID、姿态/Kalman、电机/ESC、Linux 对接、飞控调度
- 边界：只学 PID/姿态/电机算法 + Linux 对接；不做 PCB/STM32 裸机
