## 4.1 工具覆盖范围与「危机工具」

> 本章导读 · [4.2 工具的分类与原理](./section-4.2-工具的分类与原理.md) · [4.3 核心观测数据源](./section-4.3-核心观测数据源.md) · [4.4 sar 工具](./section-4.4-sar-工具.md) · [4.5 四大追踪器](./section-4.5-四大追踪器.md) · [4.6 观测的观测](./section-4.6-观测的观测Observing-Observability.md)

---

### 本节讲什么

回答一个看起来「不像技术问题」的问题：**性能工具应该什么时候装？** Gregg 的答案是：出事之前。本节展开他的「为时已晚」论证，给出危机工具包的构成、五个包之间的依赖关系、版本匹配的具体坑位，以及 HFT runbook 里第一反应命令的组织方法。

### 要点

| # | 要点 | 一句话展开 |
|---|------|-----------|
| 1 | 危机时装工具 = 双重惩罚 | 装包本身耗时延长 MTTR；装错版本引入新问题 |
| 2 | 五大包按「数据源 → 前端」分层 | procps/sysstat 读 /proc；linux-tools 接 perf 事件；bcc/bpftrace 走 eBPF |
| 3 | perf 版本必须匹配内核 | `linux-tools-$(uname -r)`，否则事件列表为空或数据错误 |
| 4 | BCC 依赖内核 headers + clang | 危机时才发现没装 headers，等于 BPF 全线瘫痪 |
| 5 | runbook 的价值在「复制粘贴即可跑」 | 第一反应命令预先写好、预先验证过，而不是现场翻 man |
| 6 | 嵌入式例外 | BCC 运行时编译在交叉目标机上常不可用，需 libbpf+CO-RE 预编译 |

---

### 一、Gregg 的「为时已晚」论证

性能危机的典型时刻：线上 tick-to-trade P99 突然从 5µs 飙到 50µs，老板站在工位后面，每分钟都在问「查到没有」。这时你执行：

```bash
apt install linux-tools-$(uname -r) bpftrace
```

然后发生什么？

| 环节 | 危机时刻的代价 |
|------|---------------|
| **包仓库不可达** | 生产机常常无外网 / 只有内部 mirror，mirror 里未必有匹配版本 |
| **依赖解析失败** | bpftrace 依赖 libbpf、bcc 依赖 clang+headers，缺一环装不上 |
| **版本不匹配** | `uname -r` 是定制内核（云厂商 / PREEMPT_RT），仓库里根本没有对应 linux-tools |
| **装上了但不能用** | perf 报 `not found for this kernel`，BCC 报 `bcc: Unable to compile` |
| **现场学工具** | 就算装上了，第一次跑 bpftrace 现查语法，几分钟又没了 |

结论：**工具链的可用性必须在平时验证，而不是危机时赌运气**。MTTR（平均恢复时间）里最冤枉的部分，就是花在「装工具」上的那一段。

---

### 二、危机工具包：五个包的依赖关系

不是五个独立软件包，而是有层次结构的组合。理解依赖关系，才知道「装了 A 没装 B」时坏的是哪条腿：

```
┌─────────────────── 用户态前端 ───────────────────┐
│  procps        sysstat      perf      BCC    bpftrace
│  (top/vmstat   (iostat/     (剖析+     (Python   (单行DSL)
│   pidstat/ps)   sar/mpstat)  trace)     工具集)      │
└────┬─────────────┬───────────┬──────────┬─────────┬──┘
     │             │           │          │         │
     ▼             ▼           ▼          ▼         ▼
  /proc,       /proc,      perf_event  eBPF VM   eBPF VM
  /sys 读取    /sys 读取    子系统       ▲         ▲
  (纯文本)     (纯文本)    (内核内建)    │         │
                                        │         │
                              clang + 内核headers  │
                              (运行时编译)         │
                                        └────┬────┘
                                             ▼
                                    BTF / tracepoint
                                    (内核侧插桩点)
```

