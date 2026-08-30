# Ch 4 零拷贝与用户态旁路 · Zero-Copy & Userspace Bypass

> **01-Intro-Book** · 官方 Programmer's Guide · **精读**

> **实体书：** [chapter-09-hardware-offload](../chapter-09-hardware-offload/) 硬件 offload 与 mbuf 协同；[chapter-06-pcie-packet-io](../chapter-06-pcie-packet-io/) §3 DMA/描述符环基础；[chapter-07-nic-performance-optimization](../chapter-07-nic-performance-optimization/) §2 UIO/VFIO 对比表。

> **本篇分工：** 实体书已给出 UIO/VFIO 对比表和描述符环原理。
> 本篇追一个更具体的问题：**"零拷贝"这四个字，到底在哪些环节成立、哪些不成立**，
> 以及**旁路掉的究竟是哪一段完整路径**。

> **实验：** [code/mcast-minimal/](../code/mcast-minimal/)（DPDK 版与内核栈版可同口径对比）

---

## 一、"零拷贝"的精确含义（先把误解清掉）

最常见的误解是："DPDK 省掉了 DMA 拷贝"。**不对——DMA 从来就不经过 CPU。**
内核协议栈收包时，网卡同样也是 DMA 直写内存的。

真正省掉的是下面第 2、3、4 行：

| # | 环节 | 内核协议栈 | DPDK 旁路 |
|---|---|---|---|
| 1 | NIC → 内存 | DMA 直写（**本来就是零拷贝**） | DMA 直写（**一样**） |
| 2 | 内核 → 用户 | **1 次 `copy_to_user`** | 不存在（数据本来就在用户态） |
| 3 | 协议栈层间 | 组播多 socket → **每 socket 一份 skb 复制** | 不存在 |
| 4 | 应用内解析 | 通常再拷一次到业务结构 | 可避免（直接读 `payload`） |
| — | 系统调用 + 上下文切换 | 每批 1 次，含用户/内核态切换 | **0 次** |
| — | 协议处理 | IP/UDP/校验和/IGMP/ARP/分片重组 **全做** | **全不做** |

**结论：DPDK 省的不是"拷贝"，是"内核协议栈这一整段处理 + 两次特权级切换"。**
零拷贝是结果，不是手段。

这也解释了两件事：

- **为什么小包收益最夸张** —— 小包下"每包固定成本"占比最高，省掉固定成本收益最大
- **为什么延迟能压到亚微秒** —— 剩下的只有 PCIe 传输 + 描述符轮询 + 你自己的解析代码

---

## 二、旁路后的完整路径

```
         ┌────────────── 初始化（唯一有内核参与的阶段）──────────────┐
         │  vfio-pci 绑定 → mmap BAR → EAL 映射大页 → 建 mempool   │
         └──────────────────────────┬─────────────────────────────┘
                                    ↓
 ┌────────┐  PCIe   ┌──────────┐  DMA   ┌──────────────────┐
 │ 交换机  │ ──────→ │ 网卡 MAC │ ─────→ │ 描述符指向的 mbuf │
 └────────┘         └────┬─────┘        └────────┬─────────┘
                         │ 置 DD 位               │
                         ↓                       ↓
                  ┌──────────────┐       ┌──────────────────┐
                  │  RX 描述符环  │◄──────│ rte_eth_rx_burst │ ← 用户态轮询，无中断
                  └──────────────┘       └────────┬─────────┘
                                                  ↓
                                         ┌──────────────────┐
                                         │  应用解析裸以太网帧 │ ← 无协议栈
                                         └──────────────────┘

 热路径上：0 次系统调用 · 0 次中断 · 0 次上下文切换 · 0 行内核代码
```

**内核只剩下"初始化者"角色。** 之后它甚至不知道这张网卡收到了什么——
这正是 [ch05](./chapter-05-组播行情接入.md) 里 IGMP 会失效的根因：
内核已经不替你跟交换机说话了。

---

## 三、UIO vs VFIO：实操与排错

对比表见实体书 §2，这里给命令和坑。

```bash
# 1) 确认 IOMMU 已开
dmesg | grep -i -E "DMAR|IOMMU"
# 期望看到：DMAR: IOMMU enabled

# 2) 绑定（DPDK 20.11+ 工具已安装到 PATH；老版本在 usertools/ 下）
modprobe vfio-pci
dpdk-devbind.py --status
dpdk-devbind.py --bind=vfio-pci 0000:81:00.0

# 3) 大页
dpdk-hugepages.py --setup 2G
grep -i huge /proc/meminfo
```

### GRUB 参数（缺一不可）

```
intel_iommu=on iommu=pt
```

**`iommu=pt` 的意思**：只对**没**绑定 vfio 的设备做地址转换，
vfio 管辖的设备走直通。少了 `pt`，所有 DMA 都要过转换表，**延迟直接抬头**。

### 常见报错

| 报错 | 原因 | 处理 |
|---|---|---|
| `VFIO group is not viable` | 该 IOMMU group 里还有别的设备没绑 vfio | 同组设备一起绑，或换插槽 |
| `No IOMMU detected` | BIOS 里 VT-d / AMD-Vi 没开 | 进 BIOS 打开 |
| `Cannot allocate memory` | 大页不够 | `dpdk-hugepages.py --setup` |
| 能跑但延迟偏高 | 忘了 `iommu=pt` | 补 GRUB 参数 |

### ⚠ NOIOMMU 模式

```bash
echo Y > /sys/module/vfio/parameters/enable_unsafe_noiommu_mode
```

名字里就写着 **unsafe**：关掉 IOMMU 保护后，用户态程序可以 DMA 到**任意物理内存**
（包括其他进程、包括内核）。风险等级等同于 UIO。
**只适合本机调试**，别往生产环境带。

