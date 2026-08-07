#!/usr/bin/env python3
"""
20-bpf-observability 🔴章节新手化增强脚本
为 Ch1/2/4/5/6/10 共 52 篇笔记添加：常见陷阱(3) + 折叠自测题(3)
"""
import os, re, sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # 20-bpf-observability/

# ── helpers ──────────────────────────────────────────────────────────

def make_block(traps, quiz):
    """Generate the traps + quiz block to insert before the final '---'."""
    lines = []
    lines.append("")
    lines.append("### 常见陷阱")
    lines.append("")
    for i, (title, desc) in enumerate(traps, 1):
        lines.append(f"{i}. **{title}** — {desc}")
    lines.append("")
    lines.append("<details>")
    lines.append("<summary>\U0001f4dd 自测题（点击展开）</summary>")
    lines.append("")
    for i, (q, a) in enumerate(quiz, 1):
        lines.append(f"{i}. **{q}**")
        lines.append("")
        lines.append("   <details>")
        lines.append("   <summary>参考答案</summary>")
        lines.append("")
        lines.append(f"   {a}")
        lines.append("")
        lines.append("   </details>")
        lines.append("")
    lines.append("</details>")
    lines.append("")
    return "\n".join(lines)


def insert_before_final_sep(content, block):
    """Insert block before the last '---' line (the section separator at file end)."""
    # Find the last occurrence of a line that is just '---'
    lines = content.split("\n")
    # Search from the end for a line matching ^---\s*$
    insert_idx = None
    for i in range(len(lines) - 1, -1, -1):
        if re.match(r'^---\s*$', lines[i]):
            insert_idx = i
            break
    if insert_idx is not None:
        # Insert block before the '---' line
        new_lines = lines[:insert_idx] + [block] + lines[insert_idx:]
        return "\n".join(new_lines)
    else:
        # Fallback: append at end
        return content.rstrip("\n") + "\n" + block + "\n"


# ── data: all 52 notes ───────────────────────────────────────────────
# Each entry: (relative_path, traps_list, quiz_list)
# traps_list: [(title, desc), ...]  (3 items)
# quiz_list: [(question, answer), ...]  (3 items)

NOTES = []

# ════════════════════════════════════════════════════════════════════
# Ch1 Introduction (6 notes)
# ════════════════════════════════════════════════════════════════════

NOTES.append((
    "chapter-01-introduction/notes/section-1-基础概念.md",
    [
        ("混淆 BPF 与 eBPF", "经典 BPF 仅做包过滤（tcpdump），eBPF 是通用内核 VM；本书「BPF」默认指 eBPF，读到旧文档时要区分上下文"),
        ("以为 Sampling 能抓所有问题", "采样按固定频率快照，短命进程和微秒级延迟尖刺可能完全漏掉；HFT 延迟排查应优先用 Tracing 而非 Sampling"),
        ("把 Observability 等同于 Logging", "日志是应用主动输出，Observability 是从外部推断内部状态；BPF 的价值在于无需改代码即可透视内核+用户态"),
    ],
    [
        ("BPF 程序在哪一层运行？为什么这对性能很重要？", "BPF 程序在内核态运行（经过验证器检查安全性）。这意味着事件过滤、计数、聚合都在内核完成，只有最终结果通过 Map 送到用户态，避免了海量事件逐条拷贝到用户态的开销。"),
        ("Tracing 和 Sampling 各自适合什么场景？", "Tracing 适合「谁、何时、持续了多久」的精确事件追踪（如每次 syscall 的延迟），能捕获短事件；Sampling 适合 CPU 热点定位（profile 按频率采栈），开销低但可能漏掉短命事件。HFT 延迟尖刺排查应优先 Tracing。"),
        ("为什么说 BPF 让「内核 + 用户态」同屏可见？", "传统工具要么只看内核（/proc、perf），要么只看应用（日志、APM）。BPF 的 kprobe 看内核路径、uprobe 看用户态函数、USDT 看应用探针，可在同一工具链内关联跨层因果链。"),
    ],
))

NOTES.append((
    "chapter-01-introduction/notes/section-2-核心前端为何需要BCCbpftrace.md",
    [
        ("手写 BPF C 程序门槛太高", "直接写 C + LLVM + bpf() syscall 加载，需要理解 verifier 约束、Map 创建、helper 调用；BCC 和 bpftrace 封装了这些，让分析人员专注观测逻辑"),
        ("以为 BCC 和 bpftrace 功能完全等价", "BCC 适合复杂多用途工具（Python 前端 + C BPF 后端），bpftrace 适合快速 one-liner 和简单聚合；复杂逻辑用 BCC，快速验证用 bpftrace"),
        ("忽视前端工具的运行时开销", "BCC 的 Python 前端本身有内存和启动开销，bpftrace 更轻但高频 probe 仍有成本；HFT 热路径上工具用完即撤，不长期挂载"),
    ],
    [
        ("为什么不直接用 raw BPF C 而需要 BCC/bpftrace 前端？", "Raw BPF 需要手写 C、管理 LLVM 编译、创建 Map、调用 bpf() syscall、处理 verifier 错误，门槛极高。BCC 提供 Python 框架自动处理编译和加载，bpftrace 提供 DSL 把常见模式浓缩成 one-liner，让性能分析师专注观测而非底层工程。"),
        ("BCC 和 bpftrace 在选型上如何取舍？", "BCC 适合：复杂多步骤工具、需要 Python 后处理、团队共享的标准化工具。bpftrace 适合：快速 one-liner 验证假设、简单聚合/直方图、临时排障。经验法则：能用 bpftrace one-liner 解决的就不用 BCC。"),
        ("前端工具本身会引入开销吗？HFT 场景如何控制？", "BCC 的 Python 前端有进程启动和内存开销；bpftrace 更轻但 probe 本身的 per-hit 成本仍在。HFT 原则：(1) 排障时短跑，不长期挂载；(2) 避免在最低延迟核上 attach 高频 probe；(3) 优先用 Map 聚合而非逐行打印。"),
    ],
))

NOTES.append((
    "chapter-01-introduction/notes/section-3-BCC工具初探快速排障.md",
    [
        ("一上来就用最重的工具", "新手常直接 `perf record` 全系统采样，但快速排障应先用轻量工具（execsnoop、opensnoop）定位现象，再逐步钻取"),
        ("忽视 BCC 工具的命名规律", "BCC 工具名即问题描述：`biosnoop` = bio + snoop、`runqlat` = run queue + latency；理解命名就能快速选对工具"),
        ("在错误的层级使用工具", "比如用 `biolatency` 排查网络延迟问题——BCC 工具按资源域分类（CPU/内存/磁盘/网络），跨域使用会浪费时间"),
    ],
    [
        ("BCC 快速排障的推荐顺序是什么？", "先轻后重：(1) execsnoop/opensnoop 看进程和文件活动（低频、低开销）；(2) biolatency/runqlat 看资源延迟分布；(3) profile 做 CPU 采样定位热点。从「谁在做什么」到「花了多久」再到「CPU 在哪」。"),
        ("如何通过工具名快速判断用途？", "BCC 工具名 = 资源 + 动作：bio = block I/O、runq = run queue、off = off-CPU、snoop = 逐事件追踪、lat = 延迟直方图。例如 `offcputime` = off-CPU 时间统计，`biolatency` = block I/O 延迟分布。"),
        ("在 HFT 排障中，为什么不应一上来就全系统 perf record？", "全系统 perf record 开销大、数据多、分析慢。HFT 延迟尖刺往往是特定路径的短事件，应先用低开销工具（execsnoop 看进程切换、runqlat 看排队延迟）缩小范围，再对疑似路径做精准 BPF 追踪。"),
    ],
))

NOTES.append((
    "chapter-01-introduction/notes/section-4-BPF的可见性.md",
    [
        ("以为 BPF 能看到一切", "BPF 受 verifier 约束：不能随意解引用指针、不能无限循环、有栈深度限制；某些内核路径（如 NMI 上下文）的 probe 行为受限"),
        ("混淆「可见」和「可安全访问」", "kprobe 能 attach 到几乎任意内核函数，但函数内部数据结构无稳定 ABI，升级后 offset 可能变；BTF + CO-RE 部分缓解但不完全消除"),
        ("忽视 BPF 程序的资源限制", "BPF Map 有大小上限、栈空间有限（通常 512 字节）、指令数有验证器上限；复杂聚合逻辑可能被 verifier 拒绝"),
    ],
    [
        ("BPF 的「可见性」相比传统工具有什么本质提升？", "传统工具只能看预定义的 /proc、sysfs、perf 事件；BPF 可在几乎任意内核函数（kprobe）和用户态函数（uprobe）插桩，实时按需定义观测点，无需重新编译内核或应用。相当于从「固定摄像头」升级到「可编程探针」。"),
        ("verifier 对 BPF 可见性有哪些限制？", "(1) 指针解引用必须经过边界检查；(2) 不能无限循环（有指令数上限）；(3) 栈空间有限（~512 字节）；(4) 某些上下文（如 NMI）限制 probe 类型；(5) Map 有大小上限。这些限制保证安全但约束了复杂逻辑。"),
        ("HFT 场景中 BPF 可见性的盲区在哪？", "(1) DPDK 用户态 PMD 轮询路径——BPF 主要看内核栈，用户态 PMD 需 uprobe；(2) 硬件级别（网卡 ASIC、交换机）——BPF 看不到；(3) 极高频路径（每包 probe）——verifier 允许但开销不可接受。"),
    ],
))

NOTES.append((
    "chapter-01-introduction/notes/section-5-动态插桩vs静态插桩.md",
    [
        ("优先用 kprobe 而非 tracepoint", "kprobe 依赖内核内部函数名，升级后可能消失；tracepoint 是内核开发者承诺的稳定接口，应优先使用"),
        ("以为 uprobes 在未 attach 时也有开销", "uprobe 未 attach 时是原始指令，attach 后才插入断点；但 attach 期间高频函数的开销是真实的，HFT 热路径慎用"),
        ("混淆动态插桩的「零开销」", "动态 probe 未 attach 时零开销，但 attach 后每次命中都有 trap + BPF 程序执行成本；「零开销」仅指未使用状态"),
    ],
    [
        ("动态插桩和静态插桩的核心区别是什么？", "动态插桩（kprobe/uprobe）在任意指令地址插桩，灵活但无 ABI 保证；静态插桩（tracepoint/USDT）由开发者预埋，名称和字段格式稳定。优先级：tracepoint > kprobe，USDT > uprobe。"),
        ("为什么说「能 tracepoint 就不用 kprobe」？", "Tracepoint 是内核开发者维护的稳定接口，有 format 文件描述字段，跨内核版本兼容；kprobe 依赖内部函数名和参数布局，内核升级后可能改名或改签名，导致脚本失效。"),
        ("USDT 在未 attach 时的开销是什么？HFT 如何利用？", "USDT 未 attach 时通常编译为 nop 指令，开销几乎为零。HFT 应用可在关键路径（如订单接收、策略执行）预埋 USDT 探针，日常零开销，排障时用 bpftrace attach 即可获取精确时延，无需改代码重新部署。"),
    ],
))

NOTES.append((
    "chapter-01-introduction/notes/section-6-bpftrace与BCC演示追open.md",
    [
        ("演示后忘了清理 probe", "bpftrace Ctrl-C 退出会自动 detach，但 BCC 工具异常退出可能残留 attached probe；用 `bpftool prog list` 检查是否有残留"),
        ("忽视追 open 的过滤条件", "全系统追 open 会产生海量事件；应按 PID 或 comm 过滤，否则输出被噪音淹没"),
        ("只看 open 不看 openat", "现代 glibc 的 fopen/open 常走 openat 系统调用，仅追 open 会漏掉大量文件访问"),
    ],
    [
        ("用 bpftrace 追踪 open 系统调用的基本命令是什么？", "`sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf(\"%s %s\\n\", comm, str(args->filename)); }'`。注意现代系统用 openat 而非 open，需追 sys_enter_openat。"),
        ("BCC 的 opensnoop 和 bpftrace one-liner 各有什么优劣？", "opensnoop 是成熟工具，输出格式固定、有 PID/UID/返回值/文件名，适合标准化排障；bpftrace one-liner 更灵活，可自定义过滤和输出字段，适合快速验证假设。团队标准化用 opensnoop，临时排查用 bpftrace。"),
        ("追踪 open/openat 时如何避免被海量事件淹没？", "(1) 按 PID 过滤：`/pid == 12345/`；(2) 按 comm 过滤：`/comm == \"myapp\"/`；(3) 只看失败：`/args->ret < 0/`；(4) 用 Map 聚合而非逐行打印：`@[comm] = count()` 看谁打开了最多文件。"),
    ],
))

# ════════════════════════════════════════════════════════════════════
# Ch2 Technology Background (7 notes)
# ════════════════════════════════════════════════════════════════════

