#!/usr/bin/env python3
"""19-systems-performance: Add traps + folded quiz to all 🔴 chapter notes."""
import os, re

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD19 = BASE  # script is in 19-systems-performance/scripts/, parent is 19-systems-performance/

def make_block(traps, quiz):
    """Build the traps + quiz block to insert before navigation."""
    lines = []
    lines.append("\n### 常见陷阱\n")
    for i, t in enumerate(traps, 1):
        lines.append(f"{i}. {t}")
    lines.append("")
    lines.append("<details>")
    lines.append("<summary>自测题（点击展开）</summary>")
    lines.append("")
    for i, (q, a) in enumerate(quiz, 1):
        lines.append(f"{i}. {q}")
        lines.append(f"   <details><summary>答</summary>{a}</details>")
    lines.append("")
    lines.append("</details>")
    lines.append("")
    return "\n".join(lines)

def insert_before_nav(content, block):
    """Insert block before the ending --- navigation section."""
    nav_marker = "[本章导读](../README.md)"
    if nav_marker not in content:
        # Try alternate: just [本章导读]
        nav_marker = "[本章导读]"
    if nav_marker not in content:
        return None  # Can't find navigation
    nav_idx = content.find(nav_marker)
    before_nav = content[:nav_idx]
    # Find all lines that are just ---
    dash_positions = [m.start() for m in re.finditer(r'^---$', before_nav, re.MULTILINE)]
    if not dash_positions:
        # Insert right before nav
        return content[:nav_idx].rstrip() + "\n\n" + block + content[nav_idx:]
    # The ending block starts at the first --- in the trailing sequence
    # Find the earliest --- that has only whitespace between it and the next ---
    insert_pos = dash_positions[-1]
    if len(dash_positions) >= 2:
        between = before_nav[dash_positions[-2]:dash_positions[-1]].strip()
        if between == '':
            insert_pos = dash_positions[-2]
    # Also check for 3+ dashes
    if len(dash_positions) >= 3:
        between2 = before_nav[dash_positions[-3]:dash_positions[-2]].strip()
        if between2 == '':
            insert_pos = dash_positions[-3]
    return content[:insert_pos].rstrip() + "\n\n" + block + "\n\n" + content[insert_pos:]

