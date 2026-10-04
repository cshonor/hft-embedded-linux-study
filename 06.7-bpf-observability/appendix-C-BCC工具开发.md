# 附录 C BCC 工具的开发

> 底本：《BPF之巅》附录 C（印刷 p778–792），第 4 章的扩展，感兴趣读者的可选内容。bpftrace 是更高级的语言（第 5 章），很多情况下首选；最小化开销的讨论见第 18 章。

## 资源

作者创建并免费放在 BCC 仓库的三个文档（在线并由贡献者维护）：

1. **BCC Python Developer Tutorial**：超过 15 个使用 Python 接口的课程，每个关注不同学习细节
2. **BCC Reference Guide**：BPF C API 及 BCC Python API 全部参考，含每个能力的短代码示例
3. **Contributing BCC/eBPF scripts**：向 BCC 仓库贡献工具的清单——作者多年开发和维护跟踪工具的经验教训

本附录用 4 个例子讲解：**hello_world.py**（基础）、**sleepsnoop.py**（逐事件输出）、**bitehist.py**（直方图映射表/函数原型/结构体）、**biolatency.py**（真实工具）。

## 5 个技巧（开发 BCC 工具前必知）

1. **BPF C 是受限的**：没有循环和内核函数调用。只能用 bpf_* 内核辅助函数和一些编译器内置函数
2. **所有内存必须通过 bpf_probe_read() 读取**（做必要检查）。`a->b->c->d` 的引用解析先直接写——BCC 有**重写器**会转换为必要的 bpf_probe_read()；不行再加显式调用。内存数据只能读到 **BPF 堆栈或 BPF 映射表**（堆栈有大小限制，大对象用映射表）
3. **三种内核态→用户态输出方法**：
   - `BPF_PERF_OUTPUT()`：自定义结构体逐事件发送（推荐）
   - `BPF_HISTOGRAM()` 或其他 BPF 映射：键/值哈希做摘要统计/直方图，定期从用户态读取（**高效**）
   - `bpf_trace_printk()`：**仅用于调试**——写 trace pipe，可能与其他程序和跟踪器冲突
4. **尽量用静态插桩（跟踪点、USDT）而非动态插桩（kprobes、uprobes）**：动态插桩 API 不稳定，代码变化工具就停
5. **跟进 BCC 和 bpftrace 的开发进展**，新功能可行时切换

## 工具 1：hello_world.py

```python
#!/usr/bin/python
from bcc import BPF
b = BPF(text="""
int kprobe__do_nanosleep() {
    bpf_trace_printk("Hello, World!\\n");
    return 0;
}
""")
b.trace_print()
```

- `kprobe__` 前缀是**插桩快捷方式**，剩余字符串即目标函数（do_nanosleep）；老工具用 `BPF.attach_kprobe()` Python 调用
- `bpf_trace_printk()` 打印到共享跟踪缓冲区；`trace_print()` 从内核取回并打印
- 换行符需额外 `\` 转义以保留到编译阶段
- **只用 trace_pipe 是为了示例简短**——与其他跟踪工具共跑可能输出冲突（可从 /sys/kernel/debug/tracing/trace_pipe 读取）

## 工具 2：sleepsnoop.py（perf 输出缓冲区）

```python
#!/usr/bin/python
from bcc import BPF
b = BPF(text="""
struct data_t {          // 输出结构体：u64 ts + u32 pid
    u64 ts;
    u32 pid;
};
BPF_PERF_OUTPUT(events);                       // perf 事件输出缓冲区
int kprobe__do_nanosleep(void *ctx) {
    struct data_t data = {};                   // 初始化必需！BPF 验证器拒绝未初始化内存
    data.pid = bpf_get_current_pid_tgid();
    data.ts = bpf_ktime_get_ns() / 1000;
    events.perf_submit(ctx, &data, sizeof(data));
    return 0;
}
""")
print("%-18s %-6s %s" % ("TIME(s)", "PID", "CALL"))   # 表头

def print_event(cpu, data, size):               # perf 缓冲区回调
    event = b["events"].event(data)             # 新版 BCC 自动解码结构体
    print("%-18.9f %-6d Hello, World!" % ((float(event.ts)/1000000), event.pid))

b["events"].open_perf_buffer(print_event)       # 注册回调
while 1:
    try:
        b.perf_buffer_poll()                    # 轮询
    except KeyboardInterrupt:
        exit()
```

**优化要点**：事件频繁时 Python 常被唤醒——有些工具在 while 循环加小睡眠缓冲事件降低 Python CPU 次数；更优做法是**在内核上下文中汇总事件**（下一工具）。

## 工具 3：bitehist.py（直方图映射表）

```python
from bcc import BPF
from time import sleep
b = BPF(text="""
#include <uapi/linux/ptrace.h>
BPF_HISTOGRAM(dist);
int kprobe__blk_account_io_completion(struct pt_regs *ctx,
        void *req, unsigned int bytes) {
    dist.increment(bpf_log2l(bytes/1024));
    return 0;
}
""")
print("Tracing block I/O... Hit Ctrl-C to end.")
try:
    sleep(99999999)
