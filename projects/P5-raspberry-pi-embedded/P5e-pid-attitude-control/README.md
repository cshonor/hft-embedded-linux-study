# P5e — PID 姿态控制（可选）

> 在 Linux 用户态实现 PID 闭环姿态控制，让"控制算法"和"Linux 对接"都落地。
> **做法：项目驱动，[`11`](../../../10-motion-control/) 笔记当字典。**

---

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [ch01 PID 离散控制](../../../10-motion-control/chapter-01-pid-discrete-control/) | PID 离散化、抗积分饱和 |
| [ch02 姿态 Kalman IMU](../../../10-motion-control/chapter-02-attitude-kalman-imu/) | 互补滤波、Kalman |
| [ch03 电机 PWM ESC](../../../10-motion-control/chapter-03-motor-pwm-esc/) | PWM/ESC 协议 |
| [ch04 Linux 对接](../../../10-motion-control/chapter-04-linux-drivers-integration/) | sysfs PWM、调度 |
| [ch05 飞控调度](../../../10-motion-control/chapter-05-flight-control-scheduling/) | 控制环周期、抖动 |

---

## 项目目标

把 PID 离散化、姿态解算（Kalman/互补滤波）、电机 PWM 输出在 Linux 用户态串成闭环。验证飞控算法的工程实现路径（注意：本项目只做算法 + Linux 对接，不做 PCB/STM32 裸机）。

## Phase 1：PID 离散化 + 仿真（1-2 小时）

### 做什么

在纯 C/Python 里实现离散 PID（位置式 + 增量式），在仿真环境验证。

### 代码骨架

```c
// src/pid.c

// 位置式 PID（含抗积分饱和 + 微分滤波）
struct pid {
    double kp, ki, kd;        // 增益
    double integral;           // 积分累计
    double prev_error;         // 上次误差（微分用）
    double out_min, out_max;   // 输出限幅
    double integral_limit;     // 积分限幅（抗饱和）
    double d_filter;           // 微分低通滤波系数 (0-1)
    double prev_derivative;    // 上次微分值
};

double pid_update(struct pid *p, double setpoint, double measured, double dt) {
    double error = setpoint - measured;

    // 积分（带抗饱和）
    p->integral += error * dt;
    if (p->integral > p->integral_limit) p->integral = p->integral_limit;
    if (p->integral < -p->integral_limit) p->integral = -p->integral_limit;

    // 微分（带低通滤波）
    double derivative = (error - p->prev_error) / dt;
    derivative = p->d_filter * p->prev_derivative + (1.0 - p->d_filter) * derivative;
    p->prev_derivative = derivative;
    p->prev_error = error;

    // 输出
    double output = p->kp * error + p->ki * p->integral + p->kd * derivative;
    if (output > p->out_max) output = p->out_max;
    if (output < p->out_min) output = p->out_min;
    return output;
}
```

### 分步实现

1. **写位置式 PID**：P + I + D 三项，含输出限幅
2. **加抗积分饱和**：积分累计超限时截断（防止 integral windup）
3. **加微分滤波**：低通滤波器平滑微分项（减少噪声放大）
4. **仿真**：模拟一个一阶系统（如 `G(s) = 1/(s+1)`），用 PID 控制它，画阶跃响应
5. **调参**：先 P（增大到轻微振荡），再加 D（抑制振荡），最后加 I（消除稳态误差）——这就是 Ziegler-Nichols 法

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 积分饱和 | 阶跃响应超调巨大 | 不加 integral_limit，积分项无限增长 |
| 微分噪声 | 输出抖动 | 传感器噪声被微分放大，必须滤波 |
| dt 不固定 | 控制不稳定 | 必须固定采样周期，或用实际 dt 计算 |
| 增益符号反 | 发散 | 确认负反馈：error = setpoint - measured |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| PID 离散化原理 | [ch01 PID](../../../10-motion-control/chapter-01-pid-discrete-control/) |
| 抗饱和方法 | [ch01 PID](../../../10-motion-control/chapter-01-pid-discrete-control/) |

---

## Phase 2：IMU 姿态解算（1-2 小时）

### 做什么

从 P5c 驱动读 IMU 数据（加速度 + 陀螺仪），用互补滤波算姿态角。

### 代码骨架

