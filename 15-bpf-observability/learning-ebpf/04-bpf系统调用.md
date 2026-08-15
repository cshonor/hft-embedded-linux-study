# Learning eBPF · 第 4 章：bpf() 系统调用

> 底本：`../LEARNING-EBPF-BILINGUAL.pdf`。用 strace 逐条解剖用户态到底发了哪些 syscall——这是全书最"内核视角"的一章，也是理解所有 eBPF 库（BCC/libbpf）本质的上限。

## 本章目标

1. 掌握 `bpf()` 系统调用签名与常用命令
2. 看懂 strace 下"加载程序 + 建 map + 挂事件 + 收数据"的完整 syscall 序列
3. 理解 BPF 对象的生命周期：fd → 引用计数 → pin → BPF link

## 1. bpf() 总览

```c
int bpf(int cmd, union bpf_attr *attr, unsigned int size);
```

- `cmd`：要执行的命令（BPF_MAP_CREATE、BPF_PROG_LOAD…全量清单在 `linux/bpf.h`，内核源码是最好文档）
- `attr`：命令参数（联合体，各命令用不同字段）
- `size`：attr 字节数

**关键分层**：内核里的 eBPF 程序访问 map **不走 syscall**（用 helper 函数）；syscall 接口只属于用户态。库（BCC/libbpf）的抽象与这些命令几乎一一对应。

## 2. strace 实例全景（hello-buffer-config.py）

```
bpf(BPF_BTF_LOAD, ...)                         = 3   # 加载 BTF 数据 → fd 3
bpf(BPF_MAP_CREATE, {PERF_EVENT_ARRAY ...})    = 4   # output perf buffer → fd 4
bpf(BPF_MAP_CREATE, {HASH, key 4B, value 12B,
     max_entries=10240, btf_fd=3})             = 5   # config 哈希表 → fd 5
bpf(BPF_PROG_LOAD, {KPROBE, insn_cnt=44,
     insns=..., license="GPL", prog_btf_fd=3}) = 6   # 程序 → fd 6（验证失败返回负值）
bpf(BPF_MAP_UPDATE_ELEM, {map_fd=5, ...})      = 0   # 写 config 表项
```

**各字段细节：**
- BPF_BTF_LOAD：跨内核版本可移植的类型信息（第 5 章）；老内核看不到这条
- config map 的 `key_size=4`（u32 存 UID）、`value_size=12`（user_msg_t）、`max_entries=10240` 是 **BCC 默认值**（源码没写就是它）
- `btf_fd=3` 让 bpftool 能漂亮打印 key/value 结构
- `insn_cnt`：字节码指令条数
- `expected_attach_type=BPF_CGROUP_INET_INGRESS` 看着像网络程序？其实该字段只对部分程序类型有意义，kprobe 不用；这个值只是枚举表第一个（=0）的默认占位
- 文件描述符是**进程私有**的：hello 程序里 fd 5 = config map，bpftool 里同一 map 可能是 fd 3

## 3. BPF 对象生命周期：引用计数

规则：**引用计数归零 → 内核删除对象**。计数来源：

1. **fd**：用户态进程持有；进程退出即释放（这就是 BCC 程序 Ctrl+C 后程序消失的原因）
2. **pin 到 bpffs**：`/sys/fs/bpf/` 伪文件系统（内存态，重启即失）；bpftool prog load 必须指定 pin 路径，否则它退出时程序立刻被删，加载毫无意义
3. **挂到 hook**：附加本身也计引用
   - 追踪类（kprobe/tracepoint）与用户态进程绑定，进程退即减
   - **网络栈/cgroup 类不绑定进程**——`ip link set dev eth0 xdp obj ...` 命令退出后程序仍在
4. **BPF link**：程序与事件之间的抽象层，link 自身可 pin；加载器进程退出后程序靠 link 的引用活着。libbpf 默认走 `bpf(BPF_LINK_CREATE)`（练习 8 可见）

补充命令：`BPF_PROG_BIND_MAP` 把 map 绑到程序（程序源码定义了却没用到的 map——如存放元数据的全局变量——不会被自动引用，需显式绑定防清理）。maps 同样有引用计数与 pin 能力。

## 4. 挂 kprobe：bpf() 之外的三件套

挂 kprobe **不用 bpf()**：

```
perf_event_open({type=6, ...})          = 7   # kprobe 也是一种 perf PMU 事件！
ioctl(7, PERF_EVENT_IOC_SET_BPF, 6)     = 0   # 把程序 fd 6 绑到 kprobe 事件 fd 7
ioctl(7, PERF_EVENT_IOC_ENABLE, 0)      = 0   # 使能
```

- type=6 来自 `/sys/bus/event_source/devices/kprobe/type`——kprobe 是动态注册的 PMU（perf 子系统本身就是 eBPF 的宿主基础设施）
- 对照：raw tracepoint 挂载就一条 `bpf(BPF_RAW_TRACEPOINT_OPEN, {name="sys_enter", prog_fd=6})`；cgroup 程序用 `bpf(BPF_PROG_ATTACH)`。**附加机制因程序类型而异**