NOTES.append((
    "chapter-02-technology-background/notes/section-1-BPF与eBPF.md",
    [
        ("混淆 BPF 程序和 BPF Map", "BPF 程序是执行逻辑（探针命中时跑的代码），Map 是数据存储（跨事件共享结果）；新手常把两者混为一谈"),
        ("以为 CO-RE 消除了所有内核版本兼容问题", "CO-RE 解决结构体偏移重定位，但不保证函数签名和语义不变；kprobe 目标函数本身可能被重命名或删除"),
        ("忽视 verifier 的安全检查对编程的限制", "verifier 要求所有指针访问有边界检查、循环有上界，这限制了能写的逻辑复杂度；复杂聚合需拆分为多个简单 Map 操作"),
    ],
    [
        ("经典 BPF 和 eBPF 的主要区别有哪些？", "(1) 寄存器数：2 → 10；(2) 数据宽度：32 → 64 bit；(3) Map 无上限（经典 BPF 只有简单累加器）；(4) 可调用内核 helper 函数；(5) 经验证器保证安全终止。eBPF 从包过滤器升级为通用内核 VM。"),
        ("BPF Map 的作用是什么？为什么对性能工具至关重要？", "Map 是 BPF 程序与用户态之间、以及多次 probe 触发之间共享数据的键值存储。性能工具在内核态用 Map 聚合结果（如延迟直方图、计数表），用户态只读最终汇总，避免海量事件逐条上报到用户态。"),
        ("CO-RE 解决了什么问题？有什么局限性？", "CO-RE (Compile Once – Run Everywhere) 解决不同内核版本间结构体成员偏移不同的问题——编译期记录重定位信息，运行时按目标内核修正。局限性：(1) 仅解决偏移，不解决函数签名变化；(2) 需要 BTF 信息支持；(3) kprobe 目标函数仍可能被重命名。"),
    ],
))

NOTES.append((
    "chapter-02-technology-background/notes/section-2-堆栈追踪遍历.md",
    [
        ("栈 ID 过期导致栈信息错误", "BPF 用 stackid(Map) 返回栈的 ID 而非完整栈，Map 有大小限制，旧栈 ID 可能被新栈覆盖；用 `BPF_F_USER_STACK` 区分内核/用户栈"),
        ("忽视符号解析的依赖", "栈 ID 只是地址，需要符号表（/proc/kallsyms、二进制 debug info）解析为函数名；strip 过的二进制无法解析用户栈"),
        ("在 HFT 热路径上频繁采栈", "每次 stackid 调用有一定开销，高频 probe 上采栈会放大延迟；HFT 应用低频采样（如 99Hz）或按事件触发而非 per-hit"),
    ],
    [
        ("BPF 如何获取和存储调用栈？", "BPF 程序调用 `bpf_get_stackid()` 将当前栈哈希后存入专用 Map，返回一个整型 ID。分析时用户态用该 ID 查 Map 获取地址列表，再通过符号表解析为函数名。这种方式避免每次都传完整栈数据到用户态。"),
        ("栈 ID 过期是什么问题？如何缓解？", "stackid Map 有大小上限（默认有限），当栈数量超过上限时旧 ID 被覆盖，导致后续查询返回错误或错误栈。缓解：(1) 增大 Map；(2) 使用 `BPF_F_USER_STACK` / `BPF_F_KERNEL_STACK` 分别存储；(3) 接受少量丢失，关注高频栈。"),
        ("为什么 strip 过的二进制无法解析用户态栈？", "符号解析需要地址到函数名的映射表，strip 操作删除了 .symtab 和 .strtab 段。解决方案：(1) 保留未 strip 版本用于分析；(2) 使用 USDT 探针替代栈追踪；(3) 用 DWARF debug info（-g 编译）。"),
    ],
))

NOTES.append((
    "chapter-02-technology-background/notes/section-3-火焰图.md",
    [
        ("把火焰图当作 profile 的替代品", "火焰图是 profile/stackcount 的可视化形式，不是独立数据源；先确保数据采集正确（正确的 probe、足够的采样时长），再画图"),
        ("误读火焰图的宽度", "火焰图横向宽度 = 该函数在采样中出现的比例，不代表单次执行时间；宽的函数可能因为被调用次数多而非单次慢"),
        ("忽视采样频率的选择", "默认 99Hz 是为了避免与定时器事件共振；HFT 场景可能需要更高频率，但频率越高开销越大"),
    ],
    [
        ("火焰图的横轴和纵轴分别代表什么？", "纵轴 = 调用栈深度（底部是调用者，顶部是被调用者），横轴 = 该函数在所有采样中出现的比例（不是时间轴）。一个「宽」的顶部条表示该函数占用了较多 CPU 采样。"),
        ("为什么采样频率常用 99Hz 而非 100Hz？", "99 是质数，避免与系统中其他 100Hz/1000Hz 的周期性事件（如定时器、心跳）产生共振效应，导致采样总是命中同一相位。99Hz 让采样点均匀散布在不同执行阶段。"),
        ("HFT 场景中火焰图有什么局限性？", "(1) 采样可能漏掉微秒级延迟尖刺；(2) 只显示 on-CPU 时间，off-CPU 等待不显示（需 off-CPU 火焰图）；(3) 宽度代表频次不是延迟，宽函数可能是高频低延迟调用。HFT 应结合 offcputime 和直方图使用。"),
    ],
))

NOTES.append((
    "chapter-02-technology-background/notes/section-4-动态插桩kprobes与uprobes.md",
    [
        ("在极高频函数上 attach uprobe", "如对 malloc/recv 每 hit 跑 BPF 程序，可能导致目标进程减速数倍；HFT 热路径绝对禁止 per-hit uprobe"),
        ("依赖 kprobe 追踪的内核函数名", "内核内部函数无 ABI 保证，升级后可能重命名（如 do_sys_open → do_sys_openat2）；应优先用 tracepoint"),
        ("忽视 kprobe 的 instrument 限制", "某些内核函数（如 __schedule 中的部分路径）不适合 kprobe，可能递归或死锁；内核有 `NOKPROBE_SYMBOL` 标记禁止插桩的函数"),
    ],
    [
        ("kprobe 和 uprobe 的插桩机制有什么共同点和区别？", "共同点：都是动态在指令地址插入断点（x86 用 int3），命中时执行 BPF 程序。区别：kprobe 在内核函数插桩，uprobe 在用户态二进制/库插桩；kprobe 受内核 NOKPROBE 限制，uprobe 受文件映射和权限限制。"),
        ("为什么 HFT 热路径上绝对禁止 per-hit uprobe？", "每次 uprobe 命中需要：trap → 上下文切换 → BPF 程序执行 → 返回，这个开销在微秒级。HFT 策略循环中每次 recv/send 都触发 probe，会把延迟从微秒级推到毫秒级。如需观测，用低频采样或 Map 聚合。"),
        ("kprobe 未 attach 时有开销吗？attach 后呢？", "未 attach 时零开销——原始指令不受影响。Attach 后每次命中需要 int3 trap + BPF 程序执行，开销取决于 BPF 程序复杂度和命中频率。原则：用完即撤，不长期挂载。"),
    ],
))

NOTES.append((
    "chapter-02-technology-background/notes/section-5-静态插桩Tracepoints与USDT.md",
    [
        ("不知道如何查找可用的 tracepoint", "tracepoint 在 /sys/kernel/debug/tracing/events/ 下按子系统组织；用 `bpftrace -l 'tracepoint:*'` 或 `cat /sys/kernel/debug/tracing/available_events` 查找"),
        ("USDT 探针需要重新编译应用", "USDT 需在编译时用 dtrace 宏插入探针点；已有应用如果没有 USDT，只能用 uprobe 替代"),
        ("忽视 tracepoint format 文件", "每个 tracepoint 有 format 文件描述字段名和类型，不看 format 直接写 args->fieldname 会出错"),
    ],
    [
        ("Tracepoint 相比 kprobe 的三个优势是什么？", "(1) 稳定的名称和 ABI——内核开发者承诺不随意改名；(2) 有 format 文件描述字段（`cat .../format`），知道 args 有哪些字段；(3) 可以用 `bpftrace -l 'tracepoint:sched:*'` 方便查找。优先级：tracepoint > kprobe。"),
        ("如何查看某个 tracepoint 的可用字段？", "`cat /sys/kernel/debug/tracing/events/<子系统>/<事件>/format`。例如查看 sched_switch 的字段：`cat /sys/kernel/debug/tracing/events/sched/sched_switch/format`。bpftrace 中用 `args-><字段名>` 访问。"),
        ("USDT 在 HFT 应用中如何使用？有什么前提？", "在关键路径代码中用 `DTRACE_PROBE()` 宏插入探针点（编译时为 nop，零开销）。排障时用 `bpftrace -e 'usdt:myapp:probe_name { ... }'` attach。前提：(1) 编译时开启 USDT 支持；(2) 二进制保留探针信息；(3) 知道探针名称。"),
    ],
))

NOTES.append((
    "chapter-02-technology-background/notes/section-6-PMCs与perf_events.md",
    [
        ("混淆 PMC 计数模式和采样模式", "计数模式只累计事件总数，采样模式在计数溢出时中断记录 IP+栈；分析热点需要采样模式，看趋势用计数模式"),
        ("忽视 PMI 的 skid 问题", "性能监控中断（PMI）不是精确的——从事件发生到中断响应有若干指令的滑移，记录的 IP 可能不是真正触发事件的指令；用 PEBS 缓解"),
        ("在虚拟化环境中期望 PMC 可用", "云 VM 通常无法直接访问 PMC（需要直通或 vPMU 支持）；HFT 如果跑在 VM 里，PMC 分析可能不可用"),
    ],
    [
        ("PMC 的计数模式和溢出采样模式有什么区别？", "计数模式：累计硬件事件总数（如 L3 miss 总次数），适合看宏观趋势。溢出采样模式：计数到阈值后触发 PMI 中断，记录 IP 和栈，适合定位热点代码。`perf stat` 用计数模式，`perf record` 用采样模式。"),
        ("什么是 PMI skid？PEBS 如何解决？", "PMI（性能监控中断）有延迟——从硬件事件触发到 CPU 响应中断，期间会执行若干条指令（skid），导致记录的 IP 偏离真正触发事件的指令。PEBS（Intel）用硬件缓冲区在事件发生时立即保存处理器状态，大幅减少 skid，适合微架构级分析。"),
        ("BPF 如何与 PMC/perf_events 结合？", "BPF 程序可附加到 perf_event（`BPF_PROG_TYPE_PERF_EVENT`），在 PMC 溢出时执行 BPF 逻辑而非传统 perf 中断处理。这样可以在溢出时用 BPF Map 收集栈、关联上下文，比纯 perf 更灵活。日常 HFT 更多直接用 `perf record` + BCC `profile`。"),
    ],
))

NOTES.append((
    "chapter-02-technology-background/notes/section-7-技术组件地图后文工具如何挂接.md",
    [
        ("把技术组件当作独立工具", "kprobes、tracepoints、PMCs 是底层机制，BCC/bpftrace 是前端；工具（biolatency、runqlat）是前端对底层机制的封装组合"),
        ("忽视组件间的依赖关系", "如火焰图依赖 stackid，stackid 依赖 kprobe/uprobe/tracepoint；理解依赖链有助于排查「为什么工具没输出」"),
        ("选工具时忽视事件频率", "高频事件应用 Map 聚合工具（funccount/argdist），低频事件可用逐行打印工具（trace）；选错工具要么漏数据要么拖慢系统"),
    ],
    [
        ("BPF 性能工具的技术栈从底到顶有哪几层？", "(1) 硬件层：PMC、PEBS；(2) 内核插桩层：kprobes、tracepoints、perf_events；(3) 用户态插桩层：uprobes、USDT；(4) BPF 核心层：verifier、JIT、Map、helper；(5) 前端层：BCC、bpftrace；(6) 工具层：biolatency、runqlat 等具体工具。"),
        ("当 BPF 工具没有输出时，如何沿技术栈排查？", "从顶到底：(1) 前端语法是否正确？(bpftrace -d 看编译结果)；(2) probe 是否匹配到目标？(bpftrace -l 查)；(3) BPF 程序是否加载成功？(bpftool prog list)；(4) 目标事件是否真的触发？(用 strace/perf 先验证)；(5) 权限是否足够？(root/cap_bpf)。"),
        ("HFT 场景中，如何根据事件频率选择技术组件？", "高频事件（如每包 recv）：用 tracepoint + Map 聚合，避免 per-hit 打印。中频事件（如 syscall enter）：可用 bpftrace 聚合 + 定时输出。低频事件（如进程创建）：可用 trace 逐行打印。PMC 溢出采样适合 CPU 热点定位，不适合短延迟事件。"),
    ],
))

# ════════════════════════════════════════════════════════════════════
# Ch4 BCC (7 notes)
# ════════════════════════════════════════════════════════════════════

