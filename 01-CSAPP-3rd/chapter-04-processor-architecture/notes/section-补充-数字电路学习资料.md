## 补充 · 数字电路学习资料（服务 CSAPP Ch4 · HFT · STM32）

> [章导读](../README.md) · 电路零件 [§4.2](./section-4.2-HCL逻辑与组合电路.md) · 流水线 [§4.4](./section-4.4-流水线原理与局限.md) · 冒险 [§4.5](./section-4.5-PIPE流水线与冒险.md)

> **目标：** 吃透 CSAPP 第四章；服务 **HFT (x86)** + **嵌入式 STM32 / FreeRTOS**。  
> **原则：** 程序员友好 — **避开复杂模电**，聚焦 CPU / MCU 底层数字逻辑。

---

### 专属学习路线（按目标排序）

**中文快车道（零基础）：**

1. B 站 **清华王红** 前 6 章 + **阎石** 必学章节  
2. **Logisim-Evolution** 搭 MUX / 加法器 / 寄存器  
3. 回读 CSAPP **§4.2～4.5**  
4. 进阶 **MIT 6.004** → STM32 时序拓展  

**纯外文车道（CSAPP 配套 · 推荐主力）：** 见下文「外文专线」— *Code* → **Harris** → 6.004+Logisim → Wakerly → COD/Hennessy。

**六大核心（其余浅看或跳过）：** MUX · 全加器 · D 触发器 · 寄存器 · 存储器 · 时序延迟（setup/hold）

---

### 一、国内教材（零基础 · 看懂 §4.2/4.3）

#### 1. 《数字电子技术基础》阎石 · 第六版（高校标配）

**必学（跳过模电 / AD·DA / 脉冲）：**

| 章 | 抓什么 | 对应 CSAPP / 你的栈 |
|----|--------|---------------------|
| 1 数制码制 | 补码、二进制 | ALU、Ch2 |
| 2 逻辑代数 | 布尔、卡诺图 | MUX 化简直觉 |
| 3 门电路 | 与/或/非、CMOS 基础 | 门级底层 |
| **4 组合逻辑** | **MUX、全加器、译码器** | **§4.2 重中之重** |
| 5 触发器 | D 触发器 | 寄存器最小单元 |
| 6 时序逻辑 | 寄存器、计数器、移位 | 流水线锁存器 |
| 7 半导体存储器 | SRAM/ROM | Cache / MCU 内存直觉 |

**直接跳过：** 可编程器件、脉冲整形、AD/DA（软件底层用不上）。

中文细、框图多 — 看完再读 CSAPP 电路图会轻松很多。

#### 2. 《电子设计从零开始》（杨欣）

几乎无复杂公式；纯软件出身建立数字直觉；顺带看 STM32 GPIO 硬件结构。

---

### 外文专线（完全英文 · 贴 CSAPP / HFT / STM32 · 避 PCB）

> 全部聚焦 **CPU/MCU 内部数字逻辑**；不学模拟电路、制版、电源。与 CSAPP Ch4、Y86 SEQ/PIPE 同思路。

#### 外文学习顺序（按你的目标定制）