| 包 / 组件 | 提供什么 | 依赖什么 | 缺了会怎样 |
|-----------|----------|----------|-----------|
| **procps** | `ps`、`top`、`vmstat`、`pidstat`、`free` | 仅 /proc、/sys | 60 秒清单前 10 条全部失效 |
| **sysstat** | `iostat`、`mpstat`、`sar`、`sadc` | /proc、/sys | 磁盘/CPU 分核视图失效、无历史归档 |
| **linux-tools-common + linux-tools-$(uname -r)** | `perf` | perf_event 子系统 + **版本匹配的内核** | perf 报版本错误或事件不可用 |
| **bcc-tools** | biolatency、runqlat、biostacks、ext4slower… | eBPF VM + **clang + 内核 headers** | 编译报错，所有 BCC 工具瘫痪 |
| **bpftrace** | 单行/脚本 eBPF 追踪 | eBPF VM + BTF（推荐） | 即兴查询能力消失 |

关键认知：**前两个包是「纯读者」**（只 cat /proc），几乎不会坏；**后三个是「内核合作者」**，需要内核侧配合（perf_event、eBPF VM、BTF、headers），版本错位就罢工。危机清单的验证深度也应对应分层——读者类验证存在性即可，合作者类必须实际跑通一条最小命令。

---

### 三、版本匹配的三个坑

#### 坑 1：perf 与内核

```bash
# 正确安装（Debian/Ubuntu 系）
apt install linux-tools-common linux-tools-$(uname -r)

# 验证
perf stat -e cycles,instructions true   # 能出数 = OK
```

云厂商定制内核（AWS Nitro、Azure 内核）和 PREEMPT_RT 补丁内核常常在发行版仓库里**没有对应的 linux-tools 包**。这时 perf 要么从内核源码树自己编译（`tools/perf/`），要么用静态链接发行版。HFT 用 RT 补丁内核的团队，这一条要在镜像制作时就固化。

#### 坑 2：BCC 与 headers

BCC 的运行时编译模型（详见 [15.1 BCC](../../chapter-15-bpf/)）要求**目标机上有 clang 和当前内核的 headers**。生产机按最小化原则装系统时，headers 几乎必然缺席。两条出路：

| 方案 | 做法 | 代价 |
|------|------|------|
| 补装 headers | `apt install linux-headers-$(uname -r)` + clang | 目标机多出几百 MB；定制内核同样可能无包 |
| libbpf + CO-RE | 开发机上预编译成 .o，目标机只有 libbpf 加载器 | 失去 BCC 快速迭代；换 15.1.7 的工作流 |

#### 坑 3：BTF 与 bpftrace

新版 bpftrace 的很多功能（`kfunc`、结构体定义）依赖内核开启 `CONFIG_DEBUG_INFO_BTF`。`ls /sys/kernel/btf/vmlinux` 存在即开启。老内核（<5.2 或发行版没开 BTF）上 bpftrace 退化为只能用 kprobe 猜偏移的模式。

---

### 四、runbook：第一反应命令的组织法

工具预装只是硬件条件；**runbook 是把「我知道该跑什么」变成「值班的人也会跑」的机制**。组织原则：

**1. 按「越跑越深」分梯度，而不是按工具分类**

```text
S0  无害常驻（永远在跑）
    sar 10s 归档 · node_exporter · 超时计数器

S1  60 秒清单（复制粘贴一整块，10 条命令一起看）
    uptime · vmstat 1 · mpstat -P ALL 1 · pidstat 1 · iostat -xz 1
    · free -m · sar -n DEV 1 · ss -tiepm · dmesg | tail · top

S2  定向深挖（按 S1 指到的方向选一两条）
    CPU 方向   → perf record -F 99 -g -p <PID> / runqlat
    磁盘方向   → biolatency / biostacks / iostat -x 精读
    网络方向   → ss -tiepm 逐字段 / tcpretrans
    内存方向   → vmstat si/so / PSI / drsnoop

S3  全量插桩（需授权，限时窗口）
    bpftrace 汇总脚本 · perf trace 短窗口
```

**2. 每条命令预设「正常长什么样」**

只有命令没有基线，值班的人跑完还是不会判断。S1 的每条命令旁注明该机器的常态值（例：`vmstat 1 的 cs 正常 5k~8k，>50k 才是异常`）。

**3. 演练优先**

runbook 写完不演练等于没写——S1 块在例行维护窗口实际跑一遍，确认每条都能出数、无版本报错。这与 [Ch 16 案例研究](../../chapter-16-case-studies/)的「先跑清单再深入」流程互为表里：ch16 是分析方法的演练，本节是工具可用性的演练。

---

### HFT / 嵌入式关联

**HFT 裸机 checklist（开机验收项）：**

