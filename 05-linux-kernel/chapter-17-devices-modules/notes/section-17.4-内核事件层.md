## ④ 内核事件层 · Kernel Events Layer

建立在 **kobject** 之上的 **内核 → 用户** 通知。

| 模型 | 从某 **kobject**（对应 **sysfs 路径**）发出 **事件** |
|------|------------------------------------------------------|
| 动作字符串 | 如 **`add`**、**`remove`**、**`change`**、**`move`**、**`bind`** |
| 传递 | **netlink** 套接字（`NETLINK_KOBJECT_UEVENT`）→ 用户态（**udev/systemd-udevd**） |

```
热插拔 U 盘
    ▼
内核 kobject 注册 + kobject_uevent(KOBJ_ADD)
    ▼
netlink 广播 ──► udev/systemd-udevd 收到 ──► 建节点、跑规则、加载模块…
```

### uevent 消息长什么样

uevent 是**纯文本的环境变量块**，通过 netlink 发出：

```
ACTION=add
DEVPATH=/devices/pci0000:00/0000:00:14.0/usb1/1-1
SUBSYSTEM=usb
DEVNAME=/dev/sdb
DEVTYPE=usb_device
MAJOR=189
MINOR=0
SEQNUM=4321          ← 单调递增序号，用户态靠它检测丢事件
```

| 字段 | 作用 |
|------|------|
| `ACTION` | `add` / `remove` / `change` / `move` / `bind` / `unbind` |
| `DEVPATH` | 设备在 `/sys` 下的路径（**用户态据此去 sysfs 查详情**） |
| `SUBSYSTEM` | 属于哪个子系统 → udev 规则靠它匹配 |
| `DEVNAME` + `MAJOR`/`MINOR` | 有设备节点时的路径与设备号 |
| `SEQNUM` | 序号，**用户态发现跳号就知道丢了事件**（netlink 是广播，可能丢） |

**观测与手动触发：**

```bash
udevadm monitor --kernel --property   # 实时看 uevent
udevadm info -q all -n /dev/sda       # 查某个设备的完整属性
echo add > /sys/bus/pci/devices/0000:00:14.0/uevent   # 手动重放一次 add 事件
```

> 每个设备目录下那个 **`uevent` 文件**是双向的：读它得到当前事件的键值，
> 写 `add`/`remove`/`change` 进去就**手动触发**一次 uevent——排障时判断"规则为什么不生效"非常好用。

---

### 版本断崖：`/dev` 下的节点现在是**内核**创建的（devtmpfs，v2.6.32+）

LKD 写作时（2.6 中期），`/dev` 节点是 **udev 收到 uevent 后 `mknod` 创建的**。现在分工变了：

| 阶段 | 谁创建 `/dev/sda` | 问题 |
|------|------------------|------|
| 早期 | **udev**（用户态 `mknod`） | 早期启动阶段 udev 还没跑起来，根文件系统挂载不了 |
| 现代（**v2.6.32+**） | **内核的 devtmpfs**（`drivers/base/devtmpfs.c`，v6.6 仍在） | — |

```
现在的两步分工：
  ① 内核 devtmpfs：设备一注册就**自动**在 /dev 下建节点（不依赖任何用户态进程）
  ② udev：负责**权限、所有者、符号链接、重命名**（策略层）
```

| devtmpfs | udev |
|----------|------|
| 在内核里，启动时就挂载到 `/dev` | 用户态守护进程（systemd-udevd） |
| 只做 `mknod`（名字由内核定，如 `sda`） | 做 `/dev/disk/by-uuid/...` 稳定符号链接 |
| 无条件执行 | 按 `/etc/udev/rules.d/` 规则 |
| 不涉及权限 | 设 mode/owner/group（如 `render` 组可访问 GPU） |

> **为什么这个变化重要：** 早期用户态 `mknod` 有鸡生蛋问题——要挂载根文件系统需要 `/dev/root`，
> 但创建 `/dev/root` 的 udev 又在根文件系统上。devtmpfs 把"建节点"这件最小的事挪进内核，
> udev 退化成纯策略层，**启动更快也更健壮**。
>
> 这也是与 [Ch 5.6](../../chapter-05-system-calls/notes/section-5.6-添加系统调用与替代方案.md) 一致的思路：
> **内核提供机制，用户态负责策略。**

| 用户态 | **udev rules** — 绑 IRQ、权限、符号链接、重命名网卡 |

#### 网卡命名：udev 最著名的策略

| 时代 | 命名 | 问题 |
|------|------|------|
| 老 | `eth0`/`eth1` | **按探测顺序**，重启后可能换（多网卡机器上的噩梦） |
| 现代（systemd） | **`enp1s0f0`** | **可预测命名**：`en`（以太网）+ `p1s0`（PCI 1:0.0）+ `f0`（功能 0） |

> 依据就是 sysfs 里的**物理拓扑路径**——所以 17.3 讲的"真实设备树"是这套命名的地基。
> HFT 服务器上建议**关掉**可预测命名（`net.ifnames=0`）回到 `eth0`，或**反过来**用
> `udev` 按 MAC/PCI 地址固定成 `mkt0`/`ord0` 这类语义名——**关键是不可漂移**。

---

### HFT 怎么用 uevent

| 场景 | 做法 |
|------|------|
| 感知网卡热插拔/掉线 | 监听 netlink `NETLINK_KOBJECT_UEVENT`，过滤 `SUBSYSTEM=net` + `ACTION=add/remove` |
| 自动重建行情通道 | 收到 `add` → 重新配置 `ethtool -C`/RPS/中断亲和 → 重启收包线程 |
| 检测丢事件 | 看 `SEQNUM` 是否连续，跳号说明 netlink 缓冲区溢出（需调 `net.core.*` 或改用主动轮询 sysfs） |

