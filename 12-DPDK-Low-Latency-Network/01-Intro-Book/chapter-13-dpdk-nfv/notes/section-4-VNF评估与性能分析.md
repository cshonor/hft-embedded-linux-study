## 4. VNF 特性评估与性能分析方法论

---

### 一、VNF 分型（部署前）

| 类型 | 特征 | 示例 |
|------|------|------|
| **I/O 密集** | 包速率、带宽、PCIe | 数据面 **vRouter**、vSwitch |
| **计算密集** | 复杂协议、状态机 | 控制面 **vBRAS** 部分逻辑 |
| **内存带宽密集** | 大表、深 buffer | 路由表、DPI 规则 |

**错误选型：** 用 I/O 指标评 **控制面** VNF — 或反之。

→ [Ch5 模块划分](../chapter-05-packet-forwarding/notes/section-2-网络处理模块划分.md)

---

### 二、闭环性能分析法

业界常用 **「非虚 → 虚拟、自上而下、闭循环」**：

```
1. 设定 baseline（裸金属 / 单 VNF）
2. 分析瓶颈（硬件 / OS / 软件）
3. 改一项配置或设计
4. 复测 — 有效则保留，否则回退
5. 重复直至逼近硬件极限
```

与 [03 SysPerf](../../../13-Systems-Performance-2nd/) **USE 法、实验迭代** 同构。

---

### 三、系统级调优维度

| 层次 | 手段 | 全书对应 |
|------|------|----------|
| **硬件** | 加核、升级网卡、NUMA | Ch7、Ch8 |
| **OS** | 大页、`isolcpus` | Ch2、Ch7、Ch10 |
| **软件** | DPDK poll、SIMD、Pipeline | Ch3、Ch5、Ch7 |
| **虚拟化** | SR-IOV vs Virtio、vhost | Ch10–12 |

**HFT 迁移：** 闭环法同样适用于 **裸金属 tick 网关** — 先 baseline 再动一项。

---

← [3. OPNFV 与 API](./section-3-OPNFV与DPDK扩展API.md) · 下一节 [5. VNF 优化设计](./section-5-VNF深度优化设计.md)
