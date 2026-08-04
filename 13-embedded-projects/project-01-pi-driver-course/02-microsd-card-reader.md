# Project #1 · 板子准备：microSD + 读卡器

> 实物登记 · 刷机前置 · 对齐 [RASPBERRY-PI5-LABS §1.5](../RASPBERRY-PI5-LABS.md)

---

## 手头硬件

| 件 | 型号 / 标记 | 用途 |
|----|-------------|------|
| **microSD** | SanDisk Ultra **32GB** · microSDHC · **UHS-I** · Class 10 · **U1** · **A1** | 树莓派系统盘（OS + rootfs） |
| **读卡器** | 川宇（Kawasau）USB-A · 型号约 `KUW204` | PC 上写镜像；刷完拔卡插 Pi |

**够不够：** 32GB 做驱动课 / Labs Phase A–C 足够。以后内核树、大量日志再换更大卡或外置盘。

**A1 是什么：** Application Performance Class — 随机小 I/O 比「只标 Class 10」的老卡更适合当系统盘。刷机选型时优先 A1/A2。

---

## 刷机最短路径（官方工具）

Labs §0 说「系统烧录向导可跳过当教材」，但 **第一次把系统写上卡** 仍要做一次：

1. PC 插读卡器 → 插入 microSD  
2. 装 [Raspberry Pi Imager](https://www.raspberrypi.com/software/)  
3. 选 **Pi 5** → 选 OS（建议 **Raspberry Pi OS Lite 64-bit**，驱动课够用，少桌面干扰）  
4. 齿轮里开：**SSH**、设用户名密码、可选 Wi‑Fi  
5. 写入 → 安全弹出 → **卡拔出读卡器，插进 Pi5 卡槽** → 上电  

官网步骤（查细节用）：[Install using Imager](https://www.raspberrypi.com/documentation/computers/getting-started.html)

---

## 和驱动课的关系

```
读卡器写镜像（一次性） → Pi 能 SSH / 串口登录
        ↓
再谈交叉编译、.ko、GPIO（课内主线）
```

卡和读卡器本身不是「驱动学习对象」；它们只是让板子活起来。活起来之后，主战场仍是内核模块（见 [01 三层图](./01-userspace-kernel-hardware.md)）。

---

## 自检

- [ ] Imager 写完能弹出、卡插 Pi 能亮灯启动  
- [ ] 同网段能 `ssh user@pi`（或串口进 shell）  
- [ ] 知道：日常改代码用 SSH/交叉编译，**不必**每次改完都重刷整张卡
