# 学习路线：CSAPP vs Harris（嵌入式 Linux）

> **你的目标：** 嵌入式 Linux 驱动 / 系统 —— 理解硬件如何影响软件性能与正确性，**不自己设计 CPU、不画门级电路**。

← [组合深度](./学习深度_组合对Linux驱动.md) · [时序深度](./学习深度_时序对Linux驱动.md) · [CSAPP↔Harris](./cross_ref/csapp_ch4_link.md)

## 先纠一书名对应

| 你说的 | 实际是 |
|--------|--------|
| 「CSAPP 第4章用 **Verilog** 写简易 CPU」 | CSAPP Ch4 主线是 **Y86 + HCL**（教学用硬件控制语言）写 SEQ/PIPE，**不是** Verilog |
| **Verilog** | 在 **Harris 第4章**（本路线可整章跳过） |
| 「软件视角看指令怎么执行」 | ✅ 正是 **CSAPP Ch4**（及 Ch6 Cache / Ch9 VM）的强项 |

深度「够用且不冗余」——这句话对 **CSAPP 处理器+存储相关章** 成立；别误安在 Verilog 头上。

## 两本书怎么分工

| | **CSAPP** | **Harris（ARM 数电）** |
|--|-----------|------------------------|
| 视角 | **程序员 / 系统** 看硬件 | **硬件如何实现** ISA |
| Ch4 核心 | 取指→译码→执行；流水线、冒险、转发 | （Harris 对等在 **Ch7** 微结构） |
| 门/触发器 | 黑盒一带（HCL 积木） | 可抠到门级（**你已定为可跳/归档**） |
| 对驱动 | 流水线停顿、Cache miss、内存模型 → 性能与并发直觉 | setup/hold、模块黑盒、ARM 汇编词汇 |

```
CSAPP：指令「在 CPU 里怎么走完」+ 存储层次怎么拖慢你
Harris：组合/时序黑盒词汇 +（可选）ARM ISA 对照
         门级 / Verilog → 本路线不主攻
```

## 嵌入式 Linux：优先吃透什么

| 优先级 | 内容 | 书 |
|--------|------|-----|
| **P0** | 流水线 · 冒险 · 为何会 stall/bubble | CSAPP **Ch4**（PIPE） |
| **P0** | Cache · AMAT · 局部性 | CSAPP **Ch6** |
| **P0** | 虚拟内存 · 缺页 · 与驱动/用户态关系 | CSAPP **Ch9** |
| **P1** | 机器级程序 / 调用约定（真 x86 或再加 ARM） | CSAPP Ch3 · Harris Ch6 |
| **P1** | 组合延迟/毛刺、setup/hold、FIFO/FSM **思想** | Harris 已按 [组合](./学习深度_组合对Linux驱动.md)/[时序](./学习深度_时序对Linux驱动.md) 收窄 |
| **P2** | Harris Ch7 单周期/流水线（与 CSAPP Ch4 **同构对照**） | 有余力再看，加深「硬件绑定」 |
| **跳过** | Harris 门级搭 FF、Verilog 综合全流程 | — |

## 为何 CSAPP Ch4「刚好」

1. **不抠** 门与触发器内部，从软件合同往下看执行  
2. 用简化 ISA + 数据通路，打通 **指令 → 硬件一步步做完**  
3. 直接服务：高性能路径、调度/并发直觉、Cache miss 排查的底层语言  
4. HCL 读懂控制逻辑即可，**不必默写、不必真综合上板**

Harris 仍有用：当「硬件名词字典」和 ARM 补充；**主粮是 CSAPP 体系结构+存储，不是 Harris 门级。**

## 一句话

**Linux 驱动路线：CSAPP 处理器/缓存/VM 为主粮；Harris 只取黑盒时序与模块语义。Ch4 简易 CPU 在 CSAPP 里是 HCL/Y86，不是 Verilog。**
