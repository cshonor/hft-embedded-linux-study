#!/usr/bin/env python3
"""
Ch12 并发编程 — 新手化批量改造脚本
8 个 section：替换空壳自测 → 常见陷阱(3) + 折叠自测题(4)
对无自测段的文件(12.5)：在导航行前插入
"""

import os

NOTES_DIR = os.path.join(os.path.dirname(__file__), "..", "chapter-12-concurrent-programming", "notes")

SHELL_PATTERN = """### 口述巩固 · 自测

1. （待口述补）本节核心一句话？"""

SECTIONS = [
    # ── 12.1 基于进程的并发编程 (42行) ──
    {
        "filename": "section-12.1-基于进程的并发编程.md",
        "expand": None,
        "traps": [
            "**fork 复制页表不复制物理页（COW）** — 开销仍比线程大（复制页表结构 + 内核数据结构）",
            "**父子必须关闭不需要的 fd** — fork 后父子共享 fd 表项，不关会导致 fd 泄漏和连接无法正常关闭",
            "**进程隔离好但共享数据难** — 线程共享地址空间天然方便，进程需 IPC（管道/共享内存/消息队列）",
        ],
        "quiz": [
            ("Q1: 基于 fork 的并发服务器中，父子进程分别关闭哪些 fd？为什么？",
             "子进程关闭 listenfd（不负责 accept），父进程关闭 connfd（不负责 echo）。fork 后父子共享 fd 表项，不关会导致 fd 泄漏和引用计数无法归零。"),
            ("Q2: fork 后父子进程的地址空间关系？开销在哪里？",
             "COW 机制：fork 复制页表但不复制物理页，首次写才分配新页。开销在复制页表结构（mm_struct、VMA、页表项）和内核进程控制块（task_struct）。"),
            ("Q3: 为什么 HFT 不用 fork 做并发网关？",
             "fork 开销大（微秒级），且进程间共享数据需 IPC（额外延迟）。HFT 用线程池或单线程 epoll reactor，避免进程创建/切换开销。fork 更适合隔离场景（沙箱、子进程跑脚本）。"),
            ("Q4: SIGCHLD 和 waitpid 在并发服务器中的作用？",
             "子进程退出后变成僵尸进程（Z 状态），占用 PID 和少量内核资源。父进程捕获 SIGCHLD 信号后调 waitpid 回收子进程，释放资源。不回收会导致 PID 耗尽。"),
        ],
    },

    # ── 12.2 I/O多路复用 (17行, 近空) ──
    {
        "filename": "section-12.2-基于I-O多路复用的并发编程.md",
        "expand": """### I/O 多路复用并发模型

- **核心思想：** 单线程同时监听多个 fd，哪个就绪就处理哪个
- **select / poll / epoll：**

| API | 机制 | 限制 | HFT 适用 |
|-----|------|------|----------|
| `select` | 遍历 fd_set，内核检查每个 fd | FD_SETSIZE=1024 | 不用 |
| `poll` | 传 fd 数组，无数量限制 | 仍 O(n) 遍历 | 不用 |
| `epoll` | 内核维护就绪列表，只返回就绪 fd | O(1) 就绪通知 | 标配 |

- **epoll 触发模式：**
  - **LT（水平触发）** — 只要 fd 有数据可读，每次 epoll_wait 都返回（默认）
  - **ET（边缘触发）** — 只在状态变化时通知一次，必须非阻塞 + 读完

**事件驱动服务器骨架：**
```c
epfd = epoll_create1(0);
epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev); // EPOLLIN
while (1) {
    n = epoll_wait(epfd, events, MAXEV, -1);
    for (i = 0; i < n; i++) {
        if (events[i].data.fd == listenfd) { /* accept */ }
        else { /* read/write */ }
    }
}
```

**HFT：** epoll ET + 非阻塞 socket 是低延迟网关标配；但 tick 线程可能用 busy-poll 而非 epoll_wait。

""",
        "traps": [
            "**select 有 FD_SETSIZE 限制（默认 1024）** — 连接数超 1024 必须用 poll 或 epoll",
            "**epoll 比 select/poll 高效（O(1) vs O(n)）** — 内核维护就绪列表，只返回有事件的 fd，不遍历全部",
            "**ET 模式必须非阻塞 + 读完** — 否则可能丢失数据（只通知一次）；LT 模式更安全但可能多一次 epoll_wait",
        ],
        "quiz": [
            ("Q1: select、poll、epoll 的主要区别？",
             "select：fd_set 位数组，有 FD_SETSIZE=1024 限制，每次调用遍历所有 fd（O(n)）。poll：用 fd 数组，无数量限制，但仍 O(n) 遍历。epoll：内核维护就绪列表，epoll_wait 只返回就绪 fd（O(1)），适合大量连接。"),
            ("Q2: epoll 的 LT 和 ET 模式有什么区别？HFT 用哪个？",
             "LT（水平触发）：fd 有数据就持续通知，直到读完。ET（边缘触发）：状态变化时只通知一次，必须非阻塞读完。HFT 通常用 ET + 非阻塞，减少 epoll_wait 次数；但需小心确保读完所有数据。"),
            ("Q3: 为什么 I/O 多路复用适合网络服务器但不适合计算密集任务？",
             "I/O 多路复用解决的是「等待 I/O 时不阻塞」的问题，适合 I/O 密集（网络服务器）。计算密集任务不等待 I/O，单线程多路复用无法利用多核，需要多线程/多进程。"),
            ("Q4: HFT 网关中 epoll 和 busy-poll 各有什么适用场景？",
             "epoll：适合管理大量连接（admin API、多客户端），事件驱动，CPU 占用低。busy-poll（SO_BUSY_POLL）：适合 tick 线程，内核持续轮询网卡，延迟最低但 CPU 100% 占用。"),
        ],
    },

    # ── 12.3 基于线程的并发编程 (53行) ──
    {
        "filename": "section-12.3-基于线程的并发编程.md",
        "expand": None,
        "traps": [
            "**线程共享地址空间，一个线程崩溃全进程崩溃** — 不像进程有隔离保护；线程错误（如栈溢出）影响整个进程",
            "**每连接一线程在连接数多时爆炸** — 线程虽比进程轻，但仍有栈开销（默认 8MB/线程），1000 连接 = 8GB 栈空间",
            "**pthread_detach 后不能再 join** — 分离线程结束后资源自动回收，但无法获取返回值或等待完成",
        ],
        "quiz": [
            ("Q1: 线程和进程的核心区别？为什么线程比进程轻？",
             "线程共享地址空间（代码/堆/fd），进程独立。线程轻因为创建时不复制页表（共享），切换时不需要切换地址空间（不刷 TLB）。"),
            ("Q2: pthread_create、pthread_join、pthread_detach 分别做什么？",
             "create：创建新线程，从指定函数开始执行。join：等待线程结束并获取返回值（类似 waitpid）。detach：将线程标记为分离状态，结束后资源自动回收，不能再 join。"),
            ("Q3: 每连接一线程的模式有什么问题？如何改进？",
             "问题：1) 线程数随连接数增长，栈空间耗尽；2) 线程切换开销累积；3) 线程太多导致缓存抖动。改进：线程池（固定 N 个 worker），预线程化（提前创建好线程等任务）。"),
            ("Q4: 主线程调用 exit() 会怎样？和 pthread_exit() 有何区别？",
             "exit() 终止整个进程（所有线程立即结束）。pthread_exit() 只退出当前线程，其他线程继续运行。主线程从 main 返回等价于调用 exit()，会终止所有线程。"),
        ],
    },

    # ── 12.4 共享变量 (17行, 近空) ──
    {
        "filename": "section-12.4-多线程程序中的共享变量.md",
        "expand": """### 共享变量与竞态条件

- **共享变量：** 多线程可访问的变量（全局变量、堆上的数据、通过指针传递的栈变量）
- **竞态条件（race condition）：** 多线程并发读写共享变量，结果依赖执行顺序

**经典示例 — counter++ 不是原子操作：**

```c
// 看似一条语句，实际三步：
// 1. load  counter 到寄存器
// 2. add   寄存器 +1
// 3. store 寄存器回 counter
// 两个线程同时执行 → 可能丢失一次增量
```

| 变量类型 | 是否共享 | 需要同步？ |
|----------|----------|-----------|
| 局部变量（栈） | 否（每线程独立栈） | 否 |
| 全局变量 | 是 | 是 |
| 堆变量（malloc） | 是（若多线程访问） | 是 |
| register 变量 | 否（每线程独立寄存器） | 否 |

**HFT：** 热路径尽量不共享（每线程独立数据），必须共享时用无锁结构（SPSC 队列）。

""",
        "traps": [
            "**共享变量需同步，声明 volatile 不够** — volatile 只防编译器优化（不缓存到寄存器），不防 CPU 乱序和竞态",
            "**counter++ 不是原子操作** — load-add-store 三步，两线程并发可能丢失一次增量",
            "**栈变量默认不共享** — 但通过指针传递给其他线程后就变成共享变量，需同步",
        ],
        "quiz": [
            ("Q1: counter++ 在汇编层面是几条指令？为什么不是原子操作？",
             "三条指令：load（从内存读到寄存器）、add（寄存器+1）、store（写回内存）。两线程同时 load 可能读到相同值，各自 +1 后写回，结果只增加 1 而非 2。"),
            ("Q2: volatile 能解决竞态条件吗？为什么？",
             "不能。volatile 只防止编译器将变量缓存到寄存器（保证每次读写访问内存），但不防止 CPU 指令乱序执行，也不保证 load-add-store 的原子性。需要 mutex/atomic 解决。"),
            ("Q3: 哪些变量是线程私有的？哪些是共享的？",
             "私有：局部变量（栈）、register 变量、线程局部存储（TLS）。共享：全局变量、static 变量、堆变量（malloc）。注意：栈变量通过指针传给其他线程后变为共享。"),
            ("Q4: HFT 热路径如何处理共享变量？",
             "最佳：不共享（每线程独立数据，thread-local）。必须共享时：1) SPSC 无锁队列（单生产者单消费者，无竞争）；2) 原子操作（std::atomic with relaxed/release/acquire）；3) 避免互斥锁（不确定性延迟）。"),
        ],
    },

    # ── 12.5 信号量与预线程化 (60行, 无自测) ──
    {
        "filename": "section-12.5-信号量与预线程化.md",
        "expand": None,
        "traps": [
            "**sem_wait/sem_post 顺序不能反** — 生产者先 wait(empty) 再 wait(mutex)，反了会死锁（持有 mutex 等 empty）",
            "**生产者-消费者需要三个信号量** — mutex（互斥）+ empty（空槽数）+ full（满槽数），缺一不可",
            "**预线程化是线程池的思想来源** — 固定 N 个 worker 等任务，避免每连接创建/销毁线程的开销",
        ],
        "quiz": [
            ("Q1: 信号量的 P（wait）和 V（post）操作分别做什么？",
             "P（sem_wait）：信号量值减 1，如果结果 < 0 则阻塞等待。V（sem_post）：信号量值加 1，如果有等待者则唤醒一个。P/V 是原子操作。"),
            ("Q2: 生产者-消费者模式中三个信号量各起什么作用？顺序能否调换？",
             "mutex：保护缓冲区互斥访问。empty：空槽数，生产者 wait（有空槽才能放）。full：满槽数，消费者 wait（有数据才能取）。顺序不能反：生产者必须先 wait(empty) 再 wait(mutex)，否则持有 mutex 等 empty 会死锁。"),
            ("Q3: 预线程化（prethreading）和每连接一线程有什么区别？",
             "每连接一线程：accept 后 pthread_create，连接结束 pthread_exit，频繁创建/销毁。预线程化：启动时创建固定 N 个 worker 线程，主线程 accept 后将 connfd 放入任务队列，worker 取出处理。避免创建/销毁开销，控制线程数。"),
            ("Q4: HFT 为什么用无锁队列替代信号量+全局锁？",
             "信号量+全局锁有内核态切换开销（sem_wait 阻塞时 syscall），延迟不确定（被调度时机不可控）。无锁 SPSC 队列用原子操作（CAS/fence），纯用户态，延迟确定（纳秒级），适合热路径。"),
        ],
    },

    # ── 12.6 使用线程提高并行性 (19行, 薄) ──
    {
        "filename": "section-12.6-使用线程提高并行性.md",
        "expand": None,
        "traps": [
            "**并发（concurrency）不等于并行（parallelism）** — 单核可并发（时间片切换），但不并行；多核才能并行",
            "**Amdahl 定律限制加速比上限** — 串行部分占 10%，即使无限核也只能加速 10 倍",
            "**绑核减少迁移但可能降低负载均衡** — HFT 优先确定性，绑核是标配；但多任务不均时某些核空闲",
        ],
        "quiz": [
            ("Q1: 并发和并行的区别？单核 CPU 能并行吗？",
             "并发：逻辑上同时进展（时间片切换，单核可并发）。并行：物理上同时执行（需要多核）。单核 CPU 不能并行，只能并发。"),
            ("Q2: Amdahl 定律是什么？对 HFT 有什么启示？",
             "加速比上限 = 1 / (串行比例 + 并行比例/N)。即使无限核，加速比受串行部分限制。启示：优化热路径的串行部分（如锁、I/O）比增加线程数更有效。"),
            ("Q3: HFT 为什么要绑核（CPU affinity）？有什么副作用？",
             "绑核减少线程迁移（避免 cache/TLB 冷失效），降低延迟抖动。副作用：负载不均时某些核空闲；独占一个核可能浪费（非热路径任务无核可用）。HFT 通常给 tick 线程独占一个核。"),
            ("Q4: 计算密集任务和 I/O 密集任务分别适合什么并发模型？",
             "计算密集：多线程/多进程，线程数 = 核心数，绑核。I/O 密集：I/O 多路复用（epoll），单线程管理多连接，减少线程切换。HFT 网关：混合模型 — tick 线程绑核跑计算，I/O 线程用 epoll 管连接。"),
        ],
    },

    # ── 12.7 其他并发问题 (62行) ──
    {
        "filename": "section-12.7-其他并发问题.md",
        "expand": None,
        "traps": [
            "**线程安全不等于可重入** — 可重入更严格（不依赖任何外部状态），可重入一定线程安全，反之不然",
            "**strtok/ctime 等返回静态缓冲区的函数不是线程安全的** — 用 strtok_r/localtime_r 等可重入版本替代",
            "**死锁四条件缺一不可** — 打破任一条件即可避免：锁顺序一致（打破循环等待）、trylock（打破不可抢占）",
        ],
        "quiz": [
            ("Q1: 线程安全和可重入的区别？举例说明。",
             "线程安全：多线程并发调用结果正确（可用锁实现）。可重入：被中断后重入仍正确（不依赖外部状态，不用锁）。可重入一定线程安全，线程安全不一定可重入。例：用 mutex 保护的函数线程安全但不可重入（中断时可能死锁）。"),
            ("Q2: 哪些标准库函数不是线程安全的？如何替代？",
             "返回静态缓冲区：ctime→localtime_r，gethostbyname→getaddrinfo。隐式全局状态：strtok→strtok_r，rand→rand_r。errno 是例外，用线程局部存储（TLS）实现，各线程独立。"),
            ("Q3: 死锁的四个必要条件是什么？如何打破？",
             "1) 互斥；2) 持有并等待；3) 不可抢占；4) 循环等待。打破任一即可：锁顺序一致（打破循环等待）、trylock+超时（打破不可抢占）、一次获取所有锁（打破持有并等待）。"),
            ("Q4: HFT 在不同路径上如何选择同步策略？",
             "热路径（tick）：SPSC 无锁队列，零等待零锁。温路径（风控）：细粒度 mutex/shared_mutex，短临界区。冷路径（日志/配置）：粗锁可接受。绝不：持锁跨 I/O、嵌套锁顺序不一致、热路径用 mutex。"),
        ],
    },

    # ── 12.8 小结 (17行, 近空) ──
    {
        "filename": "section-12.8-小结.md",
        "expand": """### Ch12 全章要点

| 主题 | 核心概念 | HFT 关联 |
|------|----------|----------|
| §12.1 | 进程并发（fork） | 隔离场景，不用做网关 |
| §12.2 | I/O 多路复用（epoll） | 网关标配，单线程多连接 |
| §12.3 | 线程并发（pthread） | 线程池替代每连接一线程 |
| §12.4 | 共享变量、竞态 | 热路径不共享，用无锁 |
| §12.5 | 信号量、生产者-消费者 | SPSC 无锁队列替代 sem |
| §12.6 | 并行性、Amdahl、绑核 | tick 线程独占一核 |
| §12.7 | 线程安全、死锁 | 锁顺序一致，热路径无锁 |

**一句话：** 并发编程三种模型（进程/I/O多路复用/线程），核心挑战是共享变量同步；HFT 用 epoll+线程池+无锁队列，热路径避免 mutex，绑核降抖动。

""",
        "traps": [
            "**三种并发模型各有适用场景** — 进程（隔离）、I/O多路复用（大量连接）、线程（计算并行）",
            "**HFT 热路径用无锁，不用 mutex** — mutex 有内核态切换和调度不确定性，无锁队列纯用户态纳秒级",
            "**同步原语选择影响延迟尾部分布** — mutex 的 P99/P99.9 远高于无锁，HFT 关注尾部而非均值",
        ],
        "quiz": [
            ("Q1: 三种并发编程模型分别是什么？各适合什么场景？",
             "1) 进程（fork）：隔离好，适合沙箱/子进程。2) I/O 多路复用（epoll）：单线程多连接，适合网络服务器。3) 线程（pthread）：共享地址空间，适合计算并行。HFT 网关用 epoll+线程池混合。"),
            ("Q2: HFT 为什么在热路径避免 mutex？用什么替代？",
             "mutex 有内核态切换开销（sem_wait 阻塞时 syscall）、调度不确定性（被唤醒时机不可控）、优先级反转风险。替代：SPSC 无锁队列（原子操作，纯用户态，纳秒级）、thread-local 数据（不共享就不需要同步）。"),
            ("Q3: 生产者-消费者模式在 HFT 中的演进路径？",
             "信号量+全局锁（教学版）→ mutex+条件变量（通用）→ SPSC 无锁队列（HFT 热路径）。演进方向：减少锁粒度 → 消除锁 → 消除共享。"),
            ("Q4: 死锁如何避免？HFT 有什么特殊策略？",
             "通用策略：锁顺序一致、trylock+超时、避免嵌套锁。HFT 特殊策略：热路径不用锁（无锁队列）；必须用锁时短临界区（不持锁跨 I/O）；不同路径用不同策略（热路径无锁、冷路径可粗锁）。"),
        ],
    },
]


