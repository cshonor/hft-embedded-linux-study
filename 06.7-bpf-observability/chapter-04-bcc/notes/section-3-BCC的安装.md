# 4.3 BCC 的安装

> 底本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.3 节

## 内容详解

### 前置要求

| 要求 | 说明 |
|------|------|
| 内核版本 | **Linux 4.9+**（x86_64 上 4.9 起功能较全；部分工具需更高版本） |
| 内核配置 | `CONFIG_BPF=y`、`CONFIG_BPF_SYSCALL=y`、`CONFIG_BPF_JIT=y`，以及各事件源选项（kprobes、uprobe、tracepoints 等） |
| 权限 | root（或 `CAP_SYS_ADMIN`/`CAP_BPF` 等能力） |

内核配置的具体开关与对应能力（部署前核对清单）：

```bash
grep -E 'CONFIG_BPF=|CONFIG_BPF_SYSCALL|CONFIG_BPF_JIT|CONFIG_UPROBE|\
CONFIG_KPROBE|CONFIG_TRACING|CONFIG_FTRACE_SYSCALLS|CONFIG_DEBUG_INFO_BTF' \
  /boot/config-$(uname -r)
```

| 开关 | 缺了会怎样 |
|---|---|
| `CONFIG_BPF_SYSCALL` | bpf(2) 不存在，一切免谈 |
| `CONFIG_BPF_JIT` | 走解释器，观测开销大数倍 |
| `CONFIG_KPROBE(E)s` | kprobe 系工具全灭 |
| `CONFIG_UPROBE_EVENTS` | uprobe/USDT 工具全灭（云厂商裁剪内核高发项） |
| `CONFIG_FTRACE_SYSCALLS` | `t:syscalls:*` 跟踪点不存在 |
| `CONFIG_DEBUG_INFO_BTF` | CO-RE 工具（libbpf-tools）无法重定位 |

### 各发行版安装（书中给出的现实情况：包名/前缀不统一）

| 发行版 | 包/命令 | 特点 |
|--------|---------|------|
| Ubuntu | `apt install bpfcc-tools` | 所有工具带 **`-bpfcc` 后缀**：`opensnoop-bpfcc`、`biolatency-bpfcc` |
| Ubuntu（iovisor 仓库） | 从 iovisor GitHub 仓库安装最新版 | 版本更新，与内核匹配更好 |
| snap 包 | `snap install bcc` | 工具带 **`bcc.` 前缀**：`bcc.opensnoop` |
| RHEL 7.6+ | `yum install bcc-tools` | 工具在 `/usr/share/bcc/tools/` |

要点：

- **同一个工具 opensnoop 可能叫 `opensnoop-bpfcc`、`bcc.opensnoop` 或裸 `opensnoop`**，取决于安装方式——写 runbook 时必须写全路径。
- 源码安装（`./setup.py build && ./setup.py install`）可获最新功能，但需要全套 Clang/LLVM 开发环境。
- 内核太老（如 4.1）时很多工具不可用：先查 `uname -r`，再对照 BCC 兼容矩阵。

### BCC 版本与内核的匹配规则（为什么"装不上/跑不了"常发生）

BCC 工具的内核态 C 是**运行时编译**的，它在目标机上 include 的 headers 必须与运行内核同源：

```text
运行内核 4.15 + 安装的 linux-headers-4.15  → 编译通过，字段偏移正确
运行内核 4.15 + 只有 headers-5.x           → 编译可能过（接口没变）或挂（接口变了）
运行内核 4.15 + 完全没装 headers           → 第 2 步就失败：找不到 linux/xxx.h
```

第三种在最小化安装的生产机上最常见，报错形态是一串 Clang fatal error——这不是 BPF 的问题，是**编译环境缺失**。CO-RE 版工具完全绕开这条链（不吃 headers，吃 BTF），这正是它适合生产分发的原因。

## HFT 关联

- 交易机内核通常是 LTS 定制版，**部署前先核对内核版本 + CONFIG_BPF* 选项**（上面 grep 清单）；
- 生产机尽量用发行版包（版本锁定、可审计），源码装留给开发/测试机；定制内核的场合，直接把 libbpf-tools 静态二进制打进部署物，摆脱对目标机软件源的依赖。
- 交易机最小化镜像的采购清单：`/sys/kernel/btf/vmlinux`（CO-RE 前提）+ `CAP_BPF/CAP_PERFMON`（容器场景的 capability 白名单）——这两个比"装 BCC"更接近本质需求。

## 陷阱

- ⚠️ `opensnoop: command not found` 十有八九是包名后缀问题——先 `ls /usr/sbin/*bpfcc*` 找真名。
- ⚠️ 云厂商定制内核可能裁剪了 uprobe/USDT 支持，工具加载报 `Operation not permitted` 时先查 config 而不是怀疑工具。
- ⚠️ 容器里跑 BCC 工具：需要挂 `/sys/kernel/debug`、`/sys/kernel/tracing` 且容器要有 perfmon 类 capability——镜像没配好时工具行为是"随机半失效"（有的探针挂得上、有的挂不上），最难排查。

<details>
<summary>自测题</summary>

1. Ubuntu 发行版包安装后 opensnoop 的实际命令名是什么？
   <details><summary>答案</summary>`opensnoop-bpfcc`（bpfcc-tools 包所有工具带 `-bpfcc` 后缀）。</details>

2. BCC 对内核版本的最低要求大约是多少？
   <details><summary>答案</summary>Linux 4.9+（部分特性需更高版本）。</details>

3. BCC 工具在目标机上编译失败报"找不到 linux/xxx.h"，根因是什么？CO-RE 工具为什么没这个问题？
   <details><summary>答案</summary>BCC 运行时编译要 include 与运行内核同源的 headers，目标机没装（或版本错位）即失败。CO-RE 工具编译期已完成，运行时靠 BTF（/sys/kernel/btf/vmlinux）做字段重定位，不吃 headers。</details>

4. 列出三个"部署前必查"的内核配置及其缺失后果。
   <details><summary>答案</summary>如 CONFIG_BPF_SYSCALL（bpf(2) 不存在）、CONFIG_UPROBE_EVENTS（uprobe/USDT 工具全灭）、CONFIG_DEBUG_INFO_BTF（CO-RE 无法重定位）——任一缺失对应一类工具不可用，先查 config 再怀疑工具。</details>
</details>