---

## 四、为什么 DMA 必须要大页

```
4KB 页：  8GB 收包区 = 约 200 万个页表项
          → TLB 装不下，每次 DMA 都可能 TLB miss
          → 且 4KB 页不保证物理连续，跨页的 mbuf 需要多次地址转换

2MB 大页：同样 8GB = 4096 个页表项 → TLB 命中率极高
          → 大页内物理连续，IOVA 映射简单
```

DPDK 有两种 IOVA 模式：

| 模式 | 要求 | 何时用 |
|---|---|---|
| `IOVA=PA` | IOVA = 物理地址，**要求物理连续** → 必须有大页 | 以 root 直跑、无 IOMMU 时的常见默认 |
| `IOVA=VA` | IOVA = 虚拟地址，IOMMU 负责转换，**不要求物理连续** | 有 IOMMU 时可用；`--iova-mode=va` 强制 |

选哪种别猜，**看 EAL 启动日志**：

```
EAL: Detected IOVA as 'PA'      ← 或 'VA'
```

HFT 场景通常仍是 **PA + 2MB（甚至 1GB）大页**：
1GB 大页能进一步压 TLB miss，代价是启动时就要预留、碎片后难回收。

---

## 五、旁路掉了什么（完整清单）

[ch05](./chapter-05-组播行情接入.md) 列出了组播场景最关键的四项（IGMP/ARP/校验和/分片）。完整清单：

| 内核原本做的 | 旁路后 |
|---|---|
| IGMP Membership Report | ❌ 自己发，或交换机配静态组播 |
| ARP 请求 / 应答 | ❌ 网卡不响应 ARP |
| IP / UDP / TCP 校验和验证 | ❌ 坏包照样递给你 |
| IP 分片重组 | ❌ 分片各自独立到达 |
| 组播复制（多进程各一份） | ❌ 用户态自己分发 |
| TCP 状态机、拥塞控制、重传 | ❌ 完全不存在 |
| socket 缓冲区与背压通知 | ❌ 换成描述符环 + `rx_nombuf` |
| 邻居子系统、路由表 | ❌ |
| Netfilter / nftables 防火墙 | ❌ **安全边界消失**，防护要前移到交换机 |
| `tcpdump` | ❌ 抓不到（网卡不归内核管）→ 用 `dpdk-dumpcap` 或在应用里落盘 |

最后一项实操上很痛：出问题想抓包时，惯用的 `tcpdump` 直接失效，
排错工具链要重建。

---

## 六、与 AF_XDP 怎么选

AF_XDP 也能零拷贝、也在用户态轮询——差别在**网卡归谁管**。

| | DPDK | AF_XDP（zero-copy） |
|---|---|---|
| 网卡归属 | 完全归 DPDK，**内核看不到** | 仍归内核，只把指定流重定向到用户态 |
| 生效粒度 | 整张网卡 | **可以只旁路一条流**，其余照常走内核 |
| 迁移成本 | 高：整卡接管，管理流量要另走一张卡 | 低：可渐进迁移、可回退 |
| copy 模式 | 无此模式 | 有，但**仍分配 sk_buff + 一次 memcpy**，只有 zc 才有意义 |
| 延迟量级 | ~0.3–1μs | zc ~0.5–2μs |
| 驱动支持 | PMD 覆盖广 | 依赖驱动实现 zc（i40e / ixgbe / mlx5 等） |

**选型判断：**

- 已有系统要提速、不能动基础设施 → **AF_XDP**，先旁路一条行情流试试水
- 全新专用行情接入卡 → **DPDK**，要的就是最后那点确定性
- 详细结构对照 → [12.5/chapter-06/notes/03-af-xdp-umem-layout](../../../12.5-modern-networking/chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md)

---

## 七、旁路的代价（别只看收益）

1. **独占网卡** —— SSH 断了是经典事故，管理口必须另走一张卡
2. **协议栈功能全部自己写** —— 顺序、重传、gap 检测、补单通道
3. **`tcpdump` 失效** —— 排错工具链要重建
4. **安全边界消失** —— Netfilter 全部绕过
5. **一整颗核 100% 空转** —— 见 [ch03](./chapter-03-PMD与轮询模式.md) 第四节
6. **DPDK ABI/API 版本敏感** —— 升级要重编，跨版本结构体字段会变
7. **多进程共享要自己设计** —— 内核的组播复制没了，得用 ring / 共享内存分发

---

## 相关章节

- 实体书：[chapter-09-hardware-offload/](../chapter-09-hardware-offload/) · [chapter-06-pcie-packet-io/](../chapter-06-pcie-packet-io/) · [chapter-07-nic-performance-optimization/](../chapter-07-nic-performance-optimization/)
- 上一章：[chapter-03-PMD与轮询模式.md](./chapter-03-PMD与轮询模式.md)
- 下一章：[chapter-05-组播行情接入.md](./chapter-05-组播行情接入.md)
- UMEM/所有权交接对照：[12.5/chapter-06/notes/03-af-xdp-umem-layout](../../../12.5-modern-networking/chapter-06-af-xdp/notes/03-af-xdp-umem-layout.md)
- 内核栈组播路径：[12.5/chapter-02/notes/05-multicast-rx-path](../../../12.5-modern-networking/chapter-02-napi-rx-path/notes/05-multicast-rx-path.md)
- 下一梯度：[02-Advanced note-openonload-rdma对比](../../02-Advanced-Book/notes/note-openonload-rdma对比.md)
- 实验：[code/mcast-minimal/](../code/mcast-minimal/)
