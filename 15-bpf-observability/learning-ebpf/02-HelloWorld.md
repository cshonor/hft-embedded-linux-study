# Learning eBPF · 第 2 章：eBPF 的 "Hello World"

> 底本：`../LEARNING-EBPF-BILINGUAL.pdf`。本章用 BCC Python 框架写三个渐进的 Hello World，引出 helper 函数、maps、perf/ring buffer、尾调用四大构件。

## 本章目标

1. 建立心智模型：**用户态程序（加载/读数据） + 内核态 eBPF 程序（事件触发执行）**
2. 掌握三种数据出口：trace_pipe（调试用）→ hash map（轮询）→ perf/ring buffer（事件推送）
3. 理解函数调用限制与尾调用机制

## 1. 第一个 Hello World（BCC 版）

```python
#!/usr/bin/python
from bcc import BPF
program = r"""
int hello(void *ctx) {
    bpf_trace_printk("Hello World!");
    return 0;
}
"""
b = BPF(text=program)                          # BCC 现场编译 C 字符串并加载进内核
syscall = b.get_syscall_fnname("execve")       # execve 的内核实现函数名随架构不同（x86: __x64_sys_execve）
b.attach_kprobe(event=syscall, fn_name="hello") # 挂 kprobe
b.trace_print()                                 # 无限循环读 trace
```

**分层理解：**
- eBPF 程序是 C，由 BCC 在运行时编译（下一章手工做这一步）
- `bpf_trace_printk()` 是 **helper 函数**——eBPF 程序不能调用任意内核函数，只能调用内核白名单里的 helper（区分 eBPF 与 classic BPF 的特性之一）
- 输出固定写到 `/sys/kernel/debug/tracing/trace_pipe`——**全机唯一**，多程序混写、只支持字符串、无结构化 → 只配调试用

**权限要点：**
- root 最简单；"Operation not permitted" 第一个怀疑非特权
- `CAP_BPF`（5.8+）只是基础：加载跟踪程序还需 `CAP_PERFMON`，加载网络程序还需 `CAP_NET_ADMIN`

**行为验证**：程序加载前就在跑的进程调用 execve 也触发——动态生效、零重启。

## 2. BPF Maps：结构化数据通道

定义在 `uapi/linux/bpf.h`，本质都是 key-value 存储。三大用途：
1. 用户态写配置 → eBPF 读
2. eBPF 存状态 → 另一个（或将来的）eBPF 程序读
3. eBPF 写结果/指标 → 用户态展示

**类型谱系：**
- 数组（key 恒为 4 字节索引）vs 哈希表（任意类型 key）
- 专用优化：FIFO 队列、LIFO 栈、LRU、最长前缀匹配（trie）、Bloom 过滤器
- 对象型：`sockmap`/`devmap`（socket/网卡，供网络程序重定向流量）、`PROG_ARRAY`（存程序 fd，实现尾调用）、map-of-maps
- **per-CPU 变体**：每核一块独立内存——读写免锁，是高性能计数器的标准做法
- 非 per-CPU map 的并发：5.1 起部分 map 支持自旋锁

### 哈希表示例：按 UID 统计 execve 次数

```c
BPF_HASH(counter_table);                    // BCC 宏

int hello(void *ctx) {
  u64 uid;
  u64 counter = 0;
  u64 *p;
  uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;  // 低32位=UID，高32位=GID（掩掉）
  p = counter_table.lookup(&uid);                // 查表，返回值指针；无命中返回 0
  if (p != 0) { counter = *p; }
  counter++;
  counter_table.update(&uid, &counter);
  return 0;
}
```

注意 `counter_table.lookup()` 这种"结构体方法"**不是合法 C**——BCC 先把源码重写成真正的 C 再交给编译器。BCC 的"C"是一门方言。

用户态每 2 秒轮询打印：`b["counter_table"].items()`。`sudo ls` 会计两次：501 执行 sudo 一次、root(0) 执行 ls 一次。

**局限**：用户态必须不停轮询 → 引出事件驱动的缓冲区。

## 3. Perf / Ring Buffer：事件推送

环形缓冲区原理：读写指针各自前进；读追上写 = 无数据；写要追上读 = **丢数据，drop 计数器 +1**（读操作能感知丢失）。缓冲区大小要按读写速率的抖动余量调。

5.8+ 优先用 **BPF ring buffer**（取代 perf buffer，单个共享缓冲区、更省内存；Andrii Nakryiko 有专文）。

### 结构化事件示例

```c
BPF_PERF_OUTPUT(output);
struct data_t {
   int pid;
   int uid;
   char command[16];
   char message[12];
};
int hello(void *ctx) {
   struct data_t data = {};
   char message[12] = "Hello World";
   data.pid = bpf_get_current_pid_tgid() >> 32;    // 高32位=PID(tgid)，低32位=线程ID
   data.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
   bpf_get_current_comm(&data.command, sizeof(data.command));  // 当前命令名（字符串要传地址写入）
   bpf_probe_read_kernel(&data.message, sizeof(data.message), message);
   output.perf_submit(ctx, &data, sizeof(data));   // 提交到 ring buffer
   return 0;
}
```

