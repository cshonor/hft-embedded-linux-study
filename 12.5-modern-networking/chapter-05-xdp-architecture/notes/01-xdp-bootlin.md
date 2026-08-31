# 01 — XDP 实操：工具链、模式选择与排障

> **Bootlin 课程模块：** XDP
> **对应 Rosen:** 无（书出版时 XDP 不存在）
> **内核版本:** 以 v6.6 为准，`XDP_FLAGS_*` 取自 `include/uapi/linux/if_link.h`

## 文档概述

本篇是 chapter-05 里**唯一动手的一篇**：怎么把 XDP 程序装上去、装不上去怎么办、怎么验证它真的在工作。

本篇与兄弟篇的分工：

| 篇 | 讲什么 |
|----|--------|
| **01（本篇）** | **实操**：工具链、三种模式怎么选、加载失败排查、实验方法 |
| [02-xdp-rings](02-xdp-rings.md) | AF_XDP 的**四个 ring**（UMEM 与无锁同步） |
| [03-xdp-architecture](03-xdp-architecture-lwn.md) | XDP **架构全景**：五个动作、verifier 约束、与 DPDK 的定位区别 |

原笔记只有 1.1 KB，给了几条命令和一张实验清单。问题是**它没说这些命令失败了该怎么办**——而 XDP 实操里 80% 的时间都花在"为什么加载不上去"上。本篇把这部分补上。

---

## 一、三种挂载模式怎么选

模式不是"性能越高越好"，而是**你的驱动和硬件支持哪个**。

```c
/* include/uapi/linux/if_link.h:1295 */
#define XDP_FLAGS_UPDATE_IF_NOEXIST	(1U << 0)
#define XDP_FLAGS_SKB_MODE		(1U << 1)
#define XDP_FLAGS_DRV_MODE		(1U << 2)
#define XDP_FLAGS_HW_MODE		(1U << 3)
#define XDP_FLAGS_REPLACE		(1U << 4)
#define XDP_FLAGS_MODES			(XDP_FLAGS_SKB_MODE | \
					 XDP_FLAGS_DRV_MODE | \
					 XDP_FLAGS_HW_MODE)
```

| 模式 | flag | 挂载点 | sk_buff 分配了吗 | 性能 | 什么时候用 |
|------|------|--------|-----------------|------|-----------|
| **Native / DRV** | `XDP_FLAGS_DRV_MODE` | 驱动 Rx poll 里 | ❌ 没有 | **最高** | **默认目标**；驱动必须实现 XDP hook |
| **Generic / SKB** | `XDP_FLAGS_SKB_MODE` | `__netif_receive_skb_core()` | ✅ **已分配** | 最低 | 驱动不支持时；**开发调试用** |
| **Offloaded / HW** | `XDP_FLAGS_HW_MODE` | 网卡硬件（SmartNIC） | ❌ 不进主机 | 极致（不占 CPU） | 只有少数网卡支持（Netronome、部分 mlx） |

### ⚠️ 关键认知：Generic 模式不省 sk_buff

很多人以为"装了 XDP 就快了"。**Generic 模式下不是的**——它的挂载点在 `net/core/dev.c:5373` 的 `do_xdp_generic()`，那时 skb 早就分配好了。Generic 模式的价值是**功能验证**（不用挑网卡、不用挑驱动），不是性能。

```
驱动 Rx → page_pool → [native XDP hook] → napi_build_skb() → GRO
                            ↑                                    ↓
                      这里才省 skb                    __netif_receive_skb_core()
                                                        ├─ do_xdp_generic()  dev.c:5373
                                                        │    ↑ 这里 skb 已分配
                                                        ├─ ptype_all（tcpdump） dev.c:5394
                                                        └─ sch_handle_ingress（tc）dev.c:5412
```

顺带一个有用的推论：**generic XDP 在 `ptype_all` 之前**（5373 < 5394），所以 generic 模式的 `XDP_DROP` 包 **tcpdump 也看不到**——和 native 模式一致。

---

## 二、工具链：三条路，别混用

