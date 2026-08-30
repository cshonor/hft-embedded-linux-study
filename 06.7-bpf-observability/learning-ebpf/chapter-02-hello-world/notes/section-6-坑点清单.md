# 坑点清单

1. trace_pipe 全机共享且慢——生产一律用 ring buffer / map
2. `bpf_get_current_pid_tgid()` 的 PID 在**高** 32 位（tgid），UID 在**低** 32 位——位移方向最容易写反
3. shell 内建命令（echo 等）不 execve，不触发事件
4. BCC 方言 C 不是标准 C，换 libbpf 时 lookup/update 语法要全部重写
5. ring buffer 太小 → 静默丢数据，务必检查 drop 计数
6. 尾调用 33 层上限、5.10 前与子程序互斥
7. **内核里拿到用户态指针不能直接解引用**（真机实测）——如 openat 的 `ctx->filename` 是用户地址，内核态直接读会崩，必须 `bpf_probe_read_user_str()` 先拷到栈数组；verifier 会在加载时强制这一点
8. **`bpf_trace_printk` 的 `%s` 收的是指针**——传本地栈数组合法，传用户指针非法；所以"先 probe_read 再打印"是两步操作，不能一步到位
