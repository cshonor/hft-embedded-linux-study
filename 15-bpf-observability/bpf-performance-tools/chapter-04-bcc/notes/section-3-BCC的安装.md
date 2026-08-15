# 4.3 BCC 的安装

> 底本：《BPF之巅》第 4 章 BCC（印刷 p91–136），4.3 节

## 内容详解

### 前置要求

| 要求 | 说明 |
|------|------|
| 内核版本 | **Linux 4.9+**（x86_64 上 4.9 起功能较全；部分工具需更高版本） |
| 内核配置 | `CONFIG_BPF=y`、`CONFIG_BPF_SYSCALL=y`、`CONFIG_BPF_JIT=y`，以及各事件源选项（kprobes、uprobe、tracepoints 等） |
| 权限 | root（或 `CAP_SYS_ADMIN`/`CAP_BPF` 等能力） |

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

## HFT 关联

- 交易机内核通常是 LTS 定制版，**部署前先核对内核版本 + CONFIG_BPF* 选项**（`grep -E 'CONFIG_BPF|CONFIG_UPROBE' /boot/config-$(uname -r)`）；
- 生产机尽量用发行版包（版本锁定、可审计），源码装留给开发/测试机。

## 陷阱

- ⚠️ `opensnoop: command not found` 十有八九是包名后缀问题——先 `ls /usr/sbin/*bpfcc*` 找真名。
- ⚠️ 云厂商定制内核可能裁剪了 uprobe/USDT 支持，工具加载报 `Operation not permitted` 时先查 config 而不是怀疑工具。

<details>
<summary>自测题</summary>

1. Ubuntu 发行版包安装后 opensnoop 的实际命令名是什么？
   <details><summary>答案</summary>`opensnoop-bpfcc`（bpfcc-tools 包所有工具带 `-bpfcc` 后缀）。</details>

2. BCC 对内核版本的最低要求大约是多少？
   <details><summary>答案</summary>Linux 4.9+（部分特性需更高版本）。</details>
</details>
