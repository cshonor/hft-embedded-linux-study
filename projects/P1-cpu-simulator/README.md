# P1 — CPU 模拟器（实战指南）

> 用 Logisim / Verilog 搭一个 8-bit CPU，把「程序在硬件上怎么跑」从黑盒变白盒。  
> **做法：项目驱动，[`00`](../../00-digital-logic-cpu/) 笔记当字典——先上路，卡住再查。**

---

## 核心理念（先读这段再动手）

仓库里模块 `00` 的笔记是**参考书 / 地图**，不是读完才配出发的课本。

| | 先读完笔记再写项目 | **先写项目再查笔记（推荐）** |
|---|---|---|
| 动力 | 低（看不到终点） | 高（目标清晰：「让这条指令跑起来」） |
| 查笔记 | 从头到尾，90% 用不上 | **带着问题查**，查到的立刻能用 |
| 留存 | 一周忘大半 | 亲手接通过的信号记得牢 |
| 反馈 | 慢 | 快（波形 / 探针对不对立刻知道） |

> **笔记是地图，项目是路。不能把地图背完再出发——先上路，迷路了再看地图。**

你现在缺的往往不是「再读 100 篇」，而是**被一个具体问题卡住**，然后查 → 接好 → 跑通 → 想做下一个。

---

## 最小预备（1–2 小时，不是 1–2 周）

**不要**读完 `00` 全书。只翻标题 / 一张图，知道「有这么个东西」即可：

| 瞄一眼 | 只要留下印象 |
|--------|----------------|
| [5.2 算术电路](../../00-digital-logic-cpu/ch05_digital_blocks/5.2_算术电路.md) | ALU ≈ 加法器 + MUX 选运算 |
| [7.3 单周期处理器](../../00-digital-logic-cpu/ch07_microarchitecture/7.3_单周期处理器.md) | PC → IMem → Reg → ALU → Mem 这条路 |
| [6.4 机器语言](../../00-digital-logic-cpu/ch06_architecture/6.4_机器语言.md) | 指令大致有 R / I / J 三类格式 |

细节**不做预习**——搭电路时自然会逼你回来查。

可选暖手（各 10 分钟）：[lab_logisim](../../00-digital-logic-cpu/lab_logisim/) 里拖一个加法器、一个 MUX 看输出。

---

## 项目目标

在硬件层面建立 CPU 模型——ALU 怎么算、寄存器怎么存、控制器怎么按时节拍驱动指令周期。  
不追求商业级，追求**自己讲得通每根信号为什么这么连**。

## 交付物

- [x] ALU（加减与/或/移位 + 标志位 Z/C/N/V）→ [part-a-alu-host](./part-a-alu-host/)
- [x] 寄存器堆（4 个通用寄存器 + PC + IR）
- [x] 指令译码器（自定义 16-bit 指令集，R/I/J）
- [x] FSM 控制器（取指→译码→执行→写回，多周期）
- [x] 内存接口（IMem 16-bit + DMem 256×8）
- [x] 三个程序：加法累加、斐波那契、内存拷贝 → [part-b-multicycle](./part-b-multicycle/)

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`00` digital-logic-cpu](../../00-digital-logic-cpu/) | 组合/时序逻辑、寄存器、FSM、CPU 数据通路 |

## 前置

无（Phase1 第一个项目）。

## 学习目标

- setup/hold、时钟、寄存器锁存的物理含义  
- 控制信号如何由 FSM 按节拍产生  
- 指令集 = 数据通路 + 控制信号的编码约定  
- 为后续 `02` CSAPP Ch4（Y86 流水线）建立硬件直觉  

---

## 里程碑 +「卡住时翻哪篇」

### 今天就可以开的第一步

本机没有 Logisim 也能动手——WSL 里：

```bash
cd projects/P1-cpu-simulator/part-a-alu-host && make test
cd ../part-b-multicycle && make test
./cpu_sim --trace sum    # 按拍看 FETCH/DECODE/EXECUTE/WB
```

想看见门：打开 Logisim → 拖一个加法器 → 接两个输入 → 对照 part-a 的 C 标志位。  
从这一步开始，**不用「准备好」**。

### M1 — ALU + 寄存器堆（手动单步能算）

