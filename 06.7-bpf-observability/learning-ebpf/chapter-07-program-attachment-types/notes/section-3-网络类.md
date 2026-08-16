# 网络类

与追踪类的本质区别：**定制行为**而非观察——(1) 返回码决定包的去向（正常处理/丢弃/重定向）；(2) 可修改网络包、socket 配置。

上下文 = 网络消息本身，结构取决于栈中位置：栈底是 L2 原始字节包，栈顶是 socket buffer。

| 挂点 | 程序类型 | 说明 |
|---|---|---|
| socket 过滤 | `SOCKET_FILTER` | **史上第一个** eBPF 程序类型；过滤的是发给 tcpdump 等观察工具的**数据副本**，不是应用数据 |
| socket 操作 | `SOCK_OPS` | 拦截 socket 上各种事件，设 TCP 超时等参数；socket 只存在于连接端点 |
| sockmap | `SK_SKB` | 配合持有 socket 引用的特殊 map，在 socket 层重定向流量 |
| TC（流量控制） | tc 分类器/过滤器 | 内核完整 TC 子系统，可挂 ingress/egress；Cilium 的基石之一；`tc` 命令可直接操作 |
| XDP | `xdp` | 收包最早期（驱动层，可 offload 到网卡）；**按接口挂载**，不同接口可挂不同程序 |
| 流解析器 | `FLOW_DISSECTOR` | 自定义包头部解析 |
| 轻量隧道 | `LWT_*` | eBPF 做网络封装；实践少见 |
| cgroup | `CGROUP_SOCK` / `CGROUP_SKB` 等 | 只对某 cgroup 内进程生效（容器/pod 隔离机制之一）；可做网络安全策略、甚至"骗"进程它连上了某个地址（服务网格）；另有 `CGROUP_SYSCTL` 挂 sysctl |
| 红外 | `LIRC_MODE2` | 红外协议解码——证明"追踪/网络"二分法并不完备 |

XDP 常用命令：

```
ip link set dev eth0 xdp obj hello.bpf.o sec xdp    # 挂载
ip link set dev eth0 xdp off                         # 卸载
# ip link show 里可见 prog/xdp id 1255 tag ... jited
```
