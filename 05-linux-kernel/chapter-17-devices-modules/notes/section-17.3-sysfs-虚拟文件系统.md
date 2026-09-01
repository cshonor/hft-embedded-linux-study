## ③ sysfs 虚拟文件系统

| 属性 | 说明 |
|------|------|
| **本质** | 内存中 **VFS** — 把 **kobject 层次** 导出到用户态 |
| **挂载点** | 通常 **`/sys`** |
| **映射** | kobject → **目录** · 属性（attributes）→ **文件** |

```
/sys/block/nvme0n1/queue/scheduler
/sys/class/net/eth0/...
/sys/devices/pci0000:00/...
```

| 用途 | 查看 **拓扑** · **读写驱动参数** · 脚本调优 |

---

### 版本断崖：sysfs 底层已换成 kernfs（v3.14）

LKD 讲 sysfs 基于 `sysfs_dirent`。**v6.6 里 sysfs 是 kernfs 的一个实例**：

```c
/* include/linux/sysfs.h:16 — v6.6 */
#include <linux/kernfs.h>
...
struct kernfs_node *sysfs_break_active_protection(struct kobject *kobj,
						  const struct attribute *attr);
```

而 `struct kobject` 里那个 sysfs 指针的类型已经变成了 kernfs 的：

```c
/* include/linux/kobject.h:71 — v6.6 */
struct kernfs_node	*sd;	/* sysfs directory entry */
```

| 代 | 实现 | 说明 |
|----|------|------|
| sysfs 初版（2.6） | `sysfs_dirent` 树 | 自己实现目录项、dentry 缓存 |
| **v3.14+（现役）** | **kernfs** | 把"内存中的目录树"抽成通用层，sysfs 与 **cgroupfs** 共用它 |

> **为什么关心这个？** 因为你在 `/sys/fs/cgroup/` 下看到的目录结构和 `/sys/devices/` 是**同一套代码**渲染的。
> 而且 kernfs 引入了 **active reference** 机制——删除一个 sysfs 文件时，会先"停活"再等所有正在进行的读写退出，
> 这解决了长期存在的"rmmod 时 sysfs 回调还在跑"的竞态。

---

### 一文件一值：sysfs 的设计约束

sysfs 强制约定：**一个文件只表达一个值**，内容长度**不超过一页（PAGE_SIZE，通常 4096 字节）**。

```c
/* include/linux/device.h:156/179 — v6.6 */
#define DEVICE_ATTR(_name, _mode, _show, _store) \
	struct device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show, _store)
#define DEVICE_ATTR_RW(_name) \
	struct device_attribute dev_attr_##_name = __ATTR_RW(_name)
```

```c
/* 典型的 show/store 回调 */
static ssize_t foo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", my_value);   /* 写进 buf，返回长度 */
}
static ssize_t foo_store(struct device *dev, struct device_attribute *attr,
			 const char *buf, size_t count)
{
	int v;
	if (kstrtoint(buf, 0, &v))      /* 从字符串解析 */
		return -EINVAL;
	my_value = v;
	return count;                    /* 返回 count 表示全部吃掉 */
}
static DEVICE_ATTR_RW(foo);
```

| 约束 | 后果 |
|------|------|
| **一文件一值** | 想导出一张表？不行，得建目录 + 一堆文件（或改用 debugfs/netlink） |
| ≤ PAGE_SIZE | 超过一页的数据放不下（大块统计请走 procfs/debugfs/seq_file） |
| 文本格式 | 每次读写都要字符串↔二进制转换，有开销（**不适合高频路径**） |

> **sysfs 不是给热路径用的。** 每次 `cat /sys/...` 都是一次内核回调 + 字符串格式化。
> HFT 里只在**启动/配置阶段**读 sysfs，绝不能放进 tick 循环。

---

### /sys 的顶层布局与符号链接

