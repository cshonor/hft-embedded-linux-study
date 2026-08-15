# 2. BPF 安全配置与分析策略（11.1.2–11.1.4）

> 底本：《BPF之巅》第 11 章 安全，11.1 节（印刷 p521–523）

## 2.1 无特权 BPF 用户（11.1.2）

Linux 5.2 中，无 `CAP_SYS_ADMIN` 的用户只能使用套接字过滤类 BPF。`kernel/bpf/syscall.c` 的检查：

```c
if (type != BPF_PROG_TYPE_SOCKET_FILTER &&
    type != BPF_PROG_TYPE_CGROUP_SKB &&
    !capable(CAP_SYS_ADMIN))
        return -EPERM;
```

- BCC 工具报"需要超级用户权限"；bpftrace 检查 UID==0，否则拒绝运行——这就是本书所有工具都在手册页第 8 节（超级用户工具）的原因。
- 未来方向（LSFMM 2019 讨论）：/dev/bpf 设备 + task_struct 标志（exec 时自动关闭）。容器内跑 BPF 的场景见第 15 章。

## 2.2 配置 BPF 安全策略：sysctl（11.1.3）

```bash
# sysctl -a | grep bpf
kernel.unprivileged_bpf_disabled = 1
net.core.bpf_jit_enable    = 1
net.core.bpf_jit_harden    = 0
net.core.bpf_jit_kallsyms  = 0
net.core.bpf_jit_limit     = 264241152
```

| sysctl | 取值 | 说明 |
|---|---|---|
| kernel.unprivileged_bpf_disabled | 0/1 | 禁非特权用户访问 BPF；**一次性设置，置 1 后改回 0 被拒绝** |
| net.core.bpf_jit_enable | 0/1/2 | 0 禁用（默认）；1 启用（性能+安全双收益）；2 启用+调试日志（仅调试）。CONFIG_BPF_JIT_ALWAYS_ON 可在编译层排除解释器（Spectre v2 缓解）。Netflix/Facebook 默认启用。x86_64/arm64 生产级完备；ppc64/s390x/sparc64/mips64/riscv 不一定 |
| net.core.bpf_jit_hardens | 0/1/2 | 0 关（默认）；1 仅非特权用户启用强化；2 所有用户。缓解 JIT 泼洒攻击，牺牲性能 |
| net.core.bpf_jit_kallsyms | 0/1 | 向特权用户发布 JIT 镜像符号表；启用 harden 时被禁用 |
| net.core.bpf_jit_limit | 字节 | JIT 可用内存上限；到达后非特权请求回落到解释器 |

更多强化细节：Cilium BPF 参考文档（Daniel Borkmann 撰写的强化章节）。

## 2.3 分析策略（11.1.4）

对未被现有工具覆盖的系统活动：

1. 先查是否有对应**内核跟踪点或 USDT 探针**；
2. 再查可跟踪的 **LSM 内核钩子**（以 `security_` 开头的函数）；
3. 最后用 **kprobes/uprobes** 插桩原始代码。

## HFT 关联

- 生产交易机基线：`unprivileged_bpf_disabled=1` + `bpf_jit_enable=1`（性能与 Spectre 缓解兼得）+ 视威胁模型决定 jit_harden。
- "跟踪点→LSM→kprobe"三级降级策略同样适用于性能排障的工具选型。

<details>
<summary>自测题</summary>

1. kernel.unprivileged_bpf_disabled 有什么特殊行为？
2. jit_harden 的 0/1/2 分别是什么？代价是什么？
3. 三级分析策略的顺序及理由？
</details>
