# Ch3 · PWM · 无刷电机 · ESC

← [23 总览](../README.md)

> **范围：** 原理与协议常识 — **不做** PCB / 硬件设计

---

## 要点

| 主题 | 内容 |
|------|------|
| **PWM 调速** | 占空比 → 平均电压 → 转速 |
| **无刷 + ESC** | 三相换相由 ESC 完成 |
| **协议** | PWM 脉宽 · **OneShot** / **DShot**（了解即可，按板级选型） |
| **安全** | 上电解锁顺序 · 失败保护 |

## 不学

- MOSFET 选型 · 绕线 · PCB Layout  
- STM32 TIM 寄存器直控 PWM — 用 **Linux PWM 子系统 / 驱动**

→ 驱动实现：[Ch4 Linux 对接](../chapter-04-linux-drivers-integration/)
