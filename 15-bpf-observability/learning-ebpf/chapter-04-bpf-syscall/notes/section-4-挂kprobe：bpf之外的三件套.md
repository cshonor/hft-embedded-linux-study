# 挂 kprobe：bpf() 之外的三件套

挂 kprobe **不用 bpf()**：

```
perf_event_open({type=6, ...})          = 7   # kprobe 也是一种 perf PMU 事件！
ioctl(7, PERF_EVENT_IOC_SET_BPF, 6)     = 0   # 把程序 fd 6 绑到 kprobe 事件 fd 7
ioctl(7, PERF_EVENT_IOC_ENABLE, 0)      = 0   # 使能
```

- type=6 来自 `/sys/bus/event_source/devices/kprobe/type`——kprobe 是动态注册的 PMU（perf 子系统本身就是 eBPF 的宿主基础设施）
- 对照：raw tracepoint 挂载就一条 `bpf(BPF_RAW_TRACEPOINT_OPEN, {name="sys_enter", prog_fd=6})`；cgroup 程序用 `bpf(BPF_PROG_ATTACH)`。**附加机制因程序类型而异**
