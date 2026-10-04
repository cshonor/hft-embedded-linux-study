# 8.3 BPF 工具：目录缓存、inode 缓存与挂载（8.3.18–8.3.20）

> 底本：《BPF之巅》第 8 章 文件系统，8.3 节（印刷 p336–341）

| 工具 | 来源 | 一句话 |
|---|---|---|
| dcstat | BCC/BT | dcache 命中率每秒统计 |
| dcsnoop | BCC/BT | 逐条打印 dcache 查找/命中失败（下钻用） |
| mountsnoop | BCC | 跟踪 mount(2)/umount(2)，容器调试利器 |
| icstat | BT | inode 缓存查找命中率每秒统计 |

## dcstat —— dcache 命中率

```
# dcstat
TIME      REFS/s   SLOW/s   MISS/s  HIT%
22:48:20: 661815   27942    20814   96.86%
22:48:23: 434353   37       105     99.99%
```

- REFS/s：dcache 每秒查找次数（生产可达 50 万+/秒）。
- SLOW/s：未走快速路径的查找——自 Linux 2.5.11 起 dcache 对常用项（""、"/usr"）做了 CPU 缓存友好优化，这列是慢路径计数。
- MISS/s：查找失败（目标目录项可能还在内存，只是 dcache 没返回）。
- 探针：kprobe `lookup_fast`（命中）、kretprobe `d_lookup`（retval==0 为 miss）。lookup_fast/d_lookup 高频 ⇒ 开销不可忽视，先测试再上生产。
- 百分比计算用三元符防除零：`$percent = $refs > 0 ? 100 * $hits / $refs : 0`（BPF 校验器层面其实也有除零保护，但发之前检查更稳）。

## dcsnoop —— 命中失败下钻

```
# dcsnoop -a
TIME(s)    PID   COMM   T FILE
0.005463   2663  snmpd  R proc/sys/net/ipv6/conf/eth0/forwarding
0.005471   2663  snmpd  R sys/net/ipv6/conf/eth0/forwarding
0.005479   2663  snmpd  R net/ipv6/conf/eth0/forwarding   ← 逐级向上
```

- T 列：R=查找，M=命中失败（miss）；默认只显示 miss，`-a` 全显。
- 展示了路径查找的**逐级解析**过程（每级目录一次 lookup）。
- 每事件一行，中等负载下开销就很高——只作 dcstat 发现命中率问题后的短期下钻工具。
- bpftrace 版需要自己声明 `struct nameidata { struct path path; struct qstr last; ... }`（内核头文件里没有，取 `last.name` 要靠它）。

## mountsnoop —— 挂载事件审计

```
PID   TID   MNT_NS      COMM           CALL
1392  1392  4026531840  systemd-logind mount("tmpfs","/run/user/116","tmpfs", MS_NOSUID|MS_NODEV,"mode=0700,...")=0
```

跟踪 mount(2)/umount(2)（kprobe 系统调用内核函数），输出含**挂载命名空间 MNT_NS**——容器场景下定位是哪个容器的挂载动作。挂载操作低频，开销可忽略。容器启动时挂载了什么文件系统，一眼看清。

## icstat —— inode 缓存命中率

```
REFS   MISSES  HIT%
21647  0       100%     ← 第一秒全命中
38925  33781   8%       ← find /var -ls 遍历，大量冷 inode
```

kretprobe `find_inode_fast`，retval==0 计 miss。`find` 遍历目录树时 inode 缓存命中率骤降的演示直观展示了冷元数据扫描的代价。

## HFT 关联

- 长路径深、目录层级多的数据目录（按日期分片的行情落盘）天然拉高 dcache miss；dcstat 可量化目录结构设计的代价。
- 容器化部署的交易服务用 mountsnoop 审计启动挂载（tmpfs 配置、secret 挂载）是否符合预期。

## 常见陷阱

- dcstat 的 miss ≠ 文件不在内存：inode 可能常驻，只是 dcache 路径查找没命中。
- dcsnoop 输出量巨大（每秒几千行），不要长时间挂着。

<details>
<summary>自测</summary>

1. dcstat 的 SLOW/s 列是什么？为什么会有快慢两条路径？
   <details><summary>答案</summary>SLOW 是没走 dcache 快速路径的查找次数。自 2.5.11 起 dentry 查找有两条路：快速路径用 seqlock/RCU 无锁遍历（对 ""、"/usr" 等热项做了 CPU 缓存友好优化），慢路径走传统加锁 d_lookup。SLOW 高说明查找大量落在加锁路径或 miss——它是"快速路径失效"的计数器。</details>

2. dcsnoop 默认只显示什么？为什么这样设计？
   <details><summary>答案</summary>默认只显示 miss（T=M）。dcache 查找每秒几十万次，逐事件打印全部的开销会淹没系统；而 dcstat 已经告诉你命中率——下钻要看的只是"哪些路径没命中"，miss 通常占很小比例，默认过滤正是把输出量压到可承受的必要手段（与 BCC 工具"丢数据型过滤下沉内核态"的共同参数模式一致）。</details>

3. mountsnoop 输出中的 MNT_NS 列对容器调试有什么意义？
   <details><summary>答案</summary>挂载命名空间 ID 把一条 mount(2) 调用归属到具体容器——宿主机上全局看 mount 事件时，MNT_NS 是"这是谁挂的"的唯一可靠线索（容器里进程的 pid/comm 可以重名，命名空间不重样）。排查"容器启动挂载了什么/哪个容器在偷偷挂东西"直接按 MNT_NS 过滤。</details>
</details>