| 目录 | 内容 | 组织维度 |
|------|------|---------|
| **`/sys/devices/`** | **唯一的真实设备树** | 按**物理拓扑**（PCI 域 → 总线 → 槽位） |
| `/sys/class/` | 按**功能**分类的视图 | 全是**符号链接**指向 `/sys/devices/` |
| `/sys/block/` | 块设备的视图 | 符号链接（**遗留接口**，逐步并入 class） |
| `/sys/bus/` | 按**总线类型**的视图 | 每个总线下有 `devices/` 和 `drivers/` |
| `/sys/module/` | **已加载模块**及其参数 | 模块名 → `parameters/`、`holders/` |
| `/sys/kernel/` | 内核子系统参数 | `mm/`、`debug/`（kdump）、`irq/` |
| `/sys/firmware/` | 固件接口 | `acpi/`、`efi/`、`devicetree/` |
| `/sys/fs/` | 各文件系统的挂载控制 | `cgroup/`、`bpf/`、`ext4/` |

```
/sys/class/net/eth0            （功能视图，好找）
        │ symlink
        ▼
/sys/devices/pci0000:00/0000:00:1f.6/net/eth0   （物理视图，能看到真实总线位置）
        │
        └── device/  →  ../0000:00:1f.6        （回到 PCI 设备本身）
               ├── numa_node                    ← HFT 关心：网卡在哪个 NUMA 节点
               ├── local_cpulist                ← 本地 CPU 是哪些
               └── sriov_numvfs                 ← SR-IOV 虚拟功能数量
```

> **拓扑溯源技巧：** 从 `/sys/class/net/eth0` 出发，`readlink -f` 就能得到它在 PCI 树上的**真实位置**，
> 从而知道"这块网卡挂在哪个 CPU 的 PCIe 根复合体下"——这是绑核的依据。

---

### sysfs 是 ABI：一旦发布就不能改

与 [Ch 5.6 的系统调用 ABI](../../chapter-05-system-calls/notes/section-5.6-添加系统调用与替代方案.md)、
[Ch 16.7 的 tracepoint 化石](../../chapter-16-page-cache/notes/section-16.7-历史演进与避免拥塞.md)同源的一条铁律：

```
Documentation/ABI/ 下的四个等级：
  stable/     —— 至少保证 2 年不变，用户态可以放心依赖
  testing/    —— 可能变，但会提前通知
  obsolete/   —— 即将删除，有迁移路径
  removed/    —— 已删除（保留记录供后人查询）
```

> 一个 sysfs 文件一旦被用户态工具依赖，它的**路径、格式、单位**就都成了契约。
> 内核开发者的正确做法是：加新接口，而不是改旧接口。

**HFT 示例：**

| 路径 | 调什么 |
|------|--------|
| `/sys/block/*/queue/scheduler` | I/O 调度器（Ch 14） |
| `/sys/class/net/*/queues/...` | RSS/RPS 等（→ Rosen Ch14） |
| **`/sys/class/net/*/device/numa_node`** | 网卡在哪个 NUMA 节点 → 决定绑哪组核 |
| **`/sys/class/net/*/device/local_cpulist`** | 与该网卡同节点的 CPU 列表 |
| **`/sys/class/net/*/queues/rx-*/rps_cpus`** | RPS 的 CPU 掩码 |
| **`/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages`** | 预留 2MB 大页数量（DPDK 前提） |
| `/sys/class/net/*/device/sriov_numvfs` | 开启 SR-IOV 虚拟功能 |
| `/proc/irq/*/smp_affinity` | 网卡中断绑核（**这是 procfs 不是 sysfs**） |

---

### 三种"内核 ↔ 用户态"通道的选型

| 通道 | 适合 | 不适合 |
|------|------|--------|
| **sysfs** | 稳定的、面向设备的**配置与状态**，一值一文件 | 高频读写、批量数据、复杂命令 |
| **netlink** | 网络配置、事件通知（`ip`/`ss` 的背后） | 极简开关（杀鸡用牛刀） |
| **debugfs** | **调试用**，格式随意、不做兼容承诺 | 生产依赖（随时可能改名消失） |

> Ch 5.6 讲过"添加系统调用的替代方案"——**sysfs 就是那个首选替代**。
> 一个内核参数的读写，用 sysfs 实现比加系统调用简单一个数量级，且不侵占 ABI。

→ [06.6 SysPerf Ch9 scheduler](../../../06.6-systems-performance/chapter-09-disks/notes/section-9.4-硬件与软件架构.md) · [Ch 5](../../chapter-05-system-calls/) **优先 sysfs 而非新 syscall**



<details>
<summary>自测题（点击展开）</summary>

**Q1.** sysfs 如何反映设备拓扑？HFT 可以从 sysfs 获取什么信息？

<details><summary>答案</summary>

