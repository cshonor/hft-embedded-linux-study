# Ch 11 安全 · Security

> **BPF Performance Tools** · Brendan Gregg · **跳过 ⚪**

> 本章定位：**安全分析与性能工程的交叉** — eBPF 源于 **包过滤 / 防火墙 / IDS**；本章展示 BPF 用于 **入侵检测、行为白名单、权限最小化、实时取证**。许多工具与 [Ch 6](./chapter-06-CPU.md)/[Ch 8](./chapter-08-文件系统.md)/[Ch 10](./chapter-10-网络.md) **同名复用**（`execsnoop`、`opensnoop`、`tcpconnect`），视角从「性能」转为「谁干了什么」。  
> **HFT：** 生产机 **⚪ 默认跳过** 精读；与 **合规/共置隔离** 相关时查阅 **`capable`/`setuids`/`tcpconnect`** 做 **最小权限白名单**；**零日应急响应** 用 bpftrace 快速写 probe。勿与低延迟热路径 **长期** 同机全开。  
> **上一章：** [chapter-10-网络.md](./chapter-10-网络.md) · **下一章：** [chapter-12-语言.md](./chapter-12-语言.md)

---

## 1. 安全与 BPF 的共性

| 维度 | 性能工程 | 安全分析 |
|------|----------|----------|
| 目标 | 找瓶颈、降延迟 | 找异常、降风险 |
| 手段 | 追踪 syscall/栈/网络 | **同一套** 追踪能力 |
| 输出 | 直方图、火焰图 | 审计事件、告警 |

**BPF 安全任务：**

- 实时取证（live forensics）  
- 权限调试与 **最小 capability** 设计  
- 恶意软件行为路径  
- HIDS/CIDS（主机/容器入侵检测）  
- 与 **seccomp** 配合的系统调用策略  

---

## 2. BPF 在安全上的优势

### 零日 / 应急响应

| 传统 | bpftrace/BCC |
|------|--------------|
| 等 vendor 补丁或静态规则 | **数分钟** 写自定义脚本 |
| 固定 audit 规则 | **kprobe/uprobe/tracepoint** 追任意函数参数 |

**例：** 新披露漏洞涉及某 syscall → 临时 `tracepoint:syscalls:sys_enter_*` 或 `kprobe` 看调用栈与参数。

→ 语法：[Ch 5 bpftrace](./chapter-05-bpftrace.md)

### 性能 vs `auditd`

Gregg 2016 内部对比（书中引用）：

| | **auditd** | **等价 BPF** |
|---|------------|--------------|
| 开销 | 基准 | 约 **低 6×** |
| 粒度 | 规则驱动日志 | 内核过滤 + 聚合 |

**HFT：** 若合规要求 syscall 审计，评估 **BPF 替代/补充** 对 **P99** 的影响 — 仍须短规则、限 PID。

### seccomp + BPF

**seccomp** 用 **BPF 程序** 在 **syscall 入口** 决定 allow/deny — 安全策略即「小型 BPF 过滤器」。

```
syscall 入口 → seccomp BPF 程序 → ALLOW / ERRNO / TRAP / KILL
```

**HFT：** 策略进程 **沙箱化** 时可参考 `capable`/`eperm` 观测结果反推 seccomp 规则集。

### BPF 自身安全配置

|  sysctl / 配置 | 作用 |
|----------------|------|
| **`kernel.unprivileged_bpf_disabled=1`** | 禁止非 root 加载 BPF（推荐生产） |
| **`net.core.bpf_jit_harden`** / **`bpf_jit_harden`** | JIT 强化，降低代码注入面 |
| 限制 perf_event / ptrace | 纵深防御（发行版策略各异） |

**运维：** 观测工具需要 **root/CAP_BPF/CAP_PERFMON** — 与 **最小权限** 平衡。

---

## 3. 进程与模块执行

### `execsnoop`

全系统 **`execve`** — 新进程执行。

```bash
sudo execsnoop-bpfcc
```

| 安全 | 性能（Ch 6） |
|------|--------------|
| 发现恶意/未知二进制 | 发现短命脚本消耗 CPU |

### `elfsnoop`

追踪 **ELF 二进制加载** — 比 `execsnoop` 更贴近内核 ELF 处理路径，抓 **隐蔽加载**。

```bash
sudo elfsnoop-bpfcc
```

### `modsnoop`

监控 **内核模块加载**（`modprobe` 等）— **Rootkit** 常见手法。

```bash
sudo modsnoop-bpfcc
```

**HFT 共置机：** 交易节点不应动态加载未知 **ko** — 非零输出即告警。

---

## 4. 终端与 Shell 监控（实时取证）

### `bashreadline`

静默追踪 **Bash `readline()`** — 抓取交互式输入命令（含受限 shell 场景）。

```bash
sudo bashreadline-bpfcc
```

### `shellsnoop`

**镜像** 指定 Shell 会话的 **STDOUT/STDERR** — 看攻击者屏幕输出。

### `ttysnoop`

在 **TTY/PTS 设备层** 监视会话 — 适用于 **SSH 登录** 实时旁观。

**HFT：** 仅 **堡垒机/运维跳板** 场景；**策略服务器** 不应有交互 shell — 工具用于 **事故调查**，非常态监控。

---

## 5. 权限与能力 (Capabilities)

Linux **capabilities** 拆分 root 特权 — 最小权限设计的基础。