## 5. perf buffer 初始化：为什么是每核一个

每颗 CPU 核执行一轮：

```
perf_event_open({PERF_TYPE_SOFTWARE, config=PERF_COUNT_SW_BPF_OUTPUT, ...}, -1, X, ...)
                                        # pid=-1, cpu=X → 测量该核上所有进程
ioctl(Y, PERF_EVENT_IOC_ENABLE)
bpf(BPF_MAP_UPDATE_ELEM, {map_fd=4, ...})   # map 第 X 项指向该核的 perf 缓冲
```

四核机器 = 4 轮 = `PERF_EVENT_ARRAY` 里 4 个条目——"array" 就是每核一缓冲的数组。之后用户态用 `ppoll()` 同时等 4 个 fd，哪个核触发读哪个。

## 6. Ring buffer：单缓冲 + epoll

`BPF_MAP_CREATE` 一条搞定（`key_size=0, value_size=0, max_entries=4096`），无每核 setup。优点：性能更好 + **跨核提交保序**（perf buffer 各核独立、顺序无保证）。

等待机制从 ppoll 升级为 epoll：

```
epoll_create1(EPOLL_CLOEXEC)                       = 8   # 内核里建一个 epoll 实例
epoll_ctl(8, EPOLL_CTL_ADD, 4, {EPOLLIN})          = 0   # 把 ringbuf fd 4 加入集合
epoll_pwait(8, ...)                                 # 阻塞等数据
```

ppoll 每次返回都要重新传整个 fd 集合；epoll 的 fd 集合由**内核对象**持有，注册一次即可。（与 03.5/04-cpp 网络编程模块里 poll vs epoll 的对比完全同源。）

## 7. 遍历 map：bpftool map dump 的 syscall 序列

找 map（对每个已加载 map 重复三连）：

```
bpf(BPF_MAP_GET_NEXT_ID, {start_id=N})     # 下一个 map id
bpf(BPF_MAP_GET_FD_BY_ID, {map_id})        # 换 fd
bpf(BPF_OBJ_GET_INFO_BY_FD, {...})         # 拿名字比对；无更多 map 时 GET_NEXT_ID 返回 ENOENT
```

读元素（迭代键值对）：

```
bpf(BPF_MAP_GET_NEXT_KEY, {key=NULL, next_key=...})   # key=NULL → 第一个有效键
bpf(BPF_MAP_LOOKUP_ELEM, {map_fd, key, value})        # 取值
... 循环直到 GET_NEXT_KEY 返回 ENOENT
```

配套还有 `BPF_MAP_DELETE_ELEM`（删除）。

## 坑点清单

1. fd 是进程私有的——跨进程传 fd 数字没有意义（同一 map 两个进程 fd 值不同）
2. bpffs 是内存伪文件系统：**重启后 pin 的程序全部消失**，开机自启要靠 systemd/skeleton 重载
3. BCC 程序 Ctrl+C 程序即卸载；想持久必须 pin 程序（或用 libbpf 的 link）
4. strace 里 `expected_attach_type` 出现网络类值不代表程序是网络程序（默认 0 占位）
5. perf buffer 每核一缓冲，事件跨核乱序；需要时间序就用 ring buffer（5.8+）
6. max_entries=10240 这种"魔法数字"是 BCC 默认，生产要显式设定

## HFT 关联

- **这一章就是"eBPF 的 TLPI 时刻"**：与 03-linux-userspace-api 模块学 perf_event_open/ioctl 的路径打通——eBPF 追踪复用 perf 子系统的基础设施，理解 perf_event_open 才能理解 kprobe 挂载为何长这样
- ring buf + epoll 是用户态收集内核事件的标准范式，与行情网关的 epoll 收包同构；跨核保序对延迟直方图/事件序列重建至关重要
- 引用计数的 pin 机制用于常驻观测 agent：交易机上 eBPF 监控随开机加载、独立于加载器进程存活，机器重启自动重载需要落地为 systemd unit
- strace -e bpf 是排查"eBPF 工具为何加载失败"的第一工具（看 BPF_PROG_LOAD 返回值即知验证是否通过）

## 自测题

1. bpf() 三个参数各是什么？内核里的 eBPF 程序访问 map 走什么路径？
2. BPF 对象的引用计数有哪四种来源？追踪类与网络类程序的差异？
3. 挂 kprobe 需要哪三个 syscall（按序）？为什么挂 raw tracepoint 只需一条 bpf()？
4. PERF_EVENT_ARRAY 里的条目数由什么决定？ring buffer 为何没有这个问题？
5. ppoll 和 epoll 管理 fd 集合的本质区别？
6. `bpftool map dump name config` 在找 map 和读元素两个阶段各用什么命令迭代？

## 交叉引用

- BTF 数据的内部结构 → `05-CO-RE一次编译处处运行.md`
- BPF_PROG_LOAD 触发的验证流程 → `06-验证器.md`
- 各程序类型的附加方式 → `07-程序类型.md`
- perf_event_open 通用机制 → 03-linux-userspace-api 模块 perf 相关笔记
