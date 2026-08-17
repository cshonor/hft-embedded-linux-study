# 第14章 Python 高性能混合架构（索引）

> **原书第 10 章 · Python – Interpreted but Open to High Performance**
> **研究生态 · GIL · C++/Python 混合 · Boost / Cython / SWIG**

← [chapter-09 Java/JVM](../chapter-09-java-jvm-低延迟系统/README.md) · [chapter-08 C++ 微秒征途](../chapter-08-超低延迟核心引擎开发/README.md)

---

## 本章定位

Python **不适合 μs 级订单执行**，却是 **策略研究、数据与建模** 的绝对主力。原书 **Ch10** 核心架构：

> **Python 负责控制与研究 + C++ 负责极速执行** — 混合 HFT 系统。

| 主题 | 本章 | 交叉 |
|------|------|------|
| μs 热点执行 | **14.3–14.4** | [Ch8](../chapter-08-超低延迟核心引擎开发/README.md) |
| LOB / Gateway / OMS | C++ 扩展 | [Ch8 §7](../chapter-08-超低延迟核心引擎开发/8.7-关键路径组件.md) |
| Java 低延迟对照 | — | [Ch9](../chapter-09-java-jvm-低延迟系统/README.md) |

**编号说明：** 本仓库 **Ch10 = 原书 Ch7（测量/日志）**；原书 **Ch10 Python → 本章 Ch14**（与 Ch13 策略同为扩展编号）。

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [14.1](./14.1-Python在HFT中的角色.md) | Python 在 HFT 中的角色 | pandas/ML 研究 + C++ 执行的流水线 |
| [14.2](./14.2-为什么Python慢.md) | 为什么 Python 慢 | 解释执行 · 无 JIT · GIL |
| [14.3](./14.3-CPP扩展绑定.md) | C/C++ 扩展绑定 🔴 | Boost / Cython / ctypes / SWIG 四工具 |
| [14.4](./14.4-优化四步法.md) | 优化四步法 | 向量化 → Profile → C++ 重写 → 分工 |
| [14.5](./14.5-三语言分工总览.md) | 三语言分工总览 | C++ μs 路径 · Java 引擎 · Python 研究 |

## 本章小结

| 原书 Ch10 主题 | 手段 |
|----------------|------|
| **角色** | pandas/ML 研究 · C++ 执行 |
| **瓶颈** | 解释 · 无 JIT · **GIL** |
| **破局** | `.so` 扩展 — Boost / Cython / SWIG / ctypes |
| **实战** | 向量化 → Profile → C++ 重写 → **明确分工** |

**不要因慢抛弃 Python** — 用 C++ 封装热点，享受 **Python 快速研发 + C++ μs 执行**。

## 原书章节对照

| 原书 | 本仓库 |
|------|--------|
| Ch10 §1 Python 角色 | **本章 14.1** |
| Ch10 §2 慢的原因 | **本章 14.2** |
| Ch10 §3 C++ 集成 | **本章 14.3** |
| Ch10 §4 优化步骤 | **本章 14.4** |
| Ch11 FPGA/Crypto | **Ch15** |
| Ch7 测量/日志 | **Ch10** |

## Python 速查（Do / Don't）

| Do | Don't |
|----|-------|
| **NumPy 向量化** | 嵌套 Python `for` 算大数组 |
| **Profile 后 C++ 重写瓶颈** | 全栈纯 Python 上生产 μs 路径 |
| **Boost/Cython 封装 .so** | 热点依赖 GIL 多线程 |
| Python **编排/回测/配置** | Python **直连交易所发单** |
