# 树莓派 5 · 实战执行序（对齐 `00`–`23`）

> **板卡：** Raspberry Pi 5（BCM2712 + RP1）  
> **定位：** 吃透原理、验证代码；极致低延迟生产环境后续上 x86。  
> **主线：** 仍按 [LEARNING-PATH-LOCKED.md](../LEARNING-PATH-LOCKED.md) 推进；本文是 **动手清单**，不是另开一条书。  
> **硬件归类（Primer Ch3）：** BCM2712 是 **ARM SoC**，不是独立 CPU+南北桥 — [说明](../11-embedded-boot-build/primer-system-overview/chapter-03-processor-basics/3.2-raspberry-pi-is-soc.md)。  
> **官方镜像：** 评估板级开箱可用；自研 PCB 时 BSP 只是模板 — [BSP FAQ](../11-embedded-boot-build/primer-system-overview/chapter-03-processor-basics/3.2-bsp-is-template-not-product.md)。

---

## 0. 官方文档怎么用（先定规矩）

| 态度 | 做法 |
|------|------|
| **不当教材** | 不从头到尾通读操作手册 / 桌面 / Python 教程 |
| **当工具书** | 写驱动、改 DTS、编内核、查引脚时 **随时检索** |
| **版本优先** | 硬件/内核以 **官网英文** 为准；第三方中文（多基于 Pi4/32 位）仅作线索 |

### 可直接跳过

- 系统烧录向导、桌面配置、Python / 多媒体 / 小游戏案例  
- 零基础「点点鼠标」入门向导  

### 必须收藏（底层开发相关）

