## 15.1 BCC (BPF Compiler Collection)

> 章节导航：[15.0 BPF 背景与架构](./section-15.0-BPF背景与架构.md) · 上一篇 ← · 下一篇 [15.1.7 BCC vs bpftrace](./section-15.1.7-BCC-vs-bpftrace.md) · [本章导读](../README.md)

**本节讲什么**：BCC 的三层构成（框架/库/预制工具）、一次 BCC 工具调用的完整生命周期（LLVM 即时编译 → 验证器 → attach → ring buffer 输出）、工具地图与输出精读、BCC 开发模型与 libbpf+CO-RE 的演进关系。

### 要点

| # | 要点 | 一句话 |
|---|------|--------|
| 1 | BCC = **框架 + 库 + 70+ 预制工具** | 日常跑预制，开发用 Python |
| 2 | **运行时才编译** | 每台目标机都要装 clang/LLVM + 内核头 |
| 3 | 预制工具 = **runbook 弹药** | runqlat/biolatency/tcpretrans 日常化 |
| 4 | 输出走 **ring buffer + map** | 内核聚合，高频事件不打爆用户态 |
| 5 | 新代码走 **libbpf + CO-RE** | BCC Python 是旧路径但仍是最快原型法 |

---

### 一、BCC 是什么：三层构成

```
┌──────────────────────────────────────────────┐
│ bcc-tools（预制工具层）                        │
│   runqlat biolatency tcplife execsnoop ...    │ ← 日常直接跑
├──────────────────────────────────────────────┤
│ bcc 库（开发框架层）                           │
│   Python API: BPF(text=...)/attach_*/[]      │ ← 写自定义工具
├──────────────────────────────────────────────┤
│ 底座：clang/LLVM 编译 + bpf() syscall + 验证器 │ ← 内核机制
└──────────────────────────────────────────────┘
```

| 组成 | 说明 | 谁在用 |
|------|------|--------|
| **bcc 库** | Python/Lua/C++ 绑定，`BPF(text='...')` 一行加载 | 工具开发者 |
| **bcc-tools** | 70+ 预制单用途工具，`/usr/share/bcc/tools/` | 所有人 |
| **libbpf 时代** | 新工具渐迁 libbpf + CO-RE（BTF 重定位），详见 [06.7-BPF ch04](../../../06.7-bpf-observability/02-bpf-performance-tools/chapter-04-bcc/) | 新项目 |

### 二、一次 BCC 工具调用的完整生命周期

以 `runqlat-bpfcc 10` 为例，从敲回车到出直方图，内核与用户态各发生了什么：

```
用户态                                内核态
──────                                ──────
1. Python 启动，读内嵌 BPF C 源码
2. clang/LLVM 编译成 BPF 字节码
   （机器上的 clang + /lib/modules/`uname -r`/build 头文件）
3. bpf(BPF_PROG_LOAD) ──────────────→ 4. Verifier 静态验证
   （DAG 分析：所有路径可达返回、               指针安全、循环展开上限
    栈 512B 上限、map 访问检查）
5. attach：tracepoint/kprobe 注册 ──→ 6. 插桩点挂上（复用 ftrace 基建，
                                       见 ch14 的 ftrace_ops 多路复用）
7. 事件流：每次 sched_wakeup …──────→ 8. BPF 程序执行，@start[tid] 记时间戳
9. perf_event mmap / ring buffer ←── 10. map 聚合（直方图在内核算）
11. Ctrl-C 时读 map，用户态渲染直方图
```

三个关键点：

1. **为什么 BCC 每台机器都要装内核头**：第 2 步是运行时编译，`#include <linux/sched.h>` 直接用目标机当前内核的头文件——结构体布局天然匹配，代价是每台机器都要装 clang/LLVM（几百 MB）。CO-RE 就是为了消灭这个代价（BTF 记录布局 + 加载时重定位）。
2. **为什么安全**：第 4 步验证器在加载时做静态分析，保证 BPF 程序不会死循环、不会野指针访问、不会崩溃内核——加载失败 = 改程序，绝不能 `--force` 绕过。
3. **为什么快**：第 10 步聚合发生在内核——每秒百万次 sched_switch 也不会往用户态灌水，用户态只在结束时读一次 map。

### 三、工具地图（与前文章节对照）