> **注意 netlink 是广播且可能丢**：`NETLINK_KOBJECT_UEVENT` 的接收缓冲区满时事件被丢弃，
> 用户态只能靠 `SEQNUM` 跳号发现。对"网卡掉了"这种关键事件，
> 稳妥做法是**双保险**：uevent 做快速通知 + 定时轮询 `/sys/class/net/*/operstate` 做兜底。

→ [Ch 17.3 sysfs](./section-17.3-sysfs-虚拟文件系统.md) · [Ch 17.2 统一设备模型](./section-17.2-统一设备模型.md)



<details>
<summary>自测题（点击展开）</summary>

**Q1.** 内核事件层（uevent）如何通知用户态？HFT 热插拔网卡如何感知？

<details><summary>答案</summary>

内核通过 kobject_uevent 发送 uevent（ADD/REMOVE/CHANGE），经 netlink(KOBJECT_UEVENT) 广播到用户态。用户态 udev 守护进程接收 uevent → 执行规则（重命名网卡/加载驱动/配置 IP）。HFT 系统可以监听 netlink uevent 感知网卡热插拔，自动重新初始化行情通道。

</details>

**Q2.** 现在的 `/dev` 节点还是 udev 创建的吗？devtmpfs 和 udev 各负责什么？

<details><summary>答案</summary>

**不是了**。`v2.6.32` 起，`/dev` 下的节点由**内核的 devtmpfs** 创建（`drivers/base/devtmpfs.c`，v6.6 仍在）。

现在的分工是两层：

| | **devtmpfs**（内核） | **udev / systemd-udevd**（用户态） |
|---|---|---|
| 干什么 | 设备一注册就**自动 `mknod`** | 设**权限、所有者、符号链接、重命名**、跑外部程序 |
| 何时 | 设备注册瞬间，不依赖任何用户态 | 收到 netlink uevent 之后（异步） |
| 名字 | 内核定的默认名（`sda`、`ttyUSB0`） | 可改（`/dev/disk/by-uuid/...`、`mkt0`） |
| 权限 | 默认 root/root，mode 固定 | 按 `/etc/udev/rules.d/*.rules` 定制（如加到 `render` 组） |

**为什么这么改？** 老方案有鸡生蛋问题：
挂载根文件系统需要 `/dev/root` → 但 `/dev/root` 这个节点得由 udev 创建 → 而 udev 这个用户态程序本身又住在根文件系统上。
早期靠 initramfs 绕开，devtmpfs 则把"建节点"这件最小的事挪进内核，**udev 退化成纯策略层**。

这正是 LKD 反复强调的 **机制与策略分离**（Ch 1.3）：
- 机制（内核）：这个设备存在，它叫 sda，主次设备号 8:0 → devtmpfs 建出 `/dev/sda`；
- 策略（用户态）：谁可以访问它、该不该加个别名、要不要触发备份脚本 → udev 规则。

**排障提示：** 如果 `/dev` 下节点存在但权限不对 → 是 udev 规则的问题；
如果节点**根本不存在** → 去查 `dmesg` 和 `/sys/kernel/debug/devices_deferred`（驱动 probe 延迟，见 17.2）。

</details>

**Q3.** 用 uevent 监控网卡状态够可靠吗？为什么需要双保险？

<details><summary>答案</summary>

**不够可靠**——netlink 广播是**尽力而为**的，缓冲区满时会**丢弃事件**，而且用户态不会收到任何"你丢了事件"的通知。

机制上：`NETLINK_KOBJECT_UEVENT` 是广播型 netlink，内核往每个监听者的 socket 接收缓冲区写，
**写不进去就直接丢**。用户态唯一的线索是 uevent 里的 **`SEQNUM`** 字段：

```
ACTION=add
DEVPATH=/devices/.../net/eth0
SUBSYSTEM=net
SEQNUM=4321        ← 单调递增
```

`SEQNUM` 是全局递增的，你只要发现"上一个是 4319，这次是 4321"，就知道 **4320 丢了**。

**为什么会丢：**
1. udev 或你的监听程序**处理太慢**（回调里做了阻塞操作），接收缓冲区堆积；
2. 开机瞬间**大量设备**同时上报，缓冲区瞬间被打满；
3. 监听程序**重启期间**的事件全部错过（netlink 没有重放机制）。

**双保险设计（HFT 推荐）：**

| 通道 | 作用 | 特点 |
|------|------|------|
| **netlink uevent** | 快速通知（毫秒级感知） | 可能丢，但快 |
| **定时轮询 `/sys/class/net/*/operstate`** | 兜底校验 | 慢（秒级），但**绝不丢** |

```bash
# 兜底轮询：operstate 只有 up/down/unknown/lowerlayerdown
cat /sys/class/net/eth0/operstate
# 更细的：carrier（链路层是否检测到载波）
cat /sys/class/net/eth0/carrier
```

**实践要点：**
- uevent 回调里**只做入队**，不要做重活（配置网卡、重启线程都挪到工作线程）；
- 启动时要**先全量扫描一遍 sysfs**（枚举现有网卡），再开始监听增量事件——否则会漏掉开机时已经存在的设备；
- 关键链路状态（行情通道是否活着）**永远不要只依赖事件**，要有独立的**心跳/超时检测**。
  这条原则在分布式系统里是通用的：事件通知是加速手段，不是正确性来源。

</details>

</details>
---
