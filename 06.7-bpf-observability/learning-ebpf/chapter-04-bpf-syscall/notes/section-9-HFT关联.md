# HFT 关联

- **这一章就是"eBPF 的 TLPI 时刻"**：与 03-linux-userspace-api 模块学 perf_event_open/ioctl 的路径打通——eBPF 追踪复用 perf 子系统的基础设施，理解 perf_event_open 才能理解 kprobe 挂载为何长这样
- ring buf + epoll 是用户态收集内核事件的标准范式，与行情网关的 epoll 收包同构；跨核保序对延迟直方图/事件序列重建至关重要
- 引用计数的 pin 机制用于常驻观测 agent：交易机上 eBPF 监控随开机加载、独立于加载器进程存活，机器重启自动重载需要落地为 systemd unit
- strace -e bpf 是排查"eBPF 工具为何加载失败"的第一工具（看 BPF_PROG_LOAD 返回值即知验证是否通过）