1. **打底：** *Code: The Hidden Language…*（**Petzold**）— 故事向，建立直觉  
2. **主力：** **Harris & Harris《Digital Design and Computer Architecture》**（ARM 或 RISC-V 版二选一）  
3. **实操：** [MIT 6.004](https://computationstructures.org) 讲义 + **Logisim**（及课程 Beta 仿真）  
4. **巩固：** Wakerly《Digital Design: Principles and Practices》— 时序、竞争、延迟  
5. **拔高：** Patterson & Hennessy《Computer Organization and Design》→ 再接仓库 [03 Hennessy 定量体系结构](../../../03-Computer-Architecture-6th/)（HFT 流水线/缓存）

---

### 二、外文教材 · 入门首选（与 Ch4 联动）

#### 1. 《Digital Design and Computer Architecture》（Harris & Harris）— **最适合你**

门 → MUX → 全加器/ALU → 触发器 → 寄存器堆 → **五级流水线 CPU** — 与 Y86 **SEQ/PIPE 同一套搭积木逻辑**；回刷 §4.2～4.5 框图会快很多。

| 版本 | 适配你的哪条线 |
|------|----------------|
| **ARM 版** | STM32 / 嵌入式案例全程偏 ARM |
| **RISC-V 版** | 流水线、缓存、冒险讲得更深 → 服务 **HFT 低延迟直觉**（真干活仍是 x86，原理通用） |

极少模电；站在软件/体系结构视角；深挖 **MUX、转发旁路、流水线寄存器、时序延迟** — 正是 Ch4 痛点。

#### 2. 《Digital Design: Principles and Practices》（John F. Wakerly）

美计算机系标配；MUX、超前进位加法器、时序、竞争延迟严谨。CSAPP **HCL 式**「用布尔/选择描述电路」的味道与这类教材一脉。适合吃透组合/时序后，盯每一条信号通路。

#### 3. 进阶衔接（数字电路之后）

| 书 | 作用 |
|----|------|
| **《Computer Organization and Design》** Patterson & Hennessy | 数字电路 → 真机流水线/缓存/指令延迟；支撑 HFT 调优直觉 |
| **《Code》** Charles Petzold | 零公式故事书：开关 → CPU/汇编；硬啃 Harris 前的软铺垫 |
| **Hennessy《Computer Architecture: A Quantitative Approach》** | 仓库已有笔记：[03-Computer-Architecture-6th](../../../03-Computer-Architecture-6th/) — 定量微架构 |

---

### 三、MIT 英文公开课（免费讲义）

#### MIT 6.004 Computation Structures — **强推**

- 官网：https://computationstructures.org  
- 路径：晶体管 → 门 → MUX/加法器 → 时序寄存器 → **五级流水线 CPU** — 与 CSAPP Ch4 **近 1:1**；Y86 教学思路同源  
- 资源：英文 lecture PDF、视频、电路习题 — 免费  
- 重点：流水线寄存器、**转发 MUX**、stall 控制 — 解「MUX 在 CPU 里干什么」  

另：**Berkeley EECS150** — setup/hold、stall 与信号延迟（HFT 时序拔高）。

---

### 四、免费英文仿真

| 工具 | 用法 |
|------|------|
| **Logisim-Evolution** | 英文界面；MUX、多位加法器、流水线分段 — 复现 CSAPP 模型 |
| **MIT 6.004 Beta 等课程仿真** | 配套课：看冒险、转发旁路 |

**建议实操：** 4 位全加器 · 4 选 1 MUX · D 触发器+8 位寄存器 · 简易五级锁存  

**浏览器备选：** DigiSim.io — 碎片时间看信号流转。

---

### 五、中文视频（可选 · 与「一」配合）

1. **清华王红《数字逻辑电路》**（B 站）— MUX、全加器、触发器  
2. **哈工大李琰《数字电子技术基础》** — 组合/时序核心  
3. **中国大学 MOOC · 上交大数字电子** — 计数器/移位 ↔ STM32 定时器直觉  
4. MIT 6.004 / Berkeley EECS150 — 亦有 B 站搬运，正式讲义仍以英文官网为准  

---

### 避坑（中英共通）

1. **不必学模电**（运放、三极管放大、电源）— 除非画 PCB / 焊板  
2. **不必深挖 Verilog/FPGA 产品开发** — 看懂逻辑即可（CSAPP HCL 同理）  
3. 资源再多，优先死磕 **六大核心**；外文主力认准 **Harris + 6.004**  

---

### 口述巩固 · 自测

1. 外文主力该啃哪本？ARM / RISC-V 版怎么选？  
2. 阎石哪一章最对应 §4.2？ — **组合逻辑（MUX/全加器）**  
3. 6.004 官网？和 CSAPP Ch4 什么关系？  
4. 学完数字电路下一本体系结构接什么？ — COD → [Hennessy](../../../03-Computer-Architecture-6th/)  

---

← [本章导读](../README.md)