| 工具 | 定位 | 适合 |
|------|------|------|
| **libbpf + CO-RE**（推荐） | 自己写 C、自己编译、自己加载 | 生产程序、需要精细控制 |
| **xdp-tools** | 一套现成工具：`xdp-loader` / `xdp-dump` / `xdp-filter` | 快速验证、不想写加载器 |
| **bpftool** | 内核自带，最底层 | 排查、查看已加载状态 |

### libbpf + CO-RE（生产路线）

```bash
# 编译（注意 -target bpf，不是本机架构）
clang -target bpf -O2 -g -c xdp_prog.c -o xdp_prog.o

# 生成 BTF（CO-RE 重定位需要；内核需 CONFIG_DEBUG_INFO_BTF=y）
# 大部分发行版内核在 /sys/kernel/btf/vmlinux
ls -l /sys/kernel/btf/vmlinux        # 没有就说明内核没开 BTF

# 查看程序信息
llvm-objdump -h xdp_prog.o           # 看 section
bpftool prog load xdp_prog.o /sys/fs/bpf/xdp_prog
```

### xdp-tools（快速验证路线）

```bash
apt install xdp-tools

xdp-loader load eth0 xdp_program.o        # 自动选最佳模式
xdp-loader status                          # 看加载到了哪种模式 ← 最重要
xdp-loader unload eth0 <id>

# 用现成的过滤器，不用写程序
xdp-filter load eth0 --mode native
xdp-filter port 12345
xdp-filter status
```

### bpftool（排查路线）

```bash
bpftool net show dev eth0        # 看挂了什么程序、什么模式
bpftool prog show id <id>        # 看 JIT 了没、run_time、run_cnt
bpftool map dump id <id>         # 看你的统计 map
bpftool prog tracelog            # 配合 bpf_printk 调试
```

> ⚠️ **`xdp-loader status` 是你每次加载后第一个要看的东西**。它告诉你实际落到了
> native / generic / offloaded 哪一种。很多人以为自己在 native 模式，其实静默降级到
> generic 了，然后得出"XDP 没什么用"的错误结论。

---

## 三、加载失败排查清单（本篇最实用的一节）

XDP 加载失败的原因出乎意料地集中。按出现频率排序：

### 1. 加载时静默降级到 generic 模式

**症状**：`xdp-loader status` 显示 `generic`，不是 `native`。

**原因**：驱动没实现 XDP hook。

```bash
# 确认驱动是否支持：看驱动源码有没有 XDP 相关回调
#   网卡驱动里要有 ndo_bpf 回调
ethtool -i eth0                     # 先确认驱动名
modinfo <driver> | grep -i xdp      # 未必准，看内核源码最可靠
```

常见支持 native XDP 的驱动：mlx5、ice、i40e、ixgbe（部分）、virtio-net、veth、tun、bnxt。
**veth 支持 native XDP**，所以本机 veth 实验是可以跑在 native 模式的。

### 2. ring 大小不匹配

**症状**：`Error: ... ring size` 或加载后收不到包。

**原因**：native XDP 要求 Rx ring 与 TX ring 大小一致（部分驱动如此）。

```bash
ethtool -g eth0                       # 看当前
ethtool -G eth0 rx 2048 tx 2048       # 拉成一样
```

### 3. MTU 超过单页

**症状**：改 MTU 后 XDP 挂不上，或大包被丢。

**原因**：native XDP 依赖 page_pool 的单页 buffer。MTU 太大（> 约 3500 字节）时，
一个 page（4 KB）装不下「headroom + 包 + tailroom + skb_shared_info」。

```bash
ip link set eth0 mtu 1500             # 先降回常规值验证
```

### 4. verifier 拒绝

**症状**：`libbpf: prog 'xdp_prog': -- BEGIN PROG LOAD LOG -- ... invalid access to packet`

**原因**：**verifier 要求你在解引用前逐字节检查边界**。这是最常见的写错点：

