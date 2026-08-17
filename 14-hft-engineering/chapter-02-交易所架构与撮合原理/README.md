# 第2章 交易系统关键组件（索引）

> **原书第 2 章 · The Critical Components of a Trading System**

← 总览：[chapter-01](../chapter-01-高频交易基础与生态/README.md) · 引擎实现：[chapter-08](../chapter-08-超低延迟核心引擎开发/README.md)

---

## 小节索引

| 节 | 主题 | 一句话 |
|----|------|--------|
| [2.1](./2.1-关键路径总览与网关.md) | 关键路径总览与网关 | Gateway IN/OUT 分进程；T2T 优化每一跳 |
| [2.2](./2.2-订单簿构建器.md) | 订单簿构建器 | O(1) + 预分配数组；本地 LOB 是真相副本 |
| [2.3](./2.3-策略引擎与OMS.md) | 策略引擎与 OMS | Signal + Execution；违规单不出门 |
| [2.4](./2.4-网络通信与API.md) | 网络通信与 API | 订单 TCP · 行情 UDP 组播 |
| [2.5](./2.5-非关键路径组件.md) | 非关键路径组件 | C&C · Position · 异步日志 · Viewers |
| [2.6](./2.6-撮合原理.md) | 撮合原理 | Price-time priority → queue position |

## 本章小结

| 关键路径 | Gateway → Book → Strategy → OMS → Gateway |
|----------|---------------------------------------------|
| **Book** | **Vector/预分配** · **O(1)** · 本地 LOB |
| **Strategy** | **Signal + Execution** |
| **OMS** | **内部风控** 前置 |
| **非关键** | C&C · Position · Log · Viewers |

**下一步：** [chapter-03 订单簿实现](../chapter-03-订单簿深度与行情解析/README.md) · [chapter-08 C++ 引擎规范](../chapter-08-超低延迟核心引擎开发/README.md)
