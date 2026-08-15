# 串口配置（含树莓派 UART）

> 🔴 精读

## 概念详解

### 树莓派 5 UART 配置

```bash
# 1. 硬件连接
# USB-TTL 串口线 → 树莓派 5 GPIO
# GPIO 14 (TXD) → 串口线 RXD
# GPIO 15 (RXD) → 串口线 TXD
# GND → 串口线 GND

# 2. 启用串口 (config.txt)
echo "enable_uart=1" >> /boot/firmware/config.txt

# 3. 内核 boot 参数
# console=ttyAMA0,115200 kgdboc=ttyAMA0,115200

# 4. 开发机连接
screen /dev/ttyUSB0 115200
# 或
minicom -D /dev/ttyUSB0 -b 115200
```

### 树莓派 5 的 UART

| UART | 设备名 | 用途 | 特点 |
|------|--------|------|------|
| PL011 | ttyAMA0 | 主串口/Console | 全功能 UART |
| mini UART | ttyS0 | 辅助串口 | 简化版，不稳定 |

### KGDB 专用串口

```bash
# KGDB 可以使用与 console 相同的串口
# kgdboc=ttyAMA0,115200
# 但建议用独立串口避免 console 输出干扰

# 树莓派 5 有多个 UART:
# PL011 (ttyAMA0) — 做 KGDB
# mini UART (ttyS0) — 做 console
# 配置:
# console=ttyS0,115200 kgdboc=ttyAMA0,115200
```

### KGDB 和 Console 共用串口

```bash
# 共用模式 (kgdboc)
# 优点: 只需要一个串口
# 缺点: console 的 printk 输出干扰 GDB 协议

# 切换机制:
# 正常运行: 串口做 console (printk 输出)
# 进入 KGDB: 串口切换为 GDB 协议模式
# 退出 KGDB: 串口恢复为 console

# 进入 KGDB:
echo g > /proc/sysrq-trigger
# 此时串口切换为 GDB 模式
# 开发机 GDB 连接: target remote /dev/ttyUSB0
```

### 串口波特率

```bash
# 常用波特率
# 115200 — 标准速率, ~14KB/s
# 230400 — 2x, ~28KB/s
# 460800 — 4x, ~56KB/s
# 921600 — 8x, ~112KB/s

# KGDB 传输大量数据时高波特率更快
# 但高波特率可能不稳定 (长线缆/干扰)
# 树莓派推荐: 115200 (稳定) 或 460800 (快速)
```

### 开发机串口工具

```bash
# screen (推荐)
screen /dev/ttyUSB0 115200

# minicom
minicom -D /dev/ttyUSB0 -b 115200

# picocom
picocom -b 115200 /dev/ttyUSB0

# GDB 直连 (KGDB 模式)
aarch64-linux-gnu-gdb vmlinux
(gdb) target remote /dev/ttyUSB0
```

### HFT 关联应用

```bash
# HFT 开发环境串口配置
# 树莓派 5:
#   /boot/firmware/config.txt: enable_uart=1
#   /boot/cmdline.txt: console=ttyAMA0,115200 kgdboc=ttyAMA0,115200 nokaslr

# 开发机:
#   USB-TTL 串口线连接
#   screen /dev/ttyUSB0 115200 (console + KGDB)

# 进入 KGDB:
#   echo g > /proc/sysrq-trigger (在串口终端)
#   或 Alt+SysRq+g
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KGDB 和 console 能共用同一个串口吗？

> 可以但有问题。Console 的 printk 输出会干扰 GDB 的二进制协议。建议用独立串口，或使用 `kgdboc` 时在 KGDB 模式下 console 暂停。树莓派 5 有 PL011 和 mini UART 两个串口，可以分别用于 console 和 KGDB。

**Q2:** kgdboc 的 "oc" 代表什么？

> oc = Over Console，kgdboc 复用控制台串口做 KGDB 通信。配置：`kgdboc=ttyAMA0,115200`。通过 SysRq+g 切换到 KGDB 模式，GDB 连接后控制台暂停。

**Q3:** KGDB over USB serial 和 over network 哪个更适合 HFT？

> 串口更可靠但慢（115200 baud ≈ 14KB/s）。网络快但需要网络栈工作（如果网络栈崩溃无法用）。HFT 开发用网络（快），生产崩溃分析用串口（可靠）。也可以用 USB serial（快 + 可靠）。

**Q4:** 树莓派 5 的 PL011 和 mini UART 有什么区别？

> PL011 是全功能硬件 UART（基于 ARM PrimeCell），稳定可靠，适合做 KGDB。mini UART 是简化版（基于 BCM2835 AUX），功能有限且不稳定，只适合简单 console。推荐 PL011 做 KGDB。

**Q5:** 为什么串口波特率影响 KGDB 调试体验？

> KGDB 通过串口传输命令和数据。低波特率（115200）传输慢，查看大内存区域或长栈回溯时等待时间长。高波特率（460800+）更快但可能不稳定。HFT 调试建议用 460800 或网络连接。

</details>

## 交叉引用

- [05.6 ch11 KGDB 原理与架构](../../chapter-11-kgdb/notes/01-kgdb-architecture.md)
- [05.6 ch11 GDB 连接内核](../../chapter-11-kgdb/notes/03-gdb-connection.md)
- [05.6 ch11 QEMU + KGDB](../../chapter-11-kgdb/notes/07-qemu-kgdb.md)