```c
/* ❌ 错：没检查边界就解引用 */
struct ethhdr *eth = data;
if (eth->h_proto != htons(ETH_P_IP)) ...   // verifier 拒绝

/* ✅ 对：先验证指针范围，再解引用 */
void *data = (void *)(long)ctx->data;
void *data_end = (void *)(long)ctx->data_end;
struct ethhdr *eth = data;
if ((void *)(eth + 1) > data_end)
    return XDP_DROP;                        // 先验证
if (eth->h_proto != htons(ETH_P_IP))        // 再解引用
    return XDP_PASS;
```

**每一层协议头都要这么检查一遍**（eth → ip → udp），verifier 才会放行。

```bash
bpftool prog load xdp_prog.o /sys/fs/bpf/p 2>&1 | tail -40   # 看 verifier 日志
```

### 5. 忘了卸载旧程序

**症状**：`XDP_FLAGS_UPDATE_IF_NOEXIST` 报错，或行为不符合预期（旧程序还在）。

```bash
xdp-loader status                     # 看看挂了几个
xdp-loader unload eth0 --all          # 全卸掉重来
```

### 6. 程序返回了 `XDP_ABORTED` 而不自知

**症状**：verifier 通过、程序加载成功，但**所有包都被丢弃**。

**原因**：`enum xdp_action` 里 **`XDP_ABORTED = 0`**（第一个）。如果你某条路径
忘了写 return（或 return 0），返回的就是 `XDP_ABORTED`——**语义是"程序出错，丢弃并记 tracepoint"**，
不是 `XDP_PASS`。

```c
/* include/uapi/linux/bpf.h */
enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};
```

这个坑非常隐蔽：**返回 0 ≠ 放行**。检查办法：

```bash
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'
#   看到 XDP_ABORTED 计数在涨 → 你的程序有路径返回了 0
```

---

## 四、实验清单（含验证方法）

原笔记列了 5 个实验，但没说**怎么证明实验成功**。补上验证方法：

| # | 实验 | 怎么做 | **怎么验证真的生效** |
|---|------|--------|---------------------|
| 1 | XDP DROP all | 加载 `return XDP_DROP` 的程序 | `ping` 不通；`ethtool -S eth0` 的 rx 计数**仍在涨**（说明包进网卡了，只是被 XDP 丢了） |
| 2 | 按端口过滤 | 检查 UDP dst port，非目标端口 DROP | `xdp-filter status` 看命中数；用 `bpftool map dump` 看你自己的计数 map |
| 3 | 模式对比 | 同一程序分别用 native / generic 加载 | 用 `bpftool prog show id <id>` 的 `run_time / run_cnt` 算**单包耗时** |
| 4 | XDP_REDIRECT cpumap | 把包转到指定 CPU | `bpftrace -e 'tracepoint:xdp:xdp_redirect { @[cpu] = count(); }'` |
| 5 | AF_XDP 收包 | 用 xdpsock 收包 | `xdpsock -i eth0 -r` 的统计输出；`rx_dropped` 是否非 0 |
| 6 | **XDP 打时间戳** | 在 XDP 里 `bpf_ktime_get_ns()` 存进 map | 与用户态收包时刻相减 = NIC→用户态延迟。**这是 HFT 最有用的一个实验** |
| 7 | **验证 tcpdump 看不到 XDP_DROP** | 一边 `tcpdump` 一边 `XDP_DROP` | tcpdump **什么都抓不到**，但 `ethtool -S` 的 rx 在涨 → 证明 XDP 在 skb 之前 |

第 7 个实验是我推荐的**第一个**实验：它用最直观的方式证明了"XDP 在 skb 分配之前"，
而这正是 XDP 全部性能收益的来源。

### veth 实验环境（无需特殊硬件）

```bash
ip link add veth0 type veth peer name veth1
ip link set veth0 up
ip link set veth1 up
ip addr add 10.0.0.1/24 dev veth0
ip addr add 10.0.0.2/24 dev veth1

# veth 支持 native XDP，不用退到 generic
xdp-loader load --mode native veth0 xdp_prog.o
xdp-loader status          # 确认真的是 native

# 从 veth1 发包测试
ping -I veth1 10.0.0.1
```

