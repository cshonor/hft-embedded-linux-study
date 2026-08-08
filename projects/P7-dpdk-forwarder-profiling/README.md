# P7 — DPDK 转发 + 延迟剖析

> 用 DPDK 写一个 packet forwarder，再用 perf 火焰图和 bpftrace 延迟探针把它剖析透——HFT 收发路径的"性能层"。
> **做法：项目驱动，[`18`](../../18-dpdk/) / [`19`](../../19-systems-performance/) / [`20`](../../20-bpf-observability/) 笔记当字典。**

---

## 核心理念

从内核栈跳到用户态旁路。实现一个最小 DPDK 转发器，测量单跳延迟，用性能工具找瓶颈。这是进入 `21` HFT 引擎前的最后一道网络性能关。

## 最小预备

| 瞄一眼 | 只要留下印象 |
|--------|-------------|
| [DPDK ch01 intro](../../18-dpdk/01-Intro-Book/chapter-01-dpdk-intro/) | DPDK 是什么、为什么旁路内核 |
| [DPDK ch02 cache](../../18-dpdk/01-Intro-Book/chapter-02-cache-and-memory/) | 大页、NUMA、缓存行 |
| [DPDK ch05 转发](../../18-dpdk/01-Intro-Book/chapter-05-packet-forwarding/) | PMD 轮询收发 |
| [SysPerf ch02 方法论](../../19-systems-performance/chapter-02-methodologies/notes/) | USE 方法、火焰图 |
| [SysPerf ch04 观测工具](../../19-systems-performance/chapter-04-observability-tools/notes/) | perf / bpftrace |

---

## Phase 1：DPDK 环境跑通 testpmd（1-2 小时）

### 做什么

安装 DPDK，配置大页 + VFIO，跑通自带的 testpmd 转发测试。

### 分步实现

1. **安装 DPDK**：
   ```bash
   sudo apt install dpdk dpdk-dev libdpdk-dev
   # 或从源码编译最新版
   git clone https://dpdk.org/git/dpdk
   meson setup build && ninja -C build && sudo ninja -C build install
   ```
2. **配置大页**：
   ```bash
   echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
   sudo mkdir -p /mnt/huge
   sudo mount -t hugetlbfs nodev /mnt/huge
   ```
3. **绑定网卡到 VFIO**：
   ```bash
   sudo modprobe vfio-pci
   sudo dpdk-devbind.py -b vfio-pci eth1  # eth1 旁路给 DPDK
   ```
4. **跑 testpmd**：
   ```bash
   sudo dpdk-testpmd -l 0-1 -n 4 -- -i
   # testpmd> show port stats 0
   # testpmd> start forwarding
   ```
5. **验证**：用另一台机器发包，testpmd 显示收发包计数

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| 大页不够 | EAL 初始化失败 | `nr_hugepages` 要够（每 2MB 一页）|
| 网卡没绑定 VFIO | 找不到端口 | `dpdk-devbind.py --status` 确认 |
| VFIO IOMMU 没开 | 绑定失败 | BIOS 开 VT-d，内核加 `intel_iommu=on` |
| 没有 root | EAL 失败 | DPDK 需要 root 或特殊权限 |
| CPU 不支持 | vfio-pci 加载失败 | 虚拟机里可能需要 `iommu=pt` |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 大页原理 | [DPDK ch02 cache](../../18-dpdk/01-Intro-Book/chapter-02-cache-and-memory/) |
| EAL 初始化 | [DPDK ch01](../../18-dpdk/01-Intro-Book/chapter-01-dpdk-intro/) |
| VFIO/UIO | [DPDK ch06 PCIe](../../18-dpdk/01-Intro-Book/chapter-06-pcie-packet-io/) |

---

## Phase 2：自写最小转发器（2-3 小时）

### 做什么

写一个最小 DPDK forwarder：收包 → 改 MAC → 转发。

### 代码骨架

