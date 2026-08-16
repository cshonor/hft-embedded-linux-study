# Perf / Ring Buffer：事件推送

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
