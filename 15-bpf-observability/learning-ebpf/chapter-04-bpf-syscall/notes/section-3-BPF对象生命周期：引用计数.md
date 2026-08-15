# BPF 对象生命周期：引用计数

规则：**引用计数归零 → 内核删除对象**。计数来源：

1. **fd**：用户态进程持有；进程退出即释放（这就是 BCC 程序 Ctrl+C 后程序消失的原因）
2. **pin 到 bpffs**：`/sys/fs/bpf/` 伪文件系统（内存态，重启即失）；bpftool prog load 必须指定 pin 路径，否则它退出时程序立刻被删，加载毫无意义
3. **挂到 hook**：附加本身也计引用
   - 追踪类（kprobe/tracepoint）与用户态进程绑定，进程退即减
   - **网络栈/cgroup 类不绑定进程**——`ip link set dev eth0 xdp obj ...` 命令退出后程序仍在
4. **BPF link**：程序与事件之间的抽象层，link 自身可 pin；加载器进程退出后程序靠 link 的引用活着。libbpf 默认走 `bpf(BPF_LINK_CREATE)`（练习 8 可见）

补充命令：`BPF_PROG_BIND_MAP` 把 map 绑到程序（程序源码定义了却没用到的 map——如存放元数据的全局变量——不会被自动引用，需显式绑定防清理）。maps 同样有引用计数与 pin 能力。
