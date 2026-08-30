# 第一个 Hello World（BCC 版）

> 本节讲什么：全书第一个程序。逐行拆解 BCC 版 Hello World，搞清"我的 C 代码是怎么跑进内核的"。BCC 是脚手架——它替你干的活（编译、加载、挂载）第 3 章会手工做一遍，理解了这层才不虚。

## 1. 完整程序

```python
#!/usr/bin/python
from bcc import BPF
program = r"""
int hello(void *ctx) {
    bpf_trace_printk("Hello World!");
    return 0;
}
"""
b = BPF(text=program)                          # ① BCC 现场编译 C 字符串并加载进内核
syscall = b.get_syscall_fnname("execve")       # ② execve 的内核实现函数名随架构不同（x86: __x64_sys_execve）
b.attach_kprobe(event=syscall, fn_name="hello") # ③ 挂 kprobe
b.trace_print()                                 # ④ 无限循环读 trace
```

## 2. 逐行拆解

**① `BPF(text=program)` 一次干了四件事：**

```
C 源码字符串 ──clang 编译──▶ BPF 字节码(.o) ──bpf() syscall──▶ verifier 检查 ──▶ JIT 编译成机器码驻留内核
   (你的机器上: BCC 调 clang)                    (这里是第6章的关卡)        (你的Pi上是ARM64机器码)
```

注意：**编译发生在你运行 Python 的那一刻**——所以 BCC 机器上必须有完整编译器头文件，且每次运行都重新编译（这是 BCC 后来被 libbpf 取代的最大原因，第 5 章）。

**② 为什么不能直接写 "execve"？** kprobe 挂的是**内核函数**，syscall 在内核里的实现函数名随架构/内核版本变化（x86_64 上是 `__x64_sys_execve`，你的 Pi 上是 ARM64 的符号）——`get_syscall_fnname()` 帮你查。

**③ kprobe 是什么？** 内核提供的动态插桩机制：在任意内核函数入口放置断点（int3 一类机制），执行到该函数时跳去执行你的 eBPF 程序，完了继续原函数。你在 lab02 挂的是同类的 tracepoint（更稳定但点位固定，第 7 章展开两者区别）。

**④ `trace_print()`**：循环读 `/sys/kernel/debug/tracing/trace_pipe` 打印到屏幕。

## 3. `bpf_trace_printk` 是 helper——这里有个重要概念

eBPF 程序**不能调用任意内核函数**，只能调用内核白名单里的 **helper 函数**（`bpf_` 前缀，约 200 个）。`bpf_trace_printk` 就是内核里的 printf。

为什么不能随便调？想想 verifier 的承诺（第 1 章 §5）：要证明程序安全。如果允许调用任意内核函数，任何一次 API 变动/深锁依赖都可能崩内核——白名单 helper 是"验证过的安全出口"。每个 helper 的参数约束都登记在案，verifier 按清单检查（第 6 章 §3.2 有真实报错示例）。

## 4. trace_pipe：为什么只配调试

输出固定写 `/sys/kernel/debug/tracing/trace_pipe`，三个硬伤：

| 局限 | 后果 |
|---|---|
| 全机唯一，所有程序混写 | 多程序输出无法区分（只能靠格式约定） |
| 只支持字符串 | 没法传结构化数据，用户态还要反向 parse |
| 每次 printk 都有锁和格式化开销 | 高频事件下性能灾难 |

替代方案就是本章主线：**map**（§2，拉模式）和 **ring buffer**（§3，推模式）。

> 你在 lab02 用它看过 openat 输出——完全正确的用法：调试期用 trace_pipe，转正式工具就换 ring buffer。

## 5. 权限模型

- root 最简单
- 非 root 报 `Operation not permitted` 时查 capability：`CAP_BPF`（5.8+）只是基础——加载**跟踪**程序还需 `CAP_PERFMON`，加载**网络**程序还需 `CAP_NET_ADMIN`
- 三个都要用 `bpftool` 检查，见 ebpf-gate 的真机环境（bpftrace 也要 root，lab01 里验证过）

## 6. 行为验证：动态生效

程序加载前就在跑的进程调用 execve 也触发——因为 kprobe 挂在内核函数上，与"哪个进程调用"无关。这是第 1 章"动态加载"优势的亲眼版。验证法：开两个终端，一个跑本程序，另一个随便敲命令。

---

**衔接**：trace_pipe 只能打字符串，想让内核里的数据结构化地流到用户态，需要 BPF maps——下一节。