```c
// src/attitude.c

// 互补滤波：加速度计低频可信，陀螺仪高频可信
struct attitude {
    double roll, pitch;      // 欧拉角（度）
    double alpha;            // 互补滤波系数 (0.95-0.99)
    uint64_t last_update;
};

void attitude_update(struct attitude *a, int16_t *accel, int16_t *gyro, uint64_t now) {
    double dt = (now - a->last_update) / 1e9;  // 纳秒转秒
    a->last_update = now;

    // 从加速度计算 roll/pitch（低频，受重力方向影响）
    double accel_roll  = atan2(accel[1], accel[2]) * 180.0 / M_PI;
    double accel_pitch = atan2(-accel[0],
        sqrt(accel[1]*accel[1] + accel[2]*accel[2])) * 180.0 / M_PI;

    // 陀螺仪积分（高频，短期准确）
    double gyro_roll  = a->roll  + gyro[0] * dt;
    double gyro_pitch = a->pitch + gyro[1] * dt;

    // 互补滤波融合
    a->roll  = a->alpha * gyro_roll  + (1.0 - a->alpha) * accel_roll;
    a->pitch = a->alpha * gyro_pitch + (1.0 - a->alpha) * accel_pitch;
}
```

### 分步实现

1. **加速度计算角度**：`atan2` 从重力方向算 roll/pitch（低频准，高频抖）
2. **陀螺仪积分**：角速度 × dt = 角度增量（高频准，低频漂移）
3. **互补滤波**：`alpha * gyro + (1-alpha) * accel`，典型 alpha=0.98
4. **选做 Kalman**：比互补滤波更精确，但参数调优复杂

### 为什么用互补滤波不用 Kalman

互补滤波是 Kalman 的简化版——两者本质都是"高频信陀螺、低频信加速度"，但互补滤波用固定权重，Kalman 用动态协方差。工程上互补滤波 90% 场景够用，调参简单。

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 互补滤波 | [ch02 姿态](../../../10-motion-control/chapter-02-attitude-kalman-imu/) |
| Kalman | [ch02 姿态](../../../10-motion-control/chapter-02-attitude-kalman-imu/) |
| 四元数 vs 欧拉角 | [ch02 姿态](../../../10-motion-control/chapter-02-attitude-kalman-imu/) |

---

## Phase 3：PWM 输出 + 闭环（1-2 小时）

### 做什么

通过 Linux sysfs PWM 接口输出 ESC 信号，把 PID 闭环跑起来。

### 分步实现

1. **开 PWM**：
   ```bash
   echo 0 > /sys/class/pwm/pwmchip0/export
   echo 20000000 > /sys/class/pwm/pwmchip0/pwm0/period   # 20ms (50Hz, ESC 标准)
   echo 1000000  > /sys/class/pwm/pwmchip0/pwm0/duty_cycle # 1ms = 最小油门
   echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable
   ```
2. **ESC 校准**：最大油门(2ms)上电 → 听 beep → 最小油门(1ms) → 听 beep → 校准完成
3. **控制环**：
   ```c
   for (;;) {
       uint64_t t_start = get_time_ns();
       read_imu(accel, gyro);              // P5c 驱动
       attitude_update(&att, accel, gyro, t_start);
       double output = pid_update(&pid, target_angle, att.roll, dt);
       set_pwm(output);                     // 输出到 ESC
       uint64_t t_end = get_time_ns();
       sleep_until_next_period(t_start, 1000000);  // 1ms = 1kHz
   }
   ```
4. **仿真先行**：在仿真里让 PID 稳定后再上硬件

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| PWM 频率不对 | ESC 不响应 | 标准 ESC = 50Hz (20ms 周期)，OneShot = 300Hz |
| 控制环周期不稳 | 控制抖动 | 用 `timerfd` 或 `clock_nanosleep` 精确定时 |
| 电机方向反 | 反向推力 | 确认 PID 输出符号和电机方向匹配 |
| 安全！ | 电机飞了 | 先拆螺旋桨！在测试台架上调参 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| PWM/ESC 协议 | [ch03 电机](../../../10-motion-control/chapter-03-motor-pwm-esc/) |
| Linux PWM 对接 | [ch04 Linux 对接](../../../10-motion-control/chapter-04-linux-drivers-integration/) |
| 控制环调度 | [ch05 飞控调度](../../../10-motion-control/chapter-05-flight-control-scheduling/) |

---

## Phase 4：（可选）上硬件悬停

### 做什么

在测试台架上验证闭环稳定，条件允许再上飞行。

### 安全须知

- **永远先拆螺旋桨调参**
- 在台架上测试电机响应和 PID 稳定性
- 确认紧急停止机制（kill switch）
- 第一次飞行在开阔室外，有保护

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 飞控调度 | [ch05 飞控调度](../../../10-motion-control/chapter-05-flight-control-scheduling/) |

---

## 状态

⬜ 未开始 → 建议先在纯仿真里调通 PID（Phase 1），不要直接上硬件。

← [P5 索引](../README.md) · [14 模块](../../../10-motion-control/)
