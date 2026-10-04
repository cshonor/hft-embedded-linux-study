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

## 2.4 BPF 自身作为攻击面：三条防线（sysctl 的攻防视角）

安全章的 sysctl 不只是"性能调优"——观测工具本身也是内核攻击面，每个开关对应一类威胁：

| 威胁 | 防线 | 说明 |
|---|---|---|
| 非特权用户加载恶意 BPF（侦察/提权第一步） | `unprivileged_bpf_disabled=1` | 一次性开关：置 1 后运行期改回 0 被内核拒绝——管理员想临时放开都做不到，必须重启。这是刻意的防回退设计 |
| JIT 泼洒攻击（JIT 镜像中的常量被猜出后构造 ROP） | `bpf_jit_harden=2` | 常量盲化（每次 JIT 随机化立即数编码），代价是二次编译性能损失；折中值 1 只对非特权用户启用 |
| 非特权用户灌爆 JIT 内存（DoS） | `bpf_jit_limit` | 到上限后非特权请求回落解释器执行——慢但不拒服务 |
| Spectre v2（BTI 注入） | `CONFIG_BPF_JIT_ALWAYS_ON` + jit_enable=1 | 排除解释器路径后，BPF 执行始终走可预测的 JIT 镜像 |

> 判断顺序：先威胁模型（谁能登录这台机器？）再定档。管理面机器 unprivileged=1 + harden=2；交易核心机 unprivileged=1 + harden=0（省每一点 CPU）。

## HFT 关联

- 生产交易机基线：`unprivileged_bpf_disabled=1` + `bpf_jit_enable=1`（性能与 Spectre 缓解兼得）+ 视威胁模型决定 jit_harden。
- "跟踪点→LSM→kprobe"三级降级策略同样适用于性能排障的工具选型。

<details>
<summary>自测题</summary>

1. kernel.unprivileged_bpf_disabled 有什么特殊行为？
2. jit_harden 的 0/1/2 分别是什么？代价是什么？
3. 三级分析策略的顺序及理由？

<details><summary>参考答案</summary>

1. **一次性设置**：置 1 后再写 0 被内核拒绝，运行期不可回退（需重启进内核参数）。目的是防攻击者先放开它再加载恶意 BPF。
2. 0=关闭（默认）；1=仅对非特权用户的程序启用强化；2=所有程序。机制是 JIT 常量盲化（随机化立即数编码，防 JIT 泼洒/ROP），代价是 JIT 输出变慢。注意 harden=2 时 jit_kallsyms 自动禁用（符号表会泄露盲化信息）。
3. tracepoint/USDT（稳定、有承诺）→ LSM `security_*` 钩子（安全语义内聚、比裸 kprobe 稳）→ kprobe/uprobe（任意覆盖但脆弱）。理由与性能章一致：稳定性递减、覆盖面递增，稳定优先。
</details>
</details>
