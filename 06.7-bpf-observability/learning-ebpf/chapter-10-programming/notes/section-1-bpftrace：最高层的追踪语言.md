# bpftrace：最高层的追踪语言

- 类 awk/C 的高级追踪语言，工具把脚本转成 eBPF 代码——**用户完全感知不到内核/用户空间分界**
- 建在 BCC 之上（脚本→BCC 程序→运行时 LLVM/Clang 编译）
- 只支持追踪类事件（kprobe/uprobe/tracepoint），不含网络类

```sh
bpftrace -l "*execve*"                    # 列出所有含 execve 的挂点

bpftrace -e 'kprobe:do_execve { @[comm] = count(); }'
# 按进程名统计 execve 次数：@[node]: 6  @[sh]: 6  @[cpuUsage.sh]: 18
```

### opensnoop.bt：entry/exit 配对的标准范式

```c
tracepoint:syscalls:sys_enter_open,
tracepoint:syscalls:sys_enter_openat          // 一个程序挂多个事件
{
    @filename[tid] = args->filename;          // 入口存参数（tid 为 key）
}
tracepoint:syscalls:sys_exit_open,
tracepoint:syscalls:sys_exit_openat
/@filename[tid]/                                // 过滤器：map 里存在才执行
{
    $ret = args->ret;
    $fd = $ret > 0 ? $ret : -1;
    $errno = $ret > 0 ? 0 : -$ret;
    printf("%-6d %-16s %4d %3d %s\n", pid, comm, $fd, $errno, str(@filename[tid]));
    delete(@filename[tid]);
}
```

- 为什么 entry+exit 都挂：**entry 拿参数（文件名），exit 拿结果（fd/errno）**——单一挂点拿不齐
- `Attaching 6 probes` = 4 个事件 + BEGIN/END 两个特殊探针（初始化/清理，同 awk）
- `bpftool prog list` 可看到 4 个 tracepoint 程序 + hash map（缓存文件名）+ perf_event_array（printf 输出通道）——高层语言生成的正是本书前几章手写的那些东西
