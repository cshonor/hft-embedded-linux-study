# Ch2 · 实验环境（树莓派 5 适配）

> **《ARM64体系结构编程与实践》** Ch2 · 原书基准 = **Pi4B（BCM2711 · Cortex-A72）**  
> **本仓库硬件** = **Pi5（BCM2712 · Cortex-A76 · ARMv9-A）**  
> [章导读](../README.md) · [全书总结](../../BOOK-SUMMARY.md) · [AArch64 命名](../../AARCH64-NAMING.md) · [模块级适配总览](../../PI5-ADAPT.md)

---

## 本章定位（适配后）

原书 Ch2：串口 / JTAG / BenOS / QEMU 双通路上板。  
**你手里是 Pi5**：AArch64 概念与指令 **100% 通用**；差异在 **板级外设基址、GPU 启动细节、JTAG/`config.txt`**。

| 层 | Pi4B 原书 | 你的 Pi5 | 策略 |
|----|-----------|----------|------|
| A64 / EL / MMU / 屏障 / 原子 | 通用 | 通用 | **主力 QEMU**（`-cpu cortex-a76`） |
| PL011 / GIC 寄存器基址 | BCM2711 | **BCM2712 不同** | 勿直接抄 BenOS 上板 |
| AArch32 | 可选 | **已移除** | 本书本就 A64，无负面影响 |
| 工具链 | aarch64-linux-gnu-* | 同左 | **完全不变** |

---

## 1. 重要硬件差异

1. **A76 无 AArch32** — 只跑纯 AArch64；不能跑 32 位裸机/32 位系统。本书实验本来就是 A64，对你无害。  
2. **SoC：BCM2712 ≠ BCM2711**  
   - PL011 串口、GIC（Pi5 侧常谈 **GIC-600** 一类实现）**寄存器基地址变了**  
   - 原书 BenOS 直接写 4B 外设地址 → **Pi5 上会无输出/死机**  
   - **解法：** CPU 架构实验先在 **QEMU virt** 做完；外设再改基址适配 Pi5  
3. **启动：** 仍是 VideoCore GPU 先引导 ARM 核（与 4B 同逻辑）  
4. **JTAG：** Pi5 引脚与 `config.txt` 项和 4B **不同**；前期可不急上 J-Link  

---

## 2. 实操路线（推荐）

### 阶段① · QEMU 优先（约 Ch3–21 的 CPU 侧）

指令集、汇编、链接、EL、页表、Cache、屏障、原子、简易 OS 上下文切换 —— **与板外设无关**，`virt` 平台可复现。

```bash
qemu-system-aarch64 -M virt -cpu cortex-a76 -nographic -kernel benos.bin -s -S
```

- `-cpu cortex-a76`：贴近 Pi5 核型号  
- `-s -S`：GDB stub；用 `aarch64-linux-gnu-gdb` 连上单步  

**优点：** 不纠结 BCM2712 手册、不反复烧 SD；精力留在 AArch64 本身。

### 阶段② · 再上树莓派 5（外设相关）

串口、GIC 中断等板级实验时改两处：

1. 裸机代码里 **PL011 / GIC 基址** → BCM2712  
2. boot 分区 **`config.txt`** → Pi5 串口 / JTAG  

> BenOS 原版为 4B：原样丢进 Pi5 boot → **串口常无输出**。

### 调试方式

| 方式 | 建议 |
|------|------|
| **USB-TTL 串口** | 可用：TX/RX/GND，**不接 5V**；UART 映射改 Pi5 的 `config.txt` |
| **J-Link** | 引脚≠4B，要查 BCM2712；**前期可只用 QEMU + 串口** |

---

## 3. 工具链（不变）

`aarch64-linux-gnu-gcc` / `objdump` / `readelf` / `objcopy` —— 同一套 A64 二进制，**QEMU 与 Pi5 都能跑**（板上要外设适配后才有可见输出）。

---

## 4. 设备两套用途并存

| 环境 | 用来做什么 |
|------|------------|
| **QEMU** | 裸机 BenOS：指令 / MMU / 异常 / 屏障 / OS 骨架 |
| **Pi5 实物** | 适配后的裸机外设；**64 位 Raspberry Pi OS** 做 TLPI / 驱动课用户态·内核模块 |

烧卡与 Imager 步骤可对照：[project-01 · microSD](../../../../projects/P5-raspberry-pi-embedded/P5f-pi-driver-course/02-microsd-card-reader.md)。

---

## 5. 风险提醒

> **不要**把 4B 裸机 `bin` 直接丢进 Pi5 boot 分区指望「指令一样就能亮」。  
> CPU 指令集兼容；**板上外设地址不兼容** → 黑屏/无串口。

---

## 6. 原书 Ch2 仍要读什么

即使主力 QEMU，原书 Ch2 仍有用：

- BenOS 链接脚本、启动汇编（多核仅主核）、Makefile 流程  
- QEMU + GDB 调试习惯（把 `-cpu` 换成 `cortex-a76`）  
- 「串口驱动 = MMIO 写寄存器」的**方法**（地址表换成 2712 即可）

板级接线、4B 专用 `config.txt`、4B JTAG 图 → **对照读，勿照抄到 Pi5**。

---

## Checklist（Pi5 适配版）

- [ ] 交叉工具链 `aarch64-linux-gnu-*` 可用  
- [ ] QEMU：`virt` + `-cpu cortex-a76` 能跑 BenOS / 书中样例并 GDB 断点  
- [ ] 理解：架构实验 ≠ 必须立刻上 Pi5 裸机  
- [ ] 上板前：查清 BCM2712 串口/GIC 基址与 Pi5 `config.txt`  
- [ ] Pi5 可另装 64-bit OS，供用户态/驱动课（与 BenOS 裸机实验分开卡或分开流程）

下一章可继续：**第 3 章 LDR/STR**（实验默认按 QEMU 路线写）。
