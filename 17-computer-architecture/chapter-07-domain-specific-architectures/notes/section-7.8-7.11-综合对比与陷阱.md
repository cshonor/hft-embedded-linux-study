## 7.8–7.11 交叉问题、综合对比与陷阱

### 7.8 SoC 集成

现代 **手机/边缘 SoC** 集成：

```
应用 CPU + GPU + ISP/IPU + DSP + 基带 + …
```

DSA 以 **IP 块** 形式复用 — 面积与功耗预算内各司其职。

| HFT 视角 |
|----------|
| 服务器异构：**x86 + SmartNIC FPGA + 可选 GPU** — 同类「分工」逻辑 |

---

### 7.9–7.10 CPU vs GPU vs TPU（Roofline）

书中用 **Roofline** 对比 **Haswell CPU、NVIDIA K80 GPU、Google TPU**（DNN 推理）：

| 结论（数量级） | TPU vs 通用 |
|----------------|-------------|
| **性能** | 约 **15–30×**（推理任务） |
| **性能/瓦特** | 约 **30–80×** |

**原因：** 去掉乱序/SMT 等、**INT8**、巨大脉动阵列、scratchpad 减数据移动。

→ [Ch4 Roofline](../../chapter-04-vector-simd-gpu/notes/section-4.3-多媒体SIMD扩展.md)

| HFT 视角 |
|----------|
| 同一 Roofline 可用于判断：**行情解析** 是算力 bound 还是内存 bound |
| 高 MAC 吞吐 **不能** 直接换成 **低 ns 发单** — 指标是 **延迟分位数** 非 IPS |

---

### 7.11 谬误与陷阱

| 类型 | 内容 | 真相 |
|------|------|------|
| **谬误** | 定制芯片必花 **1 亿美元** | 简化 DSA（无复杂通用核）**NRE 可低得多** |
| **陷阱** | 只用 **吞吐量 (IPS)** 评 DNN 硬件 | 在线服务有 **严格响应时间** — 与 TPU 确定性设计相关 |
| **陷阱** | 忽视 **编程/移植成本** | DSL、编译器、Shell/Role 生态决定落地 |

---

### 全书 Hennessy 正文章节收束

```
Ch1 量化框架 → Ch2 内存 → Ch3 ILP → Ch4 DLP → Ch5 TLP → Ch6 WSC → Ch7 DSA
```

**HFT 路线对照：**

| 章 | HFT 权重 |
|----|----------|
| Ch2、Ch5 | 🔴 订单簿、无锁、NUMA |
| Ch1、Ch3 | 🟡 延迟分解、分支 |
| Ch4、Ch6、Ch7 | ⚪ 场景触发（回测云、ML、FPGA NIC） |

---
