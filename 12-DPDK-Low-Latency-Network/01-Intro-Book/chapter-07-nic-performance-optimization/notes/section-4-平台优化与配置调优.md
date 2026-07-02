## 4. 平台优化及其配置调优

> 硬件布局与 BIOS/内核参数决定 **理论上限**

---

### 一、PCIe Extended Tag

| | 关闭 | 开启 Extended Tag |
|---|------|-------------------|
| 并发未完成请求 | ~32 | **~256** |
| 影响 | 高带宽端口易 **流水线饥饿** | **40G+** 端口收益明显 |

**操作：** BIOS + OS 侧确认 PCIe 特性开启 — 与 [Ch6 TLP 带宽](../chapter-06-pcie-packet-io/notes/section-2-PCIe事务与带宽.md) 联调。

---

### 二、NUMA 就近原则

**同一 NUMA Node 内对齐：**

```
网卡 PCIe 插槽 ─┬─ DPDK lcore（-l / taskset）
               ├─ 大页 / mempool / mbuf（socket_id）
               └─ （可选）DDIO 本地内存 [Ch2]
```

**跨 Node / 跨 QPI/UPI** 访存 — tail latency 与吞吐 **双杀**。

→ [Ch2 DDIO 与 NUMA](../chapter-02-cache-and-memory/notes/section-6-DDIO与NUMA.md)

---

### 三、CPU 隔离：`isolcpus`

内核启动参数示例：

```text
isolcpus=2-7   # 专用 DPDK 核，不参与 CFS 负载均衡
```

| 收益 | 说明 |
|------|------|
| 无 **内核线程** 抢同一逻辑核 | 包处理 **抖动↓** |
| 配合 **taskset / EAL -l** | 控制面与数据面分离 |

→ [ULK Ch7 调度](../../../04-Understanding-Linux-Kernel/chapter-07-process-scheduling/) · [15 HFT 绑核](../../../15-HFT-Low-Latency-Practice/)

---

### 四、测试流量：防 RSS 倾斜

多队列 + RSS 压测时：

- 配置 **足够多随机流**（如 **随机源 IP**）  
- 避免 **单流单队列** 导致部分核 **空闲、部分核饱和** — 测不出真实扩展性  

→ [Ch8 RSS / 多队列](../chapter-08-flow-classification-multiqueue/notes/section-3-硬件流分类.md)

---

← [3. I/O 深度优化](./section-3-IO性能深度优化.md) · 下一节 [5. 队列长度](./section-5-队列长度及阈值设置.md)