sysfs（/sys）以目录树映射设备模型：/sys/devices/ 下按总线/层级组织设备。每个设备目录下有属性文件（uevent/numa_node/cpulist）。HFT 用 sysfs：1) 查网卡 NUMA 节点（`cat /sys/class/net/eth0/device/numa_node`）确保绑同核组；2) CPU 拓扑（/sys/devices/system/cpu/）；3) 设置 smp_affinity（/proc/irq/X/smp_affinity）。

</details>

**Q2.** `/sys/class/net/eth0` 和 `/sys/devices/pci0000:00/.../net/eth0` 是什么关系？为什么要分成两套？

<details><summary>答案</summary>

**`/sys/devices/` 是唯一真实的树**，其余都是视图（符号链接）。

- `/sys/devices/pci0000:00/0000:00:1f.6/net/eth0` —— **按物理拓扑**组织的真实位置：PCI 域 → 总线 → 设备 → 功能 → 网卡；
- `/sys/class/net/eth0` —— **按功能分类**的视图，本身是个 **symlink**，指向上面那个真实路径。

分成两套是因为**使用者关心的维度不同**：

| 谁用 | 关心什么 | 用哪个 |
|------|---------|--------|
| 驱动/电源管理 | 父子关系、总线位置、关电顺序 | `/sys/devices/`（真实拓扑） |
| 运维/脚本 | "给我这台机器的所有网卡" | `/sys/class/net/`（按功能聚合） |
| 匹配驱动 | 这条总线上有哪些设备/驱动 | `/sys/bus/pci/devices/` + `drivers/` |

**实用技巧——从网卡反查物理位置：**
```bash
readlink -f /sys/class/net/eth0
# → /sys/devices/pci0000:00/0000:00:1f.6/net/eth0
cat /sys/class/net/eth0/device/numa_node      # -1 表示无 NUMA 信息
cat /sys/class/net/eth0/device/local_cpulist  # 与该卡同节点的 CPU
```

这条链路对 HFT 是**绑核的依据**：CPU 应当与网卡处于**同一个 NUMA 节点**，
否则每个报文都要跨节点访问内存，多出几十纳秒的跨节点延迟，且占用 QPI/UPI 带宽。

注意 `/sys/block/` 是**遗留视图**（早期内核遗留），新代码应走 `/sys/class/block/`；
这也是 sysfs ABI 铁律的一个例证——即便设计上已经过时，也不能把老路径删掉。

</details>

**Q3.** 能不能在交易主循环里读 sysfs 来监控设备状态？为什么？

<details><summary>答案</summary>

**不能。** sysfs 是给配置和低频监控用的，不是给热路径用的。原因有三层：

**1. 每次读写都是一次内核回调 + 字符串格式化**
```c
static ssize_t foo_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sysfs_emit(buf, "%d\n", my_value);   /* 二进制 → 字符串 */
}
```
你要的那个整数被格式化成 ASCII，你再解析回来。来回两次转换 + 一次 `open/read/close` 三个系统调用，
比直接读一个共享内存里的整数**贵 3~4 个数量级**。

**2. 可能触发锁竞争**：属性回调常常要拿设备锁、总线锁，甚至 `device_lock()`。多线程同时读一个属性会串行化。

**3. sysfs 有 active reference 语义**（kernfs 引入）：删除文件时要等所有在途的读写退出。
热路径上高频读意味着这个"等待"可能变长，与 rmmod/热插拔互相拖累。

**正确的分层做法：**

| 需求 | 用什么 |
|------|--------|
| 启动期读一次（NUMA 节点、队列数、中断号） | ✅ sysfs，读完缓存到进程内存 |
| 配置变更（调度器、RPS 掩码、大页数） | ✅ sysfs，运维窗口内做 |
| 运行时高频指标（收发包数、丢包、队列深度） | ❌ sysfs —— 用 `ethtool -S`（ioctl/netlink）、`/proc/net/dev`、或 **eBPF** 在内核里聚合好后一次吐出 |
| 每报文级别的打点 | eBPF + perf/ring buffer（见 02 模块全书） |

经验法则：**sysfs 读取频率上限是"每秒几次"这个量级**，超过就该换通道。
HFT 系统里所有 sysfs 读取都应该集中在**启动初始化阶段**，之后进程只查自己的内存副本。

</details>

</details>
---
