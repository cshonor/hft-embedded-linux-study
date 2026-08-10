# 11.2 串口配置（含树莓派 UART）

> 🔴 精读

## 本节要点

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

### KGDB 专用串口

```bash
# KGDB 可以使用与 console 相同的串口
# kgdboc=ttyAMA0,115200
# 但建议用独立串口避免 console 输出干扰
# 树莓派 5 有多个 UART，可以用 mini UART 做 console，PL011 做 KGDB
```

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** KGDB 和 console 能共用同一个串口吗？

> 可以但有问题。Console 的 printk 输出会干扰 GDB 的二进制协议。建议用独立串口，或使用 `kgdboc` 时关闭 console 输出。树莓派 5 有 PL011 (ttyAMA0) 和 mini UART (ttyS0) 两个串口，可以分别用于 console 和 KGDB。

</details>