```c
// src/forwarder.c
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>

#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024
#define NUM_MBUFS 8191
#define MBUF_CACHE_SIZE 250
#define BURST_SIZE 32

static const struct rte_eth_conf port_conf = {
    .rxmode = { .max_rx_pkt_len = RTE_ETHER_MAX_LEN }
};

int main(int argc, char **argv) {
    // 1. EAL 初始化
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) rte_panic("EAL init failed\n");

    uint16_t nb_ports = rte_eth_dev_count_avail();
    if (nb_ports < 2) rte_panic("Need at least 2 ports\n");

    // 2. 创建 mempool（大页分配）
    struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
        "MBUF_POOL", NUM_MBUFS, MBUF_CACHE_SIZE, 0,
        RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    // 3. 初始化两个端口
    uint16_t portid;
    RTE_ETH_FOREACH_DEV(portid) {
        rte_eth_configure(portid, &port_conf);
        rte_eth_rx_queue_setup(portid, 0, RX_RING_SIZE,
            rte_eth_dev_socket_id(portid), NULL, mbuf_pool);
        rte_eth_tx_queue_setup(portid, 0, TX_RING_SIZE,
            rte_eth_dev_socket_id(portid), NULL);
        rte_eth_promiscuous_enable(portid);
    }

    // 4. 启动端口
    RTE_ETH_FOREACH_DEV(portid)
        rte_eth_dev_start(portid);

    // 5. 转发循环
    struct rte_mbuf *bufs[BURST_SIZE];
    for (;;) {
        // 从 port 0 收，转发到 port 1
        uint16_t nb_rx = rte_eth_rx_burst(0, 0, bufs, BURST_SIZE);
        if (nb_rx > 0) {
            // 改目的 MAC（示意）
            for (int i = 0; i < nb_rx; i++) {
                struct rte_ether_hdr *eth =
                    rte_pktmbuf_mtod(bufs[i], struct rte_ether_hdr *);
                // 改 MAC...
            }
            rte_eth_tx_burst(1, 0, bufs, nb_rx);
        }
        // 反方向：port 1 → port 0
        uint16_t nb_rx2 = rte_eth_rx_burst(1, 0, bufs, BURST_SIZE);
        if (nb_rx2 > 0)
            rte_eth_tx_burst(0, 0, bufs, nb_rx2);
    }
}
```

### 分步实现

1. **EAL 初始化**：解析 `-l`（lcore）、`-n`（内存通道）等参数
2. **mempool 创建**：预分配 mbuf 池，大页内存，零拷贝
3. **端口配置**：rx/tx 队列、混杂模式、启动
4. **转发循环**：`rte_eth_rx_burst` 收包 → 处理 → `rte_eth_tx_burst` 发包
5. **编译**：`gcc -o forwarder forwarder.c $(pkg-config --cflags --libs libdpdk)`

### 常见坑

| 坑 | 症状 | 原因 |
|----|------|------|
| mbuf 没释放 | mempool 耗尽 | 转发后 mbuf 自动回收，但如果丢弃要手动 `rte_pktmbuf_free` |
| 两个端口在不同 NUMA | 跨节点延迟高 | `rte_eth_dev_socket_id()` 检查，mempool 分到同节点 |
| burst size 太小 | 吞吐低 | 一次收 32 个比收 1 个效率高 10x |
| 没绑核 | 调度抖动 | `-l 0,1` 绑到核 0 和核 1 |

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| mempool/mbuf | [DPDK ch02](../../18-dpdk/01-Intro-Book/chapter-02-cache-and-memory/) |
| PMD 轮询 | [DPDK ch05](../../18-dpdk/01-Intro-Book/chapter-05-packet-forwarding/) |
| NUMA 绑定 | [DPDK ch03](../../18-dpdk/01-Intro-Book/chapter-03-parallel-computing/) |

---

## Phase 3：延迟测量 + 分布统计（1-2 小时）

### 做什么

在转发器里打时间戳，统计 p50/p99/p999 延迟。

### 代码骨架

```c
// 收包时打时间戳
uint64_t t_rx = rte_get_timer_cycles();

// ... 转发处理 ...

// 发包时算延迟
uint64_t t_tx = rte_get_timer_cycles();
uint64_t latency_cycles = t_tx - t_rx;
uint64_t latency_ns = latency_cycles * 1e9 / rte_get_timer_hz();

// 直方图（复用 P5d 的延迟统计代码）
record_latency(latency_ns);

// 每 100 万包打印一次
if (++packet_count % 1000000 == 0)
    print_latency_stats();
```

### 分步实现

1. **`rte_get_timer_cycles()`**：高精度时间戳（比 `clock_gettime` 快）
2. **收包打点 → 发包打点**：差值 = 单跳处理延迟
3. **直方图**：复用 P5d 的对数桶统计
4. **绑核 + 实时优先级**：`chrt -f 99 ./forwarder` 看 PREEMPT_RT 影响

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| 延迟统计方法 | [SysPerf ch02](../../19-systems-performance/chapter-02-methodologies/notes/) |