```
[ ] perf stat -e cycles,instructions true  能出数（版本已匹配）
[ ] bpftrace -e 'BEGIN { printf("ok\n"); exit(); }'  能加载
[ ] bcc 的 runqlat 5 2 能出直方图（clang + headers 就位）
[ ] sar/sadc 已配置历史归档（非热路径机器也建议有）
[ ] /sys/kernel/btf/vmlinux 存在（CO-RE 可用）
[ ] runbook S1 块演练通过一次，记录基线值
```

**嵌入式交叉目标机例外：** BCC 的运行时编译需要目标机上的 clang——交叉编译的嵌入式板卡上通常既没有 clang 也没有磁盘空间放 headers。嵌入式场景的「危机工具包」是 **libbpf + CO-RE 预编译 .o + 最小加载器**，在开发机上编译、随固件一起部署。这与 [15.1 BCC 三篇](../../chapter-15-bpf/)中「嵌入式交叉编译不可用」的结论一致。

**共置机的额外一条：** 危机工具本身有开销（见 [4.6 观测的观测](./section-4.6-观测的观测Observing-Observability.md)）。S2/S3 级命令对热路径是侵入性的——runbook 里应写明「S2 起需值班长批准，S3 限时 30s 窗口」。

---

### 衔接

- 上一节（[4.2 工具分类](./section-4.2-工具的分类与原理.md)）回答「工具分几类」；本节回答「什么时候装、怎么验证」。
- 下一节 [4.4 sar](./section-4.4-sar-工具.md)：危机包里唯一自带「历史回溯」能力的工具。
- [Ch 1 的 60 秒清单](../../chapter-01-intro/) 是 S1 块的原始出处；[附录 C bpftrace 单行命令](../../appendix-C-bpftrace单行命令.md) 是 S2/S3 的弹药库。

---

### 常见陷阱

1. 危机时才装工具——出事时才发现 perf/BPF 没装或版本不匹配，应该预装并验证
2. 危机工具不熟——出事时现学 man page 太慢，runbook 应预设好第一反应命令
3. 工具和内核版本不匹配——perf 需匹配 linux-tools-$(uname -r)，BCC 需匹配内核 headers
4. 只验证「装了」不验证「能跑」——`which perf` 存在不等于 `perf stat` 能出数；合作者类工具必须实际执行最小命令
5. runbook 无基线——命令跑出来了，值班的人不知道什么算异常；每条命令要配常态值
6. 嵌入式照搬服务器清单——BCC 运行时编译在交叉目标机不可用，要换 libbpf+CO-RE 路线

<details>
<summary>自测题（点击展开）</summary>

1. 什么是「危机工具包」？为什么需要预装？
   <details><summary>答</summary>出事时第一时间需要的工具（vmstat/mpstat/perf/ss/iostat）——出事时才装可能网络不通或版本不对；且装包耗时直接延长 MTTR</details>
2. perf 工具的版本匹配要求是什么？
   <details><summary>答</summary>perf 需匹配 linux-tools-$(uname -r)，不匹配会导致事件不可用或数据错误；定制内核（RT 补丁、云厂商内核）可能仓库无对应包，需从内核源码 tools/perf 自编译</details>
3. HFT runbook 中危机工具应该怎么组织？
   <details><summary>答</summary>预设好第一反应命令（如 vmstat 1; mpstat -P ALL 1; ss -tiepm），复制粘贴即可跑；按 S0 常驻 → S1 60 秒清单 → S2 定向深挖 → S3 全量插桩分梯度，每条配常态基线值并定期演练</details>
4. 五个包里哪两个是「纯读者」、哪三个是「内核合作者」？区别是什么？
   <details><summary>答</summary>procps 和 sysstat 是纯读者（只读 /proc、/sys，几乎不会坏）；perf、BCC、bpftrace 是合作者（需要 perf_event 子系统、eBPF VM、BTF/headers 配合），版本错位就罢工，必须实际跑通最小命令验证</details>
5. 嵌入式板卡上为什么 BCC 经常不可用？替代方案是什么？
   <details><summary>答</summary>BCC 运行时编译需要目标机上的 clang 和当前内核 headers，交叉编译的嵌入式设备通常都没有；替代是 libbpf + CO-RE——开发机预编译 .o，目标机只放最小加载器，随固件部署</details>

</details>


---

← [本章导读](../README.md)
