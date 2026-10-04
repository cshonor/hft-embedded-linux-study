## 16.1.3–16.1.4 统计数据与静态配置

> **出处：** Gregg《性能之巅》Ch 16.1.3–16.1.4 · 案例排查的第一层——**便宜的观测先上**：宏观统计建 baseline，静态配置找「变了什么」。
> **HFT 实操要点：** 这一步的目标是 **淘汰便宜的假设**（负载变了？邻居变了？配置 drift？），把昂贵的 PMC/trace 留给真正需要的机制问题。

```
  统计（Statistics）                配置（Configuration）
  「系统的行为变了吗？」             「系统的参数变了吗？」
  ─────────────────                ─────────────────────
  vmstat / mpstat / sar 归档         sysctl -a 快照 diff
  对照：案发前 vs 案发后             对照：部署前镜像 vs 部署后镜像
  排除：负载/邻居/环境假设           排除：配置 drift 假设
  ↓ 都解释不了？                    ↓ 找到 drift？
  进入 PMC（16.1.5）                 定位到具体参数 → 验证因果
```

---

### 一、常规统计（Statistics）——先问「分母变了吗」

**目标：** 对 Unexplained Win，统计层的第一个问题是 **工作负载特征**——「快」是真的处理变快，还是 **要处理的量变少了**（Ch 2 工作负载分析）。

| 层级 | 工具/数据 | 看什么 | 对应章 |
|------|-----------|--------|--------|
| 全局负载 | `vmstat 1`、`uptime` | run queue（r 列）、cs/in（上下文切换/中断速率） | [Ch 6](../../chapter-06-cpus/) |
| 分核 CPU | `mpstat -P ALL 1` | **每核**利用率——尤其「别的核」是否同时变闲（邻居撤离信号） | Ch 6 |
| 进程 | `pidstat -t 1` | 策略进程各线程 %CPU、 involuntary cs | Ch 5 |
| 内存 | `vmstat -s`、`sar -B` | pgmajfault（主缺页）、扫描页（compaction 迹象） | [Ch 7](../../chapter-07-memory/) |
| 磁盘 I/O | `iostat -dx 1`、`sar -d` | 日志/回放盘是否阻塞过热路径 | [Ch 9](../../chapter-09-disks/) |
| 网络 | `ss -s`、`nstat -az`、`sar -n DEV` | 重传、丢包、软中断速率 | [Ch 10](../../chapter-10-network/) |
| 云租户 | steal time（`mpstat` %steal）、cgroup throttle | 宿主机争用是否消失 | [Ch 11](../../chapter-11-cloud-computing/) |
| **历史归档** | **`sar` 每日文件**（`/var/log/sa/`） | 案发前后同时段对比——**统计排查的本钱** | [附录 B sar 总结](../../appendix-B-sar总结.md) |

**读法示例（vmstat，HFT 视角）：**

```
$ vmstat 1
procs -----------memory---------- ---swap-- -----io---- -system-- ------cpu-----
 r  b   swpd   free   buff  cache     si   so    bi    bo   in   cs us sy id wa st
 2  0      0 812000 124000 6210000      0    0     0    12  18k 4.2k 62  8 30  0  0
 1  0      0 811200 124000 6210300      0    0     0    10  16k 3.1k 65  7 28  0  0
```

- `r`（runnable）稳定 1–2：无调度排队 → 「变快因为队列变空」的假设不成立。
- `cs` 从 4.2k 降到 3.1k：上下文切换少了——记下，供 16.1.6 软件事件验证。
- `st`（steal）= 0：无云宿主机偷取（若案发前 >0，**邻居假设的证据来了**）。

**sar 归档对比方法论（同周同时段原则）：**

```bash
# 案发日 vs 上周同 weekday 同时段（排除工作日节奏差异）
sar -u -f /var/log/sa/sa30    # 案发日 CPU
sar -u -f /var/log/sa/sa23    # 上周同日 CPU
sar -q -f /var/log/sa/sa30    # load/runq 对比
```

**要回答的三个问题（顺序固定）：**

1. **利用率是真降，还是测量窗口/采样变了？**（对照附录 B 的 sar 参数口径）
2. **负载 workload 变轻了吗？**——请求率、tick 速率、数据集大小前后对比（Ch 2）。
3. **「邻居」同时段是否可比？**——同机其他租户/进程、云宿主机、共置策略的指标同屏对比。