---

## Phase 4：perf 火焰图定位热点（1-2 小时）

### 做什么

用 `perf record` 采样转发器，生成火焰图找热点函数。

### 分步实现

1. **perf 采样**：
   ```bash
   sudo perf record -F 99 -g -p $(pgrep forwarder) -- sleep 10
   ```
2. **生成火焰图**：
   ```bash
   sudo perf script > out.perf
   git clone https://github.com/brendangregg/FlameGraph
   ./FlameGraph/stackcollapse-perf.pl out.perf | ./FlameGraph/flamegraph.pl > flame.svg
   ```
3. **看火焰图**：
   - `rte_eth_rx_burst` 占多少？→ 如果很宽 = CPU 大部分时间在轮询收包（正常）
   - 你的处理逻辑占多少？→ 如果很宽 = 需要优化
   - `malloc`/`free` 出现？→ 不应该有，DPDK 是预分配
4. **优化一轮**：根据火焰图找最宽的函数，优化它

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| perf 采样 | [SysPerf ch04](../../19-systems-performance/chapter-04-observability-tools/notes/) |
| 火焰图 | [SysPerf ch06](../../19-systems-performance/) (找 perf 章节) |
| USE 方法 | [SysPerf ch02](../../19-systems-performance/chapter-02-methodologies/notes/) |

---

## Phase 5：bpftrace 找尾延迟源 + 对比内核栈（1-2 小时）

### 做什么

用 bpftrace 追踪调度/中断对 DPDK 转发的干扰，对比内核栈（P6）和 DPDK 旁路的延迟。

### 分步实现

1. **追踪调度切换**：
   ```bash
   sudo bpftrace -e '
   tracepoint:sched:sched_switch /args->prev_pid == PID/ {
       printf("%lld scheduled out\n", nsecs);
   }
   '
   ```
   → 如果 DPDK 线程被调度出去，回来后延迟会飙——这就是尾延迟源

2. **追踪中断**：
   ```bash
   sudo bpftrace -e '
   tracepoint:irq:irq_handler_entry {
       @irq[pid, comm] = count();
   }
   '
   ```
   → 看哪些中断打断了 DPDK 轮询线程

3. **隔离 CPU**：
   ```bash
   # 内核启动参数
   isolcpus=1 nohz_full=1 rcu_nocbs=1
   # 绑中断到其他核
   echo 0 > /proc/irq/<N>/smp_affinity_list
   ```

4. **对比内核栈 vs DPDK**：
   - 内核栈（P6 的抓包路径）：p99 ~50-200us
   - DPDK 旁路：p99 ~1-5us
   - 差距 = 内核网络栈的开销

### 卡住翻哪篇笔记

| 卡住了… | 翻这里 |
|---------|--------|
| bpftrace 调度追踪 | [BPF ch03](../../20-bpf-observability/chapter-03-performance-analysis/notes/) |
| off-CPU 分析 | [BPF](../../20-bpf-observability/) |
| CPU 隔离 | [SysPerf](../../19-systems-performance/) |

---

## 交付物

- [ ] DPDK 环境：大页、UIO/VFIO、lcore 绑核
- [ ] mempool + mbuf 预分配
- [ ] PMD 轮询收发（rx/tx burst）
- [ ] 最小转发逻辑（MAC 改写 + 转发）
- [ ] 延迟测量：打时间戳，统计 p50/p99/p999
- [ ] perf 采样 + 火焰图，定位热点函数
- [ ] bpftrace 探针，追踪软中断/调度对延迟的扰动
- [ ] 对比：DPDK 旁路 vs 内核栈延迟分布

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`18` dpdk](../../18-dpdk/) | EAL、大页、NUMA、mbuf/mempool、PMD、零拷贝 |
| [`19` systems-performance](../../19-systems-performance/) | perf 采样、火焰图、USE 方法、延迟分解 |
| [`20` bpf-observability](../../20-bpf-observability/) | bpftrace 延迟探针、off-CPU、调度追踪 |

## 前置

[P6](../P6-network-protocol-analyzer/)（内核网络栈与抓包过关）。

## 状态

⬜ 未开始 → 建议先装 DPDK + 配大页，跑通 testpmd。

← [projects 总览](../README.md) · [18 模块](../../18-dpdk/) · [19 模块](../../19-systems-performance/)
