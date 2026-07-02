# 运动控制与电机 · PID · 姿态解算 · 飞控整合

**文件夹 23** · [返回嵌入式支线](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线18–23)

> **定位：** 无人机项目的 **算法内核** — 只学 **控制逻辑**，**不学** STM32 裸机 / FreeRTOS / PCB。  
> **落地：** 全部依托 **ARM-A + 嵌入式 Linux**（[18–22](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线18–23)）。  
> **优先级：** **HFT 主线优先**；本模块用 **业余时间**，不抢占 `17→10→15` 进度。

---

## 为什么必须学（但边界要清）

| 原因 | 说明 |
|------|------|
| **无人机完整落地** | 无 PID / 姿态解算 → 只能做 WiFi、图传、视觉 — **飞不起来** |
| **岗位竞争力** | 驱动 + DT = 底层适配；**+ 自控** = 飞控 / 伺服整机 — 薪资上限更高 |
| **与 HFT 互补** | 飞控要 **严格周期时序**；用 **绑核 / PREEMPT_RT / 延迟测量**（03–05 · 15）补 Linux 实时性 |

---

## 学习边界

### ✅ 要学（本文件夹）

| 块 | 内容 |
|----|------|
| **自控** | 位置式 / 增量式 PID · 抗积分饱和 · **离散闭环** |
| **姿态** | 三轴向量 · 矩阵 · **卡尔曼滤波** IMU 融合 |
| **Linux 对接** | PWM 驱动 · **I2C/SPI IMU** 驱动 · PID → 电机 |
| **常识** | PWM 调速 · 无刷 + **ESC 协议**（理论，不做硬件设计） |

### ❌ 坚决不学

| 排除 | |
|------|---|
| Cortex-M **裸机寄存器** | STM32-F4/H7 路线 |
| 纯 **FreeRTOS** 飞控栈 | 用 **Linux + 实时补丁** 替代 |
| **PCB / 硬件电路设计** | 只读原理，不画板 |

---

## 必读书（3 本 · 只学算法 + Linux 落地）

| # | 书目 | 读什么 | 对应章节 |
|---|------|--------|----------|
| 1 | **《自动控制原理》**（胡寿松） | PID · 离散化 · 稳定性 · 抗饱和 | [Ch01 PID](./chapter-01-pid-discrete-control/) |
| 2 | **《卡尔曼滤波与组合导航原理》**（秦永元） | Kalman · IMU 融合 · 姿态估计 | [Ch02 姿态](./chapter-02-attitude-kalman-imu/) |
| 3 | **《嵌入式 Linux 无人机开发实战》**（与 22 共用） | 飞控环整合 · 调度 · 系统集成 | [Ch04–05](./chapter-04-linux-drivers-integration/) |

> 电机 / ESC 理论：**Ch03** 笔记 + datasheet；驱动实现复用 [20 LDD](../20-Linux-Device-Driver/)。

---

## 子目录

| 章 | 文件夹 | 内容 |
|----|--------|------|
| 1 | [chapter-01-pid-discrete-control/](./chapter-01-pid-discrete-control/) | 位置式/增量式 PID · 离散闭环 · 抗饱和 |
| 2 | [chapter-02-attitude-kalman-imu/](./chapter-02-attitude-kalman-imu/) | 三轴 · 旋转矩阵 · Kalman · IMU |
| 3 | [chapter-03-motor-pwm-esc/](./chapter-03-motor-pwm-esc/) | PWM · 无刷 · ESC 协议（理论） |
| 4 | [chapter-04-linux-drivers-integration/](./chapter-04-linux-drivers-integration/) | PWM/I2C 驱动 · 用户态飞控进程 |
| 5 | [chapter-05-flight-control-scheduling/](./chapter-05-flight-control-scheduling/) | 控制环周期 · PREEMPT_RT · 绑核 |

→ 完整提纲：[OUTLINE.md](./OUTLINE.md)

---

## 控制环架构（ARM Linux）

```
IMU (I2C/SPI) ──► 姿态解算 (Kalman) ──► PID (角速率/角度)
                                              │
PWM / ESC 驱动 ◄──────────────────────────────┘
        ▲
   Linux: 字符/PWM 驱动 + 用户态或 RT 线程
   绑核 / PREEMPT_RT / cyclictest 验证周期
```

---

## 从 HFT 迁移

| HFT | 飞控 |
|-----|------|
| [05 绑核 / SCHED_FIFO](../03-Linux-Kernel-Development/) | 控制线程 **isolcpus** |
| [03 p99 延迟](../13-Systems-Performance-2nd/) | 控制环 **周期 jitter** |
| [15 无锁环 / 异步日志](../15-HFT-Low-Latency-Practice/) | 传感器 → 控制 **低延迟路径** |
| [14 零拷贝思想](../12-DPDK-Low-Latency-Network/) | 共享内存传 IMU 批次 |

---

## 验收

- [ ] 手写 **离散 PID**（位置式 + 增量式）并在仿真/日志中验证  
- [ ] 实现 **最小 IMU → 姿态角** 解算（可先互补滤波，再上 Kalman）  
- [ ] Linux 下 **PWM + I2C IMU** 驱动与用户态闭环跑通  
- [ ] **cyclictest / perf** 证明控制周期 p99 达标  

**上一章：** [22 嵌入式实战](../22-Embedded-Linux-Practice/) · **总路线：** [HFT-READING-ROADMAP §六](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线18–23)

---

## GitHub 简介表述（中英）

**English**

> My primary research interest lies in HFT quantitative-trading backend development. As a long-term secondary path, I also learn embedded Linux on the ARM-A platform. I implement self-coded PID control algorithms, motor-driver programming, IMU-sensor communication and flight-control scheduling logic, avoiding STM32-M4 bare-metal development, to build a self-developed drone project as an alternative-career track.

**中文**

> 核心主攻方向为高频量化（HFT）后端开发；同时拓展 ARM-A 平台下的嵌入式 Linux，自研实现 PID 控制算法、电机驱动、IMU 传感器通信与飞控调度逻辑，绕开 STM32-M4 单片机裸机开发，自研无人机项目，作为职业备选路线。
