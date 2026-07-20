# Harris ·《数字设计和计算机体系结构：ARM版》

> **结构约定（按你的要求）：一章一个文件夹 · 一小节一个 md**  
> 父章：[CSAPP Ch4](../../README.md) · [资料总表](../section-补充-数字电路学习资料.md)

## 文件夹地图

```
digital_logic_harris_arm/
├── ch01_binary/              # 第1章 … 每小节一个 md
├── ch02_combinational/       # 第2章  ★2.8 MUX  2.9 时序
├── ch03_sequential/          # 第3章  ★3.2 触发器  3.5 时序约束
├── ch04_hdl/                 # 第4章  整章跳过（仍保留空壳便于对照目录）
├── ch05_digital_blocks/      # 第5章  ★5.2 ALU  5.5 存储器阵列
├── ch06_architecture/        # 第6章  ★ARM ISA  + 6.8 x86
├── ch07_microarchitecture/   # 第7章  ★7.3 单周期  7.5 流水线冲突
├── ch08_memory/              # 第8章  ★8.3 Cache  8.4 VM
├── ch09_io_online/           # 在线第9章 I/O / RPi
├── cross_ref/                # ↔ CSAPP / STM32 / HFT
├── lab_logisim/
└── summary/
```

进入某一章先看该目录下的 `README.md`（小节索引表）。

| 章文件夹 | 书名 |
|----------|------|
| [ch01_binary](./ch01_binary/README.md) | 第1章 二进制 |
| [ch02_combinational](./ch02_combinational/README.md) | 第2章 组合逻辑设计 |
| [ch03_sequential](./ch03_sequential/README.md) | 第3章 时序逻辑设计 |
| [ch04_hdl](./ch04_hdl/README.md) | 第4章 HDL（跳过） |
| [ch05_digital_blocks](./ch05_digital_blocks/README.md) | 第5章 常见数字模块 |
| [ch06_architecture](./ch06_architecture/README.md) | 第6章 体系结构 |
| [ch07_microarchitecture](./ch07_microarchitecture/README.md) | 第7章 微结构 |
| [ch08_memory](./ch08_memory/README.md) | 第8章 存储器系统 |
| [ch09_io_online](./ch09_io_online/README.md) | 在线第9章 I/O |

## 档位（写在每个小节文件头）

- **精读** — 必抠（MUX / FF / ALU / ARM / 流水线冲突 / Cache）  
- **浅读** — 扫过建立直觉  
- **跳过** — 第4章及 HDL 相关  
- **可选\*** — 书中带星拓展  

## 学习节奏

1. `ch02` 2.8–2.9 + `ch05` 5.2/5.5 + Logisim  
2. `ch03` 3.2/3.5  
3. `ch06` ARM + 6.8 x86  
4. `ch07` 7.3/7.5 → `cross_ref/csapp_ch4_link.md`  
5. `ch08` Cache/VM → `cross_ref/hft_x86_timing.md`  
6. `ch09` 选做  

## 小节笔记模板（已预置）

```markdown
> **Core Concept:** …
> **Link Target:** …
> **档位:** 精读 | 浅读 | 跳过 | 可选*

## 状态
- [ ] 已读
- [ ] 已写要点

## 笔记
（子小节如 2.8.1 用本文件内 ## 标题展开）
```

## 一句话

**一章一夹、一小节一文件**；正文 1–8 + 在线 9；对照 CSAPP 盯 **ch07 / 7.5 流水线冲突**。