⚠️ 注意：**veth 上的性能数据不能代表物理网卡**。veth 没有 DMA、没有真实的
Rx ring，测出来的单包耗时没有参考价值。veth 只适合做**正确性验证**，性能要上物理网卡测。

---

## 五、观测

```bash
# 1) 挂了什么、什么模式（第一件事）
xdp-loader status
bpftool net show dev eth0

# 2) 程序执行了多少次、花了多少时间（算单包耗时）
bpftool prog show id <id>
#   run_cnt / run_time → 单包平均耗时

# 3) 异常（XDP_ABORTED 在这里）
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'

# 4) REDIRECT 成功/失败
bpftrace -e 'tracepoint:xdp:xdp_redirect { @ok = count(); }'
bpftrace -e 'tracepoint:xdp:xdp_redirect_err { @[args->err] = count(); }'

# 5) 驱动层：确认包确实进了网卡（XDP_DROP 之后 rx 计数仍在涨）
ethtool -S eth0 | grep -E "rx_packets|rx_dropped"

# 6) page_pool 是否健康（XDP 依赖它）
ethtool -S eth0 | grep -i "rx_pp_"
#   → [chapter-04/01](../../chapter-04-page-pool/notes/01-page-pool.md)
```

---

## HFT 要点

- **每次加载后第一件事是 `xdp-loader status`**：静默降级到 generic 会让你的性能结论完全错误。
- **Generic 模式不省 sk_buff**，它的挂载点在 skb 分配之后（dev.c:5373）。只有 native/offloaded 才省。
- **`XDP_ABORTED = 0`**：返回 0 不是放行，是"出错丢弃"。忘了写 return 的路径会静默吃掉所有包。用 `xdp_exception` tracepoint 查。
- **veth 只能验证正确性，不能测性能**：没有 DMA、没有真实 Rx ring。
- **最有用的实验是"XDP 打时间戳"**：`bpf_ktime_get_ns()` 存进 map，与用户态收包时刻相减，直接得到 NIC → 用户态的端到端延迟分段。
- **tcpdump 看不到 XDP_DROP 的包**——这既是排障的坑，也是验证 XDP 在 skb 之前的最直观方法。
- **native XDP 要求 MTU 别太大**：超过约 3500 字节时单页装不下，会挂不上或丢大包。
- **verifier 要求逐层边界检查**：eth → ip → udp 每层都要先验证指针再解引用，这是写 XDP 程序最常卡住的地方。

## 与 Rosen 3.x 的差异

Rosen 写作时 XDP 不存在，所以整章都是新增内容。但从**方法论**上有一处重要差异值得点出：

| Rosen 时代的网络编程 | XDP 时代 |
|---------------------|---------|
| 写内核代码 = 写内核模块（风险高、要跟进内核版本） | 写 eBPF 程序，**verifier 保证不会搞崩内核** |
| 出问题 = oops / panic | 出问题 = 加载被拒（**编译期就拦住**） |
| 包处理在协议栈里 | 包处理在**协议栈之前** |
| 观测靠 printk | 观测靠 map + tracepoint + `bpftool` |

**verifier 是 XDP 相对于内核模块的核心优势**：它把"运行时崩溃"变成了"加载时拒绝"。
代价就是你必须按它的规则写（逐层边界检查），这也是新手最不适应的一点。

---

## 代码自测

<details>
<summary>Q1：你加载了 XDP 程序，<code>ping</code> 不通了，以为成功了。但 <code>ethtool -S eth0</code> 的 rx_packets 也完全不涨。这说明什么？</summary>

<b>答：</b>这不是 XDP 的功劳——<b>包根本没进到 XDP 那一层</b>。

正确的现象应该是：XDP_DROP 时 `ethtool -S` 的 Rx 计数<b>仍在上涨</b>（包进了网卡、被 DMA 到
Rx ring、驱动也处理了），只是在 XDP 层被丢掉。这是"XDP 在 skb 之前"的直接证据。

rx_packets 不涨，说明丢点在更前面，按这个顺序查：