| 卡住了… | 翻这里 |
|---------|--------|
| 减法怎么做？补码？ | [5.2 算术电路](../../00-digital-logic-cpu/ch05_digital_blocks/5.2_算术电路.md) |
| 标志位 Z / C / N / V | [5.2](../../00-digital-logic-cpu/ch05_digital_blocks/5.2_算术电路.md) ·（A64 对照可后看）[NZCV](../../07-arm-architecture/aarch64-practice/NZCV.md) |
| 用 MUX 选加/减/与/或 | [2.8.3 MUX](../../00-digital-logic-cpu/ch02_combinational/2.8.3_MUX.md) · [2.8 组合模块](../../00-digital-logic-cpu/ch02_combinational/2.8_组合逻辑模块.md) |
| 加法器内部 | [2.8.5 加法器](../../00-digital-logic-cpu/ch02_combinational/2.8.5_加法器.md) |
| 寄存器怎么写？WE？ | [5.4 时序电路模块](../../00-digital-logic-cpu/ch05_digital_blocks/5.4_时序电路模块.md) · [3.2 锁存/触发器](../../00-digital-logic-cpu/ch03_sequential/3.2_锁存器和触发器.md) |
| Logisim 怎么搭 | [lab_logisim](../../00-digital-logic-cpu/lab_logisim/) · [数字电路补课](../../02-computer-systems/chapter-04-processor-architecture/notes/section-补充-数字电路学习资料.md) |

**M1 做完回头扫一眼 5.2：** 漏了 V？MUX 能优化？没用到的概念先跳过——正确决策。

### M2 — 指令集 + 译码器真值表

| 卡住了… | 翻这里 |
|---------|--------|
| R / I / J 格式长什么样 | [6.4 机器语言](../../00-digital-logic-cpu/ch06_architecture/6.4_机器语言.md) |
| 汇编助记符 ↔ 编码 | [6.2 汇编语言](../../00-digital-logic-cpu/ch06_architecture/6.2_汇编语言.md) |
| 译码器 / 编码器 | [2.8.2 译码器](../../00-digital-logic-cpu/ch02_combinational/2.8.2_译码器.md) · [2.8.1 编码器](../../00-digital-logic-cpu/ch02_combinational/2.8.1_编码器.md) |

### M3 — FSM 控制器，多周期跑通第一条指令

| 卡住了… | 翻这里 |
|---------|--------|
| FSM 怎么画、怎么出控制信号 | [3.4 有限状态机](../../00-digital-logic-cpu/ch03_sequential/3.4_有限状态机.md) |
| 取指→译码→执行→写回长什么样 | [7.3 单周期](../../00-digital-logic-cpu/ch07_microarchitecture/7.3_单周期处理器.md)（先建立整图；本项目用**多周期**拆拍） |
| 数据通路总览 | [7.1 引言](../../00-digital-logic-cpu/ch07_microarchitecture/7.1_引言.md) |
| 为何多周期 / 性能直觉 | [7.2 性能分析](../../00-digital-logic-cpu/ch07_microarchitecture/7.2_性能分析.md) |

建议第一条指令：`MOVI R0, #5`——跑通后 R0 真的是 5，再加下一条。

### M4 — 三个程序跑通，能按周期讲信号

| 卡住了… | 翻这里 |
|---------|--------|
| 访存、指针式拷贝在硬件上要什么信号 | [7.3](../../00-digital-logic-cpu/ch07_microarchitecture/7.3_单周期处理器.md) 数据通路图 + 自己的译码表 |
| 和 CSAPP Y86 对照「一条指令拆几级」 | [02 Ch4 README](../../02-computer-systems/chapter-04-processor-architecture/README.md)（**后置**，不挡 P1） |

---

## 学习循环（贴在显示器旁边）

```
卡住 → 查上表笔记 → 接线/改 FSM → 跑通 → 成就感 → 下一个零件
```

不要：读完 00 →「觉得懂了」→ 再开 Logisim（那条路读不完、也留不住）。

---

## 工具

- **Logisim**（或 Logisim-Evolution）：图形化，快速验证——**默认推荐**  
- Verilog（iverilog + GTKWave）：更接近真实，M1 之后可选  
- 深度约束：组合/时序取黑盒语义，门级不主攻——能讲通功能即可  

## 目录约定

```
P1-cpu-simulator/
  README.md              ← 本指南
  part-a-alu-host/       ← 8-bit ALU + Z/C/N/V（可 make test）
  part-b-multicycle/     ← 多周期 CPU + 三个程序（可 make test）
```

踩坑请自己写——**那才是你的知识**；`00` 里的长文继续当字典查。  
不要空的 `src/` / `notes/` / `refs/` 占位。

## 状态

🔄 进行中 — part-a ALU 与 part-b 多周期 CPU 已可在 WSL `make test`。Logisim 电路仍可选。

← [projects 总览](../README.md) · [00 模块](../../00-digital-logic-cpu/)
