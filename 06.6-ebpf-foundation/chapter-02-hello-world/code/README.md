# ch02 code · BCC Hello World 系列

> 《Learning eBPF》第 2 章示例补全为可运行文件；笔记中 BCC 库内部源码注解（2.0 块3-7）非示例，不落盘。
> 运行全部需要 root；`BPF(text=...)` 编译本身不需要 root。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04，bcc 0.29.1） |
|------|------|------|------|
| [hello.py](./hello.py) | 2.0 块2、2.1 块1 | 第一个 BCC 程序：kprobe execve + `bpf_trace_printk` + trace_print 循环 | `py_compile` ✓；C 编译 ✓（map 创建才报权限错 = C 已编过） |
| [hello_counter.py](./hello_counter.py) | 2.0 块8、2.1 块2/3 | 数据通道① `BPF_HASH` map：按 UID 统计 execve，2 秒一轮读差 | `py_compile` ✓；C 编译 ✓ |
| [hello_perf_output.py](./hello_perf_output.py) | 2.1 块4/5 | 数据通道② `BPF_PERF_OUTPUT`：结构化事件推送 + perf_buffer_poll 回调 | `py_compile` ✓；C 编译 ✓ |
| [tail_call_demo.py](./tail_call_demo.py) | 2.2 块8/9 | 尾调用分发：`BPF_PROG_ARRAY` 按 syscall 号跳程序，噪音静默、未登记报号 | `py_compile` ✓；C 编译 ✓ |

## 运行

```bash
sudo python3 hello.py                # 另开终端敲命令触发
sudo python3 hello_counter.py        # 每 2 秒一轮 UID 计数
sudo python3 hello_perf_output.py    # 逐条事件推送
sudo python3 tail_call_demo.py       # execve 打印 / 噪音静默 / 其他报号
```

## 数据通道分工（本章核心）

| 通道 | 方向 | 回答 | 对应文件 |
|------|------|------|----------|
| map（HASH/ARRAY） | 用户态**拉** | "累计多少" | hello_counter.py |
| perf/ring buffer | 内核**推** | "发生了什么" | hello_perf_output.py |
