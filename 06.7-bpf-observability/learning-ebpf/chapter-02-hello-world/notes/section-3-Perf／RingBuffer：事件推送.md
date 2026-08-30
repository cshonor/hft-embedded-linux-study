# Perf / Ring Buffer：事件推送

> 本节讲什么：环形缓冲区的机制（为什么丢数据可被感知）、perf buffer 与 ring buffer 的差异，以及"结构化事件"的完整代码——这是从"玩具"到"工具"的分水岭。

## 1. 环形缓冲区机制

一块固定大小的内存 + 一个写指针 + 一个读指针，各自单调前进，到头回卷（这就是"环形"）：

```
        ┌───写指针(W)───▶
   [....已写....][....未写....]
                       ┌──读指针(R)───▶

   W == R          → 空（读追上了写）
   W 再前进将越过 R → 满：写不进去 → **丢数据，drop 计数器 +1**
```

**设计精髓在"丢数据可感知"**：溢出不是静默覆盖，而是丢整条事件并累计 drop 计数；用户态读操作能拿到丢失数。观测工具因此可以自检数据完整性——这是它和"默默覆盖旧数据"方案的本质差别。

尺寸调法：缓冲区大小 = 按事件突发速率（内核侧写入速率）与用户态消费速率的最坏差值留余量。丢数据说明用户态消费太慢或 buffer 太小。

## 2. 两种缓冲区：perf buffer vs BPF ring buffer

| | perf buffer（老） | BPF ring buffer（5.8+，新） |
|---|---|---|
| 缓冲区数量 | **每 CPU 一个**（§lab 里 5 核=5 块） | **全机一个共享** |
| 内存效率 | 按最大核数预留 | 更省（N 核不再 ×N） |
| 事件乱序 | 跨核顺序无保证 | 保证事件顺序 |
| 通知机制 | perf mmap 页 + epoll | epoll（与用户态集成更好） |

新代码一律用 ring buffer（Andrii Nakryiko 的专文 "BPF ring buffer" 值得读，libbpf 作者写的）。BCC 里两者 API 长得几乎一样，概念也通用。

## 3. 结构化事件：完整代码

**内核侧**（每次 execve 触发，收集上下文打包提交）：

```c
BPF_PERF_OUTPUT(output);
struct data_t {
   int pid;
   int uid;
   char command[16];
   char message[12];
};
int hello(void *ctx) {
   struct data_t data = {};                              // ① 零初始化（verifier 要求栈上先初始化）
   char message[12] = "Hello World";
   data.pid = bpf_get_current_pid_tgid() >> 32;         // ② 高32位=PID(tgid)，低32位=线程ID
   data.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;   //    UID 在低 32 位——注意与上行方向相反！
   bpf_get_current_comm(&data.command, sizeof(data.command)); // ③ 当前进程名写进缓冲区
   bpf_probe_read_kernel(&data.message, sizeof(data.message), message);
   output.perf_submit(ctx, &data, sizeof(data));        // ④ 提交进 ring buffer
   return 0;
}
```

要点：

- **②③ 在内核里就把上下文收集齐**：pid/uid/comm 这类信息只有内核知道得准（用户态工具猜进程身份总有竞态）。打包成 `struct data_t` 一次拷贝出内核——**无每事件一次的用户态同步**，这就是 eBPF 观测低开销的机制根源
- **③ 写型 helper 的约定**：`bpf_get_current_comm(目的地址, 大小)`——字符串类 helper 都是"你给缓冲区我填"，因为你不能直接解引用任意内核指针
- struct 定义在两个文件间必须一致（或共享头文件），错位 = 读出乱码

**用户侧**（注册回调 + 轮询）：

```python
def print_event(cpu, data, size):
   data = b["output"].event(data)          # 字节流 → 结构体
   print(f"{data.pid} {data.uid} {data.command.decode()} {data.message.decode()}")
b["output"].open_perf_buffer(print_event)  # 注册回调
while True:
   b.perf_buffer_poll()                    # 底层是 epoll：无事件时休眠，零忙等
```

**数据流全景**（记住这张图）：

```
execve 发生 ─▶ kprobe 触发 eBPF 程序 ─▶ 收集上下文填充 data_t ─▶ perf_submit
                                                                        │
用户态: epoll 唤醒 ◀── ring buffer ◀────────────────────────────────────┘
   └─▶ 回调 print_event ─▶ 解码打印
```

对比 map 轮询（§2）：事件驱动的推送模型，无事件零开销，事件延迟从"轮询周期"降到"调度唤醒"。

## 4. helper 可用性预告

本节用到的 helper（`perf_submit` 底层的 `bpf_perf_event_output`、`bpf_get_current_*`）能否用，**取决于程序类型**（kprobe 能拿 pid，XDP 不能——它不在进程上下文）。规则第 7 章展开，先留印象：**报 `unknown func` 时先查程序类型再查拼写**。

---

**衔接**：程序开始变长，eBPF 里怎么组织代码？函数调用和尾调用——两节。