NOTES.append((
    "chapter-04-bcc/notes/section-1-BCC是什么.md",
    [
        ("把 BCC 当作单一工具", "BCC 是框架（Python 前端 + C BPF 后端 + libbpf），包含数十个预置工具；新手常以为 BCC 就是一个命令"),
        ("忽视 BCC 对内核头文件的依赖", "BCC 运行时编译 BPF C 代码需要内核头文件（kernel-devel/kernel-headers）；缺少头文件会报编译错误"),
        ("在容器中使用 BCC 不做特殊配置", "容器需要 CAP_SYS_ADMIN/CAP_BPF + 挂载 debugfs/tracefs；普通容器默认无法运行 BCC 工具"),
    ],
    [
        ("BCC 的架构由哪几部分组成？", "(1) Python 前端：用户交互、参数解析、结果输出；(2) Clang/LLVM 运行时编译器：把 BPF C 编译为字节码；(3) libbpf：加载 BPF 程序、创建 Map；(4) 内核 BPF 子系统：验证、JIT、执行。开发者写 Python + C，BCC 负责编译加载。"),
        ("BCC 运行时编译需要什么依赖？为什么？", "需要内核头文件包（kernel-devel 或 linux-headers），因为 BCC 在运行时用 Clang 编译 BPF C 代码，需要内核头文件中的类型定义和宏。缺少头文件会报 `fatal error: linux/xxx.h: No such file` 等编译错误。"),
        ("在容器中运行 BCC 工具需要哪些特殊配置？", "(1) 以 privileged 模式运行或添加 CAP_SYS_ADMIN/CAP_BPF；(2) 挂载 debugfs 和 tracefs（`-v /sys/kernel/debug:/sys/kernel/debug`）；(3) 容器内安装内核头文件匹配宿主内核版本；(4) 确认宿主内核支持 BPF（4.x+）。"),
    ],
))

NOTES.append((
    "chapter-04-bcc/notes/section-2-BCC架构与特性.md",
    [
        ("忽视 BCC 运行时编译的启动延迟", "BCC 每次运行都用 Clang 编译 BPF C 代码，启动有 1-3 秒延迟；对 HFT 的快速排障（秒级响应）有影响，bpftrace 启动更快"),
        ("混淆 BCC 和 libbpf+CO-RE 的部署模型", "BCC 需要目标机器有 Clang+内核头文件；libbpf+CO-RE 预编译为单一二进制，无需编译器——部署更简单"),
        ("以为 BCC 工具修改 BPF 程序不需要重启", "BPF 程序一旦加载就运行在内核中，修改需重新编译加载（重启工具）；不能热修改已加载的 BPF 逻辑"),
    ],
    [
        ("BCC 的运行时编译模型有什么优缺点？", "优点：(1) 可用内核头文件中的任意类型和宏；(2) 修改 BPF C 代码后直接运行，无需预编译。缺点：(1) 需要目标机器安装 Clang + 内核头文件（部署重）；(2) 启动有 1-3 秒编译延迟；(3) 编译错误在运行时才暴露。"),
        ("BCC 和 libbpf+CO-RE 的部署模型有什么区别？", "BCC：目标机器需要 Clang + LLVM + kernel-headers，运行时编译，部署重但灵活。libbpf+CO-RE：预编译为单一二进制（含 BTF 重定位信息），目标机器无需编译器，部署轻但需要内核支持 BTF。新工具链渐迁 libbpf+CO-RE。"),
        ("HFT 场景中 BCC 的启动延迟如何影响排障？", "BCC 工具启动有 1-3 秒编译延迟，如果延迟尖刺只持续毫秒级，等 BCC 编译完现象可能已消失。对策：(1) 预先启动工具用 interval 模式持续监控；(2) 短暂现象用 bpftrace（启动更快）；(3) 复杂工具可预编译为 BPF 字节码避免运行时编译。"),
    ],
))

NOTES.append((
    "chapter-04-bcc/notes/section-3-单用途vs多用途设计哲学.md",
    [
        ("用多用途工具做简单任务", "用 trace 做简单计数是大材小用——多用途工具参数复杂、输出冗余；简单任务用单用途工具更高效"),
        ("忽视单用途工具的局限性", "单用途工具（如 biolatency）只回答一个问题，不能做交叉分析；复杂排障需要多个单用途工具组合或用多用途工具"),
        ("混淆 BCC 工具的两种设计取向", "单用途工具追求「一行命令出结果」，多用途工具追求「灵活组合探针和输出」；选型取决于问题复杂度"),
    ],
    [
        ("BCC 工具的「单用途」和「多用途」设计哲学分别适合什么场景？", "单用途工具（biolatency、runqlat）：回答一个明确问题，参数少、输出固定，适合标准化排障和快速定位。多用途工具（trace、argdist、funccount、stackcount）：灵活组合探针和输出，适合探索性分析和交叉验证。简单问题用单用途，复杂调查用多用途。"),
        ("为什么说「用 trace 做简单计数是大材小用」？", "trace 是多用途逐行打印工具，参数格式复杂（需指定函数、格式字符串、参数），输出冗余（逐行而非聚合）。简单计数用 funccount 一行搞定，开销更低、输出更清晰。trace 的价值在于看每次事件的细节，不是做频率统计。"),
        ("HFT 排障时如何组合单用途和多用途工具？", "第一步：用单用途工具（runqlat/biolatency）快速定位异常资源域。第二步：用多用途工具（stackcount/argdist）对异常路径做深度分析。第三步：用 trace 逐行追踪关键路径确认根因。从「哪里有问题」到「为什么有问题」逐步钻取。"),
    ],
))

NOTES.append((
    "chapter-04-bcc/notes/section-4-四大多用途工具.md",
    [
        ("在高频事件上用 trace 逐行打印", "trace 每次命中都打印一行到用户态，高频事件（如 vfs_read）会导致输出爆炸和系统减速；高频应用 funccount/argdist 聚合"),
        ("混淆 stackcount 和 profile 的触发方式", "stackcount 由你指定的事件触发（如某函数入口），profile 由定时器采样触发；回答的问题不同——「谁走了这条路径」vs「CPU 时间在哪」"),
        ("忽视 argdist 的直方图模式", "argdist 默认做频率计数，但直方图模式（-H）能看参数分布——如 recv 大小分布，对 HFT 判断「大量小包」很有用"),
    ],
    [
        ("BCC 四大多用途工具分别回答什么问题？", "funccount：「某函数/事件被调了多少次？」（高频适用，Map 聚合）。stackcount：「哪些调用栈路径触发了事件？」（中高频，栈聚合+火焰图）。trace：「每次事件的细节是什么？」（低频，逐行打印）。argdist：「参数/返回值的分布是什么？」（中高频，频率或直方图）。"),
        ("事件频率如何决定工具选择？", "高频（每秒万次+）：funccount/argdist（Map 聚合，不逐行上报）。中高频：stackcount（栈聚合）。低频（每秒<千次）：trace（逐行打印细节）。选错工具的后果：高频用 trace → 输出爆炸+系统减速；低频用 funccount → 信息不够。"),
        ("HFT 场景中 argdist 的直方图模式有什么价值？", "用 `-H` 模式看参数分布：recv 的 size 分布判断是否大量小包（HFT 大忌）、write 的 size 分布判断 I/O 模式、malloc 的 size 分布判断内存分配特征。直方图按 2 的幂分桶，一眼看出分布形态，比平均值更有信息量。"),
    ],
))

NOTES.append((
    "chapter-04-bcc/notes/section-5-规范的工具文档.md",
    [
        ("忽视工具的 --help 和 man 页面", "BCC 工具有详细的 man 页面和 --help 输出，包含参数说明和示例；不看文档靠猜参数会浪费时间"),
        ("混淆工具的选项和示例中的占位符", "文档示例中的函数名、PID 是占位符，需替换为实际值；直接复制示例可能因目标不存在而无输出"),
        ("不看工具的 EXAMPLES 部分", "BCC man 页面有 EXAMPLES 段落展示典型用法；很多工具的隐藏功能只在示例中体现"),
    ],
    [
        ("BCC 工具文档的标准结构包含哪些部分？", "SYNOPSIS（命令格式）、DESCRIPTION（功能描述）、OPTIONS（参数说明）、EXAMPLES（典型用法示例）、OVERHEAD（开销说明）、SOURCE（源码位置）、OS（支持的内核版本）、STABILITY（稳定性说明）、SEE ALSO（相关工具）。"),
        ("如何快速了解一个不熟悉的 BCC 工具？", "(1) `tool --help` 看参数概要；(2) `man tool`（或 `man 8 tool`）看完整文档；(3) 重点看 EXAMPLES 段落的典型用法；(4) 看 OVERHEAD 了解开销是否可接受；(5) 看 SOURCE 找到源码（/usr/share/bcc/tools/）了解实现细节。"),
        ("BCC 工具的 OVERHEAD 字段对 HFT 有什么意义？", "OVERHEAD 说明工具的预期开销（如「低开销，适合长期运行」或「高开销，仅短跑」）。HFT 场景特别关注：(1) 工具是否影响被测路径的延迟；(2) 是否适合在最低延迟核上运行；(3) 是否需要短跑（seconds）还是可长期挂载（minutes+）。"),
    ],
))

NOTES.append((
    "chapter-04-bcc/notes/section-6-BCC调试与排障.md",
    [
        ("BCC 工具无输出时不排查根因", "常见原因：目标函数不存在、PID 错误、权限不足、事件未触发；应逐一排查而非反复重试"),
        ("忽视 BPF verifier 拒绝的错误信息", "verifier 拒绝时输出详细的拒绝原因（如「invalid bpf_context access」），这些信息是调试 BPF C 代码的关键线索"),
        ("在错误的内核版本上使用工具", "BCC 工具依赖特定内核功能（如某些 tracepoint 在旧内核不存在）；查看工具的 OS 字段确认兼容性"),
    ],
    [
        ("BCC 工具无输出的常见原因有哪些？如何排查？", "(1) 目标函数不存在：用 `bpftrace -l 'kprobe:func*'` 验证；(2) PID 错误：用 `ps` 确认进程在运行；(3) 权限不足：需要 root 或 CAP_BPF；(4) 事件未触发：用 strace 先确认事件确实发生；(5) 过滤条件太严：放宽 filter 重试。"),
        ("BPF verifier 拒绝程序时如何调试？", "Verifier 输出包含拒绝点（指令编号、寄存器状态、访问的偏移）。常见原因：(1) 指针未做 bounds check；(2) 循环无确定上界；(3) 栈溢出（>512B）；(4) Map 操作类型不匹配。用 `bcc -e` 或 `bpftool prog dump` 看生成的字节码辅助调试。"),
        ("HFT 排障中 BCC 工具报「probe not found」怎么办？", "(1) 确认内核版本——函数可能在旧内核叫不同名字（用 `cat /proc/kallsyms | grep func`）；(2) 改用 tracepoint 替代 kprobe（`ls /sys/kernel/debug/tracing/events/`）；(3) 检查函数是否被 inline 或优化掉（编译时可能不存在独立符号）；(4) 考虑用 uprobe 在用户态追踪等效路径。"),
    ],
))

NOTES.append((
    "chapter-04-bcc/notes/section-7-BCCvsbpftrace预告.md",
    [
        ("以为 BCC 和 bpftrace 是竞争关系", "两者互补：BCC 适合复杂工具开发和团队标准化，bpftrace 适合快速 one-liner 和探索性分析"),
        ("忽视 bpftrace 的学习曲线", "bpftrace 的 DSL 语法简洁但需要理解探针类型、变量作用域、Map 操作；投资学习后效率远超 BCC"),
        ("在新项目中选择 BCC 而非 bpftrace", "新项目（2024+）推荐 bpftrace 或 libbpf+CO-RE；BCC 的运行时编译模型逐渐被视为遗留方案"),
    ],
    [
        ("BCC 和 bpftrace 的核心区别是什么？", "BCC：Python+C 框架，运行时编译，适合复杂工具开发，部署重（需 Clang+headers），启动慢（1-3s）。bpftrace：专用 DSL，预编译探针描述，适合 one-liner 和快速分析，部署轻（单一二进制），启动快。互补关系，不是替代。"),
        ("在什么场景下应该选择 bpftrace 而非 BCC？", "(1) 快速验证假设（one-liner 秒级出结果）；(2) 简单聚合/直方图（count/sum/hist）；(3) 临时排障无需持久化工具；(4) 环境无 Clang/headers；(5) 新项目（bpftrace 更现代、社区更活跃）。"),
        ("HFT 团队如何组合使用 BCC 和 bpftrace？", "日常排障：bpftrace one-liner 快速定位（秒级启动）。标准化监控：BCC 工具（biolatency/runqlat）做固定指标采集。深度分析：BCC + Python 后处理做复杂关联。工具开发：复杂逻辑用 BCC（可写完整 Python），简单逻辑用 bpftrace。"),
    ],
))

# ════════════════════════════════════════════════════════════════════
# Ch5 bpftrace (11 notes)
# ════════════════════════════════════════════════════════════════════