def build_replacement(entry):
    parts = []
    if entry["expand"]:
        parts.append(entry["expand"])
    parts.append("### 常见陷阱\n")
    for i, trap in enumerate(entry["traps"], 1):
        parts.append(f"{i}. {trap}\n")
    parts.append("\n")
    parts.append("### 自测题\n\n")
    for q, a in entry["quiz"]:
        parts.append(f"<details>\n<summary>{q}</summary>\n\n{a}\n\n</details>\n\n")
    return "".join(parts)


def process_file(entry):
    filepath = os.path.join(NOTES_DIR, entry["filename"])
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    replacement = build_replacement(entry).rstrip()

    if SHELL_PATTERN in content:
        new_content = content.replace(SHELL_PATTERN, replacement)
        mode = "replace shell"
    else:
        nav_marker = "\n---\n\n← [本章导读](../README.md)"
        if nav_marker in content:
            new_content = content.replace(nav_marker, "\n" + replacement + nav_marker)
            mode = "insert before nav"
        else:
            print(f"  [SKIP] {entry['filename']} — 未找到匹配模式")
            return False

    with open(filepath, "w", encoding="utf-8") as f:
        f.write(new_content)
    print(f"  [OK] {entry['filename']} ({mode})")
    return True


def main():
    print(f"Ch12 并发编程批量改造 — 共 {len(SECTIONS)} 个文件\n")
    success = 0
    skipped = 0
    for entry in SECTIONS:
        if process_file(entry):
            success += 1
        else:
            skipped += 1
    print(f"\n完成：{success} 成功，{skipped} 跳过")

    import glob
    files = glob.glob(os.path.join(NOTES_DIR, "*.md"))
    shell_remaining = 0
    details_count = 0
    for f in files:
        with open(f, "r", encoding="utf-8") as fh:
            c = fh.read()
            if "待口述补" in c:
                shell_remaining += 1
            details_count += c.count("<details>")
    print(f"\n验证：残留空壳 {shell_remaining} 个，<details> 标签 {details_count} 个")


if __name__ == "__main__":
    main()