---

### 二、静态配置（Configuration）——再问「什么被改了」

**目标：** 配置是 **状态不是事件**——案发后只能看到「现在的值」。所以这一步的本质是 **快照对比**：拿部署时/镜像里留存的快照，diff 出「变了什么」。

**标准快照采集脚本（部署时自动落盘，排查时才有得 diff）：**

```bash
#!/bin/sh
# config-snapshot.sh —— 部署钩子里调用，输出到版本化目录
SNAP=/var/lib/perf-snap/$(date +%Y%m%d-%H%M%S)
mkdir -p $SNAP
sysctl -a                     > $SNAP/sysctl.txt      2>/dev/null
cat /proc/cmdline             > $SNAP/cmdline.txt
cat /proc/mounts              > $SNAP/mounts.txt
cat /proc/interrupts          > $SNAP/interrupts.txt
lscpu                         > $SNAP/lscpu.txt
cpupower frequency-info        > $SNAP/cpufreq.txt     2>/dev/null
dmesg                         > $SNAP/dmesg.txt       2>/dev/null
dpkg -l 2>/dev/null | awk '$1=="ii"{print $2,$3}' > $SNAP/packages.txt
uname -a                      > $SNAP/uname.txt
```

**Diff 检查项分级表：**

| 类别 | 检查项 | 为什么重要（Unexplained Win 视角） | 对应章 |
|------|--------|-------------------------------------|--------|
| 内核 | 版本、boot cmdline（`mitigations=`、`isolcpus`、`hugepagesz`） | 内核升级常带「意外的快」——比如漏洞缓解（spectre/meltdown）默认开关变化 | [Ch 3](../../chapter-03-operating-systems/) |
| sysctl | `net.*`、`vm.*`（swappiness、dirty_ratio）、`kernel.sched_*` | 镜像重建时 tuned profile 差异是经典 drift 源 | Ch 7/10 |
| CPU 管理 | governor、`isolcpus`、IRQ affinity、`rcu_nocbs` | governor 从 powersave→performance = 「白捡的 win」 | [Ch 6](../../chapter-06-cpus/) |
| 内存 | THP（`/sys/kernel/mm/transparent_hugepage/enabled`）、swap 状态 | THP 设置随镜像重置是常见「莫名变快/变慢」源 | [Ch 7](../../chapter-07-memory/) |
| 实例/机型 | AWS 机型（`instance-type`）、NUMA 拓扑、宿主机代次 | **同机型可能落在不同代宿主机**——物理拓扑都不同 | [Ch 11](../../chapter-11-cloud-computing/) |
| 应用 | 库版本（glibc/JVM）、编译参数、线程池配置、绑核脚本 | 依赖库升级（如 malloc 实现）可能悄悄改变分配器行为 | [Ch 5](../../chapter-05-applications/) |
| 挂载/FS | noatime、日志盘调度器（`/sys/block/*/queue/scheduler`） | I/O 调度器随镜像变化影响日志刷盘路径 | [Ch 8](../../chapter-08-file-systems/) |

**云环境特有陷阱（Ch 11）：**

- **宿主机更换不可见**：云厂商滚动维护后，同 ID 的实例可能已经落在**新的物理宿主机**上——NUMA 拓扑、邻居租户、甚至 CPU 步进都变了，而你没有任何配置 diff 记录。侧面证据：`dmesg` 里的启动时间、`/proc/cpuinfo` 的 microcode 版本、实例 metadata。
- **CPU credits**（T 系列突发实例）：credits 状态在重启后重置，「变快」可能只是 credits 充满了。

---

### 三、HFT 配置审计 checklist（完整版）

```
[ ] 内核版本 + cmdline（mitigations=off? isolcpus? nohz_full? intel_pstate=?）
[ ] CPU governor = performance；min=max=额定频率
[ ] THP enabled/madvise/never（对照策略进程的 madvise 覆盖）
[ ] sysctl: vm.swappiness=0, vm.compaction_proactiveness, net.core.* 队列参数
[ ] IRQ affinity（网卡队列 → housekeeping 核）+ RPS/XPS 与 RSS 一致
[ ] taskset/affinity 实际生效（taskset -pc <pid>，不要信配置文件）
[ ] NUMA：策略进程与网卡在同一节点（numactl --hardware 对照）
[ ] swap off / 换页历史（sar -B pgmajfault）
[ ] 同机 noisy neighbor：别的策略/回放/压测进程是否还在
[ ] 行情源与合约集合是否相同（上游订阅 diff）
[ ] 测量口径：打点代码 diff（对照 16.1.2 的问题陈述）
```

