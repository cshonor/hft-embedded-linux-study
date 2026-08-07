# 04 — XDP 实操

> **Bootlin 课程模块：** XDP
> **对应 Rosen:** 无

## XDP 工具链

### xdp-tools

```bash
# 安装
apt install xdp-tools

# 加载 XDP 程序
xdp-loader load eth0 xdp_program.o

# 查看已加载程序
xdp-loader status

# 卸载
xdp-loader unload eth0 <id>
```

### libbpf + BPF CO-RE

```bash
# 编译
clang -target bpf -O2 -g -c xdp_prog.c -o xdp_prog.o

# 加载（通过 bpftool）
bpftool prog load xdp_prog.o /sys/fs/bpf/xdp_prog
bpftool net attach xdpgeneric name xdp_prog dev eth0
```

## 实验环境

### 树莓派 5 + veth

```bash
# 创建 veth 对
ip link add veth0 type veth peer name veth1
ip link set veth0 up
ip link set veth1 up

# 在 veth0 上加载 XDP（generic 模式）
xdp-loader load --mode generic veth0 xdp_prog.o

# 从 veth1 发包测试
ping -I veth1 10.0.0.1
```

### 实验清单

| 实验 | 目标 |
|------|------|
| XDP DROP all | 验证 XDP 生效（ping 不通） |
| XDP 按端口过滤 | 只放行特定端口 |
| XDP redirect CPUMAP | 多核收包分发 |
| AF_XDP 收包 | 用户态零拷贝接收 |
| XDP + page_pool | 观察 page_pool 统计 |
