# 第8章 超低延迟核心引擎开发

> **Gateway · Book Builder · Strategy · OMS** — 关键路径软件架构

← 总览：[chapter-01 §1](./chapter-01-高频交易基础与生态.md#1-系统核心架构关键路径)

---

## 1. Gateway IN / OUT

| | **Gateway IN** | **Gateway OUT** |
|---|----------------|-----------------|
| **方向** | 交易所 → 系统 | 系统 → 交易所 |
| **数据** | Market data（增量/快照） | New/Cancel/Replace |
| **热点** | **解析**（二进制协议）· 时间戳 · 入队 | **序列化** · 会话序 · 发送 |
| **部署** | 常 **独立进程** · **绑核** · kernel bypass 收包 |

**设计要点：**

- IN 路径：**尽早打时间戳**（硬件 TS 优于软件）
- OUT 路径：与 Strategy **解耦** — 经 OMS 再出网
- 双网关避免 **收发包争用** 同一核/NIC 队列

→ [chapter-06 协议](./chapter-06-低延迟网络与协议优化.md)

---

## 2. Book Builder

**输入：** Gateway IN 的 **add/modify/delete/trade** 事件。

**输出：** 本地 **Limit Order Book** — 至少 **BBO（best bid/offer）**，常含 L2 深度。

| 要求 | 说明 |
|------|------|
| **更新复杂度** | 理想 **O(1)** 或接近（price level 索引 +  intrusive list） |
| **内存** | **预分配** level 池；禁止热点 `new` |
| **一致性** | 序列号 / 缺口 **recovery** 策略（快照 + 增量） |

→ [chapter-03 订单簿](./chapter-03-订单簿深度与行情解析.md)

---

## 3. Strategy（Signal + Execution）

```
Book update ──► Signal（是否交易）
                    │
                    ▼
              Execution（价格/量/类型）
                    │
                    ▼
                 OMS 订单请求
```

| 模块 | 延迟敏感点 |
|------|-----------|
| **Signal** | 分支少 · 无虚函数 · 固定小数/整数 |
| **Execution** | 与风控参数 **编译期或启动期** 固定 |

**与 Book 关系：** 读 **无锁** 或 **单写者** 快照；避免 Strategy 持锁读全书。

---

## 4. Order Manager (OMS)

| 职责 | T2T 价值 |
|------|----------|
| **订单状态机** | New → Ack → Partial → Fill / Cancel |
| **内部风控** | 最大量/敞口/频率 — **违规本地拒单**（省 RTT） |
| **合规检查** | 禁售/价格带 — 失败 **不发 OUT** |
| **审计日志** | 常 **异步** 环出（Ch7），不在热点同步写盘 |

---

## 5. C++ 引擎编码规范（热点路径）

| Do | Don't |
|----|-------|
| **CRTP**、模板策略 | 虚函数多态 |
| **对象池 / 栈缓冲** | `malloc` / `std::vector` 扩容 |
| `memory_order_acquire/release` | 默认 seq_cst |
| `noexcept`、错误码 | 热点 `throw` |
| **Branch prediction 友好** | 深层 if-else 链 |

→ [chapter-01 §4 C++](./chapter-01-高频交易基础与生态.md#4-编程语言选择) · [chapter-07 无锁](./chapter-07-无锁数据结构与内存布局.md)

---

## 6. Java / Python 边界

- **Java**：非 μs 路径（风控报表、配置、监控）；GC 隔离
- **Python**：研究回测；生产 **调 C++ .so**

→ [chapter-01 §4](./chapter-01-高频交易基础与生态.md#4-编程语言选择)
