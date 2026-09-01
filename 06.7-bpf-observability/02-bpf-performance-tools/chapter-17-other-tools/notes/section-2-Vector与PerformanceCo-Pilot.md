# 2. Vector 与 Performance Co-Pilot（PCP）（17.1）

> 底本：《BPF之巅》第 17 章，17.1 节（印刷 p738–747）

**Netflix Vector** 是一个开源的**主机级性能监控工具**，可近乎实时地把高精度系统和应用程序监控指标可视化。它是一个 Web 应用程序，利用经过实践检验的开源监视框架 **Performance Co-Pilot（PCP）**，UI 每隔一秒或更长时间轮询一次指标，在完全可配置的仪表盘中呈现数据，简化跨指标关联分析。

图 17-1 架构：本地浏览器中的 Vector 从网络服务器获取应用代码（HTTP GET），然后**直接连接目标主机的 PCP**（PMWEBD JSON 调用 → pmcd → BCC/BPF PMDA → BCC 程序 → BPF + Perf 缓冲区）运行 BPF 程序。

## Vector 的功能

- 高级仪表盘显示实例多种资源（CPU、磁盘、网络、内存）利用率
- **超过 2000 种指标**可用于深入分析；可通过修改 PMDA 配置增删指标
- 随时间可视化数据，粒度可精确到**秒**
- 同时比较不同指标和**不同主机**之间的数据（包括容器 vs 主机指标）

Vector 现已支持基于 BPF 的指标——通过添加**用 BCC 前端访问 BPF 的 PCP 代理**实现（BCC 见第 4 章）。

## 17.1.1–17.1.3 三种可视化形式

**折线图**：时间序列数据（CPU/Disk/Memory/Network 利用率，图 17-2）。

**热图**（17.1.2）：显示一段时间内的直方图，**非常适合摘要可视化每秒 BPF 延迟直方图**。三轴：

| 轴 | 含义 |
|---|---|
| X | 时间流逝（每列一秒或一个间隔） |
| Y | 延迟 |
| Z（颜色饱和度） | 落在该时间和延迟范围内的 I/O 数量 |

散点图在成千上万 I/O 时会互相覆盖丢细节；热图通过按需缩放颜色范围解决。Vector 热图对相关 BCC 工具可用：块设备 I/O 延迟 `biolatency(8)`、CPU 运行队列延迟 `runqlat(8)`、文件系统延迟 `ext4-/xfs-/zfs-dist`。图 17-3 为 fio 负载下每两秒采样的块延迟热图：大部分在 **256–511 微秒**，光标处工具提示显示该桶 **805 个样本**；对比命令行 biolatency 聚合输出（256-511: 12989、512-1023: 11425…），热图更易看出 **128–256ms 范围的 I/O 是长时间一致而非短暂飙升**。

**表格**（17.1.3）：`execsnoop(8)` 事件流（Tomcat catalina.sh 启动链：dirname → catalina.sh → setuidgid → ldconfig）、`tcplife(8)` TCP 会话（amazon-ssm-agent 轮询、wget 41.595 秒收 2GB）。表格适合可视化**事件细节**。

## 17.1.4 BCC 提供的指标

PCP PMDA 当前提供 **bcc-tools 软件包中大多数工具**。Vector 有预定义图表的 BCC 工具：

- `biolatency(8)` 和 `biotop(8)`
- `ext4dist(8)`、`xfsdist(8)`、`zfsdist(8)`
- `tcplife(8)`、`tcptop(8)`、`tcpretrans(8)`
- `runqlat(8)`
- `execsnoop(8)`

许多工具支持在主机上提供配置选项，还可将 BCC 工具添加到 Vector 用自定义图表/表格/热图可视化，并支持为**跟踪点、uprobes 和 USDT 事件**添加自定义事件指标。

## 17.1.5 内部实现

Vector 是**完全运行在用户浏览器中**的单页网页应用（React + D3.js 画图）。指标通过 PCP 收集输出（图 17-6）：

| 组件 | 角色 |
|---|---|
| **pmcd** | 性能指标收集守护进程，PCP 核心组件，运行在目标主机上，协调众多代理的指标收集 |
| **PMDA** | 性能指标域代理（PCP 托管代理术语），每个代理公开不同指标：procfs、Linux、xfs、kernel、NVIDIA CPU……要在 PCP 中用 BCC 指标必须安装 **BCC PMDA** |
| **pmwebd** | 到目标主机 pmcd 的 **REST 网关**，Vector 连接公开的 REST 端口与 pmcd 交互 |

典型 Linux PCP 安装默认提供 1000+ 指标，支持用自己的插件/PMDA 扩展。

