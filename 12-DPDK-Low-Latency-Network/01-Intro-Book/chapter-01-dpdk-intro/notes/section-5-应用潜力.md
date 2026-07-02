## 5. 软件包处理的应用潜力

> DPDK 足迹：**网络 · 计算 · 存储** 三大节点

---

### 一、加速网络节点（NFV）

| 场景 | 能力 |
|------|------|
| **NFV** | 虚拟路由器、虚拟防火墙等 **性能底座** |
| 吞吐示例 | 单系统 **三层转发 ~220 Gbit/s** 量级（书载案例） |

HFT：**共置机房** 内仍多见 **裸金属 DPDK** 而非 NFV，但 **转发/镜像/过滤** 节点可复用同类技术。

---

### 二、加速计算节点（C10K → C1M）

- 应对 **极高并发连接 / 极高 PPS**  
- **用户态协议栈**（DPDK 加速）支撑计算节点上的网络服务  

HFT：**UDP 组播行情**、自研 **tick 网关** — [chapter-05 组播](../chapter-05-组播行情接入.md)

对照内核 C10K 路径：[11 UNP epoll](../../../09-UNP-Vol1/) · [13-LKN L4](../../../11-Linux-Kernel-Networking/chapter-11-layer-4-protocols/)

---

### 三、加速存储节点（SPDK）

**SPDK**（Storage Performance Development Kit）：

- Intel 开源，借鉴 DPDK 思路  
- **NVMe 用户态驱动** + 轮询 + 大页  
- 显著提升 **iSCSI** 等 **IOPS**

与 HFT：**日志/WAL** 若走本地 NVMe，SPDK 与 DPDK **设计同构**（旁路内核、绑核、mempool）。

---

← [4. 方法论](./section-4-底层方法论.md) · 下一节 [6. 编程实例](./section-6-编程实例入门.md)
