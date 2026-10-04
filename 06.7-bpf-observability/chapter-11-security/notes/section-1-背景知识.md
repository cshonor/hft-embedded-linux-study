# 1. 背景知识（11.1.1 BPF 的分析能力）

> 底本：《BPF之巅》第 11 章 安全，11.1 节（印刷 p516–521）

## 1.1 安全任务光谱

安全分析、实时取证嗅探、权限调试、可执行文件白名单、恶意软件逆向、监控、定制化审计、HIDS/CIDS（基于主机/容器的入侵检测）、策略执行、网络防火墙、检测恶意软件与动态阻塞数据包。安全工程与性能工程类似：都要分析大量不同的软件。

## 1.2 BPF 能回答的安全问题

- 正在执行的进程有哪些？
- 什么网络连接正在建立，来自哪个进程？
- 什么系统权限正被请求、被谁请求？
- 系统内发生了哪些权限拒绝（EPERM/EACCES）？
- 是否在以给定参数调用某内核/用户态函数（实时入侵行为检测）？

图 11-1（BPF 安全监控目标，Netflix 2017）按层列出可跟踪目标：SSH 认证/su/sudo/shell 命令/libpam 事件（应用层）、创建进程/打开文件/删除文件/变更文件模式（OS/VFS）、权限能力使用（权限控制层）、缺页/进程崩溃/修改分区表/加载内核模块（内核）、无效网络包/套接字绑定/TCP 主动被动连接/ICMP 可疑包/端口拒绝/UDP 连接（网络）。

## 1.3 零日漏洞检测（Docker symlink-race 案例）

- bpftrace 是数分钟内创建定制检测工具的理想语言（跟踪点+USDT+kprobes/uprobes+参数）。
- 案例：Docker `docker cp` 漏洞 = 循环中用 `RENAME_EXCHANGE` 标志调用 `renameat2(2)`——生产系统上极罕见，可作检测特征：

```bash
bpftrace -e 't:syscalls:sys_enter_renameat2 /args->flags == 2/ {
  time(); printf("%s RENAME_EXCHANGE %s <-> %s\n", comm,
  str(args->oldname), str(args->newname)); }'
```

- 正常无输出；PoC 运行时刷屏输出时间戳+进程名+文件名。
- 作者构想：未来漏洞披露附带 bpftrace 单行/BPF 检测工具，自动分发到全公司基础设施（类比 Snort 共享蠕虫检测规则）。

## 1.4 安全监控 vs LKM 方案

- 传统 HIDS 用可加载内核模块观测 → 模块本身引入内核错误与漏洞风险；**BPF 自带验证器，更安全**。
- 效率：2016 年内部测试，BPF 程序开销约为 auditd 类似功能的 **1/6**。
- **高负载行为设计（关键！）**：BPF 输出缓冲区与映射表有大小限制，超限时事件会被丢弃——**攻击者可制造海量事件淹没系统以逃避日志/策略执行**。方案：
  - 记录溢出与事件缺失（满足不可反悔 Non-repudiation 要求）；
  - 每 CPU 固定计数器映射：一旦创建就**不会丢事件计数**（细节 perf 输出可能丢，但计数不丢）。

## 1.5 策略执行（BPF Beyond Observability）

| 技术 | 机制 |
|---|---|
| seccomp | 经典 BPF 程序决定系统调用允许与否：SECCOMP_RET_KILL_PROCESS / RET_ERRNO / RET_USER_NOTIF（用户态辅助程序经 fd 处理） |
| Cilium | XDP/cgroup/tc 钩子组合的透明安全网络与负载均衡（sch_clsact + cls_bpf 修改/转发/丢包） |
| bpfilter | 用 BPF 完全替代 iptables 的 PoC；iptables 规则自动转换 |
| Landlock | BPF 安全模块，细粒度访问控制（如基于用户态可更新的 inode 映射表限制文件系统访问） |
| KRSI | Google 的内核运行时安全插桩 LSM，新程序类型 BPF_PROG_TYPE_KRSI |

- `bpf_send_signal()`（Linux 5.3）：BPF 程序在内核上下文**直接**发 SIGKILL 等信号，消除检测→执行的延迟。例：

```bash
bpftrace --unsafe -e 't:syscalls:sys_enter_renameat2 /args->flags == 2/ {
  time(); printf("killing PID %d %s\n", pid, comm); signal(9); }'
```

- 过渡方案：用户态跟踪器从 perf 缓冲读事件后 `system("kill -9 %d", pid)`——异步、有延迟窗口。
- 也可用 SIGABRT 触发 coredump 做恶意软件取证。

## HFT 关联

- 交易机最小化攻击面：`capable` 输出即最小权限白名单底稿（section-6）。
- 零日应急：监管新披露漏洞时，bpftrace 单行是当天即可部署的检测/阻断手段。
- 警惕"事件洪水绕过日志"：安全类 BPF 必须监控映射溢出计数。

<details>
<summary>自测题</summary>

1. 为什么 BPF 比 LKM 方案更适合做 HIDS？
2. 攻击者如何利用 BPF 缓冲区限制逃避监控？两种防御设计是什么？
3. bpf_send_signal 与 system("kill") 的本质区别？
4. Docker symlink-race 漏洞的检测特征是什么？
</details>
