# 05 — eBPF 网络

> **Bootlin 课程模块：** eBPF Networking
> **对应 Rosen:** 无

## eBPF 网络程序全景

| 类型 | 挂载点 | 工具 |
|------|--------|------|
| XDP | 驱动层 | xdp-loader |
| tc-BPF | tc ingress/egress | tc |
| cgroup-BPF | cgroup | bpftool cgroup |
| sk_msg | socket | skmsg |
| sk_reuseport | SO_REUSEPORT | — |

## tc-BPF 实验

```bash
# 加载 tc-BPF 分类器
tc qdisc add dev eth0 clsact
tc filter add dev eth0 ingress bpf da obj tc_prog.o sec ingress

# 查看统计
tc filter show dev eth0 ingress

# 删除
tc qdisc del dev eth0 clsact
```

## BPF Map 实验

```bash
# 查看所有 BPF map
bpftool map show

# 查看 map 内容
bpftool map dump id <map_id>

# 查看加载的程序
bpftool prog show
```

## 调试 BPF 程序

```bash
# BPF trace pipe
cat /sys/kernel/debug/tracing/trace_pipe

# bpf_trace_printk() 输出到 trace pipe
# 在 BPF 程序中：
# bpf_trace_printk("pkt: port=%d\n", port);
```
