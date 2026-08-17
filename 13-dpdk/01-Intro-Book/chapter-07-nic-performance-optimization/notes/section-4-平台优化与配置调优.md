## 4. 平台优化及其配置调优

> 硬件布局与 BIOS/内核参数决定 **理论上限**

---

### 一、BIOS 调优清单

| 设置 | 推荐值 | 原因 |
|------|--------|------|
| **Hyper-Threading** | 关闭（HFT 热路径） | 消除 cache 争用 → 确定性 |
| **Turbo Boost** | 开启 | 提升单核频率（HFT tick 路径受益） |
| **C-States** | C0/C1（禁用深睡） | 深度 C-state 唤醒延迟 ~10-100μs |
| **NUMA Interleave** | 禁用 | 强制本地内存访问 |
| **DDIO** | 开启 | DMA 直达 LLC |
| **PCIe ASPM** | 禁用 | 链路省电增加延迟 |

---

### 二、内核启动参数

```bash
# /etc/default/grub → GRUB_CMDLINE_LINUX
# HFT DPDK 专用核配置
isolcpus=2,3,4,5           # 隔离 DPDK 核 — 不参与 CFS 调度
nohz_full=2,3,4,5          # 无滴答 — 减少时钟中断
rcu_nocbs=2,3,4,5          # RCU 回调迁移到其他核
intel_iommu=on iommu=pt    # 开启 IOMMU（VFIO 需要），pass-through 模式
transparent_hugepage=never # 禁用 THP — DPDK 用显式大页
nmi_watchdog=0             # 关闭 NMI watchdog — 减少额外中断
audit=0                    # 关闭审计 — 减少系统调用开销
processor.max_cstate=1     # 限制 C-state 深度
idle=poll                  # idle 时轮询不睡眠（HFT 极端配置）
```

```bash
# 应用后更新 GRUB
grub2-mkconfig -o /boot/grub2/grub.cfg
reboot

# 验证
cat /proc/cmdline
# 验证隔离
cat /sys/devices/system/cpu/isolated
# 验证 nohz
cat /sys/devices/system/cpu/nohz_full
```

---

### 三、PCIe 优化

| | 关闭 | 开启 |
|---|------|------|
| **Extended Tag** | ~32 并发请求 | **~256** — 40G+ 端口收益明显 |
| **Max Payload Size** | 128B | **256B-512B** — 减少 TLP 开销 |
| **MRRS (Max Read Request Size)** | 512B | **4096B** — 大块 DMA 更高效 |

```bash
# 查看 PCIe 当前配置
lspci -vvv -s 81:00.0 | grep -i "max\|tag\|payload"
# MaxPayloadSize: 256 bytes
# MaxReadReq: 4096 bytes
# DevCtl: Extended Tag +

# 设置 (setpci)
setpci -s 81:00.0 68.w  # 读取 Device Control 2
```

---

### 四、NUMA 就近原则

**同一 NUMA Node 内对齐：**

```
网卡 PCIe 插槽 ─┬─ DPDK lcore（-l / taskset）
               ├─ 大页 / mempool / mbuf（socket_id）
               └─ DDIO 本地内存 [Ch2]
```

**跨 Node / 跨 QPI/UPI** 访存 — tail latency 与吞吐 **双杀**。

```bash
# 确认网卡所在 NUMA 节点
cat /sys/bus/pci/devices/0000:81:00.0/numa_node
# 0

# 确认 CPU 拓扑
numactl --hardware
# available: 2 nodes (0-1)
# node 0 cpus: 0 2 4 6 8 10 12 14
# node 1 cpus: 1 3 5 7 9 11 13 15

# EAL 参数 — 只用 node 0 的核 + node 0 的大页
./my_app -l 2,4,6,8 --socket-mem=2048,0
```

 [Ch2 DDIO 与 NUMA](../../chapter-02-cache-and-memory/notes/section-6-DDIO与NUMA.md)

---

### 五、CPU 隔离：`isolcpus`

| 收益 | 说明 |
|------|------|
| 无 **内核线程** 抢同一逻辑核 | 包处理 **抖动↓** |
| 配合 **taskset / EAL -l** | 控制面与数据面分离 |
| **nohz_full** | 消除定时器中断 — 消除 ~1ms 周期性抖动 |
| **rcu_nocbs** | RCU 回调不干扰 DPDK 核 |

**验证隔离效果：**

```bash
# 检查指定核上的中断数（应为 0 或极少）
cat /proc/interrupts | awk '{print $1, $3}'  # 第 3 列 = core 2

# perf 测量调度延迟
perf sched record -a -- sleep 10
perf sched latency --sort max

# 测量尾延迟 — 应 < 1μs jitter
./latency_test -c 2 -t 60
```

 [ULK Ch7 调度](../../../../16-linux-kernel-deep/chapter-07-process-scheduling/) · [14 HFT 绑核](../../../../14-hft-engineering/chapter-05-os-kernel-tuning/)

---

### 六、测试流量：防 RSS 倾斜

多队列 + RSS 压测时：

- 配置 **足够多随机流**（如 **随机源 IP**）
- 避免 **单流单队列** 导致部分核 **空闲、部分核饱和** — 测不出真实扩展性

 [Ch8 RSS / 多队列](../../chapter-08-flow-classification-multiqueue/notes/section-3-硬件流分类.md)

---

← [3. I/O 深度优化](./section-3-IO性能深度优化.md) · 下一节 [5. 队列长度](./section-5-队列长度及阈值设置.md)