# Define content for each note: (rel_path, [traps], [(q, a), ...])
NOTES = [
    # === Ch1 Introduction (7 notes) ===
    ("chapter-01-intro/notes/section-1.1-1.3-系统性能角色与活动.md",
     ["把延迟和吞吐混为一谈——HFT 行情机吞吐够高但 P99 尖刺照样致命，两者必须分开测分开报",
      "上线后才定性能目标——SLO 应在架构设计阶段就定义（如 tick-to-trade P99 < X us），不是出事再补",
      "只盯单点 CPU 跑分——全链路 Checklist（DMA→协议栈→解码→策略→发单）任何一段都可能成为瓶颈，单点跑分不能代替端到端"],
     [("系统性能研究的对象是什么？和单进程性能分析有何区别？",
       "整个计算机系统在数据路径上的全部主要软硬件组件，而非单一进程或单块网卡"),
      ("HFT 小团队在性能角色上的特点是什么？",
       "常一人兼多角——既要懂策略（workload），又要懂绑核/网络/内核（resource）"),
      ("性能生命周期中哪两项活动对 HFT tail latency 最关键？",
       "上线前的延迟/抖动 SLO 设定 + 生产事故回顾中的 P99 尖刺排查")]),

    ("chapter-01-intro/notes/section-1.4-热路径Resource与双视角.md",
     ["只看 resource 不看 workload——CPU 高但不知道在算什么，等于白看；workload 分析先于 resource 分析",
      "workload 和 resource 视角二选一——Gregg 强调互补而非互斥，排查用 workload 定位、容量规划用 resource 量化",
      "热路径定义模糊——必须明确哪些函数/线程在 tick 关键路径上，否则优化方向跑偏"],
     [("workload 分析和 resource 分析分别问什么问题？",
       "workload 问「程序在干什么」，resource 问「哪段在等/慢」"),
      ("HFT 热路径包括哪些段？",
       "收包→进用户态→解码→策略→风控→发单，每段都有 workload 和 resource 两个维度"),
      ("为什么不能只看 resource 利用率？",       "利用率高不代表在干有用的事——可能是 spin 忙等或内核锁开销，需要 workload 视角确认在做什么")]),

    ("chapter-01-intro/notes/section-1.5-排障案例与性能挑战.md",
     ["跳过基线直接排查——没有 baseline 就无法判断什么是「异常」，每次出事都从头摸",
      "改一处不验证全局——修了 CPU 但引入了延迟抖动，性能优化必须前后对比完整指标集",
      "用平均延迟代替分位数——HFT 场景 P99/P999 尖刺才是致命的，mean 可能完全正常"],
     [("性能排障的第一步应该做什么？",
       "建立基线（baseline）——知道系统正常时的指标范围，才能判断异常"),
      ("HFT 场景为什么 mean 延迟不够用？",
       "P99/P999 尖刺导致错单或漏单，mean 可能完全正常但 tail 已爆"),
      ("Gregg 的性能挑战核心方法论是什么？",
       "科学方法——假设→实验→验证，避免盲目改参数（随机调优反模式）")]),

    ("chapter-01-intro/notes/section-1.6-延迟指标与读法.md",
     ["只看平均值——HFT 必须看 P99/P999/最大值，分布的尾部才是交易风险",
      "延迟分解不彻底——只说「慢」不分解到 DNS/TCP/应用/内核各段，无法定位",
      "混用不同测量点的延迟——TTFB ≠ RTT ≠ 端到端，口径不一致导致对比无意义"],
     [("TTFB 和 RTT 的区别是什么？",
       "TTFB = 请求→首字节（含服务端 think time），RTT = 网络往返时间"),
      ("HFT 延迟分解应该覆盖哪些段？",
       "signal→encode→send()→内核 TCP→NIC→wire RTT→交易所 ACK，每段独立测量"),
      ("为什么 P99 比 mean 更重要？",
       "HFT 中一次 P99 尖刺就可能导致错价或超限，mean 被大量正常样本稀释")]),

    ("chapter-01-intro/notes/section-1.7-观测工具四层递进.md",
     ["一上来就用最重工具——应按计数器→采样→追踪→剖析四层递进，先用轻量工具定方向",
      "生产环境不限时长跑追踪——高频率 tracepoint/kprobe 会产生 CPU 负载，必须限时限量",
      "忽略工具自身的开销——perf record、BPF trace 本身消耗 CPU/内存，测量结果可能被污染"],
     [("观测工具的四层递进是什么？",
       "计数器（/proc、stat）→ 采样（perf record）→ 追踪（tracepoint/kprobe）→ 剖析（火焰图）"),
      ("为什么生产环境要先轻后重？",
       "轻量工具开销低可长时间跑，重工具（trace、抓包）开销大必须限时，避免影响生产"),
      ("HFT 中工具开销对测量结果有什么影响？",
       "BPF/perf 本身消耗 CPU，可能改变调度行为和 cache 状态，导致测到的延迟不是真实延迟")]),

    ("chapter-01-intro/notes/section-1.8-实验与微观宏观基准.md",
     ["微基准结果外推到生产——微观 bench 无并发/无干扰，生产有调度抖动/NUMA/网络争用，直接外推必翻车",
      "宏基准不控制变量——同时改了绑核+分配器+编译选项，无法归因哪个改了起作用",
      "压测时间太短——HFT 的 tail latency 需要足够长的采样才能暴露 P99/P999 尖刺"],
     [("微基准和宏基准分别适合什么场景？",
       "微基准适合隔离测试单个函数/组件性能，宏基准适合端到端系统级验证"),
      ("为什么微基准结果不能直接外推到生产？",
       "微基准无并发/无调度抖动/无 NUMA 影响，生产环境有这些干扰因素"),
      ("HFT 压测应该持续多久才能暴露 tail latency？",
       "至少跑足够长时间覆盖 P99/P999 分位数——短压测只能看 mean，看不到尾部尖刺")]),

    ("chapter-01-intro/notes/section-1.9-1.11-云计算方法论与案例.md",
     ["云环境指标直接套裸机——云有 steal time、邻居噪声、虚拟化开销，裸机 checklist 不能直接用",
      "忽略虚拟化层开销——HFT 共置裸机 vs 云实例延迟差数量级，不可混用 SLO",
      "云上监控盲区——hypervisor 层事件（迁移、限流）对 guest 不可见，需要宿主机级监控"],
     [("HFT 为什么通常选择裸机共置而非云实例？",
       "云有 steal time、虚拟化开销、邻居噪声，HFT 需要可预测的微秒级延迟"),
      ("云环境性能分析有哪些额外挑战？",
       "steal time、hypervisor 迁移、cgroup 限流、邻居噪声——guest 内不完全可见"),
      ("云上 Noisy Neighbor 问题如何检测？",
       "看 steal time（mpstat）、PSI、cgroup throttle 事件——非零说明被宿主机或其他租户挤")]),

    # === Ch2 Methodologies (15 notes) ===
    ("chapter-02-methodologies/notes/section-2.1-HFT术语与团队对齐.md",
     ["术语不对齐——开发说「延迟」指 P50、运维说「延迟」指 mean、交易员说「延迟」指最大值，必须统一口径",
      "IOPS 和吞吐混淆——小块高 IOPS ≠ 大块高吞吐，HFT 行情包小但pps高，和存储吞吐是两回事",
      "Latency 和 Response Time 不区分——Gregg 定义 latency = 服务时间，response time = latency + queue time"],
     [("Latency 和 Response Time 的区别是什么？",
       "Latency = 服务时间（处理本身），Response Time = latency + 等待队列时间"),
      ("HFT 团队为什么需要对齐术语？",
       "不同角色（开发/运维/交易员）对「延迟」「吞吐」「IOPS」理解不同，不对齐会导致沟通失误"),
      ("Utilization 高是否一定意味着瓶颈？",
       "不一定——利用率高但饱和度低（run queue 短）说明 CPU 在干活但没排队；饱和度高才是瓶颈信号")]),

    ("chapter-02-methodologies/notes/section-2.2-术语与命令速查.md",
     ["命令记不住就用复杂工具——先掌握 vmstat/mpstat/ss/iperf3 五个基础命令，再上 BPF",
      "命令参数不记——vmstat 的 r 列、si/so 列含义不同，混淆会导致误判",
      "只看一条命令的输出——vmstat + mpstat + iostat 三条合在一起看才能定位是 CPU/内存/IO 哪段"],
     [("vmstat 输出中 r 列和 b 列分别表示什么？",
       "r = 运行队列长度（等 CPU 的线程数），b = 不可中断睡眠（通常等 I/O）的线程数"),
      ("HFT 排障第一反应应该跑哪几条命令？",
       "vmstat 1（全局）、mpstat -P ALL 1（每核）、ss -tiepm（网络）、iostat -x 1（IO）"),
      ("为什么 uptime 的 load average 不能代替 per-CPU 分析？",
       "load average 是指数移动平均且混合了 D 态线程，8 核 load=8 不等于 100%，要看 mpstat 每核")]),

    ("chapter-02-methodologies/notes/section-2.3.1-时间尺度与排查走查.md",
     ["跳档优化——纳秒级问题用毫秒级工具查（如用 top 查微秒级抖动），工具精度不够根本看不到",
      "时间尺度不匹配——HFT tick-to-trade 在微秒级，但 sar 默认 10 分钟粒度，完全看不到尖刺",
      "忽略时钟源——TSC vs CLOCK_MONOTONIC vs gettimeofday 精度不同，混用导致测量不准"],
     [("HFT 延迟排查为什么要按时间尺度分层？",
       "不同延迟量级需要不同精度的工具——纳秒级用 PMC/rdtsc，微秒级用 BPF，毫秒级用 vmstat"),
      ("为什么 sar 默认粒度对 HFT 没用？",
       "sar 默认 10 分钟平均，微秒级尖刺被完全平均掉，需要 sar -I 1 或更高频工具"),
      ("HFT 测量应该用哪个时钟源？",
       "CLOCK_MONOTONIC 或 TSC（rdtsc），避免 gettimeofday 的闰秒/调整问题")]),

    ("chapter-02-methodologies/notes/section-2.3.2-性能权衡.md",
     ["延迟和吞吐同时优化——加 batch 提吞吐但增单笔延迟，HFT 通常牺牲吞吐保延迟",
      "只优化不量化 trade-off——改了 Nagle/TCP_CORK 但没测延迟变化，无法判断改对了没有",
      "忽略二级效应——绑核降调度延迟但增单核热点，cache miss 可能反而升高"],
     [("HFT 中延迟和吞吐的 trade-off 体现在哪里？",
       "batch 合并提吞吐但增单笔等待延迟，HFT 通常选低延迟（小包直发）牺牲吞吐"),
      ("为什么性能优化必须量化 trade-off？",
       "没有量化就不知道改动是否真的改善——改了 CPU 但引入延迟抖动，需要前后对比完整指标"),
      ("绑核的二级效应是什么？",
       "绑核降调度延迟但所有负载集中到一个核，可能增 cache miss 和热点争用")]),

    ("chapter-02-methodologies/notes/section-2.3.3-负载与架构.md",
     ["把负载问题当架构问题——加机器解决不了单线程瓶颈，先确认是负载饱和还是架构限制",
      "已知未知不区分——已知问题（可预测的瓶颈）和未知问题（随机故障）排查路径完全不同",
      "架构调优不改负载——先减不必要的工作（负载优化）再改架构，顺序反了浪费资源"],
     [("负载分析和架构分析的区别是什么？",
       "负载分析看「在干多少活」，架构分析看「能干多少活」——先确认负载合理再改架构"),
      ("已知未知在性能排查中如何区分？",
       "已知 = 可预测的瓶颈（容量不够），未知 = 随机故障（尖刺/抖动）——排查路径不同"),
      ("为什么先优化负载再改架构？",
       "消除不必要的工作（负载优化）ROI 最高，架构改动成本大且可能引入新问题")]),

    ("chapter-02-methodologies/notes/section-2.4-两种分析视角.md",
     ["只自底向上（资源视角）——从 CPU/内存往上推，容易陷入无目标的数据收集，HFT 应先自顶向下定位段",
      "只自顶向下（工作负载视角）——从应用往下推，可能漏掉内核/硬件层的隐藏开销",
      "两种视角不同时用——Gregg 强调互补：自顶向下定方向、自底向上钻根因"],
     [("自顶向下和自底向上分析的区别？",
       "自顶向下从应用/工作负载开始往下追，自底向上从 CPU/内存等资源开始往上推"),
      ("HFT 排障应该先用哪种视角？",
       "先自顶向下（workload）定位是哪段慢，再自底向上（resource）钻根因"),
      ("为什么不能只用一种视角？",       "只自顶向下可能漏掉内核/硬件隐藏开销，只自底向上容易无目标地收集数据")]),

    ("chapter-02-methodologies/notes/section-2.5-性能分析方法论.md",
     ["用反模式排查——Blame-Someone-Else（甩锅给别人）、Random-Change-Tuning（随机改参数）是反面教材",
      "USE 方法只套 CPU——内存、网络、IO 每个资源都要单独跑 USE，不能只查 CPU 就下结论",
      "不给容器/cgroup 单独跑 USE——容器内看到的资源是 cgroup 限额后的，和宿主机不同"],
     [("USE 方法的三个字母分别代表什么？",
       "U = Utilization（利用率），S = Saturation（饱和度），E = Errors（错误）"),
      ("常见的性能反模式有哪些？",
       "Blame-Someone-Else（甩锅）、Random-Change-Tuning（随机调参）、我们加个缓存吧（盲目优化）"),
      ("RED 方法适用于什么场景？",
       "微服务/网络服务——Rate（请求率）、Errors（错误率）、Duration（延迟分布）")]),

    ("chapter-02-methodologies/notes/section-2.6.1-排队论概览与Kendall记号.md",
     ["忽略排队论直接加机器——不知道拐点在哪，可能在 70% 利用率就已经 tail latency 爆了",
      "M/M/1 套所有场景——服务时间确定型用 M/D/1，多服务台用 M/M/c，模型选错结论全错",
      "只看利用率不看排队长度——利用率 80% 但 queue 已经很长，延迟已经在指数上升"],
     [("Kendall 记号 A/S/c 的三个位置分别代表什么？",
       "A = 到达过程（M=泊松），S = 服务时间分布（M=指数/D=确定），c = 服务台数量"),
      ("为什么 HFT 要关心排队论？",
       "单线程网关在利用率 70%+ 时延迟指数上升（M/M/1 拐点），排队论提前算拐点设限流"),
      ("M/M/1 和 M/D/1 的区别是什么？",
       "M/M/1 服务时间随机（指数分布），M/D/1 服务时间确定——同利用率下 M/M/1 排队更长")]),

    ("chapter-02-methodologies/notes/section-2.6.2-M-M-1-拐点与预警线.md",
     ["M/M/1 在 70% 利用率才报警——实际上 70% 时 queue 延迟已经约为服务时间的 2.3 倍，HFT 应更低",
      "用线性外推排队延迟——M/M/1 延迟是 1/(1-ρ) 的非线性曲线，80%→90% 延迟翻倍不是线性",
      "忽略服务时间方差——实际服务时间不是完美指数分布，方差更大时拐点更早"],
     [("M/M/1 模型中利用率和排队延迟的关系是什么？",
       "延迟 = 服务时间 / (1 - ρ)，非线性——ρ=0.5 延迟翻倍，ρ=0.9 延迟 10 倍"),
      ("HFT 中 M/M/1 的 70% 预警线意味着什么？",
       "利用率 70% 时平均排队延迟约为服务时间 2.3 倍——对微秒级系统已经不可接受"),
      ("为什么不能线性外推排队延迟？",
       "1/(1-ρ) 是非线性曲线，从 80% 到 90% 延迟翻倍，从 90% 到 95% 再翻倍")]),

    ("chapter-02-methodologies/notes/section-2.6.3-M-D-1-M-M-c-M-G-1.md",
     ["M/M/c 当 c 个服务台独立——实际上多核间有共享资源（cache/内存控制器），不是完美独立",
      "M/D/1 延迟和 M/M/1 一样——确定型服务时间排队更短（方差为 0），M/D/1 延迟约为 M/M/1 的一半",
      "M/G/1 不验服务时间分布——通用模型需要知道方差，不测实际分布直接套结论会错"],
     [("M/M/c 和 M/M/1 的关键区别？",
       "M/M/c 有 c 个并行服务台，利用率 ρ = λ/(cμ)，多服务台下同总利用率排队更短"),
      ("M/D/1 比 M/M/1 排队延迟短多少？",
       "约为 M/M/1 的一半——服务时间方差为 0，没有长尾服务拖累队列"),
      ("HFT 什么场景适合 M/D/1 模型？",
       "固定处理时间的场景——如解码固定大小行情包，每包处理时间几乎相同")]),

    ("chapter-02-methodologies/notes/section-2.6.4-排队论计算器.md",
     ["计算器输入不校验——λ 和 μ 单位不一致（每秒 vs 每毫秒），算出来的数字差 1000 倍",
      "只算平均延迟——计算器还能输出 P90/P99，HFT 必须看分位数不看平均",
      "把模型当真理——排队论是近似模型，实际系统有 cache/调度/锁等非线性因素，模型用于估算拐点而非精确预测"],
     [("排队论计算器的输入参数有哪些？",
       "到达率 λ、服务率 μ（或服务时间 1/μ）、服务台数 c——输出利用率、排队长度、延迟"),
      ("为什么计算器结果不能当精确预测？",
       "模型假设独立到达/服务，实际有 cache 局部性、调度抖动、锁竞争等非线性因素"),
      ("HFT 用排队论计算器的主要目的是什么？",
       "估算拐点（利用率多少时延迟指数上升）和容量规划，而非精确预测延迟值")]),

    ("chapter-02-methodologies/notes/section-2.7.1-阿姆达尔与USL.md",
     ["阿姆达尔定律忽略串行部分——HFT 策略有全局锁/共享 order book，串行比比想象的大",
      "USL 的 β（ contention）和 γ（coherency）不区分——β 是资源争用，γ 是通信开销，优化方向不同",
      "加核到 USL 拐点之后——超过拐点加核反而降吞吐（负扩展性），必须先算拐点再加机器"],
     [("阿姆达尔定律的核心公式和含义？",
       "加速比 = 1 / (s + (1-s)/n)，s = 串行比例，n = 核数——串行部分限制最大加速比"),
      ("USL 比阿姆达尔多了什么？",
       "多了 γ（coherency）项——通信/一致性开销随核数平方增长，导致负扩展性"),
      ("HFT 策略加核什么时候开始降速？",
       "当 γ * n^2 项超过并行收益时——USL 拐点，需要实测确定而非猜")]),

    ("chapter-02-methodologies/notes/section-2.7.2-容量规划三步法.md",
     ["容量规划只用平均负载——HFT 要用 P99/P999 负载做规划，mean 不能代表尖峰",
      "不考虑故障降级——容量规划只算正常场景，故障切换/降级时的突增流量没算进去",
      "线性扩容假设——加机器不线性（USL），2 倍机器不等于 2 倍容量"],
     [("容量规划三步法是哪三步？",
       "1) 从业务目标反推资源需求 2) 用排队论/USL 估算拐点 3) 压测验证"),
      ("HFT 容量规划为什么不能用平均负载？",
       "P99/P999 尖峰时的负载可能远超 mean，按 mean 规划容量在尖峰时会爆"),
      ("为什么加机器不等于线性扩容？",
       "USL 的 γ 项——通信/一致性开销随节点数增长，超过拐点后加机器反而降性能")]),

    ("chapter-02-methodologies/notes/section-2.8.1-统计陷阱.md",
     ["只报 mean 不报分位数——HFT 的 P99 可能是 mean 的 10 倍，mean 正常不代表没问题",
      "σ 大但不查原因——标准差大说明有抖动，可能是调度/锁/cache 问题，不是「正常波动」",
      "采样率不够——每秒采一个点可能完全错过微秒级尖刺，HFT 需要高频采样或事件驱动"],
     [("HFT 为什么不能用 mean 代替 P99？",
       "一次 P99 尖刺就可能导致错单，mean 被大量正常样本稀释，看不出问题"),
      ("标准差大说明什么？应该怎么处理？",
       "说明有抖动/不稳定——查调度延迟、锁竞争、cache miss、NUMA 迁移等根因"),
      ("采样监控为什么可能漏掉 HFT 尖刺？",
       "低频采样（如每 10 秒采一次）完全错过微秒级事件，需要事件驱动或高频 BPF 追踪")]),

    ("chapter-02-methodologies/notes/section-2.8.2-五种图与监控栈.md",
     ["监控只看折线图——延迟分布需要直方图（histogram），折线图看不到分布形状和尾部",
      "告警阈值用绝对值——应该用相对基线（如 P99 > baseline * 1.5），绝对阈值不能适应负载变化",
      "不存历史数据——出事才想看历史趋势，Prometheus 默认 15 天可能不够，需要长期存储"],
     [("五种性能图分别是什么？",
       "折线图（趋势）、散点图（相关性）、直方图（分布）、热力图（亚秒级）、火焰图（栈热点）"),
      ("为什么延迟监控需要直方图而不是折线图？",
       "直方图显示完整分布形状和尾部，折线图只显示某个聚合值（如 mean），看不到 P99 尖刺"),
      ("HFT 监控告警应该用什么阈值策略？",
       "相对基线——如 P99 超过正常值 1.5 倍告警，而非绝对值（绝对值不适应负载变化）")]),

    # === Ch4 Observability Tools (6 notes) ===
    ("chapter-04-observability-tools/notes/section-4.1-工具覆盖范围与危机工具.md",
     ["危机时才装工具——出事时才发现 perf/BPF 没装或版本不匹配，应该预装并验证",
      "危机工具不熟——出事时现学 man page 太慢，runbook 应预设好第一反应命令",
      "工具和内核版本不匹配——perf 需匹配 linux-tools-$(uname -r)，BCC 需匹配内核 headers"],
     [("什么是「危机工具包」？为什么需要预装？",
       "出事时第一时间需要的工具（vmstat/mpstat/perf/ss/iostat）——出事时才装可能网络不通或版本不对"),
      ("perf 工具的版本匹配要求是什么？",
       "perf 需匹配 linux-tools-$(uname -r)，不匹配会导致事件不可用或数据错误"),
      ("HFT runbook 中危机工具应该怎么组织？",
       "预设好第一反应命令（如 vmstat 1; mpstat -P ALL 1; ss -tiepm），复制粘贴即可跑")]),

    ("chapter-04-observability-tools/notes/section-4.2-工具的分类与原理.md",
     ["混淆计数器和采样——计数器是精确总计（低开销），采样是定时快照（有统计误差）",
      "追踪当采样用——追踪记录每个事件（高开销），不是采样子集，高频事件会打爆",
      "不区分系统级和进程级工具——mpstat 看全局但看不到具体进程，pidstat 才能定位到 TID"],
     [("观测工具按原理分哪几类？",
       "计数器（/proc/stat）、采样（perf record）、追踪（strace/ftrace）、剖析（火焰图）"),
      ("计数器和采样的根本区别？",
       "计数器统计事件总次数（精确、低开销），采样定时取快照（有统计误差但开销可控）"),
      ("为什么高频事件不适合逐条追踪？",
       "每个事件都记录会产生大量 CPU/IO 开销，应该用 BPF map 聚合（直方图/计数）")]),

    ("chapter-04-observability-tools/notes/section-4.3-核心观测数据源.md",
     ["/proc 不等于实时——/proc 是快照，两次读之间的事件看不到，高频场景需要 tracepoint",
      "PMC 不同 CPU 代际不同——L1-misses 事件名在 Intel vs AMD 不同，复制事件名可能无效",
      "忽略 tracepoint 稳定性——tracepoint 是稳定 ABI，kprobe 不是，内核升级 kprobe 可能断"],
     [("/proc 文件系统的局限性是什么？",
       "是快照不是实时流——两次读之间的事件看不到，且格式可能随内核版本变化"),
      ("PMC 的跨平台问题是什么？",
       "不同 CPU 代际（Intel vs AMD）事件名不同，复制事件名可能无效或含义不同"),
      ("tracepoint 和 kprobe 的稳定性区别？",
       "tracepoint 是稳定 ABI 不会变，kprobe 追踪内核函数名可能随版本变更而失效")]),

    ("chapter-04-observability-tools/notes/section-4.4-sar-工具.md",
     ["sar 只看历史不告警——sar 记录历史但不主动告警，需要配合 Prometheus/Grafana 做实时告警",
      "sar 默认粒度太粗——10 分钟平均看不到 HFT 微秒级尖刺，需要 sar -I 1 或更高频",
      "不存 sar 历史数据——出事才想看昨天的趋势，但 sadc 没配或被清理了"],
     [("sar 的主要用途是什么？",
       "历史性能数据回溯——记录 CPU/内存/IO/网络等指标，事后分析趋势"),
      ("sar 对 HFT 的局限性是什么？",
       "默认 10 分钟粒度太粗，看不到微秒级尖刺；且只记录不告警"),
      ("如何让 sar 数据对 HFT 有用？",
       "调高采样频率（sar -I 1）+ 长期存储 + 配合实时监控（Prometheus）")]),

    ("chapter-04-observability-tools/notes/section-4.5-四大追踪器.md",
     ["strace 生产直接跑——strace 开销巨大（每个 syscall 两次 ptrace），生产禁用或限时",
      "perf trace 当 strace 用——perf trace 开销比 strace 低但仍有开销，生产限时长",
      "ftrace 和 BPF 不分场景——ftrace 适合内核内建追踪，BPF 适合可编程聚合"],
     [("四大追踪器分别是什么？",
       "strace（syscall）、perf trace（低开销 strace）、ftrace（内核追踪）、BPF（可编程追踪）"),
      ("为什么 strace 不能在生产环境用？",
       "每个 syscall 两次 ptrace 陷入，开销巨大——HFT 热路径会变成原来的 10-100 倍慢"),
      ("ftrace 和 BPF 各自适合什么场景？",
       "ftrace 适合内核内建 tracepoint/函数追踪，BPF 适合可编程聚合（直方图/过滤/计算）")]),

    ("chapter-04-observability-tools/notes/section-4.6-观测的观测Observing-Observability.md",
     ["观测工具不测开销——BPF/perf 本身消耗 CPU，可能改变系统行为，导致观测结果不准",
      "观测盲区不自知——以为看到了全部，实际 /proc 和采样都有盲区，短事件/极短进程可能漏",
      "不记录观测条件——内核版本/工具版本/采样率不记录，复现时条件不一致"],
     [("「观测的观测」是什么意思？",
       "观测工具本身也消耗资源，需要测量工具自身开销——否则测到的数据被工具污染"),
      ("观测工具有哪些盲区？",
       "采样漏短事件、/proc 是快照非实时、top 采不到极短进程、计数器不记录事件序列"),
      ("为什么观测结果需要记录环境条件？",
       "内核版本/工具版本/采样率不同结果不同——不记录则无法复现和对比")]),

    # === Ch5 Applications (6 notes) ===
    ("chapter-05-applications/notes/section-5.1-应用程序基础.md",
     ["应用层不剖析直接调内核——HFT 延迟 80% 在应用层（解码/策略），不先 profile 应用就调内核是本末倒置",
      "CPI 高只查 CPU——CPI 高可能是 cache miss（查数据结构）、分支预测失败（查 if/switch），不只是 CPU 算力",
      "锁粒度不分层——全局锁 vs 分区锁 vs 无锁，不同竞争强度选不同方案，全局锁在高频路径是杀手"],
     [("HFT 延迟优化的优先级是什么？",
       "先应用层（消除不必要工作、数据结构优化）→ 再编译优化 → 再调度/绑核 → 最后内核/硬件"),
      ("CPI 高的可能原因有哪些？",
       "cache miss（数据布局差）、分支预测失败（不可预测 if）、TLB miss（大页）、依赖链（ILP 不足）"),
      ("HFT 热路径锁优化方向？",
       "全局锁 → 分区锁 → lock-free（CAS/RCU）→ 无共享（per-thread 数据）")]),

    ("chapter-05-applications/notes/section-5.2-应用程序性能提升技术.md",
     ["过度优化——没 profile 就手动展开循环/内联汇编，编译器已经做得更好且更可维护",
      "伪共享不查——多线程写同一 cache line 的不同字段，cache 一致性协议打穿性能",
      "数据结构不量化——换 hashmap vs red-black tree 不测实际延迟，凭直觉选可能更慢"],
     [("HFT 应用层优化的最高 ROI 是什么？",
       "消除不必要的工作——去掉冗余拷贝/分支/日志，比优化已有代码更有效"),
      ("伪共享如何检测和修复？",
       "perf c2c 或查 cache line 对齐——修复用 alignas(64) 填充使各线程数据不在同一 cache line"),
      ("为什么换数据结构前必须 benchmark？",
       "hashmap 查找 O(1) 但 cache miss 可能比红黑树 O(log n) 更慢——实际延迟取决于 cache 行为不是理论复杂度")]),

    ("chapter-05-applications/notes/section-5.3-编程语言与垃圾回收.md",
     ["GC 语言做 HFT 热路径——Java/Go 的 GC pause 即使是 ms 级也远超 HFT 预算，热路径应用 C++",
      "C++ 不管内存分配——默认 malloc 有锁竞争和碎片，HFT 应用对象池/arena/pre-allocated",
      "不测 GC/分配器实际暂停——声称「无 GC」但不测，实际有 major page fault 或 malloc stall"],
     [("HFT 热路径为什么通常用 C++ 而非 Java/Go？",
       "GC pause 即使 ms 级也远超 HFT 微秒级预算——C++ 手动内存管理可预分配/池化，无暂停"),
      ("C++ 热路径内存管理应该怎么做？",
       "对象池/arena/pre-allocated——热路径零 malloc，避免分配器锁和 page fault"),
      ("如何验证热路径真的「无暂停」？",
       "BPF 追踪 malloc/page-fault/sched 时长——看热路径线程是否有非预期事件")]),

    ("chapter-05-applications/notes/section-5.4-性能分析方法论.md",
     ["应用 profiling 只看 on-CPU——off-CPU 时间（等锁/等 IO/等调度）可能才是延迟大头",
      "火焰图只看宽度不看深度——深度（调用链长）也是问题，每层都有开销",
      "不保留 profile 基线——优化前后不对比 perf stat，无法证明改有效"],
     [("on-CPU 和 off-CPU 剖析的区别？",
       "on-CPU 看在 CPU 上执行的时间花在哪，off-CPU 看不在 CPU 上的时间花在哪（等锁/IO/调度）"),
      ("HFT 为什么需要 on-CPU + off-CPU 双火焰图？",
       "延迟 = on-CPU 时间 + off-CPU 时间——只看 on-CPU 漏掉锁等待/调度延迟等大头"),
      ("火焰图深度和宽度分别代表什么？",
       "宽度 = 函数占样本比例（热点），深度 = 调用链层数——深调用链每层都有开销")]),

    ("chapter-05-applications/notes/section-5.5-观测工具.md",
     ["pidstat 不指定 TID——多线程程序只看进程级数据，热路径线程被其他线程平均掉",
      "perf record 不加 -g——不加 -g 只采到当前函数采不到调用栈，无法做火焰图",
      "uprobe 生产直接挂——uprobe 有开销（每次调用陷入），热路径高频函数可能显著增延迟"],
     [("pidstat 查看 HFT 多线程程序要注意什么？",
       "用 -t 指定 TID——进程级数据会把热路径线程和 housekeeping 线程平均掉"),
      ("perf record 为什么要加 -g？",
       "-g 采集调用栈——不加只能看到当前函数，无法做火焰图定位调用链热点"),
      ("uprobe 在生产环境的注意事项？",
       "有每次调用的开销——热路径高频函数会显著增延迟，应限时长或用 USDT 替代")]),

    ("chapter-05-applications/notes/section-5.6-常见陷阱Gotchas.md",
     ["编译去掉帧指针——-O2 默认 -fomit-frame-pointer，perf 栈回溯全是 [unknown]",
      "strip 掉符号表——生产二进制 strip 后 perf report 看不到函数名，应保留 debuginfo",
      "inline 过度——-O3 激进 inline 导致函数太大影响 I-cache，且火焰图栈变浅看不清层次"],
     [("为什么 HFT Release 构建要保留帧指针？",
       "perf 栈回溯需要帧指针（-fno-omit-frame-pointer）——去掉后栈全是 [unknown] 无法分析"),
      ("strip 符号表有什么后果？",
       "perf report 看不到函数名——应保留 debuginfo 包或不在生产二进制上 strip"),
      ("-O3 aggressive inline 有什么副作用？",
       "函数过大影响 I-cache（icache miss），且火焰图栈变浅看不清调用层次")]),

    # === Ch6 CPUs (5 notes) ===
    ("chapter-06-cpus/notes/section-6.1-6.3-CPU-模型与核心概念.md",
     ["SMT 假装是两个核——同核两个硬件线程争 L1/执行单元，HFT 热路径避免 SMT 共享",
      "利用率 100% = 瓶颈——run queue 长度才是饱和度，利用率高但 run queue 空 = 正常在干活",
      "IPC 低只查 CPU——IPC 低可能是 cache miss（查数据结构布局）或 TLB miss（查大页），不是 CPU 不够快"],
     [("SMT 对 HFT 热路径有什么影响？",
       "同核两个硬件线程争 L1 cache 和执行单元——热路径应绑独占物理核，避免 SMT 共享"),
      ("Utilization 100% 是否一定是瓶颈？",
       "不一定——如果 run queue 为空说明只是当前线程在忙，不是瓶颈；run queue 持续 > 0 才是饱和"),
      ("IPC 低应该查什么？",
       "先查 cache-misses（数据布局/NUMA）、再查 branch-misses（分支预测）、再查 TLB miss（大页）")]),

    ("chapter-06-cpus/notes/section-6.4-硬件与软件架构.md",
     ["C-State 不限制——深 C-State 唤醒延迟达微秒级，HFT 裸机应限制 C-State 到 C0/C1",
      "NUMA balancing 不关——内核自动迁内存到「正确」节点，但迁移过程引入延迟尖刺",
      "isolcpus 不配 IRQ 亲和——核隔离了但中断还往这核送，隔离白做"],
     [("HFT 裸机为什么要限制 C-State？",
       "深 C-State（C6）唤醒延迟达微秒级——从睡眠到执行的转换时间不可预测，应限制到 C0/C1"),
      ("NUMA balancing 对 HFT 的风险？",
       "内核自动把内存页迁移到「正确」NUMA 节点——迁移过程产生 page fault 和延迟尖刺"),
      ("isolcpus 隔离后还需要做什么？",
       "配 IRQ 亲和性把中断送到 housekeeping 核——否则中断仍会打断隔离核上的热路径线程")]),

    ("chapter-06-cpus/notes/section-6.5-性能分析方法论.md",
     ["USE 只查 Utilization——Saturation（run queue/调度延迟）才是 HFT 的关键指标",
      "profiling 不加帧指针——perf record -g 需要帧指针，编译去掉后栈全是 [unknown]",
      "只看全局 CPU——HFT 热路径在特定核上，全局平均正常但热核可能已饱和"],
     [("CPU 的 USE 方法中 HFT 最该关注哪个字母？",
       "Saturation——run queue 长度和调度延迟，HFT 延迟尖刺多因调度等待而非 CPU 不够"),
      ("perf record 剖析的前置条件是什么？",
       "编译保留帧指针（-fno-omit-frame-pointer）——否则栈回溯全是 [unknown]"),
      ("为什么 HFT 要看 per-CPU 而不是全局 CPU？",
       "热路径绑在特定核上——全局平均可能正常，但热核已 100% + run queue 堆积")]),

    ("chapter-06-cpus/notes/section-6.6-6.7-观测工具与可视化.md",
     ["load average 当 CPU 利用率——load 包含 D 态线程（等 IO），不等于 CPU 忙闲",
      "火焰图只看最宽的塔——最宽不一定是瓶颈，可能是正常主循环，要对比基线",
      "FlameScope 不用于 HFT——亚秒级热力图正好适合找 HFT P99 尖刺的时间窗口"],
     [("load average 为什么不等于 CPU 利用率？",
       "load = 可运行 + D 态（等 IO）线程的指数平均——8 核 load=8 可能是 IO 瓶颈不是 CPU 满"),
      ("火焰图最宽的塔一定是瓶颈吗？",
       "不一定——可能是正常主循环（如 epoll_wait），要和基线对比看是否异常变宽"),
      ("FlameScope 对 HFT 有什么用？",
       "亚秒级偏移热力图——在大样本里找周期性尖刺/抖动的时间窗口，再对齐到火焰图分析")]),

    ("chapter-06-cpus/notes/section-6.9-CPU-调优.md",
     ["调优从绑核开始——应先消除不必要工作（ROI 最高），再编译优化，最后才绑核/调度",
      "RT 优先级不设上限——SCHED_FIFO 不设 cap 会饿死其他线程，甚至锁死系统",
      "cgroup CPU quota 用在裸机——HFT 裸机用隔离（isolcpus）不用 quota，quota 引入 throttling stall"],
     [("CPU 调优的优先级顺序是什么？",
       "1) 消除不必要工作 2) 编译优化 3) 优先级/nice 4) 频率 governor 5) 绑核 6) cgroup/资源控制"),
      ("SCHED_FIFO 在 HFT 中的风险？",
       "RT 线程不设 cap 会饿死其他线程——需要 rt throttling 兜底（/proc/sys/kernel/sched_rt_runtime_us）"),
      ("HFT 裸机为什么用 isolcpus 而非 cgroup quota？",
       "quota 会 throttling（时间片用完被强制休眠引入 stall），isolcpus 是物理隔离无 throttling")]),

    # === Ch7 Memory (5 notes) ===
    ("chapter-07-memory/notes/section-7.1-7.2-内存核心概念.md",
     ["free 还多就以为安全——overcommit 允许承诺超过 RAM，OOM killer 随时可能触发",
      "minor fault 不当回事——大量 minor fault（COW/首次 touch）也增延迟，热路径应预热 mlock",
      "anonymous paging 当 file paging——file paging 正常（读 mmap 文件），anonymous paging（swap）= 灾难"],
     [("Overcommit 对 HFT 的风险？",
       "内核允许承诺超过物理内存——free 看着够但实际 OOM 随时触发，需算真实 RSS/PSS"),
      ("minor fault 和 major fault 的区别？",
       "minor = 页已在内存仅更新页表（轻），major = 需要 IO 读盘/swap-in（重，微秒~毫秒级）"),
      ("HFT 热路径如何避免 unexpected page fault？",
       "启动后预热 touch 关键数据结构 + mlock 锁定热页——避免运行期首次访问触发 fault")]),

    ("chapter-07-memory/notes/section-7.3-硬件与软件架构.md",
     ["不绑 NUMA——跨 socket 访存延迟 1.5-3 倍，HFT 必须 CPU + 内存同 NUMA 节点",
      "THP 当大页用——THP 自动合并但延迟不可预测（khugepaged 后台扫描），HFT 应用显式大页",
      "Direct Reclaim 不监控——内存不够时同步回收拖慢当前线程，BPF drsnoop 才能看到"],
     [("NUMA 对 HFT 的影响？",
       "跨 socket 访存延迟 1.5-3 倍——numactl --cpunodebind=0 --membind=0 绑同节点"),
      ("THP 为什么不适合 HFT？",
       "khugepaged 后台扫描合并 4KB→2MB 延迟不可预测——应用显式大页（mmap MAP_HUGETLB）"),
      ("Direct Reclaim 如何检测？",
       "BPF drsnoop 追踪 direct reclaim 路径延迟——vmstat 只看 si/so 不够")]),

    ("chapter-07-memory/notes/section-7.4-分析方法论.md",
     ["USE 只查 free——Saturation（swap/direct reclaim/PSI memory）才是关键",
      "RSS 当真实占用——RSS 包含共享库整页，PSS（按比例分摊）才反映真实占用",
      "leak 只看 RSS——RSS 涨可能是 cache/映射增多，要用 pmap -X 分项确认是 heap leak"],
     [("内存的 USE 方法中 Saturation 看什么？",
       "swap si/so、direct reclaim、PSI memory stall——任一非零说明内存压力"),
      ("RSS 和 PSS 的区别？",
       "RSS 把共享库整页算给每个进程（重复计算），PSS 按进程数分摊共享页（更真实）"),
      ("如何区分内存泄漏和正常增长？",
       "RSS 单调涨不回落 = leak；涨后平台 = 预热 cache——用 pmap -X 分项确认是 heap 还是映射")]),

    ("chapter-07-memory/notes/section-7.5-观测工具.md",
     ["vmstat 只看 free 列——si/so 才是关键，si/so 持续非零 = swap 在发生 = HFT 灾难",
      "slabtop 不看——内核 slab（dentry/inode）膨胀挤占用户内存，sar/slabtop 才能看到",
      "pmap 不用 -X——pmap -x 只给 RSS，-X 给 PSS/共享/私有分项更详细"],
     [("vmstat 中哪些列对 HFT 最关键？",
       "si/so（swap in/out）——持续非零 = anonymous paging 在发生 = 不可接受"),
      ("slabtop 能发现什么问题？",
       "内核 slab cache（dentry/inode/task_struct）膨胀——挤占用户内存导致 direct reclaim"),
      ("pmap -X 比 pmap -x 多什么信息？",
       "-X 包含 PSS（按比例分摊共享页）、Private_Dirty 等分项——定位是哪段映射占内存")]),

    ("chapter-07-memory/notes/section-7.6-调优指南.md",
     ["swappiness 设 0 就安全——0 仍可能 swap（内核 3.5+ 改为 1），且 OOM 风险更高",
      "大页不 benchmark 就上——大页减 TLB miss 但增内部碎片，HFT 需实测确认",
      "cgroup memory.max 用在裸机热路径——throttling 后 OOM kill 可能杀错进程"],
     [("swappiness=0 是否完全禁用 swap？",
       "不完全——内核 3.5+ swappiness=0 等价于 1（极端内存压力仍 swap），应 swapoff -a 彻底禁"),
      ("大页的 trade-off 是什么？",
       "减 TLB miss（好）但增内部碎片（2MB 页即使只用 1KB 也占 2MB）——需 benchmark 确认"),
      ("HFT 裸机 cgroup 内存控制的风险？",
       "memory.max throttling 后可能 OOM kill——关键策略进程不应与未知服务同 cgroup")]),

    # === Ch10 Network (5 notes) ===
    ("chapter-10-network/notes/section-10.1-10.3-核心概念与延迟指标.md",
     ["ping 延迟当业务延迟——ping 是 ICMP 不经过 TCP 栈和应用层，和实际 tick-to-trade 完全不同",
      "bufferbloat 只关注交换机——内核 socket buffer/qdisc 过大也会导致排队延迟膨胀",
      "backlog 不调——syn flood 或突发连接时 accept queue 满，丢连接但应用看不到"],
     [("ping 延迟为什么不能代表业务延迟？",
       "ping 是 ICMP 不经过 TCP 栈/应用层——实际 tick-to-trade 还包括编解码/策略/发单"),
      ("Bufferbloat 在 HFT 中的表现？",
       "内核 socket buffer 或 qdisc 过大导致排队延迟膨胀——吞吐高但 latency 差"),
      ("TCP accept queue 满会怎样？",
       "丢 SYN/ACK——客户端重传或超时，应用层看不到被丢弃的连接")]),

    ("chapter-10-network/notes/section-10.4-硬件与软件栈架构.md",
     ["Nagle 不关——TCP Nagle 合并小包增延迟，HFT 发单必须 TCP_NODELAY",
      "softirq 不绑核——网卡 RX softirq 打到任意核，可能打断热路径线程",
      "DPDK 和标准栈混用不测——旁路内核后 ss/tcpdump 都看不到，监控出现盲区"],
     [("HFT 发单为什么要 TCP_NODELAY？",
       "禁用 Nagle——小包立即发不等待合并，否则增加微秒~毫秒级延迟"),
      ("网卡 RX softirq 对 HFT 热路径的影响？",
       "softirq 可能在任意核上执行——如果不绑核可能打断热路径线程，应配 IRQ affinity"),
      ("DPDK 旁路后的监控盲区？",
       "ss/tcpdump/netstat 都看不到 DPDK 收发的包——需要 DPDK 自带统计或硬件计数器")]),

    ("chapter-10-network/notes/section-10.5-分析方法论.md",
     ["USE 只查吞吐——Errors（CRC/frame/drop）和 Saturation（重传/backlog）才是 HFT 关键",
      "抓包当首选——抓包 CPU/磁盘开销巨大，Gregg 说生产最后手段，先 ss/nstat/BPF",
      "重传率不监控——TCP 重传 = 丢包/拥塞，HFT 发单通道重传 = 延迟暴增"],
     [("网络 USE 方法中 HFT 最关注什么？",
       "Saturation（重传/backlog 满）和 Errors（CRC/drop）——吞吐够高但重传会导致延迟暴增"),
      ("为什么抓包是最后手段？",
       "CPU + 磁盘开销巨大——先 ss/nstat/BPF，抓包限时长限 filter"),
      ("HFT 发单通道的关键指标？",
       "TCP RTT、重传率、Send-Q 积压——重传 = 丢包 = 延迟翻倍")]),

    ("chapter-10-network/notes/section-10.6-观测工具.md",
     ["ss 不加 -tiepm——只看连接列表不看 RTT/重传/cwnd/mss，丢失关键 TCP 内部状态",
      "ethtool -S 不看——驱动级统计（NIC drop/no buffer）比 ss 更底层更早发现问题",
      "BPF tcpretrans 不用——重传事件+栈定位丢包根因，比 netstat -s 的计数更精确"],
     [("ss -tiepm 比普通 ss 多看什么？",
       "TCP 内部状态：RTT、cwnd、retrans、mss、mem、BBR 信息——诊断 TCP 性能必需"),
      ("ethtool -S 能发现什么 ss 看不到的？",
       "驱动级统计——NIC drop/no buffer/rx_missed 等，比 ss 更早发现硬件层丢包"),
      ("tcpretrans 比 netstat -s retrans 有什么优势？",
       "tcpretrans 给出每次重传的事件+栈——定位是哪个连接/哪段代码触发重传")]),

    ("chapter-10-network/notes/section-10.7-10.8-实验与调优.md",
     ["盲抄 Netflix sysctl——Netflix 面向高吞吐 Web，HFT 面向低延迟，参数集完全不同",
      "tcp_tw_reuse 不理解就开——TIME_WAIT 重用有风险（旧连接残留数据），需理解场景",
      "iperf3 当业务 benchmark——iperf3 测的是 TCP 吞吐上限，不是应用层 tick-to-trade 延迟"],
     [("为什么不能直接抄 Netflix 的 sysctl 配置？",
       "Netflix 面向高吞吐 Web（大 buffer/BBR），HFT 面向低延迟（小 buffer/NODELAY）——参数集相反"),
      ("tcp_tw_reuse 的风险是什么？",
       "TIME_WAIT 状态的端口重用——旧连接的残留数据可能被新连接误收，需确认场景适用"),
      ("iperf3 能替代 HFT 应用层 benchmark 吗？",
       "不能——iperf3 测 TCP 吞吐上限，不包含编解码/策略/发单，需应用级 ping/订单通道测试")]),

    # === Ch13 perf (7 notes) ===
    ("chapter-13-perf/notes/section-13.1-13.2-子命令概述与单行命令.md",
     ["perf 版本不匹配内核——perf 需要 linux-tools-$(uname -r)，不匹配时事件不可用或数据错误",
      "perf record 生产不限时长——perf record 有开销（采样写入），生产应限 PID + 限时长",
      "perf trace 当 strace 长跑——perf trace 比 strace 轻但仍非零开销，生产限时长"],
     [("perf 的核心子命令有哪些？",
       "stat（计数）、record（采样）、report（热点）、script（逐行→火焰图）、top（实时）、trace（syscall）"),
      ("perf 版本为什么必须匹配内核？",
       "perf 需要 linux-tools-$(uname -r)——不匹配时 PMC/tracepoint 事件可能不可用或数据错误"),
      ("HFT 生产环境 perf 的使用原则？",
       "stat/top 优先（低开销）；record 限 PID + 限时长；trace 仍限时长")]),

    ("chapter-13-perf/notes/section-13.3-13.7-perf-事件源.md",
     ["硬件事件当软件事件用——PMC 数量有限（通常 4-8 个），同时监测的事件有上限",
      "kprobe 不理解稳定性——kprobe 追踪内核函数名可能随版本变更，应优先 tracepoint",
      "uprobe 不测开销——uprobe 每次函数调用都陷入，热路径高频函数显著增延迟"],
     [("perf 事件源的四大类是什么？",
       "硬件事件（PMC）、软件事件（page-fault/cs）、tracepoint（稳定 ABI）、probe（kprobe/uprobe）"),
      ("kprobe 和 tracepoint 的稳定性区别？",
       "tracepoint 是稳定 ABI 不会变；kprobe 追踪的内核函数名可能随版本变更而失效"),
      ("uprobe 在热路径的风险？",
       "每次函数调用都陷入 BPF 处理——高频函数显著增延迟，应限时长或用 USDT 替代")]),

    ("chapter-13-perf/notes/section-13.8-perf-stat-事件计数.md",
     ["perf stat 只跑一次——单次有抖动，应用 -r 5 重复 5 次看方差",
      "不区分用户态/内核态——-u 只看用户态、-k 只看内核态，混在一起看不出是策略慢还是内核慢",
      "不看 per-CPU——-A 每核分开，HFT 热核和其他核可能差很大"],
     [("perf stat 的 -r 5 选项有什么用？",
       "重复 5 次看方差——单次有抖动，重复后看 std dev 判断稳定性"),
      ("如何区分用户态和内核态开销？",
       "-u 只看用户态（策略/解码）、-k 只看内核态（syscall/协议栈）——分别排查"),
      ("HFT 为什么要用 -A（per-CPU）模式？",
       "热核和其他核可能差很大——全局平均掩盖热核问题")]),

    ("chapter-13-perf/notes/section-13.9-perf-record-剖析采样.md",
     ["-F 99 不知道为什么——99 Hz 减少与 OS timer（100/250/1000 Hz）拍频共振",
      "栈回溯不用 fp——dwarf 准但慢体积大，lbr 依赖硬件，fp 最通用但需编译保留",
      "采样时间太短——HFT tail latency 需要长采样才能采到 P99 尖刺的栈"],
     [("perf record -F 99 为什么用 99 而不是 100？",
       "减与 OS timer（100/250/1000 Hz）拍频共振——避免采样总是落在同一相位"),
      ("栈回溯的三种方法及 HFT 推荐？",
       "fp（帧指针，推荐需 -fno-omit-frame-pointer）、dwarf（准但慢）、lbr（硬件依赖）"),
      ("为什么采样时间不能太短？",
       "HFT P99 尖刺是稀有事件——短采样可能采不到，需要足够长才能捕获尾部栈")]),

    ("chapter-13-perf/notes/section-13.10-perf-report-与-perf-script.md",
     ["report 只看 overhead%——Children 列（含子调用累计）也很重要，单看 overhead 会漏调用链热点",
      "script 不用 stackcollapse——直接看 perf script 输出是逐行原始样本，需要 stackcollapse + flamegraph 才可视化",
      "on-CPU 火焰图当全部——off-CPU 时间（等锁/IO/调度）用 perf record 看不到，需 BPF offcputime"],
     [("perf report 的 Overhead% 和 Children 列有什么区别？",
       "Overhead = 当前函数自身样本占比；Children = 含子调用累计占比——Children 高说明调用链热"),
      ("perf script 输出如何变成火焰图？",
       "perf script | stackcollapse-perf.pl | flamegraph.pl > out.svg——需要 FlameGraph 仓库工具"),
      ("on-CPU 火焰图的局限？",
       "只显示在 CPU 上执行的时间——等锁/IO/调度的时间不在图上，需 offcputime 补充")]),

    ("chapter-13-perf/notes/section-13.11-perf-trace-系统调用追踪.md",
     ["perf trace 生产长跑——虽然比 strace 轻但仍有开销，生产应限时长",
      "perf trace 不限事件——trace -e 指定 syscall 类型，全量 trace 开销巨大",
      "strace 和 perf trace 不区分——strace 用 ptrace（开销巨大），perf trace 用 perf 基础设施（相对轻）"],
     [("perf trace 和 strace 的根本区别？",
       "strace 用 ptrace（每 syscall 两次陷入，开销巨大）；perf trace 用 perf 基础设施（相对轻）"),
      ("perf trace 在生产环境的注意事项？",
       "限时长 + 指定事件（-e open,read,write）——全量 trace 开销巨大"),
      ("HFT 什么时候用 perf trace？",
       "发现热路径 unexpected read/mmap——开发机 perf trace 5 秒定位 syscall 类型")]),

    ("chapter-13-perf/notes/section-13.12-其他常用能力延伸.md",
     ["perf c2c 不用——伪共享是 HFT 常见杀手，perf c2c 能直接量化 cache line 争用",
      "perf sched 只看调度次数——latency 子命令看调度延迟（线程就绪到被 CPU 执行），这才是 HFT 关心",
      "perf lock 不在开发阶段用——锁竞争在生产才暴露，但开发阶段用 perf lock 可以提前发现"],
     [("perf c2c 能发现什么问题？",
       "伪共享（false sharing）——多线程写同一 cache line 不同字段，cache 一致性协议打穿性能"),
      ("perf sched latency 和 sched record 的区别？",
       "record 采集调度事件；latency 分析线程从就绪到被 CPU 执行的延迟——HFT 关心调度等待时间"),
      ("HFT 什么时候该用 perf lock？",
       "怀疑锁竞争导致 tail latency——perf lock 量化锁等待时长和持有时长")]),

    # === Ch15 BPF (4 notes) ===
    ("chapter-15-bpf/notes/section-15.1-BCC-BPF-Compiler-Collection.md",
     ["BCC 工具名带 -bpfcc 后缀不知道——Debian/Ubuntu 包名 bpfcc-tools，命令名加 -bpfcc 后缀",
      "BCC 当 bpftrace 用——BCC 适合标准化工具（团队共享），bpftrace 适合即兴诊断（个人脚本）",
      "libbpf/CO-RE 不知道——新工具渐迁 libbpf + CO-RE（一次编译到处运行），BCC Python 方式是旧路径"],
     [("BCC 工具在 Debian/Ubuntu 上的命令名后缀？",
       "-bpfcc——如 runqlat-bpfcc、biolatency-bpfcc（包名 bpfcc-tools）"),
      ("BCC 和 bpftrace 的定位区别？",
       "BCC 适合标准化团队工具（Python CLI 封装可维护），bpftrace 适合即兴诊断（单行命令极快）"),
      ("libbpf/CO-RE 相比 BCC 的优势？",
       "一次编译到处运行（CO-RE = Compile Once Run Everywhere），无需目标机装 BCC/Clang")]),

    ("chapter-15-bpf/notes/section-15.2-bpftrace.md",
     ["bpftrace 生产直接跑自定义脚本——自定义 kprobe 可能加载失败或开销过大，应先 staging 验证",
      "bpftrace 每事件输出——高频事件（sched_switch）每条 print 打爆 CPU，应用 map 聚合",
      "bpftrace 当 BCC 用——复杂状态机/多 map 协作用 BCC Python，bpftrace DSL 表达力有限"],
     [("bpftrace 自定义脚本在生产环境的风险？",
       "kprobe 可能加载失败/开销过大——应先在 staging 验证加载和开销"),
      ("为什么 bpftrace 高频事件不能每条 print？",
       "sched_switch 每秒上千次——每条送到用户态会打爆 CPU，应用 map histogram 聚合"),
      ("bpftrace 和 BCC 的分工？",
       "bpftrace 适合即兴单行/简单脚本；复杂多事件状态机用 BCC Python")]),

    ("chapter-15-bpf/notes/section-15.1.7-BCC-vs-bpftrace.md",
     ["BCC 和 bpftrace 二选一——Gregg 强调互补双剑：生产 crisis 用 BCC 标准工具，不够再上 bpftrace",
      "bpftrace 脚本不升格——重复有用的 bpftrace 脚本应升格为 BCC 工具或 runbook，不是每次重写",
      "BCC 工具不记 runbook——出事才 man page，runbook 应预设好第一反应 BCC 命令"],
     [("Gregg 的 BCC/bpftrace 工作流是什么？",
       "1) 生产 crisis → BCC 标准工具 2) 不够 → bpftrace 即兴追 3) 证明有用 → 升格 BCC/runbook"),
      ("为什么 bpftrace 脚本应该升格？",
       "重复有用的脚本应升格为 BCC 工具或 runbook——避免每次出事重写，且可团队共享"),
      ("HFT runbook 中 BCC 工具应该怎么用？",
       "预设好第一反应命令（延迟尖刺→offcputime/runqlat），复制粘贴即可跑")]),

    ("chapter-15-bpf/notes/section-BPF-背景与架构15.1-15.2-基础.md",
     ["Verifier 不理解就绕过——Verifier 是安全保证（无越界/有界循环/类型安全），绕过 = 内核崩溃风险",
      "Ring Buffer vs Maps 不分场景——高频事件用 map 聚合（低开销），低频事件用 ring buffer（明细）",
      "kprobe 和 tracepoint 不分优先级——优先 tracepoint（稳定 ABI），kprobe 是后备"],
     [("BPF Verifier 保证什么？",
       "无越界访问、有界循环（不能死循环）、类型安全指针——是生产安全的基础"),
      ("Ring Buffer 和 BPF Maps 的场景区别？",
       "Ring Buffer 适合低频事件明细输出；Maps 适合高频事件聚合（计数/直方图）——高频用 map 低开销"),
      ("为什么优先 tracepoint 而非 kprobe？",
       "tracepoint 是稳定 ABI 不会随内核版本变；kprobe 追踪的函数名可能变更而失效")]),
]

def main():
    ok, fail, skip = 0, 0, 0
    for rel_path, traps, quiz in NOTES:
        fpath = os.path.join(MOD19, rel_path)
        if not os.path.exists(fpath):
            print(f"  MISSING: {rel_path}")
            fail += 1
            continue
        with open(fpath, 'r', encoding='utf-8') as f:
            content = f.read()
        # Check if already has traps
        if '### 常见陷阱' in content:
            print(f"  SKIP (already has traps): {rel_path}")
            skip += 1
            continue
        block = make_block(traps, quiz)
        result = insert_before_nav(content, block)
        if result is None:
            # No navigation found, append at end
            result = content.rstrip() + '\n' + block
            print(f"  APPEND (no nav): {rel_path}")
        with open(fpath, 'w', encoding='utf-8') as f:
            f.write(result)
        ok += 1
        print(f"  OK: {rel_path}")
    print(f"\nDone: {ok} updated, {skip} skipped, {fail} failed")

if __name__ == '__main__':
    main()