**无状态模型**使其轻巧而强大：主机上的开销可忽略不计，因为**客户端（浏览器）负责跟踪状态、采样频率和计算**；指标不跨主机聚合、不在浏览器会话之外持久存储。

## 17.1.6 安装 PCP 和 Vector

本地试用即可；生产环境通常在不同主机上运行 Vector、PCP 代理与 PMDA。安装涉及 `pcp` 和 `pcp-webapi` 软件包，并从 Docker 容器运行 Vector 图形界面。确保 BCC PMDA 启用：

```bash
$ cd /var/lib/pcp/pmdas/bcc/
$ ./Install
[Wed Apr  3 20:54:06] pmdabcc(18942) Info: Enabled modules: [runqlat]
```

## 17.1.7 连接并显示数据

浏览器输入 `http://localhost/`（或 Vector 地址），对话框输入目标系统主机名（默认端口 44323）。连接图标变绿后，切换到 **Custom 选项卡**选择 runqlat——服务器上不可用的模块会变灰不可选。选中后显示**每秒更新的运行队列延迟热图**（图 17-9）。

## 17.1.8 配置 BCC PMDA

除非特别配置，许多 BCC PMDA 功能不可用。配置文件格式详见 `pmdabcc(1)` 手册。以启用 tcpretrans 模块为例：

```bash
$ cd /var/lib/pcp/pmdas/bcc
$ sudo vi bcc.conf

[pmda]
# List of enabled modules
modules = biolatency,sysfork,tcpperpid,runqlat,tcplife

[tcplife]
module = tcplife
cluster = 3
#process = java      # 按进程名/pid 正则过滤
#lport = 8443        # 本地端口过滤
#dport = 80,443      # 远程端口过滤
# 其他选项：session count（缓存已关闭 TCP 会话数，默认 64）、
#           buffer page count（perf 环形缓冲区页数，2 的幂，默认 20）
```

**每次更改 PMDA 配置都需要重新编译并重启**：`sudo ./Install`，然后刷新浏览器。

## 17.1.9–17.1.10 改进与阅读

Vector/PCP 与全套 BCC 工具的集成还有很多工作要做。Vector 多年来很好地为 Netflix 服务；Netflix 正在调查 **Grafana** 是否也能提供同样功能（见 17.2 节）。更多信息见原书链接 6。

## HFT 关联

- Vector 的**无状态模型**（观察才有开销、浏览器持状态）适合交易节点：不观察时零成本，出问题时不引入监控自身的扰动
- 热图形态学读法（双峰 vs 单峰、持续 vs 尖峰）应成为交易系统延迟治理的标准技能——runqlat 热图中出现的周期性亮带往往对应定时任务或批处理

<details>
<summary>自测题</summary>

1. Vector/PCP 三个核心组件 pmcd、PMDA、pmwebd 各自的角色是什么？
2. 热图的 X/Y/Z 三轴分别是什么？为什么比散点图适合海量 I/O？
3. PCP 无状态模型的含义是什么？为什么主机开销可忽略？
4. Vector 预定义图表覆盖哪些 BCC 工具？BCC PMDA 的配置文件如何启用新模块？改动后要做什么？

</details>

<details><summary>参考答案</summary>

1. **pmcd**：目标主机上的性能指标收集守护进程（PCP 核心），协调各代理；**PMDA**：PCP 托管的指标域代理，每个代理公开一类指标（procfs/xfs/kernel/……），BCC 指标必须装 **BCC PMDA**；**pmwebd**：pmcd 的 REST 网关，Vector 浏览器端通过它以 JSON(PMWEBD) 协议与 pmcd 交互。
2. X=时间（每列一个采样间隔）、Y=延迟、Z（颜色饱和度）=落在该 (时间, 延迟) 桶内的事件数。海量 I/O 时散点图互相覆盖丢细节；热图把密度压缩进颜色通道，且可按需缩放颜色范围——**密度聚合而不是逐点绘制**。
3. 服务器端不保存会话状态：**状态、采样频率、计算全在浏览器**；指标不跨主机聚合、不在会话外持久化。所以主机只被动应答轮询——不打开浏览器就零开销。
4. biolatency、biotop、ext4dist、xfsdist、zfsdist、tcplife、tcptop、tcpretrans、runqlat、execsnoop。编辑 `/var/lib/pcp/pmdas/bcc/bcc.conf` 的 `[pmda] modules = ...` 列表加入模块名（各模块 `[tcplife]` 等小节可配过滤参数）；改后必须**重新 `sudo ./Install` 并重启 PMDA**，然后刷新浏览器。

</details>