### `capable`

追踪 **capability 检查**（如 `CAP_SYS_ADMIN`、`CAP_NET_RAW`）。

```bash
sudo capable-bpfcc
```

| 用途 | 说明 |
|------|------|
| **安全** | 看谁在校验/使用特权 |
| **白名单** | 跑一遍合法 workload → 记录 **实际需要** 的 cap → 其余 **drop** |

**HFT：** 新服务上线前 **`capable` 短跑** → 写 systemd `CapabilityBoundingSet` / Docker `--cap-drop`。

### `setuids`

追踪 **`setuid` / `setresuid` / `setfsuid`** — 权限提升（`sudo`、`sshd` 等）。

```bash
sudo setuids-bpfcc
```

### `eperm`

统计 syscall 返回 **`EPERM` / `EACCES`** — 识别 **反复越权尝试** 的进程。

```bash
sudo eperm-bpfcc
```

**场景：** 漏洞利用探测、错误 seccomp 规则调试。

---

## 6. 网络与文件异常

### `tcpconnect` / `tcpaccept`

| 工具 | 安全视角 |
|------|----------|
| `tcpconnect` | **意外 outbound** — 编译器/工具链突然外连 |
| `tcpaccept` | 不应监听的服务开始 **accept** |

→ 性能视角：[Ch 10](./chapter-10-网络.md)

```bash
sudo tcpconnect-bpfcc
sudo tcpaccept-bpfcc
```

### `tcpreset`

追踪内核发送 **TCP RST** — **端口扫描** 常见特征。

```bash
sudo tcpreset-bpfcc
```

### `opensnoop`

监控 **`open()` / `openat()`** — 是否读 **`/etc/passwd`**、密钥路径等。

```bash
sudo opensnoop-bpfcc
```

→ 性能视角：[Ch 8](./chapter-08-文件系统.md)

---

## 7. BPF 单行命令 (One-Liners)

安全场景 **ad hoc** 极有价值 — PAM/SSH/sudo 监控示例（概念）：

```bash
# 监控 PAM 会话启动（tracepoint 名因发行版/版本而异，先用 bpftrace -l 搜索 pam）
bpftrace -e 'tracepoint:pam:* { printf("%s %s\n", comm, probe); }'

# 追踪 sudo 执行（示意）
bpftrace -e 'tracepoint:syscalls:sys_enter_execve /comm == "sudo"/ {
    printf("sudo by %s\n", comm);
}'
```

**原则：** 应急响应 **先 `-l` 列 probe** → 短跑验证 → 固化 BCC 脚本或 SIEM 规则。

→ [Ch 5](./chapter-05-bpftrace.md) · [附录 A](./appendix-A-bpftrace单行命令.md)

---

## 8. 工具选型速查（安全视角）

| 怀疑 | 工具 |
|------|------|
| 未知进程/脚本 | `execsnoop`、`elfsnoop` |
| Rootkit / 恶意 ko | `modsnoop` |
| 交互式攻击 | `bashreadline`、`ttysnoop` |
| 权限过大 | **`capable`** → 白名单 |
| 提权 | `setuids` |
| 越权探测 | `eperm` |
| 意外外连 | **`tcpconnect`** |
| 端口扫描 | `tcpreset` |
| 敏感文件访问 | **`opensnoop`** |

---

## 9. 与性能章节的工具对照

| 工具 | 性能章 | 安全章 |
|------|--------|--------|
| `execsnoop` | Ch 6 短命进程 | 恶意执行 |
| `opensnoop` | Ch 8 路径/配置 | 敏感文件 |
| `tcpconnect` | Ch 10 工作负载 | 异常外连 |
| `tcpaccept` | Ch 10 接入 | 非预期监听 |

**同一工具，不同叙事** — 安全 runbook 可复用性能工具链，**不必另建一套 agent**。

---

## 10. HFT 读者 Takeaway

1. **⚪ 默认跳过** — 除非合规、共置审计、post-incident 取证。
2. **`capable` + 最小 cap** — 新二进制上线前的 **权限摸底**，比事后 `setuids` 告警更省事。
3. **`tcpconnect`/`opensnoop`** — 策略机 **不应外连/读敏感路径**；与 Ch 10/8 工具相同，可纳入 **轻量合规巡检**（低频 cron，非 tick 路径）。
4. **BPF 比 auditd 轻** — 若必须用 syscall 审计，优先评估 BPF 方案对延迟的影响。
5. **`kernel.unprivileged_bpf_disabled=1`** — 生产推荐；观测脚本由 **受控 root/automation** 运行。
6. **勿在最低延迟核长期挂安全 probe** — 与性能观测同一纪律：短窗口、限 scope。
7. 语言层栈：[Ch 12](./chapter-12-语言.md) — 安全+性能需 **带符号的 ustack**。

---

## 相关章节

- 上一章：[chapter-10-网络.md](./chapter-10-网络.md)
- 下一章：[chapter-12-语言.md](./chapter-12-语言.md)
- execsnoop：[chapter-06-CPU.md](./chapter-06-CPU.md)
- opensnoop：[chapter-08-文件系统.md](./chapter-08-文件系统.md)
- tcpconnect：[chapter-10-网络.md](./chapter-10-网络.md)
- bpftrace：[chapter-05-bpftrace.md](./chapter-05-bpftrace.md)
