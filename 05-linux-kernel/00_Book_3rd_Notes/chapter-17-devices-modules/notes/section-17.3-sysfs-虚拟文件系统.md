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

**HFT 示例：**

| 路径 | 调什么 |
|------|--------|
| `/sys/block/*/queue/scheduler` | I/O 调度器（Ch 14） |
| `/sys/class/net/*/queues/...` | RSS/RPS 等（→ Rosen Ch14） |

→ [03 SysPerf Ch9 scheduler](../../../../15-systems-performance/chapter-09-disks/notes/section-9.4-硬件与软件架构.md) · [Ch 5](../../chapter-05-system-calls/) **优先 sysfs 而非新 syscall**



<details>
<summary>自测题（点击展开）</summary>

**Q1.** sysfs 如何反映设备拓扑？HFT 可以从 sysfs 获取什么信息？

<details><summary>答案</summary>

sysfs（/sys）以目录树映射设备模型：/sys/devices/ 下按总线/层级组织设备。每个设备目录下有属性文件（uevent/numa_node/cpulist）。HFT 用 sysfs：1) 查网卡 NUMA 节点（`cat /sys/class/net/eth0/device/numa_node`）确保绑同核组；2) CPU 拓扑（/sys/devices/system/cpu/）；3) 设置 smp_affinity（/proc/irq/X/smp_affinity）。

</details>

</details>
---