用户态注册回调 + 轮询：

```python
def print_event(cpu, data, size):
   data = b["output"].event(data)
   print(f"{data.pid} {data.uid} {data.command.decode()} {data.message.decode()}")
b["output"].open_perf_buffer(print_event)
while True:
   b.perf_buffer_poll()
```

**关键收益**：上下文信息（PID/UID/comm）全部在内核内收集，**无同步的用户态切换**——这就是 eBPF 观测低开销的来源。可用 helper 集合取决于程序类型和触发事件（第 7 章展开）。

## 4. 函数调用

- 早期 eBPF 禁止调用 helper 之外的函数 → 只能 `static __always_inline` 强制内联（编译器把函数体复制进调用处，无跳转指令；多处调用 = 多份拷贝）
- **内核 4.16 + LLVM 6.0 起**支持 "BPF to BPF calls"（BPF 子程序）——但 BCC 不支持，libbpf 才能用（第 3 章）
- 内联副作用：内核函数被编译器内联优化后 kprobe 挂不上（第 7 章）

## 5. 尾调用（Tail Calls）

定义：调用另一个 eBPF 程序并**替换**执行上下文——类比 `execve()` 对进程的作用，成功则**永不返回**，被调程序替换调用者栈帧。

```c
long bpf_tail_call(void *ctx, struct bpf_map *prog_array_map, u32 index)
```

- `ctx`：透传上下文
- `prog_array_map`：`BPF_MAP_TYPE_PROG_ARRAY`，存一组程序 fd
- `index`：选哪个程序
- 失败（如 index 无条目）则调用者继续往下执行——天然当默认分支用

**动机**：eBPF 栈仅 **512 字节**，尾调用串函数不增长栈；还可绕过单程序指令数限制。
**限制**：最多链 **33** 个尾调用；子程序内尾调用需 JIT 支持（写书时仅 x86，ARM 6.0 加）；与 BPF-to-BPF 调用长期互斥，**5.10** 起解除。

### 示例：sys_enter raw tracepoint + 按操作码分发

```c
BPF_PROG_ARRAY(syscall, 300);
int hello(struct bpf_raw_tracepoint_args *ctx) {
   int opcode = ctx->args[1];
   syscall.call(ctx, opcode);                     // BCC 重写为 bpf_tail_call(ctx, syscall, opcode)
   bpf_trace_printk("Another syscall: %d", opcode); // 尾调用失败才走到这 = 默认消息
   return 0;
}
```

用户态往 map 里塞 fd：`prog_array[59] = exec_fn.fd`（59=execve）；高频噪音 syscall（21/22/25…）塞 `ignore_opcode`（空函数静默）；多个 entry 可指向同一程序（222-226 全指向 hello_timer）。尾调用程序类型必须与父程序一致。

## 坑点清单

1. trace_pipe 全机共享且慢——生产一律用 ring buffer / map
2. `bpf_get_current_pid_tgid()` 的 PID 在**高** 32 位（tgid），UID 在**低** 32 位——位移方向最容易写反
3. shell 内建命令（echo 等）不 execve，不触发事件
4. BCC 方言 C 不是标准 C，换 libbpf 时 lookup/update 语法要全部重写
5. ring buffer 太小 → 静默丢数据，务必检查 drop 计数
6. 尾调用 33 层上限、5.10 前与子程序互斥

## HFT 关联

- **per-CPU map + 用户态汇总**是低开销高频计数器的标准架构（避免跨核 cache line 争用），思路与用户态无锁 per-core 计数一致
- ring buffer 的"丢数据计数"设计直接影响观测数据完整性：交易系统的延迟直方图若丢样本会误导调优，buffer 尺寸要按突发速率留余量
- 尾调用按操作码分发的模式 = 内核态"消息路由"，XDP 多级包处理（解析→过滤→转发）就用这套
- 512 字节栈限制意味着**别想在 eBPF 里放大数组/深递归**——HFT 延迟测量代码必须极简，状态进 map

## 自测题

1. trace_pipe 的三个局限是什么？生产环境用什么替代？
2. 加载 tracing 程序和 networking 程序分别需要哪些 capability？
3. `bpf_get_current_pid_tgid()` 和 `bpf_get_current_uid_gid()` 的 64 位返回值各字段怎么分布？
4. maps 的三大用途？per-CPU 变体解决什么问题？
5. 尾调用和普通函数调用的本质区别？为什么 eBPF 栈只有 512 字节使得尾调用更重要？
6. 33 层尾调用 × 100 万指令限制，对写复杂内核逻辑意味着什么？

## 交叉引用

- 编译/JIT/BPF 子程序 → `03-eBPF程序解析.md`
- ring buffer 的 bpf() 侧操作 → `04-bpf系统调用.md`
- CO-RE 与 libbpf（BCC 的替代） → `05-CO-RE一次编译处处运行.md`
- 程序类型与可用 helper → `07-程序类型.md`