| 领域 | BCC 工具 | SysPerf 章 | 测什么 |
|------|----------|-----------|--------|
| **CPU** | `profile` | Ch 6 | on-CPU 采样（perf record 替代） |
| | `runqlat` | Ch 6 | **调度延迟直方图**（金标准） |
| | `runqlen` | Ch 6 | run queue 长度 |
| | `cpudist` | Ch 6 | on-CPU 时长分布 |
| **内存** | `drsnoop` | Ch 7 | direct reclaim 逐次延迟 |
| | `wss` | Ch 7 | 工作集大小采样 |
| **文件/盘** | `opensnoop` | Ch 8 | open() 逐次（路径+fd+错误） |
| | `filetop` | Ch 8 | 文件读写 top |
| | `cachestat` | Ch 8 | page cache 命中率 |
| | `biolatency` | Ch 9 | 块 I/O 延迟直方图 |
| | `biosnoop` | Ch 9 | 块 I/O 逐次（谁发的+延迟） |
| | `biostacks` | Ch 9 | 块 I/O 全栈（含下潜路径） |
| **网络** | `tcplife` | Ch 10 | TCP 连接生命周期 |
| | `tcpretrans` | Ch 10 | 重传逐次（HFT 网络 mvp） |
| | `tcpconnect` | Ch 10 | active open 逐次+延迟 |
| **进程** | `execsnoop` | Ch 5 | 新进程逐次 |
| | `offcputime` | Ch 5 | **off-CPU 栈**（与 perf 互补） |
| **中断** | `hardirqs`/`softirqs` | Ch 6 | 中断耗时 |

```bash
# 调度延迟分布（Ch 6 金标准）
sudo runqlat-bpfcc 10

# 块 I/O 延迟直方图，按 flag 分组，毫秒单位，5 秒一轮
sudo biolatency-bpfcc -F -m 5 10

# TCP 连接生命周期（Ch 10）
sudo tcplife-bpfcc

# Off-CPU 栈：30 秒内进程离开 CPU 都在等什么（Ch 5）
sudo offcputime-bpfcc -p $(pidof strategy) 30
```

### 四、输出精读：runqlat 直方图

```
     usecs           : count    distribution
         0 -> 1      : 0        |                                      |
         2 -> 3      : 2        |                                      |
         4 -> 7      : 7        |                                      |
         8 -> 15     : 44       |*                                     |
        16 -> 31     : 202      |*****                                 |
        32 -> 63     : 816      |*********************                 |
        64 -> 127    : 1038     |***************************           |
       128 -> 255    : 622      |****************                      |
       256 -> 511    : 313      |********                              |
       512 -> 1023   : 148      |****                                  |
      1024 -> 2047   : 71       |*                                     |
      2048 -> 4095   : 29       |                                      |
```

判读三步（与 [ch16 的直方图判读法](../../chapter-16-case-studies/notes/section-16.0-案例背景An-Unexplained-Win.md)同一套语言）：

1. **形状**：单峰右偏 = 正常调度；出现第二峰（如 1ms/10ms 处）= 有周期性事件在插队（定时器/远端唤醒）。
2. **尾部**：`>4ms` 的计数就是被抢占/迁移的受害者——HFT 上 dedicated 核应接近零。
3. **对比**：改隔离参数前后各跑一次，看尾部而非均值。

### 五、BCC 开发模型：什么时候自己写

| 场景 | 选择 | 理由 |
|------|------|------|
| 预制工具已覆盖 | 直接跑 | 70+ 工具覆盖面已很大 |
| 简单假设验证 | **bpftrace** 单行 | 见 [15.2](./section-15.2-bpftrace.md) |
| 复杂多事件工具 | **BCC Python** | 状态机、多 map 协作、定时器 |
| 团队共享/产品化 | BCC → 升格 libbpf/CO-RE | 去掉运行时编译依赖 |

最小 BCC Python 骨架（体验开发模型）：

```python
from bcc import BPF

b = BPF(text='''
#include <uapi/linux/ptrace.h>
BPF_HASH(start, u32, u64);          // map: tid → 入队时间戳

int trace_wake(struct pt_regs *ctx) {
    u64 ts = bpf_ktime_get_ns();
    start.update((u32*)&(ctx->di), &ts);   // 伪代码：记入队时刻
    return 0;
}
''')
# b.attach_kprobe(event="__schedule", fn_name="trace_wake") ...
# 定时读 map 渲染
```