except KeyboardInterrupt:
    print()
b["dist"].print_log2_hist("kbytes")
```

**关键讲解**：

- 函数签名：第一个参数 `struct pt_regs *ctx` 是插桩的**寄存器状态**（非目标函数参数），其余参数来自目标函数（block/blk-core.c 的 blk_account_io_completion）
- 不需要的 `struct request *req` 可以用 **`void *req`** 代替——BPF 默认不认识 struct request，包含它会**编译失败**；替代方案是 `#include <linux/blkdev.h>`（方法 2 更轻）
- `bpf_log2l(4096/1024)=2` → `dist.increment(2)`：索引即 2 的幂桶
- 标题约定：**Tracing**（按事件跟踪；采样则说 Sampling）+ **block I/O**（什么被插桩）+ **Hit Ctrl-C to end**（何时结束）
- `print_log2_hist()` 知道范围的原因：范围**没有**从内核传到用户态，传的只是索引——因为用户态和内核的 log2 算法一致
- 另一种写法（结构体指针解析）：`#include <linux/blkdev.h>` 后 `dist.increment(bpf_log2l(req->data_len/1024))`；可删除**末尾**未用参数（保留先前参数位置）
- **签名错误不会被编译器抓住**：BPF 程序的参数被映射到调用约定寄存器（x86-64 的 %rdi、%rsi、%rdx…），签名错了照样编译成功、拿到无效数据。内核知道参数类型——但仅当装了内核调试信息（很少见，文件很大）；正在开发的 **BTF** 轻量级元数据有望消除包含头文件和重新声明签名的需要

## 工具 4：biolatency.py（真实工具全解）

完整讲解作者原始 biolatency.py 的分段结构：

| 部分 | 行 | 要点 |
|---|---|---|
| 文件头注释 | 1–12 | 工具名 + 一句介绍 + For Linux, uses BCC, eBPF + USAGE + 版权 + 变更历史 |
| 参数处理 | 14–46 | argparse；目标是类 UNIX 工具（vmstat/iostat 风格）——**只做一件事并做好**（逐事件模式做成了单独的 biosnoop.py） |
| BPF 程序 | 48–86 | `BPF_HASH(start, struct request*)` 以 request 指针为键（**把指针地址当 UUID 用**）；trace_req_start 存时间戳、trace_req_completion 查询/求差/删除；`/tsp == 0/ return 0` 处理 missed issue |
| 代码替换 | 88–105 | `-m` 换 FACTOR（usecs/msecs）、`-D` 换 STORAGE/STORE（单直方图 vs 按磁盘 disk_key_t）——"用代码写代码"让调试更难，作者尽量避免 |
| 调试 | 106–107 | `if debug: print(bpf_text)` |
| 加载与附加 | 109–118 | `b.attach_kprobe(event=..., fn_name=...)`；-Q 换 blk_account_io_start；blk_start_request/blk_mq_start_request 两个 kprobe 共用一个 BPF 函数——**因为它们第一个参数都是 struct request *** |
| 输出循环 | 119–139 | sleep(interval) → 可选时间戳 → `dist.print_log2_hist(label, "disk")` → `dist.clear()` → 倒计时退出 |

- 该工具早于跟踪点支持（用 kprobes 开发）；**应该重写为跟踪点**——稳定 API，但要求 Linux 4.7+
- `-D` 按磁盘直方图：`bpf_probe_read(&key.disk, ..., req->rq_disk->disk_name)` 多级引用解析由 BCC 重写器透明转换

## HFT 关联

- BCC 重写器（`a->b->c` 自动展开 bpf_probe_read）是把双刃剑：便利但掩盖了实际执行的成本——高频路径上显式读、减少解引用层数
- "签名错了也能编译"是 BCC 最阴险的坑：改内核或换函数时务必用 funccount/trace 验证读到的值合理（如字节数在 0–16K 而非天文数字）
- biolatency 的"指针当 UUID"配对模式与第 9 章 biolatency 笔记一致，是所有 request 类延迟工具的母版

<details>
<summary>自测题</summary>

1. 三种内核→用户态输出方法是什么？哪个只用于调试？
2. 为什么 struct data_t 必须 `= {}` 初始化？
3. 用 void* 代替 struct request* 的前提是什么？
4. 签名错误为什么编译能通过？后果是什么？
5. blk_start_request 与 blk_mq_start_request 为何能共用一个 BPF 函数？

</details>