| # | 文档 | 用途 | 链接 |
|---|------|------|------|
| 1 | **RP1 / GPIO 外设** | 写 GPIO·I2C·SPI、对引脚 | [RP1](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#rp1) · [GPIO](https://www.raspberrypi.com/documentation/computers/raspberry-pi.html#gpio) |
| 2 | **Linux 内核编译** | 配置 / 交叉编译 / 替换内核 | [Building the kernel](https://www.raspberrypi.com/documentation/computers/linux_kernel.html#building) |
| 3 | **Device Tree / Overlay** | 改 DTS、`dtoverlay`、动态加载 | [Device Tree](https://www.raspberrypi.com/documentation/computers/configuration.html#device-trees-overlays-and-parameters) |
| 4 | **config.txt / 启动** | 启动参数、固件与内核衔接 | [config.txt](https://www.raspberrypi.com/documentation/computers/config_txt.html) |
| 5 | **内核 DT 源码（Pi5）** | `bcm2712-rpi-5-b.dts` / RP1 节点对照 | [bcm2712-rpi-5-b.dts](https://github.com/raspberrypi/linux/blob/rpi-6.12.y/arch/arm64/boot/dts/broadcom/bcm2712-rpi-5-b.dts)（分支随内核版本调整） |
| 6 | **PCIe（进阶）** | RP1 挂在 PCIe；日后 ARM 侧旁路/网卡实验的官方线索 | 见 RP1 文档 PCIe 段 + 内核 `Documentation/devicetree` / 驱动树 |

通用内核 DT（与板级无关）：[Device Tree Usage](https://docs.kernel.org/devicetree/usage-model.html)（已收进 [12 驱动](../12-device-drivers-dt/)）。

---

## 1. 依次动手顺序（按仓库模块）

每步：**先读对应模块笔记骨架 → 板上做出可演示结果 → 勾选**。  
卡住再查上表官方文档，不要先刷第三方视频。

### Phase A · 用户态基础（板子当 Linux 机器用）

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| A1 | POSIX：多进程 / 多线程 / 信号 / `shm` / fd | [04](../04-linux-userspace-api/) · [01](../01-c-language/) | 自写小 Demo，不用脚本玩具 |
| A2 | 简易高性能 TCP：阻塞 → 非阻塞 → **epoll** | [15](../15-network-sockets/) · [04](../04-linux-userspace-api/) | 能压测、会看 `ss`/`tcpdump` |

- [ ] A1  
- [ ] A2  

### Phase B · 内核 & Boot（启动链）

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| B1 | 拉 Pi 内核树，`bcm2712_defconfig` 级配置 / 裁剪 / 交叉编译 / **替换运行内核** | [11](../11-embedded-boot-build/) · [07](../07-linux-kernel/) | 自编内核能启动；对照官方 Build 文档 |
| B2 | 最小 rootfs（BusyBox / Buildroot 任选） | [11](../11-embedded-boot-build/) | 串口进 shell，理解 init |
| B3 | U-Boot（或固件启动参数）+ `config.txt` / cmdline | [11](../11-embedded-boot-build/) · [10](../10-arm-architecture/) | 能改启动参数并解释 DTB 如何传入内核 |

- [ ] B1  
- [ ] B2  
- [ ] B3  

### Phase C · 驱动 + 设备树（核心）

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| C1 | GPIO **字符设备** + DTS + **DT overlay** 动态加载 | [12](../12-device-drivers-dt/) | userspace 能 `open/read/ioctl`；overlay 可开关 |
| C2 | I2C 或 SPI **从设备驱动**（外接传感器） | [12](../12-device-drivers-dt/) | `compatible` 匹配、`reg`/`interrupts` 从 DT 来 |
| C3 | **platform** + `probe`：资源全部从 DT 解析 | [12](../12-device-drivers-dt/) | 无硬编码基址；对照 RP1 / 板级 dts |

- [ ] C1  
- [ ] C2  
- [ ] C3  

### Phase D · 嵌入式工程收口

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| D1 | 交叉编译 C/C++ 部署到板 | [11](../11-embedded-boot-build/) · [06](../06-cpp/) | 工具链与板子 ABI 一致 |
| D2 | systemd 服务 + 开机自启 | [13](./) | unit 能启停、journal 可读 |
| D3 | cgroup / nice / 实时调度基础（SCHED_FIFO 等） | [13](./) · [07](../07-linux-kernel/) | 会限制 CPU/内存；知 PREEMPT_RT 边界 |

- [ ] D1  
- [ ] D2  
- [ ] D3  

### Phase E · 网络与低延迟（板子练思想，生产看 x86）

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| E1 | TCP 参数调优 + 抓包 | [16](../16-tcpip-protocols/) · [15](../15-network-sockets/) | 会改 sysctl、能讲清延迟来源 |
| E2 | 用户态高性能收发 + **延迟统计**（p50/p99） | [15](../15-network-sockets/) · [21](../21-hft-engineering/) | 绑核前后对比一组数字 |
| E3 | ARM 上 DPDK **能编过、跑通基础例程**（旁路思想） | [18](../18-dpdk/) | 不要求生产级吞吐；知与内核栈分界 |

- [ ] E1  
- [ ] E2  
- [ ] E3  

### Phase F · 观测与调优

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| F1 | `perf` / `bpftrace` 看热点与内核路径 | [19](../19-systems-performance/) · [20](../20-bpf-observability/) | 能解释一张火焰图 |
| F2 | CPU 亲和 / 中断绑定 / 缓存友好访问 | [19](../19-systems-performance/) · [21](../21-hft-engineering/) | 有前后对比数据 |

- [ ] F1  
- [ ] F2  

### Phase G · HFT 向拓展（仍可在 Pi5 上练）

| 序 | 项目 | 对应模块 | 验收 |
|----|------|----------|------|
| G1 | 精确时间 / 时间戳校准 | [21](../21-hft-engineering/) | `clock_gettime` / TSC 类取舍说得清 |
| G2 | 无锁结构 + 内存池，模拟报文收发 | [21](../21-hft-engineering/) · [06](../06-cpp/) | 单测 + 简单压测 |

- [ ] G1  
- [ ] G2  

---

## 1.5 Project #1 · 树莓派 Linux 驱动视频课（项目式实践）

> **结论：可以。** 当作嵌入式学习的**第一个项目式实践课**，不是另开理论主线，也不替代仓库模块序。

### 定位（边界）

| 当它是什么 | 不当它是什么 |
|------------|--------------|
| **项目式实践**：环境 → `.ko` → GPIO / 中断 → 板上可演示 | 替代 [01 C](../01-c-language/) / [04 TLPI](../04-linux-userspace-api/) / 官方内核文档 |
| 把 C、命令行、内核模块串成闭环 | 与 [LEARNING-PATH-LOCKED](../LEARNING-PATH-LOCKED.md) 平行的第二大纲 |
| 产出：流程 + 可运行代码 + 本仓库笔记 | 只跟做、无验收勾选 |

### 和本表 Phase 的咬合

| 视频课内容 | 对齐 Labs | 说明 |
|------------|-----------|------|
| 交叉编译、SSH、板端跑通 | **D1**（可先做热身） | 把新板用起来 |
| 内核模块 HelloWorld、`insmod`/`rmmod` | 进 **B/C** 前的热身 | 不替代自编整内核（B1） |
| 字符设备 / GPIO / 中断 | **C1**（核心） | 做完必须对照官方 **RP1 / GPIO** |
| 设备树若课中有讲 | **C1–C3** | Pi5 以官网 DT / overlay 为准 |

主线仍是：**A → B → C → …**；视频是 **执行器**，本表是 **验收清单**。

### Pi5 特别注意

- 很多第三方课基于 **Pi4 / 旧内核**；Pi5 外设经 **RP1**，引脚与寄存器叙事可能不同。  
- **卡住先查** §0 官方文档；视频只当线索，不当唯一真理。  
- 中文旧教程可作思路，**版本与硬件以英文官网为准**。

### 验收（做完算进「嵌入式项目经验」）

- [ ] 交叉编译 / SSH 远程开发流程能复述并能独立重做  
- [ ] 自写最小 `.ko`：能 `insmod` / `rmmod`，`dmesg` 有输出  
- [ ] GPIO（或课中等价外设）用户态可观测；中断路径能讲清「谁注册、谁唤醒」  
- [ ] 笔记落入 [12-device-drivers-dt](../12-device-drivers-dt/) 或本目录子文件夹，含命令与踩坑  
- [ ] 至少对照一次官方 GPIO / 内核编译文档，标出课与 Pi5 的差异点  

未勾验收 = 只算「跟过课」，不算 Project #1 完成。

**课内笔记（随视频追加）：** [project-01-pi-driver-course/](./project-01-pi-driver-course/)  
- [01 · 用户态 / 内核 / 硬件三层图](./project-01-pi-driver-course/01-userspace-kernel-hardware.md)  
- [02 · microSD + 读卡器（刷机准备）](./project-01-pi-driver-course/02-microsd-card-reader.md)  
- GPIO 概念（Primer）：[8.5](../11-embedded-boot-build/primer-system-overview/chapter-08-device-driver-basics/8.5-gpio-basics.md) · 排针↔DTS：[8.6](../11-embedded-boot-build/primer-system-overview/chapter-08-device-driver-basics/8.6-gpio-header-vs-dts.md)

---

## 2. 与仓库 Phase 的咬合（别乱跳）

```
已具备：00 硬件词汇 · 01 C · 02 系统 · 04 用户态（再开 A）
   ↓
A 用户态板上实验
   ↓
07/09 内核地图清晰后 → B 编内核 / rootfs / 启动
   ↓
12 驱动主线 → C（核心）→ D 工程化
   ↓
15–18 网络后再 E；19–20 后 F；21 上 G
```

嵌入式支线总览仍见 [HFT-READING-ROADMAP §六](../HFT-READING-ROADMAP.md#六嵌入式-linux-支线10–14)。

---

## 3. 一句话

**官方文档常备查阅，不顺序通读；项目按上表 A→G 依次做，每步对齐仓库模块号。**  
**驱动视频课 = Project #1 实践壳（§1.5），验收勾上才算项目经验。**
