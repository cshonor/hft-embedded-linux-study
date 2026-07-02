# 23 · 运动控制与电机 · 提纲

**顺序：** [18](../18-ARM64-Architecture/) → … → [22](../22-Embedded-Linux-Practice/) → **23**

---

## 章节 ↔ 书目

| 章 | 主题 | 书目 | 状态 |
|----|------|------|------|
| 1 | PID · 离散闭环 | 《自动控制原理》胡寿松 — Ch6 PID / 数字控制相关章 | 待笔记 |
| 2 | 姿态 · Kalman · IMU | 《卡尔曼滤波与组合导航原理》秦永元 | 待笔记 |
| 3 | PWM · ESC · 无刷常识 | 章节笔记 + ESC 协议 datasheet | 待笔记 |
| 4 | Linux 驱动对接 | LDD3 + [20](../20-Linux-Device-Driver/) + [22](../22-Embedded-Linux-Practice/) | 待笔记 |
| 5 | 飞控调度 · PREEMPT_RT | [05 LKD](../03-Linux-Kernel-Development/) Ch4 + [03 SysPerf](../13-Systems-Performance-2nd/) | 待笔记 |

---

## 不学清单（归档）

- STM32 / Cortex-M 裸机 · HAL 库飞控模板  
- FreeRTOS 专用飞控工程  
- Altium / KiCad PCB 设计  

---

## 时间分配原则

| 优先级 | 内容 |
|--------|------|
| **P0** | HFT：`17` C++ → `10–14` 网络 → `15` 引擎 |
| **P1** | 嵌入式 Linux：`18–22`（系统能力） |
| **P2** | 本模块 `23`（算法 + 飞控整合）— **仅 HFT 每日任务完成后** |