1. **物理层**：链路 up 了吗？`ip link show eth0`
2. **网线/光模块**：`ethtool eth0` 看 Link detected
3. **MAC 过滤/混杂模式**：目标 MAC 不是本机？组播没加组？
4. **Rx ring 描述符为 0**：`ethtool -g eth0`，有些驱动设为 0 会直接不收包
5. **驱动根本没起来**：`ethtool -i eth0` 看驱动，dmesg 看报错

<b>诊断口诀</b>：driver 计数涨 → 包进了网卡；XDP 的 map/计数涨 → 包进了 XDP；
tcpdump 看得到 → 包建了 skb。三个层次对应三个位置，逐级排除。
</details>

<details>
<summary>Q2：你的 XDP 程序加载成功、verifier 通过，但所有包都被丢弃了。代码逻辑看着没问题。最可能的原因？</summary>

<b>答：</b>某条路径返回了 <b>0</b>，而 0 是 <code>XDP_ABORTED</code>，不是 <code>XDP_PASS</code>。

看枚举定义：

```c
enum xdp_action {
	XDP_ABORTED = 0,
	XDP_DROP,
	XDP_PASS,
	XDP_TX,
	XDP_REDIRECT,
};
```

`XDP_ABORTED` 的语义是"<b>程序出错，丢弃包并记 tracepoint</b>"。所以：

- 你有一条路径忘了写 return → 返回 0 → ABORTED → 丢弃
- 或者你显式 `return 0` 想表示"放行" → 实际是"丢弃"

<b>排查</b>：

```bash
bpftrace -e 'tracepoint:xdp:xdp_exception { @[args->act] = count(); }'
```

看到 ABORTED 计数在涨就坐实了。

<b>为什么这么设计</b>：这是 C 语言的惯例——0 通常表示"无动作/默认"，
而 XDP 选择了"无动作 = 出错"这个语义，因为静默放行一个程序本该处理却没处理的包，
比丢弃更危险。

<b>教训</b>：XDP 程序里<b>永远显式返回具名常量</b>（`XDP_PASS` / `XDP_DROP`），
不要写裸数字，也不要依赖隐式返回。
</details>

<details>
<summary>Q3：你在本机 veth 上测得 XDP 单包耗时 200ns，很高兴。能把这个数字用到生产网卡上吗？</summary>

<b>答：</b>不能。<b>veth 上的 XDP 性能数据没有参考价值</b>。

原因：veth 是虚拟设备，<b>没有 DMA、没有真实的 Rx ring、不经过 page_pool 的收发循环</b>。
它走的是完全不同的代码路径——包从 veth1 的 `ndo_start_xmit` 直接送到 veth0 的接收侧，
中间是纯软件流转。

真实物理网卡的 XDP 路径包含：DMA 取描述符 → 从 page_pool 拿页 → 构造 xdp_buff
→ 跑 BPF → 决策。这些在 veth 上要么不存在，要么成本完全不同。

<b>veth 适合做什么</b>：
- ✅ 验证程序<b>逻辑正确性</b>（过滤规则对不对、动作返回对不对）
- ✅ 验证 verifier 能过
- ✅ 在没有特殊硬件时学习 XDP API
- ✅ 验证"tcpdump 看不到 XDP_DROP"这类<b>机制性</b>结论

<b>不适合做什么</b>：
- ❌ 任何性能数字
- ❌ 单包延迟对比
- ❌ PPS 容量评估

要测性能，上物理网卡（哪怕是 1G 的），并用 `bpftool prog show id <id>` 的
`run_time / run_cnt` 直接在生产环境测量。
</details>

---

→ 本篇：[01 XDP 实操](01-xdp-bootlin.md)
→ 后一篇：[02 AF_XDP 的四个 ring](02-xdp-rings.md)
→ 相关：[03 XDP 架构全景](03-xdp-architecture-lwn.md) · [chapter-04 page_pool](../../chapter-04-page-pool/) · [chapter-06 AF_XDP](../../chapter-06-af-xdp/)
