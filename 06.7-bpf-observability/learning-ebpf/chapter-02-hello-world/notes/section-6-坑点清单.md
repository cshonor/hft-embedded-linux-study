# 坑点清单

1. trace_pipe 全机共享且慢——生产一律用 ring buffer / map
2. `bpf_get_current_pid_tgid()` 的 PID 在**高** 32 位（tgid），UID 在**低** 32 位——位移方向最容易写反
3. shell 内建命令（echo 等）不 execve，不触发事件
4. BCC 方言 C 不是标准 C，换 libbpf 时 lookup/update 语法要全部重写
5. ring buffer 太小 → 静默丢数据，务必检查 drop 计数
6. 尾调用 33 层上限、5.10 前与子程序互斥