NOTES.append((
    "chapter-05-bpftrace/notes/section-1-bpftrace是什么.md",
    [
        ("把 bpftrace 当作编程语言", "bpftrace 是声明式追踪 DSL，不是通用编程语言；没有复杂控制流、没有函数定义、没有面向对象——追求简洁而非完备"),
        ("忽视 bpftrace 的安装依赖", "bpftrace 依赖 libbpf、bcc 库（部分版本）、BTF 支持；某些发行版需手动安装或从源码编译"),
        ("以为 bpftrace 不需要 root", "bpftrace 需要 root 或 CAP_BPF/CAP_PERFMON；普通用户无法加载 BPF 程序"),
    ],
    [
        ("bpftrace 是什么？它的设计理念是什么？", "bpftrace 是基于 BPF 的高级追踪语言（DSL），灵感来自 awk 和 DTrace。设计理念：用最简洁的语法描述「探针 + 过滤 + 动作」，让分析师专注观测逻辑而非底层工程。一行命令即可完成传统工具数十行代码的工作。"),
        ("bpftrace 的基本语法结构是什么？", "`probe /filter/ { actions }`。例如 `kprobe:vfs_read /pid == 1234/ { @count++; }`。probe 是事件触发点，filter 是可选条件，actions 是命中时执行的语句。多个 probe 可在一个脚本中定义。"),
        ("bpftrace 相比 BCC 的最大优势是什么？", "简洁性——bpftrace one-liner 可以替代 BCC 数十行 Python+C 代码。例如统计函数调用次数：bpftrace `kprobe:do_sys_open { @++ }` vs BCC 需要写 Python 前端 + C BPF 程序 + Map 定义。代价是灵活性不如 BCC。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-2-核心架构与编译流程.md",
    [
        ("混淆 bpftrace 的编译阶段和 BCC 的运行时编译", "bpftrace 自己用 LLVM 把 DSL 编译为 BPF 字节码，不依赖 BCC 的 Clang 运行时；两者编译路径不同"),
        ("忽视 bpftrace 对 BTF 的依赖", "CO-RE 模式需要内核提供 BTF 信息；无 BTF 的旧内核只能用 kprobe args->arg0 等低级访问方式"),
        ("以为 bpftrace 脚本修改后可热加载", "bpftrace 脚本修改后需要重新运行（重新编译+加载）；不能修改正在运行的 BPF 程序"),
    ],
    [
        ("bpftrace 的编译流程有哪几个阶段？", "(1) 解析 DSL 语法为 AST；(2) 语义分析（类型检查、探针验证）；(3) LLVM IR 生成 → BPF 字节码；(4) 加载到内核（经 verifier 验证）；(5) JIT 编译为原生指令执行。整个过程在 bpftrace 进程内完成，不需要外部 Clang。"),
        ("bpftrace 和 BCC 的编译模型有什么本质区别？", "BCC：运行时调用 Clang 编译 BPF C 源码，需要 kernel-headers + libclang。bpftrace：内置 LLVM 编译器，把 DSL 直接编译为 BPF 字节码，不需要 kernel-headers（CO-RE 模式靠 BTF）。bpftrace 启动更快、部署更轻。"),
        ("bpftrace 在无 BTF 的内核上能工作吗？有什么限制？", "可以工作，但有限制：(1) 无法用 CO-RE 访问结构体成员（如 `task->pid`）；(2) 只能用 kprobe 的 `arg0`-`arg5` 位置参数（依赖内核版本和调用约定）；(3) tracepoint 仍可用（有 format 文件）。建议升级到 5.x+ 内核启用 BTF。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-3-全栈事件源.md",
    [
        ("只关注 kprobe/uprobe 忽视其他事件源", "bpftrace 支持 tracepoint、USDT、interval、profile、hardware PMU、software event 等；不同问题应选不同事件源"),
        ("混淆 interval 和 profile 探针", "interval:s:5 每 5 秒触发一次（固定间隔），profile:hz:99 每秒采样 99 次（CPU 采样）；用途完全不同"),
        ("忽视 BEGIN/END 的初始化和收尾作用", "BEGIN 用于初始化变量和输出表头，END 用于收尾打印和清理；不利用这两个探针会导致输出格式混乱"),
    ],
    [
        ("bpftrace 支持哪些事件源类型？", "(1) kprobe/kretprobe——内核函数入口/返回；(2) uprobe/uretprobe——用户态函数；(3) tracepoint——内核静态探针；(4) USDT——用户态静态探针；(5) profile:hz:N——CPU 定时采样；(6) interval:s:N——固定间隔触发；(7) BEGIN/END——脚本启停；(8) hardware/software PMU 事件。"),
        ("profile:hz:99 和 interval:s:5 有什么区别？", "profile:hz:99：每秒在当前 CPU 上采样 99 次（基于定时器中断），用于 CPU 热点分析——采样到的是「此刻 CPU 在执行什么」。interval:s:5：每 5 秒在某个 CPU 上触发一次，用于定时输出汇总——不关心 CPU 在做什么，只用作周期性触发器。"),
        ("BEGIN 和 END 探针在 bpftrace 脚本中有什么用途？", "BEGIN：脚本启动时执行一次，用于初始化（如打印表头、设置时间基准 `@start = nsecs`）。END：脚本退出（Ctrl-C）时执行，用于收尾输出（如 `print(@map)` 自定义格式、计算总耗时）。不使用 BEGIN/END 时 Map 默认在退出时自动打印，但格式不可控。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-4-编程语法结构.md",
    [
        ("filter 写错导致无输出或全输出", "filter 是布尔表达式，写 `pid = 1234`（赋值）而非 `pid == 1234`（比较）是常见错误；bpftrace 对此不一定报错"),
        ("在 actions 中写复杂控制流", "bpftrace 支持 if/else 和有限循环，但复杂逻辑应拆分为多个 probe 或用 Map 聚合；verifier 会拒绝过深的逻辑"),
        ("忽视多条语句的分号", "actions 中每条语句必须以分号结尾；漏分号在某些版本会报语法错误"),
    ],
    [
        ("bpftrace 的基本语法形式是什么？", "`probe /filter/ { actions }`。probe 指定事件源（如 `kprobe:vfs_read`），filter 是可选的布尔条件（如 `/pid == 1234/`），actions 是花括号内以分号分隔的语句（如 `{ @bytes = sum(arg2); }`）。"),
        ("filter 表达式中 `=` 和 `==` 有什么区别？写错了会怎样？", "`==` 是比较运算符，`= `是赋值。`/pid == 1234/` 正确——只追踪 PID 1234。`/pid = 1234/` 错误——会尝试赋值，可能不报错但行为不可预期（filter 恒真或编译警告）。始终用 `==` 做比较。"),
        ("bpftrace 的语法糖 BEGIN/END/interval 各适合什么场景？", "BEGIN：初始化（`@start = nsecs;` 记录基准时间、打印表头）。END：收尾（`print(@latency)` 自定义输出格式、计算 `(@end - @start) / 1000000` 总耗时）。interval:s:5：定期输出（每 5 秒 `print(@count); clear(@count)` 实现滚动窗口统计）。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-5-三大变量类型.md",
    [
        ("混淆 $ 临时变量和 @ Map 变量", "$ 变量只在当前 probe 块内有效（不可跨事件），@ 变量持久存储在 Map 中（跨事件共享）；用 $ 做 Map key 不会持久化"),
        ("以为内置变量可以修改", "内置变量（pid/comm/nsecs 等）是只读上下文，不能赋值；尝试 `pid = 0` 会报错"),
        ("忽视 Map 的自动打印行为", "脚本退出时所有 @ Map 自动打印；如果不想要默认输出，在 END 中 clear() 或只用 print() 控制输出"),
    ],
    [
        ("bpftrace 的三大变量类型分别是什么？各有什么特点？", "(1) 内置变量（无前缀）：只读上下文，如 pid/tid/comm/nsecs/cpu/arg0-5/kstack/ustack，探针触发时自动填充。(2) 临时变量（$ 前缀）：当前 probe 块内局部，如 `$start = nsecs;`，跨事件无效。(3) Map 变量（@ 前缀）：跨事件持久存储，如 `@count[comm] = count()`，脚本退出时自动打印。"),
        ("如何用 Map 变量实现「记录函数入口时间、出口算延迟」？", "```bpftrace\nkprobe:do_sys_open { @start[tid] = nsecs; }\nkretprobe:do_sys_open /@start[tid]/ { @latency = hist(nsecs - @start[tid]); delete(@start[tid]); }\n```\n用 tid 作 key 存入口时间，retprobe 时取出算差值，用 hist() 画延迟直方图，delete 清理避免 Map 膨胀。"),
        ("Map 变量的自动打印行为是什么？如何控制？", "脚本退出（Ctrl-C）时，bpftrace 自动遍历所有 @ Map 并打印内容。控制方式：(1) END 块中 `print(@map)` 自定义输出；(2) `clear(@map)` 清空不打印；(3) `delete(@map[key])` 删除特定条目；(4) 用 interval 定期 `print(); clear()` 实现滚动窗口。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-6-Map聚合函数.md",
    [
        ("混淆 count() 和 sum()", "count() 每次命中 +1（计数），sum(x) 每次命中加 x 的值（求和）；统计调用次数用 count，统计总字节数用 sum"),
        ("忽视 hist() 和 lhist() 的区别", "hist() 按 2 的幂自动分桶，lhist() 可指定线性区间和步长；HFT 延迟分布用 hist() 看整体形态，lhist() 看特定区间细节"),
        ("Map key 设计不当导致膨胀", "用 pid 或 comm 做 key 时，长时间运行会产生大量条目；应定期清理或用更有针对性的 key"),
    ],
    [
        ("bpftrace 的 Map 聚合函数有哪些？各自用途是什么？", "count()：每次命中 +1（事件计数）。sum(x)：累加 x 的值（如总字节数）。avg(x)：平均值。min(x)/max(x)：最小/最大值。hist(x)：2 的幂次直方图（延迟分布）。lhist(x,lo,hi,step)：线性直方图（指定区间和步长）。"),
        ("hist() 和 lhist() 有什么区别？HFT 延迟分析该用哪个？", "hist() 自动按 2 的幂分桶（1,2,4,8...），适合看整体分布形态。lhist(x,lo,hi,step) 指定线性区间——如 `lhist($lat, 0, 10000, 100)` 看延迟 0-10us 每 100ns 的分布。HFT 延迟分析：先用 hist() 看整体形态定位问题区间，再用 lhist() 对异常区间做精细分析。"),
        ("如何避免 Map 因 key 过多而膨胀？", "(1) 用 interval 定期 `clear(@map)` 清空旧数据；(2) 用更有针对性的 key（如按 comm 而非 pid，减少条目数）；(3) 用 `delete(@map[key])` 在用完后清理（如 retprobe 中删除 start[tid]）；(4) 限制脚本运行时长。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-7-常用内置函数.md",
    [
        ("str() 用于非字符串参数", "str() 把指针参数当作字符串读取，如果指针指向的不是以 null 结尾的字符串会读到垃圾数据或触发 verifier 拒绝"),
        ("忽视 printf 的性能影响", "printf 每次调用都把数据送到用户态，高频 probe 中使用会导致严重开销；聚合用 Map，只在低频探针或 END 中用 printf"),
        ("混淆 kstack 和 ustack 的使用场景", "kstack 获取内核调用栈，ustack 获取用户态调用栈；分析内核路径用 kstack，分析应用逻辑用 ustack，两者可同时使用"),
    ],
    [
        ("bpftrace 常用的内置函数有哪些？", "str(ptr)：把指针读取为字符串。printf(fmt, args)：格式化打印（低频用）。join(ptr)：打印字符串数组。kstack/ustack：获取内核/用户栈。ntop(ipaddr)：IP 地址转字符串。time(fmt)：打印时间戳。system(cmd)：执行系统命令（慎用，有竞态风险）。"),
        ("str() 函数使用时有什么陷阱？", "str() 把指针参数当作 C 字符串读取到用户态，如果：(1) 指针指向非字符串数据（如二进制结构），会读到垃圾；(2) 字符串未以 null 结尾，可能越界读；(3) 指针来自用户空间需要 `str(uptr(arg0))` 而非 `str(arg0)`。verifier 会检查但不是所有情况都能拦截。"),
        ("为什么高频 probe 中不能用 printf？应该用什么替代？", "printf 每次调用都通过 ring buffer 把数据送到用户态打印，高频 probe（如每秒万次的 vfs_read）会导致 ring buffer 溢出和 CPU 开销。替代方案：用 Map 聚合（`@[comm] = count()`），在 END 或 interval 中用 `print(@map)` 一次性输出。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-8-控制流限制.md",
    [
        ("写无限循环导致 verifier 拒绝", "BPF verifier 要求循环有确定的上界（bounded loop），无限循环会被拒绝；bpftrace 的 while/until 需要有明确退出条件"),
        ("以为 bpftrace 支持函数定义", "bpftrace 不支持用户自定义函数（只支持内置函数和内联表达式）；复杂逻辑需拆分为多个 probe 或用 Map 关联"),
        ("在 probe 块中做复杂条件判断", "复杂 if-else 嵌套会增加 verifier 验证路径数，可能超限；应尽量用 filter 在进入 probe 前过滤"),
    ],
    [
        ("bpftrace 的控制流有哪些限制？为什么？", "(1) 循环必须有确定上界（verifier 要求 bounded loop）；(2) 不支持用户自定义函数；(3) 不支持 goto；(4) if/else 嵌套深度有限（verifier 路径爆炸）。这些限制源于 BPF verifier 的安全保证——必须在有限时间内确认程序会终止且不越界。"),
        ("为什么 BPF verifier 要求循环有上界？", "Verifier 通过模拟执行来验证安全性（检查所有可能的路径），无限循环会让模拟永不终止。Linux 5.3+ 支持 bounded loop（verifier 能推断循环次数上限），但仍不支持无限循环。替代方案：用 unroll 编译指令或 Map + interval 模式实现「循环效果」。"),
        ("复杂条件判断应该放在 filter 还是 actions 中？为什么？", "尽量放在 filter（`/condition/`）中。Filter 在进入 probe 块前求值，不命中的事件直接跳过，不执行 BPF 程序体。放在 actions 中的 if-else 每次命中都执行完整 BPF 程序再分支，增加开销和 verifier 验证路径。原则：过滤条件放 filter，业务逻辑放 actions。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-9-调试与排障.md",
    [
        ("不使用 -d 调试标志", "bpftrace -d 打印编译后的 BPF 字节码和 AST，是调试语法和逻辑问题的首选工具；不使用 -d 只能靠猜"),
        ("忽视 verifier 错误信息", "verifier 拒绝时输出详细日志（指令号、寄存器状态、访问偏移），这些信息是修复 BPF 程序的关键线索"),
        ("混淆 bpftrace 语法错误和 verifier 拒绝", "语法错误在编译阶段暴露（bpftrace 报错），verifier 拒绝在加载阶段暴露（内核报错）；两者排查方式不同"),
    ],
    [
        ("bpftrace 调试有哪几个层次？分别用什么工具？", "(1) 语法/语义错误：直接看 bpftrace 报错信息（行号+原因）。(2) 编译结果检查：`bpftrace -d script.bt` 打印 AST + BPF 字节码。(3) Verifier 拒绝：看内核日志（`dmesg`）中的 verifier 输出。(4) 运行时行为：在脚本中加 `printf()` 或用 `bpftrace -v` verbose 模式。"),
        ("bpftrace -d 标志输出什么？如何利用？", "`-d` 输出两阶段信息：(1) AST（抽象语法树）——验证语法解析是否正确、probe/filter/actions 是否符合预期；(2) BPF 字节码——看生成的指令序列，检查 Map 操作、helper 调用、寄存器使用。调试流程：先看 AST 确认语义，再看字节码确认编译结果。"),
        ("verifier 拒绝时如何排查？", "(1) 看 `dmesg` 中的 verifier 日志（拒绝的指令号、寄存器状态、访问偏移）；(2) 常见原因：指针未 bounds check、栈溢出、Map 操作不匹配、循环无上界；(3) 用 `-d` 看字节码定位问题指令；(4) 简化脚本——减少复杂逻辑，逐步增加功能定位问题。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-10-经典One-Liners速览.md",
    [
        ("直接复制 one-liner 不修改参数", "one-liner 中的函数名、PID 是示例值，需替换为实际目标；直接运行可能因目标不存在而无输出"),
        ("忽视 one-liner 的开销评估", "某些 one-liner（如全系统 syscall 追踪）开销很大；在生产环境运行前应评估事件频率和 probe 开销"),
        ("只记 one-liner 不理解原理", "one-liner 是速查工具，理解 probe 类型、Map 操作、聚合函数的原理才能灵活变通解决新问题"),
    ],
    [
        ("5 个最常用的 bpftrace one-liner 是什么？", "(1) 统计函数调用次数：`kprobe:do_sys_open { @++ }`；(2) 按进程统计 syscall：`tracepoint:raw_syscalls:sys_enter { @[comm] = count() }`；(3) 函数延迟直方图：`kprobe:vfs_read { @start[tid]=nsecs } kretprobe:vfs_read /@start[tid]/ { @lat=hist(nsecs-@start[tid]) }`；(4) CPU 热点采样：`profile:hz:99 { @[kstack] = count() }`；(5) 新进程追踪：`tracepoint:sched:sched_process_exec { printf(\"%s\\n\", comm) }`。"),
        ("如何把 one-liner 改编为自己的追踪脚本？", "(1) 替换 probe 目标——把 `do_sys_open` 改成你关心的函数；(2) 添加 filter——`/pid == 1234/` 只看特定进程；(3) 修改聚合方式——`count()` → `sum(arg2)` → `hist(nsecs-@start[tid])`；(4) 自定义输出——加 `BEGIN` 打印表头，`END` 用 `print()` 格式化。"),
        ("HFT 排障中最有用的 bpftrace one-liner 是什么？", "延迟分布直方图：`kprobe:tcp_sendmsg { @s[tid]=nsecs } kretprobe:tcp_sendmsg /@s[tid]/ { @lat=hist(nsecs-@s[tid]); delete(@s[tid]) }`——一行命令看到 sendmsg 延迟的完整分布，立即判断是否有微秒级异常。配合 `/comm == \"myapp\"/` 过滤只看目标进程。"),
    ],
))

NOTES.append((
    "chapter-05-bpftrace/notes/section-11-PartII预告Ch6.md",
    [
        ("以为 bpftrace 语法学完就够了", "bpftrace 是工具，核心能力在于理解各资源域（CPU/内存/IO/网络）的观测方法论；后续章节按资源域展开工具使用"),
        ("忽视 Part I（理论）和 Part II（实践）的关系", "Part I 学语法和机制，Part II 按资源域学方法论；跳过 Part I 直接用 Part II 工具会知其然不知其所以然"),
        ("试图一次性学完所有资源域", "6 个资源域（CPU/内存/文件系统/磁盘IO/网络/安全）各有深度，应按 HFT 优先级聚焦 CPU 和网络两个核心域"),
    ],
    [
        ("Part I（Ch1-5）和 Part II（Ch6+）的内容分别是什么？", "Part I：BPF/bpftrace 的理论基础——概念、技术背景、BCC 框架、bpftrace 语言。学完后能写 one-liner 但不知道该看什么。Part II：按资源域展开的实践——CPU(Ch6)、内存(Ch7)、文件系统(Ch8)、磁盘IO(Ch9)、网络(Ch10)等。学完后能针对具体问题选对工具和方法论。"),
        ("HFT 学习者应该按什么顺序学习 Part II？", "优先级排序：(1) Ch6 CPU——延迟分析的核心（runqlat/offcputime/profile）；(2) Ch10 网络——收发路径分析（tcpretrans/tcpconnlat）；(3) Ch9 磁盘IO——如用本地存储（biolatency/biosnoop）；(4) Ch7 内存——如用大页/NUMA（memleak/kmem）；(5) Ch8 文件系统——HFT 通常最小化文件 IO。"),
        ("从 bpftrace 语法到实际排障，最大的跨越是什么？", "从「知道怎么写」到「知道写什么」。语法学会了不代表知道该 attach 哪个 probe、用哪个聚合函数、看什么指标。这个跨越需要：(1) 理解各资源域的观测方法论（USE 方法等）；(2) 积累常见问题的排查路径；(3) 理解工具输出的含义和限制。"),
    ],
))

# ════════════════════════════════════════════════════════════════════
# Ch6 CPUs (11 notes)
# ════════════════════════════════════════════════════════════════════

NOTES.append((
    "chapter-06-cpus/notes/section-1-本章要回答的两个问题.md",
    [
        ("只关注 CPU 利用率忽视饱和度", "利用率高不一定有问题（可能是计算密集型正常负载），饱和度（排队等待）才是延迟的根因；HFT 更关心饱和度"),
        ("混淆 on-CPU 和 off-CPU 分析", "on-CPU 分析看「CPU 在执行什么」（热点），off-CPU 分析看「线程为何不在 CPU 上」（等待原因）；两者互补"),
        ("忽视 CPU 亲和性对 HFT 的影响", "HFT 关键线程应绑定到独立核（isolcpus+taskset），避免迁移和上下文切换开销；不设亲和性会导致 cache miss 和调度抖动"),
    ],
    [
        ("Ch6 CPU 章节要回答的两个核心问题是什么？", "(1) CPU 时间花在哪里？（on-CPU 分析——用 profile 采样、火焰图定位热点）；(2) CPU 时间没花在哪里、为什么？（off-CPU 分析——用 offcputime 看等待原因、runqlat 看排队延迟）。两个问题分别对应「利用率」和「饱和度」。"),
        ("CPU 利用率和饱和度有什么区别？HFT 更关心哪个？", "利用率（utilization）：CPU 在执行任务的比例——高利用率不一定是问题（如计算密集型任务）。饱和度（saturation）：任务排队等待 CPU 的程度——饱和度高意味着延迟增加。HFT 更关心饱和度，因为即使利用率不高，偶尔的排队等待也会造成微秒级延迟尖刺。"),
        ("on-CPU 和 off-CPU 分析分别解决什么问题？", "on-CPU 分析：线程在 CPU 上执行时，时间花在哪些函数（`profile`/`stackcount`）——定位 CPU 热点。off-CPU 分析：线程不在 CPU 上执行时，在等待什么（`offcputime`）——定位阻塞原因（IO、锁、调度排队）。HFT 延迟分析需要两者结合：on-CPU 看计算耗时，off-CPU 看等待耗时。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-2-CPU基础知识.md",
    [
        ("混淆 CPU 核数和硬件线程数", "一个物理核可支持超线程（SMT），2 个硬件线程共享一个物理核的执行单元；HFT 通常关闭 SMT 以避免资源争用"),
        ("忽视 NUMA 对延迟的影响", "跨 NUMA 节点访问内存比本地节点慢 30-50%；HFT 应确保线程和内存在同一 NUMA 节点（numactl --membind）"),
        ("混淆 CPU 频率缩放和功耗管理", "CPU 频率动态调节（cpufreq/governor）会导致指令执行速度变化；HFT 应锁定最高频率（performance governor）避免 DVFS 抖动"),
    ],
    [
        ("物理核、硬件线程（SMT）、NUMA 节点的关系是什么？HFT 如何配置？", "一个物理核可支持超线程（SMT），2 个硬件线程共享执行单元。多核 CPU 分为多个 NUMA 节点，跨节点访问内存更慢。HFT 配置：(1) 关闭 SMT（避免资源争用）；(2) 用 isolcpus 隔离关键核；(3) 用 numactl 绑定线程和内存到同一 NUMA 节点；(4) 锁定 CPU 频率（performance governor）。"),
        ("为什么 HFT 要关闭 CPU 频率缩放（DVFS）？", "DVFS（动态电压频率调节）会根据负载改变 CPU 频率——低负载时降频省电，但频率切换有延迟（微秒级），导致指令执行速度不稳定。HFT 要求每条指令的执行时间可预测，应设 `cpufreq governor = performance` 锁定最高频率，消除 DVFS 抖动。"),
        ("CPU 迁移（migration）对 HFT 有什么影响？如何避免？", "线程从 CPU A 迁移到 CPU B 时，L1/L2/L3 cache 全部 miss，需要重新预热，导致数百纳秒到微秒级的额外延迟。避免方法：(1) `taskset -c N` 绑定到固定核；(2) `isolcpus=` 内核参数隔离关键核；(3) 关闭 `CONFIG_NO_HZ_FULL` 之外的定时器中断；(4) 设置 `sched_setaffinity`。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-3-传统CPU分析工具.md",
    [
        ("只依赖 top/htop 做 CPU 分析", "top 只显示聚合利用率，看不到函数级热点和等待原因；应配合 perf（采样）和 BPF（精确追踪）使用"),
        ("忽视 mpstat 的 per-CPU 维度", "mpstat -P ALL 能看每个 CPU 核的利用率分布；如果某核 100% 而其他空闲，可能是单线程瓶颈或中断集中在某核"),
        ("用 vmstat 看 CPU 问题时忽视 run queue 长度", "vmstat 的 r 列是 run queue 长度，r > CPU 核数说明有饱和；HFT 应关注 r 是否有偶发性尖峰"),
    ],
    [
        ("传统 CPU 分析工具有哪些？各自能看什么？", "(1) top/htop：进程级 CPU 利用率（粗粒度）；(2) mpstat -P ALL：per-CPU 利用率（用户/系统/IO 等待/空闲）；(3) vmstat：系统级 CPU+内存+IO 概览（r 列=run queue）；(4) pidstat：per-进程 CPU/内存/IO；(5) perf stat/top/record：硬件计数器+调用栈采样。"),
        ("传统工具相比 BPF 的盲区有哪些？", "(1) 极短命进程（top 采样不到→需 execsnoop）；(2) 运行队列等待延迟（mpstat 只见忙闲→需 runqlat 直方图）；(3) off-CPU 原因（perf 默认 on-CPU→需 offcputime）；(4) per-进程 LLC 命中率（perf stat 粗粒度→需 llcstat）；(5) 精确事件级追踪（传统工具无→需 bpftrace/BCC trace）。"),
        ("HFT 排障中如何组合使用传统工具和 BPF 工具？", "第一步（快速概览）：mpstat -P ALL + vmstat 看系统级异常（某核 100%、r 尖峰）。第二步（定位范围）：pidstat 看哪个进程异常。第三步（深度分析）：BPF 工具钻取——runqlat 看排队延迟分布、offcputime 看等待原因、profile 采样看热点函数。传统工具做「有没有问题」，BPF 做「为什么有问题」。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-4-BPF相对传统工具的优势.md",
    [
        ("以为 BPF 完全替代传统工具", "BPF 补的是传统工具的盲区，不是替代；top/mpstat 做快速概览仍然有用，BPF 做深度钻取"),
        ("忽视 BPF 工具的开销差异", "BPF 工具开销从低到高：funccount(Map 聚合) < profile(采样) < trace(逐行) < stackcount(栈聚合)；选错工具会放大开销"),
        ("在不需要精确度时用 BPF 重工具", "如果 top 已经能看出问题（如某进程 100% CPU），不需要用 BPF trace 逐行追踪；先用轻工具确认范围再用重工具钻取"),
    ],
    [
        ("BPF 相比传统工具能补哪些盲区？", "(1) 极短命进程：top 采样不到→execsnoop 逐事件追踪；(2) 运行队列延迟：mpstat 只见忙闲→runqlat 直方图看排队时间分布；(3) off-CPU 原因：perf 默认 on-CPU→offcputime 看阻塞原因；(4) per-进程 LLC：perf stat 粗粒度→llcstat 精确计数；(5) 任意函数级追踪：传统工具做不到→bpftrace kprobe/uprobe。"),
        ("BPF 工具的开销排序是什么？HFT 如何选择？", "从低到高：(1) funccount/argdist（Map 聚合，per-hit 开销极低）；(2) profile:hz:99（定时采样，固定开销）；(3) stackcount（栈聚合，per-hit 有 stackid 开销）；(4) trace（逐行打印，per-hit 有 ring buffer 开销）。HFT 原则：热路径用 Map 聚合工具，冷路径可用 trace；所有工具用完即撤。"),
        ("什么时候应该用传统工具而非 BPF？", "(1) 快速概览系统状态——top/mpstat 秒级出结果，BCC 工具有编译延迟；(2) 确认问题是否存在——如果 top 已显示某进程 100% CPU，不需要 BPF；(3) 长期监控——传统工具开销固定且可预测，BPF 工具不适合长期挂载；(4) 无 root 权限——传统工具普通用户可用，BPF 需要 root。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-5-进程与线程生命周期.md",
    [
        ("忽视短命进程对延迟的影响", "HFT 环境中 fork/exec 产生的短命进程会抢占 CPU、引发调度抖动；execsnoop 可以发现隐藏的短命进程"),
        ("混淆线程状态转换的观测点", "线程的 ready→running 转换用 sched_wakeup tracepoint，running→blocked 用 sched_stat_sleep；用错 tracepoint 会漏事件"),
        ("忽视上下文切换的 cache 影响", "每次上下文切换都会 flush TLB、污染 cache；HFT 关键路径上的上下文切换数应最小化（isolcpus+nohz_full）"),
    ],
    [
        ("进程/线程生命周期的关键事件有哪些？bpftrace 如何追踪？", "关键事件：exec（新进程加载）、fork/clone（创建新线程/进程）、exit（退出）、sched_switch（上下文切换）、sched_wakeup（唤醒）。追踪：`tracepoint:sched:sched_process_exec`（新进程）、`tracepoint:sched:sched_switch`（上下文切换）、`tracepoint:sched:sched_process_exit`（退出）。用 `@[comm] = count()` 统计频率。"),
        ("短命进程为什么是 HFT 的隐患？如何发现？", "短命进程（存在时间 < 采样间隔）会被 top/mpstat 完全漏掉，但它们：(1) 消耗 CPU 时间片；(2) 触发上下文切换和 cache 污染；(3) 可能竞争锁和内存。发现方法：`bpftrace -e 'tracepoint:sched:sched_process_exec { printf(\"%s -> %s\\n\", strftime(\"%H:%M:%S\"), comm) }'` 或 BCC `execsnoop`。"),
        ("上下文切换对 HFT 延迟有什么影响？如何减少？", "每次上下文切换：(1) 保存/恢复寄存器（~微秒级）；(2) flush TLB；(3) L1/L2 cache 部分污染；(4) 可能跨核迁移（cache 全 miss）。减少方法：(1) isolcpus 隔离关键核；(2) nohz_full 减少定时器中断；(3) taskset 绑核避免迁移；(4) 关闭 SMT；(5) 关闭不必要的服务和守护进程。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-6-调度器与饱和度.md",
    [
        ("把 run queue 长度和调度延迟混淆", "run queue 长度是瞬时快照（r 列），调度延迟是线程从 ready 到 running 的等待时间（runqlat 测量）；长度不等于延迟"),
        ("忽视偶发调度延迟尖峰", "平均调度延迟可能正常，但偶发的毫秒级尖峰足以导致 HFT 策略超时；runqlat 直方图能看到尾部分布"),
        ("在隔离核上仍看到调度事件", "isolcpus 防止其他线程迁移到隔离核，但不阻止内核线程/定时器中断；需配合 nohz_full 和 irqaffinity 完全隔离"),
    ],
    [
        ("CPU 饱和度如何测量？runqlat 和 mpstat 的 r 列有什么区别？", "mpstat/vmstat 的 r 列是 run queue 长度的瞬时快照（采样时刻有多少线程在等 CPU）。runqlat 测量的是调度延迟——线程从变为 ready 到实际被调度运行的时间分布（直方图）。区别：r 列是「积压量」，runqlat 是「等待时间」。HFT 更关心 runqlat，因为延迟尖刺取决于等待时间而非队列长度。"),
        ("为什么平均调度延迟正常但 HFT 仍有超时？", "调度延迟分布可能有长尾——平均值 10 微秒但 99.9 分位是 500 微秒。HFT 策略的超时阈值通常是固定的（如 100 微秒），一次尾部尖峰就足以触发超时。runqlat 直方图能看到尾部分布，平均值正常不代表尾部正常。"),
        ("isolcpus 隔离后仍看到调度事件，可能的原因是什么？", "(1) 内核线程（kworker/ksoftirqd）仍在隔离核上运行——需 `rcu_nocbs=` 和 `nohz_full=`；(2) 定时器中断（timer tick）仍触发——需 `nohz_full=`；(3) 硬件中断（网卡 IRQ）路由到隔离核——需 `irqaffinity=` 设置 IRQ 亲和性；(4) isolcpus 只是阻止用户线程迁移，不阻止内核机制。完整隔离需 isolcpus + nohz_full + rcu_nocbs + irqaffinity 组合。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-7-CPU使用时间与剖析.md",
    [
        ("混淆 profile 采样频率和精确度", "99Hz 采样意味着每 ~10ms 采一次，微秒级事件可能完全采不到；profile 适合毫秒级以上的热点，不适合微秒级延迟分析"),
        ("只看 on-CPU profile 忽视 off-CPU", "on-CPU profile 只看 CPU 执行时间，如果线程大部分时间在等待（锁/IO/调度），on-CPU profile 看不到问题；需配合 offcputime"),
        ("忽视采样偏差", "采样有统计偏差——频繁调用的短函数可能采样命中少，而偶尔调用的长函数命中多；profile 结果是「采样频次」不是「执行时间」"),
    ],
    [
        ("CPU 剖析（profiling）的基本原理是什么？", "按固定频率（如 99Hz）在 CPU 上触发定时器中断，中断时记录当前指令地址（IP）和调用栈。大量采样后，某函数在采样中出现比例高 = 该函数占用了较多 CPU 时间。本质是统计采样，不是精确计时——采样频次反映 CPU 时间分布。"),
        ("on-CPU profile 和 off-CPU 分析有什么区别？什么时候需要结合使用？", "on-CPU profile：采样线程在 CPU 上执行时的栈——看「CPU 时间花在哪些函数」。off-CPU 分析：记录线程离开 CPU 时的原因和时长——看「等待时间花在什么上」。需要结合：如果 on-CPU profile 显示 CPU 利用率不高（大量空闲），但延迟仍高，说明问题在 off-CPU（等待 IO/锁/调度），需用 offcputime 钻取。"),
        ("BCC profile 工具的 99Hz 采样对 HFT 延迟分析有什么局限？", "99Hz = 每 10ms 采一次，而 HFT 延迟尖刺可能在微秒级。采样可能完全错过短事件——一个持续 5 微秒的调度延迟在 10ms 采样间隔下被命中的概率极低。HFT 延迟分析不应依赖采样，应用事件驱动的 BPF 追踪（如 runqlat 直方图、offcputime 逐事件记录）。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-8-Off-CPU时间offcputime.md",
    [
        ("把 off-CPU 时间等同于空闲", "off-CPU 包括等待 IO、等待锁、等待调度、睡眠等多种原因；不是所有 off-CPU 都是问题，但长 off-CPU 延迟需要分析"),
        ("offcputime 运行在所有进程上", "全系统 offcputime 会产生海量数据（每个上下文切换都记录）；应按 PID 或 comm 过滤目标进程"),
        ("忽视 offcputime 的开销", "offcputime 在每次上下文切换时触发 BPF 程序，高频切换环境下开销不小；HFT 环境应短跑并按进程过滤"),
    ],
    [
        ("offcputime 工具测量什么？它和 runqlat 有什么区别？", "offcputime：测量线程不在 CPU 上执行的总时间（从离开 CPU 到重新被调度的间隔），并记录离开时的调用栈和原因。runqlat：只测量调度延迟（从 ready 到 running 的等待时间）。区别：offcputime 包含所有 off-CPU 原因（IO 等待、锁等待、睡眠、调度排队），runqlat 只看调度排队这一种。"),
        ("off-CPU 分析对 HFT 延迟排查有什么价值？", "HFT 策略线程如果延迟高但 CPU 利用率低，说明时间花在了 off-CPU 等待上。offcputime 能显示：(1) 等待了多久（直方图/逐事件）；(2) 在哪个函数调用栈上等待的（定位阻塞点）；(3) 等待原因（sched_switch 的原因字段）。直接回答「线程被谁阻塞、阻塞了多久」。"),
        ("offcputime 在生产环境运行需要注意什么？", "(1) 按进程过滤：`-p $(pidof myapp)` 避免全系统追踪产生海量数据；(2) 短跑：offcputime 在每次上下文切换触发 BPF 程序，长时间运行开销累积；(3) 设置超时：`--duration N` 只显示超过 N 秒的 off-CPU 时间，过滤短等待；(4) 理解输出：栈 + 时间 + 频率，长的 off-CPU 时间是排查重点。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-9-中断与其他.md",
    [
        ("忽视软中断对 HFT 的影响", "软中断（softirq）如 NET_RX 在网络包处理时可能占用 CPU，影响 HFT 策略线程；用 runqlat + irq 查看中断影响"),
        ("混淆硬件中断和软件中断", "硬件中断由硬件触发（网卡、定时器），软件中断由内核延迟处理（softirq、tasklet）；两者都可被 BPF 追踪"),
        ("忽视定时器中断的周期性影响", "周期性定时器中断（timer tick）每毫秒触发一次调度检查，对 HFT 的微秒级延迟有影响；nohz_full 可以减少"),
    ],
    [
        ("硬件中断和软件中断有什么区别？BPF 如何追踪？", "硬件中断（hardirq）：由硬件触发（网卡 IRQ、定时器、NVMe），在 IRQ 上下文执行，不可睡眠。软件中断（softirq）：内核延迟处理机制（NET_RX 处理网络包、BLOCK 处理 IO），在 softirq 上下文执行。BPF 追踪：`irq:irq_handler_entry`（硬件中断）、`tracepoint:irq:softirq_entry`（软件中断）。"),
        ("定时器中断（timer tick）对 HFT 有什么影响？如何减少？", "定时器中断默认每秒 100-1000 次（HZ 配置），每次中断：(1) 触发调度器检查时间片；(2) 更新统计计数器；(3) 可能抢占用户态线程。HFT 微秒级延迟受周期性 tick 干扰。减少方法：`nohz_full=CPUs`（隔离核上减少 tick）、`rcu_nocbs=CPUs`（RCU 回调迁移到其他核）、`irqaffinity=`（IRQ 路由到非隔离核）。"),
        ("如何用 BPF 分析中断对 HFT 线程的影响？", "(1) 统计中断频率：`tracepoint:irq:irq_handler_entry { @[handler] = count() }`；(2) 看中断耗时：`tracepoint:irq:irq_handler_entry { @s[handler]=nsecs } tracepoint:irq:irq_handler_exit /@s[handler]/ { @lat[handler]=hist(nsecs-@s[handler]) }`；(3) 看中断与 HFT 线程调度的关系：用 sched_switch 关联中断和线程切换时间。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-10-BPF单行命令.md",
    [
        ("直接复制 one-liner 不评估开销", "CPU 相关的 one-liner 有些开销较大（如全系统 sched_switch 追踪）；运行前应评估目标事件频率"),
        ("忽视 one-liner 的输出量", "某些 one-liner（如逐事件打印 sched_switch）在繁忙系统上每秒产生数千行输出；应加 filter 或用 Map 聚合"),
        ("只记命令不理解 sched tracepoint 字段", "sched_switch 的 format 文件描述了 prev_comm/next_comm/prev_pid/next_pid 等字段；不看 format 写出的 one-liner 可能引用错误字段"),
    ],
    [
        ("CPU 分析最常用的 3 个 bpftrace one-liner 是什么？", "(1) 调度延迟直方图：`tracepoint:sched:sched_wakeup { @s[tid]=nsecs } tracepoint:sched:sched_switch /@s[args->next_pid]/ { @runqlat=hist(nsecs-@s[args->next_pid]); delete(@s[args->next_pid]) }`；(2) CPU 采样火焰图：`profile:hz:99 { @[kstack] = count() }`；(3) 上下文切换统计：`tracepoint:sched:sched_switch { @[args->prev_comm, args->next_comm] = count() }`。"),
        ("如何查看 sched_switch tracepoint 有哪些可用字段？", "`cat /sys/kernel/debug/tracing/events/sched/sched_switch/format`。常见字段：prev_comm（切出线程名）、prev_pid、prev_prio、prev_state（线程状态）、next_comm（切入线程名）、next_pid、next_prio。在 bpftrace 中用 `args->prev_comm`、`args->next_pid` 等访问。"),
        ("HFT 场景中，如何用 one-liner 快速判断调度问题？", "三步：(1) runqlat 直方图看调度延迟分布——如果尾部有毫秒级异常，说明有调度抖动；(2) sched_switch 按线程对统计——看 HFT 线程被谁抢占；(3) irq_handler 频率统计——看中断是否集中在 HFT 核上。三行命令快速定位「调度延迟→谁抢占→是否中断导致」。"),
    ],
))

NOTES.append((
    "chapter-06-cpus/notes/section-11-工具选型速查.md",
    [
        ("选工具时只看功能不看开销", "同一问题多种工具可解，但开销差异大；HFT 应优先选 Map 聚合工具（低开销）而非逐行追踪工具（高开销）"),
        ("忽视 on-CPU 和 off-CPU 的选型逻辑", "CPU 利用率高→on-CPU 分析（profile/stackcount）；CPU 利用率低但延迟高→off-CPU 分析（offcputime/runqlat）"),
        ("试图用一个工具解决所有问题", "每个工具回答一个特定问题；复杂排障需要多个工具组合，从概览到钻取逐步缩小范围"),
    ],
    [
        ("CPU 问题分析的工具选型决策树是什么？", "Step 1：CPU 利用率高？→ Yes：on-CPU 分析（profile 采样→stackcount 栈→火焰图定位热点）；No：off-CPU 分析（offcputime 看等待原因→runqlat 看调度延迟）。Step 2：有短命进程？→ execsnoop。Step 3：中断影响？→ irq_handler 追踪。Step 4：上下文切换多？→ sched_switch 统计。"),
        ("HFT CPU 排障的推荐工具组合是什么？", "快速排查（秒级）：mpstat -P ALL（看核级利用率）+ runqlat（看调度延迟分布）。深度分析（分钟级）：profile:hz:99（on-CPU 热点）+ offcputime（off-CPU 原因）+ execsnoop（短命进程）。精确追踪（秒级短跑）：bpftrace sched_switch + irq_handler。原则：从低开销概览到高开销钻取。"),
        ("如何根据 CPU 利用率判断用 on-CPU 还是 off-CPU 工具？", "CPU 利用率 > 80%（计算密集）：on-CPU 分析——用 profile/stackcount 找 CPU 热点，优化算法。CPU 利用率 < 50% 但延迟高：off-CPU 分析——用 offcputime 找等待原因（锁/IO/调度），用 runqlat 看调度延迟。两者都需要：如果 on-CPU 和 off-CPU 都没有明显异常，检查中断和内核调度策略。"),
    ],
))

# ════════════════════════════════════════════════════════════════════
# Ch10 Networking (10 notes)
# ════════════════════════════════════════════════════════════════════

NOTES.append((
    "chapter-10-networking/notes/section-1-本章要回答的问题.md",
    [
        ("只看带宽忽视延迟", "HFT 网络问题主要是延迟和抖动而非带宽；带宽利用率低不代表网络没有问题"),
        ("混淆吞吐量和延迟的观测工具", "吞吐量用计数器（ifconfig/ip -s link），延迟用追踪工具（tcpretrans/tcpconnlat）；选错工具类型会漏掉关键信息"),
        ("忽视网络栈各层的观测分工", "网络问题可能在套接字层、TCP 协议层、IP 层、网卡驱动层；不同层用不同工具，跨层分析才能定位根因"),
    ],
    [
        ("Ch10 网络章节要回答的核心问题是什么？", "(1) 网络延迟在哪里产生？（哪个层、哪个函数）；(2) 吞吐量瓶颈在哪？（带宽、丢包、重传）；(3) 连接建立是否正常？（TCP 握手延迟、失败率）。HFT 最关心延迟——从应用 send/recv 到网卡发包的每一跳都可能引入抖动。"),
        ("网络分析的层次划分是什么？每层用什么工具？", "(1) 套接字层：sockstat、BCC socketsnoop——看 connect/accept/send/recv；(2) TCP 协议层：tcpretrans、tcpconnlat、tcptop——看重传、连接延迟、吞吐；(3) IP/qdisc 层：qdisc stats、BCC tcplife——看排队和丢弃；(4) 网卡驱动层：ethtool、BCC netqos——看硬件统计和中断。"),
        ("HFT 网络排障与普通网络排障有什么区别？", "普通排障关注：带宽利用率、丢包率、连接成功率（毫秒级）。HFT 排障关注：微秒级延迟尖刺、抖动来源、尾部分布。区别：(1) 普通工具（ping/netstat）粒度太粗，HFT 需要 BPF 追踪每个包的内核路径时间；(2) HFT 更关心尾部延迟（P99.9）而非平均值；(3) HFT 关注内核网络栈的调度和中断影响。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-2-网络基础知识.md",
    [
        ("混淆 TCP 状态机的观测点", "TCP 有 11 种状态（ESTABLISHED/TIME_WAIT/SYN_SENT 等），不同状态的转换有不同的 tracepoint；不知道状态转换路径就无法选对观测点"),
        ("忽视网卡 ring buffer 的大小和溢出", "ring buffer 太小在高 PPS 下会丢包；`ethtool -g eth0` 查看，`ethtool -G eth0 rx 4096` 调大；HFT 应确保不因 ring buffer 溢出丢包"),
        ("混淆 backlog 队列和 accept 队列", "backlog 是 SYN 队列（半连接），accept 队列是已完成握手待 accept 的连接；队列满会导致丢 SYN 或连接拒绝"),
    ],
    [
        ("TCP 连接建立的关键步骤和观测点是什么？", "(1) 客户端 SYN_SENT → 发 SYN（`tracepoint:syscalls:sys_enter_connect`）；(2) 服务端收到 SYN → SYN 队列（`tracepoint:tcp:tcp_v4_syn_recv_sock`）；(3) 三次握手完成 → accept 队列（`tracepoint:syscalls:sys_enter_accept`）；(4) ESTABLISHED。tcpconnlat 测量从 connect 到 ESTABLISHED 的延迟。"),
        ("网卡 ring buffer 对 HFT 网络性能有什么影响？", "Ring buffer 是网卡和内核之间的包缓冲区。PPS（包/秒）高时，如果 ring buffer 太小，内核来不及消费就溢出丢包。HFT 场景：小包高频交易每秒可能数千包，默认 ring buffer（256）可能不够。检查：`ethtool -g eth0`；调大：`ethtool -G eth0 rx 4096 tx 4096`。"),
        ("SYN 队列（backlog）和 accept 队列满了会怎样？如何检测？", "SYN 队列满：新 SYN 包被丢弃（客户端看到 connect timeout）——`netstat -s | grep \\\"overflowed\\\"`。Accept 队列满：已完成握手的连接等待 accept，超时后拒绝——`ss -lnt` 的 Recv-Q 列非零。BPF 追踪：`tracepoint:tcp:tcp_v4_syn_recv_sock /args->status == 0/ { @drop++ }`。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-3-传统网络分析工具.md",
    [
        ("只依赖 netstat/ss 做网络分析", "netstat/ss 只显示连接状态和计数器，看不到包级延迟和重传原因；需配合 BPF 工具做深度分析"),
        ("忽视 ifconfig 统计的局限性", "ifconfig 的 RX/TX errors/dropped 是聚合计数器，不知道何时、为什么丢包；BPF 能追踪丢包时刻和路径"),
        ("用 ping 测延迟忽略内核栈开销", "ping 测的是 RTT（含内核网络栈处理），不是纯网络传输延迟；HFT 需用 BPF 分段测量各层耗时"),
    ],
    [
        ("传统网络工具有哪些？各自能看什么？", "(1) ifconfig/ip -s link：接口级 RX/TX 包数/字节/错误/丢弃；(2) netstat/ss：连接状态表（ESTABLISHED/TIME_WAIT 等）、socket 统计；(3) ping/traceroute：RTT 和路径；(4) tcpdump/wireshark：包级抓取和分析；(5) ethtool：网卡统计、ring buffer、offload。局限：聚合计数器看不到事件级细节和延迟分布。"),
        ("传统工具相比 BPF 的网络分析盲区有哪些？", "(1) 重传原因：netstat 只给重传计数→tcpretrans 追踪每次重传的时间和序列号；(2) 连接延迟：netstat 无→tcpconnlat 测每次 connect 的耗时；(3) 内核栈路径耗时：tcpdump 只看包→BPF 追踪 tcp_sendmsg→dev_queue_xmit 各层耗时；(4) 丢包位置：ifconfig 只给计数→BPF 追踪丢包发生在哪层。"),
        ("HFT 网络延迟分析为什么不能用 ping？", "Ping 测的是 ICMP RTT：(1) ICMP 走不同内核路径（不走 TCP 栈）；(2) RTT 包含两端内核处理+网络传输，无法分段定位；(3) 粒度太粗（毫秒级），HFT 需要微秒级。替代方案：用 BPF 在 tcp_sendmsg 和 tcp_recvmsg 上打时间戳，分段测量应用→内核→网卡→网络→对端→返回各段耗时。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-4-套接字层工具.md",
    [
        ("在 HFT 热路径上用 socketsnoop 逐行追踪", "socketsnoop 每次 send/recv 都打印一行，高频交易路径上会产生大量输出和开销；应用 Map 聚合或按进程过滤"),
        ("混淆 connect 延迟和 send 延迟", "connect 延迟是 TCP 握手时间（网络 RTT），send 延迟是数据从应用到内核的时间（通常微秒级）；两者原因不同"),
        ("忽视 accept 队列堆积", "如果服务端 accept 速度跟不上握手完成速度，accept 队列堆积导致延迟；ss -lnt 的 Recv-Q 非零表示堆积"),
    ],
    [
        ("套接字层的关键 BPF 工具有哪些？", "(1) socketsnoop：追踪 connect/accept/send/recv 系统调用（逐事件打印）；(2) tcpconnect：追踪主动连接建立（谁连了谁）；(3) tcpaccept：追踪被动连接接受；(4) tcplife：记录连接生命周期（开始/结束/持续时间/字节数）；(5) tcptop：按连接统计吞吐量。"),
        ("connect 延迟和 send 延迟分别说明什么问题？", "connect 延迟（tcpconnlat 测量）：TCP 三次握手耗时——反映网络 RTT 和对端响应速度。延迟大说明网络慢或对端处理慢。send 延迟（socketsnoop 测量）：send 系统调用耗时——反映内核网络栈处理速度和发送队列状态。延迟大说明内核栈慢或发送队列拥塞。HFT 两者都需关注。"),
        ("HFT 场景中如何安全地使用套接字层 BPF 工具？", "(1) 按进程过滤：`-p $(pidof myapp)` 只追踪目标进程，避免全系统开销；(2) 短跑：套接字层工具在每次 send/recv 触发，高频交易每秒数千次，短跑 5-10 秒足够采样；(3) 用 Map 聚合替代逐行：`tracepoint:syscalls:sys_enter_sendto { @[comm] = count() }` 看频率分布而非逐条；(4) 用 tcplife 看连接概览而非逐包追踪。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-5-TCP协议层工具.md",
    [
        ("把重传等同于网络拥塞", "重传可能由丢包（网络问题）或内核栈延迟（调度问题）引起；tcpretrans 能看重传时刻和序列号，但不直接告诉原因"),
        ("忽视 TCP 重传对 HFT 的微秒级影响", "一次重传至少增加一个 RTT 的延迟（通常几十微秒到毫秒）；HFT 策略循环中一次重传可能导致超时"),
        ("混淆 tcpretrans 和 tcpretrans 的追踪范围", "tcpretrans 只追踪内核 TCP 栈的重传；DPDK 用户态 TCP 栈的重传不在此工具范围内"),
    ],
    [
        ("TCP 协议层的关键 BPF 工具有哪些？", "(1) tcpretrans：追踪 TCP 重传事件（时间、源/目的、序列号、原因）；(2) tcpconnlat：测量 TCP 连接建立延迟；(3) tcptop：按连接统计发送/接收字节数；(4) tcprtt：TCP RTT 分布直方图；(5) tcpsize：读写大小分布。这些工具基于 tcp_tracepoint 或 kprobe 实现。"),
        ("TCP 重传对 HFT 延迟有什么影响？如何追踪？", "一次重传至少增加一个 RTO（重传超时，通常 200ms+）或快重传时间（3 个重复 ACK，约 1 RTT）。HFT 策略循环中一次重传可能导致策略超时。追踪：BCC `tcpretrans`——显示每次重传的时间、连接、序列号、状态。bpftrace：`tracepoint:tcp:tcp_retransmit_skb { @[src,dst] = count() }` 按连接统计重传次数。"),
        ("tcprtt 直方图对 HFT 有什么价值？", "tcprtt 显示 TCP RTT（往返时间）的分布直方图——内核根据 ACK 返回时间动态计算的 RTT 估计值。HFT 价值：(1) RTT 基线——正常 RTT 是多少，异常时偏移多少；(2) RTT 分布尾部——P99 RTT 是否远超中位数（说明有偶发网络抖动）；(3) 按连接对比——不同对端交易所的 RTT 差异。`bpftrace -e 'kprobe:tcp_rtt_estimator { @rtt = hist(arg2 / 1000) }'`。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-6-UDPDNS与其他.md",
    [
        ("忽视 DNS 解析延迟对 HFT 的影响", "HFT 启动时 DNS 解析如果走网络可能引入毫秒级延迟；应在本地缓存 /etc/hosts 或使用 IP 直连"),
        ("混淆 UDP 和 TCP 的观测方式", "UDP 无连接状态（无 connect/accept），用不同的 tracepoint 追踪；UDP 丢包不会重传，需靠应用层检测"),
        ("忽视 UDP socket buffer 大小", "UDP 不可靠，buffer 溢出直接丢包无重传；`ss -u -m` 查看 buffer 使用，`sysctl net.core.rmem_max` 调大"),
    ],
    [
        ("UDP 和 TCP 在 BPF 追踪上有什么区别？", "TCP 有连接状态机（connect/accept/retransmit 等 tracepoint 丰富）。UDP 无连接：(1) 无 connect/accept——用 sendto/recvfrom 追踪；(2) 无重传——丢包不可恢复，需追踪 udp_send_skb 和 udp_rcv 的丢包点；(3) 无 RTT——无法用 tcprtt 类工具。UDP 追踪重点：send/recv 频率、buffer 溢出丢包、DNS 解析延迟。"),
        ("DNS 解析对 HFT 有什么影响？如何优化？", "DNS 解析走网络可能引入 1-50ms 延迟（取决于 DNS 服务器和网络）。HFT 启动时如果每次连接都做 DNS 解析，会引入不可控延迟。优化：(1) /etc/hosts 预填关键地址（零 DNS 延迟）；(2) 应用层缓存 DNS 结果（首次解析后复用）；(3) 使用本地 DNS 缓存（systemd-resolved/nscd）；(4) 直接用 IP 连接（生产环境推荐）。"),
        ("如何用 BPF 追踪 UDP 丢包？", "UDP 丢包发生在：(1) socket buffer 满——`tracepoint:udp:udp_fail_queue_rcv_skb { @[reason] = count() }`；(2) 校验和错误——`kprobe:__udp4_lib_rcv /ret == 0/` 检查返回值；(3) 无 socket 匹配——`tracepoint:udp:udp_rcv { @drop++ }`。`netstat -su` 给聚合计数，BPF 能定位丢包时刻和原因。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-7-底层qdiscskb驱动.md",
    [
        ("忽视 qdisc 排队延迟", "qdisc（队列规则）是内核发送队列，包在 qdisc 中排队等待发送；排队延迟是 HFT 网络抖动的一个来源"),
        ("混淆 qdisc 丢包和网卡丢包", "qdisc 丢包发生在内核（可 BPF 追踪），网卡丢包发生在硬件（只能 ethtool 统计）；两者原因和检测方式不同"),
        ("在 HFT 服务器上用复杂 qdisc 规则", "复杂 qdisc（如 HTB/RED）增加每包处理开销；HFT 应用最简单的 pfifo_fast 或 noqueue，减少排队延迟"),
    ],
    [
        ("qdisc（队列规则）是什么？对 HFT 延迟有什么影响？", "qdisc 是内核网络发送队列的管理规则——包从 TCP 层进入 qdisc 排队，然后由网卡驱动取出发送。排队延迟 = 包在 qdisc 中等待的时间。HFT 影响：(1) 队列长时尾部延迟增加；(2) 复杂 qdisc 规则（HTB/RED）增加 per-packet 处理开销；(3) 队列满时丢包。优化：用最简单的 pfifo_fast 或 noqueue，减少队列深度。"),
        ("如何用 BPF 追踪 qdisc 延迟？", "(1) 包入队时间戳：`kprobe:dev_queue_xmit { @qdisc_start[tid]=nsecs }`；(2) 包出队时间戳：`kprobe:dev_hard_start_xmit /@qdisc_start[tid]/ { @qdisc_lat=hist(nsecs-@qdisc_start[tid]); delete(@qdisc_start[tid]) }`；(3) qdisc 丢包：`kprobe:qdisc_drop { @drop++ }`。直方图显示排队延迟分布，尾部异常说明 qdisc 有拥塞。"),
        ("HFT 网络路径上推荐的 qdisc 配置是什么？", "(1) 用 `noqueue`（如果只有单队列网卡）或 `pfifo_fast`（最简单的 FIFO）；(2) 避免使用 HTB/TBF/RED 等复杂规则——每包额外计算开销；(3) 减小 txqueuelen：`ip link set eth0 txqueuelen 100`（默认 1000，减少最大排队深度）；(4) 启用网卡 BQL（Byte Queue Limits）自动调节发送队列长度；(5) 对于 HFT 关键路径，考虑用 DPDK/XDP 绕过 qdisc。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-8-工具选型速查HFT优先.md",
    [
        ("选工具时不考虑 HFT 优先级", "HFT 网络排障应优先用低开销工具（tcpretrans/tcpconnlat）而非高开销工具（全系统 send/recv 追踪）"),
        ("忽视工具的运行时长控制", "网络 BPF 工具在高 PPS 环境下数据量爆炸；应设置运行时长或用 Map 聚合控制输出量"),
        ("试图同时运行多个网络 BPF 工具", "多个 BPF 程序同时 attach 到同一 probe 会叠加开销；应串行排查，一次只运行一个工具"),
    ],
    [
        ("HFT 网络排障的工具选型优先级是什么？", "Tier 1（低开销，长期可挂）：ethtool（网卡统计）+ ss -s（连接概览）+ netstat -s（协议计数）。Tier 2（中开销，分钟级短跑）：tcpretrans（重传追踪）+ tcpconnlat（连接延迟）+ tcprtt（RTT 分布）。Tier 3（高开销，秒级短跑）：socketsnoop（逐包追踪）+ bpftrace tcp_sendmsg 路径分析。从 Tier 1 到 Tier 3 逐步钻取。"),
        ("如何控制网络 BPF 工具的输出量？", "(1) 按进程过滤：`-p $(pidof myapp)`；(2) 按端口过滤：`/args->dport == 8080/`；(3) 用 Map 聚合：`@[src,dst] = count()` 替代逐行打印；(4) 设运行时长：`timeout 10 tcpretrans`；(5) 只看异常：`/args->retrans > 0/`；(6) 直方图替代逐事件：`hist()` 看分布而非逐条。"),
        ("为什么不应同时运行多个网络 BPF 工具？", "(1) 多个 BPF 程序 attach 到同一 probe（如 tcp_sendmsg），每次命中执行多段 BPF 代码，开销叠加；(2) 多个工具竞争 ring buffer 和 Map 内存；(3) 输出交织难以关联。正确做法：串行排查——先运行 Tier 1 工具收集概览，分析后运行 Tier 2 针对性追踪，一次一个工具。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-9-与DPDKXDP的分工.md",
    [
        ("在 DPDK 路径上用内核 BPF 工具", "DPDK 绕过内核网络栈，内核 BPF 工具（tcpretrans/tcpconnlat 等）看不到 DPDK 流量；需用 DPDK 自身统计"),
        ("混淆 XDP 和 DPDK 的定位", "XDP 在内核网卡驱动层做早期包处理（仍经过内核），DPDK 完全绕过内核（用户态驱动）；两者观测方式不同"),
        ("以为 XDP 能完全替代 DPDK", "XDP 适合包过滤/转发/丢弃，不适合完整的协议栈处理；HFT 如果需要用户态 TCP 栈仍需 DPDK"),
    ],
    [
        ("内核网络栈、XDP、DPDK 三条路径的观测手段分别是什么？", "(1) 内核网络栈 TCP/UDP：本章 BCC 工具（tcpretrans/tcpconnlat 等）+ ethtool；(2) XDP 早丢弃/转发：XDP 程序自带统计 + `bpftool prog show` + xdpdump；(3) DPDK 用户态：PMD 统计（testpmd show port stats）、应用层计数器、rte_eth_stats。关键：DPDK 口上 tcpretrans 可能无事件——工具针对内核 TCP 栈。"),
        ("为什么不能在 DPDK 路径上用内核 BPF 工具？", "DPDK 使用用户态网卡驱动（PMD），完全绕过内核网络栈——包从网卡 DMA 到用户态内存，不经 tcp_sendmsg/dev_queue_xmit 等内核函数。因此 tcpretrans、tcpconnlat、socketsnoop 等基于内核 probe 的工具看不到 DPDK 流量。需用 DPDK 自身统计接口（rte_eth_stats_get）或应用层计数器。"),
        ("XDP 和 DPDK 的定位有什么区别？HFT 如何选择？", "XDP：在内核网卡驱动层做早期包处理（仍经内核），适合包过滤/转发/丢弃，开销低于 DPDK 但灵活性受限。DPDK：完全绕过内核（用户态驱动），适合完整协议栈处理和极低延迟，但占用专用 CPU 核。HFT 选择：(1) 内核栈 + XDP 做早过滤——延迟可接受（微秒级）时优先；(2) DPDK——需要纳秒级延迟或自定义协议栈时使用；(3) 混合——XDP 做粗过滤，DPDK 做精细处理。"),
    ],
))

NOTES.append((
    "chapter-10-networking/notes/section-10-BPFbpftraceOne-Liners示意.md",
    [
        ("复制 one-liner 不修改网卡名和端口", "one-liner 中的 eth0、端口 80 是示例值，需替换为实际值；不修改可能匹配不到任何流量"),
        ("忽视网络 one-liner 的 PPS 开销", "网络 one-liner 在每包触发，高 PPS 环境下开销大；应加 filter 或用 Map 聚合而非逐包 printf"),
        ("只追踪发送不追踪接收（或反之）", "网络延迟是双向的——send 延迟 + 网络 RTT + recv 延迟；只看一个方向无法定位问题在哪一段"),
    ],
    [
        ("网络分析最常用的 3 个 bpftrace one-liner 是什么？", "(1) TCP 重传统计：`tracepoint:tcp:tcp_retransmit_skb { @[ntop(args->saddr), ntop(args->daddr)] = count() }`；(2) 连接延迟：`kprobe:tcp_v4_connect { @s[tid]=nsecs } kretprobe:tcp_v4_connect /@s[tid]/ { @connlat=hist(nsecs-@s[tid]) }`；(3) 发送字节数按进程：`tracepoint:syscalls:sys_enter_sendto /comm == \"myapp\"/ { @[comm] = sum(args->len) }`。"),
        ("如何用 bpftrace 测量网络各层延迟？", "(1) 应用→内核：`kprobe:tcp_sendmsg { @app[tid]=nsecs }` 到 `kprobe:ip_queue_xmit /@app[tid]/ { @kern_lat=hist(nsecs-@app[tid]) }`；(2) 内核→网卡：`kprobe:dev_queue_xmit { @qdisc[tid]=nsecs }` 到 `kprobe:dev_hard_start_xmit /@qdisc[tid]/ { @drv_lat=hist(nsecs-@qdisc[tid]) }`；(3) RTT：`tracepoint:tcp:tcp_probe { @rtt = hist(args->rtt / 1000) }`。分段测量定位延迟在哪一层。"),
        ("HFT 网络延迟排查的 one-liner 策略是什么？", "三步：(1) 基线测量——tcprtt 直方图看正常 RTT 分布；(2) 异常定位——tcpretrans 看是否有重传（每次重传 = 至少 1 RTT 额外延迟）；(3) 精确分段——bpftrace 在 tcp_sendmsg/dev_queue_xmit/dev_hard_start_xmit 上打时间戳，算出应用→内核→网卡各段延迟，定位抖动来源。加 `/comm == \"myapp\"/` 过滤目标进程。"),
    ],
))


# ════════════════════════════════════════════════════════════════════
# Main
# ════════════════════════════════════════════════════════════════════

def main():
    ok = 0
    missing = 0
    already = 0

    for relpath, traps, quiz in NOTES:
        fpath = os.path.join(BASE, relpath)
        if not os.path.exists(fpath):
            print(f"MISSING: {relpath}")
            missing += 1
            continue

        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()

        if "常见陷阱" in content:
            print(f"SKIP (already has traps): {relpath}")
            already += 1
            continue

        block = make_block(traps, quiz)
        new_content = insert_before_final_sep(content, block)

        with open(fpath, "w", encoding="utf-8") as f:
            f.write(new_content)

        print(f"OK: {relpath}")
        ok += 1

    print(f"\n=== Summary ===")
    print(f"OK: {ok}  |  MISSING: {missing}  |  SKIP(already): {already}  |  Total: {len(NOTES)}")


if __name__ == "__main__":
    main()
