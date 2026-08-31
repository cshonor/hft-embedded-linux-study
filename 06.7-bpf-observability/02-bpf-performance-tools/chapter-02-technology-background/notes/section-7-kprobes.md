# 2.7 kprobes（内核动态插桩：机制 / 接口 / BPF 用法）

> 底本：《BPF之巅》第 2 章技术背景，2.7 节（印刷 p47–52，含 2.7.1–2.7.4）

## 2.7.1 kprobes 是如何工作的

1. 注册时把目标地址的第一条指令备份，替换为 **int3 断点指令**。
2. CPU 执行到该处陷入内核，kprobes 机制接管：依次调用挂载的探测处理函数。
3. 单步执行被备份的原指令，然后跳回继续跑。
4. kretprobe：在函数入口插桩，**劫持返回地址到一个蹦床（trampoline）函数**，在函数返回时拿到返回值与耗时。
5. 移除探针时恢复原指令（借助 `stop_machine()` 保证其他 CPU 不在修改中执行该指令）。

注意点（书中的提醒）：

- 高频函数插桩的开销会线性叠加，可能影响系统性能。
- 部分 ARM64 平台出于安全不允许改写内核代码区 → kprobes 不可用。
- 老的 Ftrace kprobe 实现逐步被 **fentry**（内核 5.x，直接跳转替代 int3，更快）优化替代。
- 有黑名单（NOKPROBE_SYMBOL）保护关键路径不可插桩。

## 2.7.2 kprobes 接口（三种）

| 接口 | 形式 |
|---|---|
| kprobe API | register_kprobe() 等内核函数（几乎无人直接用） |
| Ftrace | 写 /sys/kernel/debug/tracing/kprobe_events |
| perf_event_open() | perf(1) 与 BPF 工具使用（4.17 起 perf_kprobe PMU） |

历史注脚：jprobes 变体已于 2018 年被维护者 Masami Hiramatsu 从内核移除。

## 2.7.3 BPF 和 kprobes

- BCC：`b.attach_kprobe(event="vfs_read", fn_name="do_read")`（支持入口/偏移）；`attach_kretprobe()`。
- bpftrace：`kprobe:vfs_read` / `kretprobe:`（仅入口，不支持偏移）。

BCC 例：vfsstat(8) 对 VFS 关键调用计数，每秒打印：

```python
b.attach_kprobe(event="vfs_read",   fn_name="do_read")
b.attach_kprobe(event="vfs_write",  fn_name="do_write")
b.attach_kprobe(event="vfs_fsync",  fn_name="do_fsync")
b.attach_kprobe(event="vfs_open",   fn_name="do_open")
b.attach_kprobe(event="vfs_create", fn_name="do_create")
```

bpftrace 例：统计所有 vfs 开头函数的调用次数（内核子系统负载画像）：

```bash
bpftrace -e 'kprobe:vfs* { @[probe] = count(); }'
# @[kprobe:vfs_read]: 5581
# @[kprobe:vfs_write]: 4977
```

## 2.7.4 更多资料

内核 Documentation/kprobes.txt；"An introduction to kprobes"；"Kernel Debugging with kprobes"。

## kprobe 开销模型（挂载前的必算账）

int3 机制的单次成本可以拆开估：

```text
单次触发 ≈ 异常陷入（~100ns，含 IDT 查找/栈切换）
         + 单步执行原指令（~100ns，含恢复断点）
         + BPF 程序执行（简单计数 ~50–100ns，带栈回溯/字符串则 µs 级）
         ≈ 0.3–1.5 µs/事件（不含输出）

预算公式: 允许观测税 ≤ 吞吐的 1% 时
          事件率上限 ≈ 0.01 × CPU核时间预算 / 单次成本
          例: 单核 1% 预算 = 10ms/s ÷ 1µs ≈ 10k 事件/s
```

这就是"挂探针前先估 QPS"的量化版：**10k/s 是单核 1% 观测税下简单探针的量级上限**，带栈回溯再降一个量级。fentry（直跳无陷入）把单次成本压到 ~50ns 级，上限提高约一个量级——但预算逻辑不变。

## HFT 关联

- kprobes 是给"没有现成 tracepoint 的内核路径"打临时探针的唯一快速手段：如跟踪收包软中断内部函数、TCP 内部状态机，量化内核协议栈对延迟的贡献。
- 高频路径（每秒百万级收包）上挂 kprobe 处理函数，其 int3 陷入开销可观——HFT 中只用于短窗口排查，不做常驻；kernel 5.x fentry 优化后开销显著降低。
- kretprobe 蹦床测量函数耗时（如 `tcp_write_xmit` 的每次耗时分布）是定位内核侧延迟毛刺的常用模式。

## 陷阱

- kprobe 依赖内核符号：函数被内联/重命名后探针静默失效（attach 报错才算好消息）。
- 内核函数可能被内联，/proc/kallsyms 里没有符号 → 根本无法插桩；换 tracepoint 或相邻函数。
- 误在每次都触发的超高频函数上做重逻辑（printf/字符串拼接）→ 拖垮整机。

## 自测

<details>
<summary>1. kprobe 的底层机制三步是什么？</summary>

保存原指令 → 替换为 int3 断点 → 触发时执行处理函数后单步原指令并跳回。
</details>

<details>
<summary>2. kretprobe 如何捕获函数返回？</summary>

入口 uprobe/kprobe 劫持返回地址指向蹦床函数，函数真正返回时先进入蹦床执行探测逻辑再回到原调用者。
</details>

<details>
<summary>3. bpftrace 的 kprobe 支持与 BCC 有何差异？</summary>

BCC 支持函数入口和任意偏移插桩；bpftrace 仅支持函数入口。
</details>
