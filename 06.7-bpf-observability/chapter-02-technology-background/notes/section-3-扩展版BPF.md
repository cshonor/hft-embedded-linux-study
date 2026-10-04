# 2.3 扩展版 BPF（eBPF 全解：架构、bpftool、API、并发、BTF、CO-RE、局限）

> 底本：《BPF之巅》第 2 章技术背景，2.3 节（印刷 p20–41，含 2.3.1–2.3.12 十二个子节）

这是全书技术密度最高的一节，覆盖 eBPF 的虚拟机、工具链、API、并发模型与可移植性。

## 2.3.1 为什么性能工具需要 BPF

对比"内核态聚合 vs 用户态处理"：传统 Ftrace/perf 每事件一次拷贝，BPF 用 map 在内核聚合（如直方图），只回传摘要。高频事件下性能差距可达数量级。

## 2.3.2 BPF vs 内核模块（LKM）的五大优势

| 维度 | 内核模块 | eBPF |
|---|---|---|
| 安全性 | 崩溃即宕机 | 验证器保证（不会崩溃/不会死循环） |
| 开发效率 | 需匹配内核版本编译 | 前端即时编译 |
| 热更新 | 不支持 | 可随时加载/卸载 |
| 跨版本 | 每版重编 | CO-RE 一次编译到处运行 |
| 调度 | 与内核抢资源 | 验证器限制资源（栈/指令数） |

## 2.3.3 编写 BPF 程序的阶梯

C（内核侧）+ 前端语言 → BCC（Python/Lua 包 C）→ bpftrace（一行式 DSL）→ 未来说明书中预置程序。门槛从高到低，表达力从全到简。

## 2.3.4 bpftool：查看 BPF 指令集与程序

```bash
bpftool prog show                          # 列出已加载程序
bpftool prog dump xlated id 63             # 反汇编 BPF 指令（含 C 源码行，若有 BTF+linum）
bpftool prog dump xlated id 63 linum       # 带源码行号
bpftool prog dump jited id 63              # 宿主机 JIT 后的真实机器码
bpftool map show                           # 列出映射
bpftool btf dump file /sys/kernel/btf/vmlinux format c   # 内核 BTF 转 C 头文件
```

xlated 输出 vs jited 输出的区别：前者是 eBPF 虚拟机指令（验证器所验证的对象），后者是 JIT 生成本机指令。若 JIT 关闭，则由解释器执行 xlated 指令。

## 2.3.5 bpftrace -v

`bpftrace -v -e '...'` 同样可以打印编译出的 BPF 指令，是轻量查看指令的方式。

## 2.3.6 BPF API（helper、bpf(2)、程序类型、映射类型）

- **helper 函数**：Linux 5.2 共 98 个。常用的如 `bpf_probe_read()`（安全读取内核内存，缺页不崩）、`bpf_ktime_get_ns()`（耗时测量基石）、`bpf_get_stackid()`（抓调用栈）、`bpf_map_update_elem()` 等。注意：函数调用必须内联（旧内核不支持 BPF 到 BPF 的调用）。
- **bpf(2) 命令**：BPF_PROG_LOAD、BPF_MAP_CREATE、BPF_MAP_UPDATE_ELEM、BPF_OBJ_PIN 等，用户态管理 BPF 对象的统一入口。
- **程序类型**：决定可挂载的事件源与可用 helper 集合（BPF_PROG_TYPE_KPROBE、TRACEPOINT、PERF_EVENT、XDP、SOCK_FILTER……）。
- **映射类型**：BPF_MAP_TYPE_HASH、ARRAY、PERF_EVENT_ARRAY、PERCPU_HASH、RINGBUF 等。

## 2.3.7 BPF 并发控制

**丢失更新问题**：多个 CPU 同时对同一 map 条目 `读-改-写`，后写覆盖先写。
解决方案：

1. 每类事件独立 key（天然隔离）
2. per-CPU map（每 CPU 一份副本，读取时聚合）
3. 内核 5.1+ 的 spinlock（bpf_spin_lock）
4. 原子指令（xadd 等）

书中实验：不用 per-CPU map 时计数明显丢失。

## 2.3.8 BPF sysfs 接口（pinning）

程序和 map 可以"钉"到 `/sys/fs/bpf/` 路径上持久存在，供多个进程共享引用（BPF_OBJ_PIN / bpftool prog pin）。

## 2.3.9 BTF（BPF Type Format）

描述 BPF 程序/映射/内核类型元数据的格式。`/sys/kernel/btf/vmlinux` 一个文件即可携带全内核类型信息（几 MB），替代数百 MB的 debuginfo，是 CO-RE 的基础。

## 2.3.10 CO-RE（一次编译，到处运行）

Compile Once – Run Everywhere：BPF 程序编译时记录对内核结构的引用，运行时用 BTF + 重定位信息适配当前内核的字段布局差异（配合 libbpf 的 BPF CO-RE 与 `__builtin_preserve_access_index`）。解决"每个内核版本重编一次"的痛点。

## 2.3.11 BPF 的局限性

| 限制 | 说明 |
|---|---|
| 循环受限 | 5.3 前完全不支持循环（须展开）；5.3+ 支持有界循环 |
| 栈仅 512 字节 | 大结构须放 map 或 per-CPU array |
| 指令数上限 | 验证器限制 4096 条 → 后来放宽至 100 万条 |
| 无法调用任意内核函数 | 只能调白名单 helper |
| 内核版本依赖 | 无 CO-RE 时结构体偏移随版本漂移 |

## 2.3.12 扩展阅读

BPF and XDP Reference Guide (Cilium)、BPF Documentation（内核文档）等。

## HFT 关联

- HFT 观测工具常需 7×24 常驻：**验证器保证不宕机**是敢在生产交易机上挂 BPF 的前提；指令数与栈限制决定了程序必须小而聚焦，符合"一次回答一个问题"的调优纪律。
- 并发控制直接对应 HFT 场景：多网卡队列/多线程交易进程同时命中探针，不用 per-CPU map 的计数在行情风暴时必然失真。
- CO-RE 让同一套自研探针在开发机（新内核）与生产机（保守内核）上复用。

## 陷阱

- 用 `bpftool prog dump` 看到的是 xlated 指令，误当成机器码；分析性能须看 `dump jited`。
- 忘记 map 的 per-CPU 语义，用户态读取时未做聚合求和，结果远小于真实值。
- 5.3 之前内核写 BPF 程序用了 `for` 循环 → 验证器直接拒绝；须 `#pragma unroll` 或改尾调用。

## 自测

<details>
<summary>1. eBPF 相对内核模块的两大安全保证来自什么机制？</summary>

验证器（verifier）静态校验：所有路径上的资源访问合法、程序必然终止（无死循环）；加上运行时约束（指令数上限、512B 栈、白名单 helper）。
</details>

<details>
<summary>2. 为什么 bpf_probe_read 而不是直接解引用指针？</summary>

BPF 程序内直接解引用内核指针若遇坏页会 Oops；bpf_probe_read 内部做了页错误保护，读失败返回错误码而不崩溃。
</details>

<details>
<summary>3. CO-RE 依赖哪两个东西解决内核版本差异？</summary>

BTF（类型布局描述，/sys/kernel/btf/vmlinux）+ 编译期记录的重定位信息（运行时由 libbpf 依据目标内核 BTF 调整字段偏移）。
</details>

<details>
<summary>4. map 出现"丢失更新"，列出至少三种对策。</summary>

按 CPU/事件分离 key；改用 per-CPU map；5.1+ 用 bpf_spin_lock；使用原子加（xadd）。
</details>