> 注意第 6 条的括号：**「不要信配置文件」**——亲和性/隔离的配置错了不报错，只表现为「看起来配了但没生效」。`taskset -pc`、`cat /proc/<pid>/status | grep Cpus_allowed` 才是事实。Unexplained Win 的一个经典真因就是「上次部署时 affinity 脚本 path 写错，这次部署修复了」。

---

### 四、本层输出：两张表

排查完统计+配置，手里应该有：

1. **统计对比表**：指标 × (案发前, 案发后, 变化方向, 是否显著)——为 16.1.5 的 PMC 选择提供线索。
2. **配置 diff 清单**：每行一个差异 + 该差异的「影响假设」——若 diff 非空，逐个验证（能回滚的在测试机 A/B）；若 diff 为空，**配置假设整体淘汰**，嫌疑转向负载/状态/真实代码。

```
统计+配置都干净 → 便宜假设全部淘汰
                    ↓
     「快」发生在微架构层（cache/分支/调度）或代码层
                    ↓
        进入 16.1.5 PMC：用硬件计数器量化「哪类快」
```

---

### 五、衔接

- 上一节：[16.1.1–16.1.2](./section-16.1.1-16.1.2-问题陈述与分析策略.md)（假设矩阵决定这里看什么）。
- 下一节：[16.1.5–16.1.6](./section-16.1.5-16.1.6-PMC-与软件事件.md)（PMC 下钻微架构）。
- sar 全量用法：[附录 B sar 总结](../../appendix-B-sar总结.md)。

---

<details>
<summary>代码自测（Q&A，先遮住答案想）</summary>

**Q1：为什么「同周同时段」对比 sar 归档，而不是昨天 vs 今天？**

A：工作负载有周期性——周一开盘与周五收盘的 tick 量、夜盘与日盘的负载完全不同。随机选对照组会把「节奏差异」误判成「行为变化」。同 weekday + 同时段是最小成本的负载匹配；更严格的做法是按 tick 速率/波动率分层后对比（Ch 12 的可比性原则）。

**Q2：配置是「状态不是事件」——这句话怎么理解，后果是什么？**

A：事件（如 CPU 利用率）被连续采样，事后可回放；配置（如 sysctl 值）只有「当前值」，没有历史序列。若部署时不落快照，案发后就**永远无法知道变过什么**——这正是很多 Unexplained Win 永远 unexplained 的原因。对策是把配置快照做成部署流水线的固定步骤（事前纪律，见案例背景篇第四节的 baseline 纪律）。

**Q3：`mpstat` 里 %steal 从 3% 变 0%，对 Unexplained Win 排查意味着什么？**

A：强证据指向「云邻居假设」：宿主机上其他租户曾经抢走 3% 的物理 CPU 时间（hypervisor 侧的 steal），现在没有邻居或邻居安静了。这是「win 来自环境而非代码」的直接证据——对策不是庆祝而是：① 确认该 win 的不稳定性（邻居可能回来），容量规划不能用这个数据；② 若业务关键，考虑专属宿主机/置放群组（Ch 11）。

**Q4：为什么「taskset -pc 看实际亲和性」比「检查部署配置」可靠？**

A：配置声明的是意图，`/proc/<pid>/status` 的 `Cpus_allowed` 才是内核事实。中间可能断链：脚本 path 错误静默失败、容器 runtime 覆盖 affinity、cgroup cpuset 与 taskset 冲突、进程被 systemd 重启后丢了亲和。Unexplained Win 的排查全程都应遵循「观测事实优先于配置声明」——这是问题陈述纪律在执行层的延续。

**Q5：配置 diff 为空时，能得出「配置没变」的结论吗？**

A：不能完全得出，只能缩小范围。① 你只能 diff **采集过的** 配置项——没进快照的（如 BIOS 设置、网卡固件参数、云宿主机状态）盲区仍在；② 云环境里「实例换宿主机」不会出现在任何实例内配置 diff 里。所以正确的表述是「**在观测范围内的** 配置假设淘汰」，残余不确定性用侧面证据（dmesg 启动时间、microcode 版本）继续压缩。

</details>

---

← [本章导读](../README.md)