三个 BCC 特有 map 宏：`BPF_HASH`（键值表）、`BPF_HISTOGRAM`（对数分桶——直方图在内核分桶）、`BPF_PERF_ARRAY`（读取内核计数器）。**直方图工具全部靠 BPF_HISTOGRAM 在内核分桶**，这就是「高频率事件不打爆用户态」的落地。

### HFT / 嵌入式关联

- **生产裸机标配三件套**：`runqlat`（隔离是否真生效）、`tcpretrans`（发单通道健康）、`offcputime`（延迟尖刺归因）——写进危机 runbook（见 [ch4 危机工具](../../chapter-04-observability-tools/)）。
- **与 perf 的分工**：perf 管计数与 on-CPU 采样（[ch13](../../chapter-13-perf/)），BCC 管「逐次事件 + off-CPU + 内核聚合直方图」——两者互补，HFT 环境都装。
- **风险纪律**：自定义 kprobe 上生产前先 staging 验证加载与开销；观测全程限 PID 限时长（观测者效应）。
- **嵌入式注意**：交叉编译环境下 BCC 运行时编译几乎不可用（目标机没有 clang），嵌入式观测走 **libbpf + CO-RE 预编译**——见 [06.7-BPF](../../../06.7-bpf-observability/)。

### 衔接

- 上一节：[15.0 BPF 背景与架构](./section-15.0-BPF背景与架构.md)（验证器与 map 的机制总览）
- 下一节：[15.1.7 BCC vs bpftrace](./section-15.1.7-BCC-vs-bpftrace.md)（双剑怎么分工）
- 深入：[06.7-BPF ch04 BCC](../../../06.7-bpf-observability/02-bpf-performance-tools/chapter-04-bcc/)（BCC 编程全书级展开）
- 单行弹药库：[附录 C bpftrace 单行命令](../../appendix-C-bpftrace单行命令.md)

---

### 常见陷阱

1. **BCC 工具名带 -bpfcc 后缀不知道**——Debian/Ubuntu 包名 bpfcc-tools，命令名加 -bpfcc 后缀（runqlat-bpfcc）。
2. **BCC 当 bpftrace 用**——BCC 适合标准化工具（团队共享），bpftrace 适合即兴诊断（单行命令极快）。
3. **libbpf/CO-RE 不知道**——新工具渐迁 libbpf + CO-RE（一次编译到处运行），BCC Python 方式是旧路径但仍是最快原型法。
4. **忘了 BCC 是运行时编译**——目标机没装 clang 或内核头不匹配（刚换内核没装 headers），工具直接起不来。
5. **把直方图当均值看**——直方图的价值在尾部与形状，均值会把第二峰抹平。

<details>
<summary>自测题（点击展开）</summary>

1. BCC 工具在 Debian/Ubuntu 上的命令名后缀？
   <details><summary>答</summary>-bpfcc——如 runqlat-bpfcc、biolatency-bpfcc（包名 bpfcc-tools）</details>
2. BCC 为什么每台目标机都要装 clang 和内核头？
   <details><summary>答</summary>BCC 是运行时编译：加载前用目标机的 clang + 当前内核头把 BPF C 源码编成字节码，结构体布局天然匹配本机内核；代价就是依赖重。CO-RE 用 BTF + 加载时重定位消灭了这个代价。</details>
3. BCC 直方图工具为什么高频事件也不打爆用户态？
   <details><summary>答</summary>BPF_HISTOGRAM map 在内核里完成分桶计数，用户态只在结束时读一次 map 渲染——每秒百万事件也只传一份聚合结果。</details>
4. runqlat 直方图出现 1ms 处的第二峰说明什么？
   <details><summary>答</summary>有周期性 ~1ms 的唤醒源在插队（典型：定时器、内核线程、远端核唤醒），需要结合 offcputime/wakeup 来源进一步定位。</details>
5. 嵌入式目标机为什么通常不能用 BCC？
   <details><summary>答</summary>BCC 需要目标机上有 clang/LLVM 和内核头做运行时编译，嵌入式板上通常没有这套重依赖；改用 libbpf + CO-RE 预编译字节码加载。</details>

</details>


---

← [本章导读](../README.md)
