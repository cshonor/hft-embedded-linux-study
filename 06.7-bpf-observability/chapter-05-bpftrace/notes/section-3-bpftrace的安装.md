# 5.3 bpftrace 的安装

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.3 节（印刷 p141–143）

## 内容详解

### 5.3.1 内核版本要求

- 推荐 **Linux 4.9（2016-12）或更新**；主要 BPF 组件在 4.1–4.9 间加入，**越新的内核越好**（BCC 文档有各版本 BPF 特性列表）；
- 内核配置（多数发行版默认开启）：`CONFIG_BPF=y`、`CONFIG_BPF_SYSCALL=y`、`CONFIG_BPF_JIT=y`、`CONFIG_HAVE_EBPF_JIT=y`、`CONFIG_BPF_EVENTS=y`。

内核特性 → 脚本能力的对应（为什么"越新越好"不止是口号）：

| 内核版本 | 解锁的能力 | 对 bpftrace 脚本的影响 |
|---------|-----------|----------------------|
| 4.1 | bpf(2) 系统调用 | 存在的前提 |
| 4.4–4.6 | 大量 helper、stackid | kstack/ustack 聚合可用 |
| 4.7 | `--no-bpf` 之外的完备 perf_events attach | profile/interval/hardware 探针 |
| 4.9 | kprobe/kretprobe 上的 BPF + BTF 前身（书中的推荐线） | kprobe 计时模板全线可用 |
| 5.3 | **BPF 有界循环** | for/while 语法（替代 unroll 硬展开） |
| 5.x+ | BTF、`bpf_probe_read_kernel/user` 拆分 | CO-RE 式结构体访问、kptr/uptr |

"书里推荐的 4.9"是 2019 年的**能跑线**；2026 年的**舒服线**是 5.10+（BTF 默认开、有界循环、kptr/uptr 全都有）。老内核上不是不能用，而是每一条语法限制都要逐一核对。

CONFIG 五开关核对（与 4.3 BCC 一致的套路）：

```bash
# 一条命令核对全部
grep -E "CONFIG_(BPF|BPF_SYSCALL|BPF_JIT|HAVE_EBPF_JIT|BPF_EVENTS)[= ]" /boot/config-$(uname -r)
```

| CONFIG | 缺了会怎样 |
|--------|-----------|
| `CONFIG_BPF=y` | 一切免谈（bpf(2) 不存在） |
| `CONFIG_BPF_SYSCALL=y` | 用户态无法加载程序 |
| `CONFIG_BPF_JIT=y` | 解释执行，每条指令开销放大 1~2 个数量级——生产环境等于不可用 |
| `CONFIG_HAVE_EBPF_JIT=y` | 只有 cBPF JIT，eBPF 程序走解释器 |
| `CONFIG_BPF_EVENTS=y` | kprobe/tracepoint 探针类型挂不上（-e 直接报错） |

### 5.3.2 Ubuntu

```bash
sudo apt-get update
sudo apt-get install bpftrace        # 19.04+ 有包
```

源码安装（依赖 bison cmake flex g++ git libelf-dev zlib1g-dev libfl-dev systemtap-sdt-dev llvm-7-dev llvm-7-runtime libclang-7-dev clang-7）：

```bash
git clone https://github.com/iovisor/bpftrace
mkdir bpftrace/build; cd bpftrace/build
cmake -DCMAKE_BUILD_TYPE=Release
make
sudo make install
```

### 5.3.3 Fedora

```bash
sudo dnf install -y bpftrace
# 源码：dnf install -y bison flex cmake make git gcc-c++ elfutils-libelf-devel \
#   zlib-devel llvm-devel clang-devel bcc-devel，然后同上 cmake 流程
```

### 5.3.4 构建后的安装步骤（验证）

```bash
sudo ./tests/bpftrace_test                                    # 跑测试
sudo ./src/bpftrace -e 'kprobe:do_nanosleep { printf("sleep by %s\n", comm); }'
sudo make install    # 二进制→/usr/local/bin/bpftrace，工具→/usr/local/share/bpftrace/tools
```

默认前缀 `-DCMAKE_INSTALL_PREFIX=/usr/local`。

验证命令选 `do_nanosleep` 是有讲究的：任何系统每秒都有进程在睡（cron、sshd 心跳……），**不需要制造负载就能看到输出**。自己换验证探针时优先选这类"天然自触发"的函数。

### 5.3.5 其他发行版

先查有无安装包；详见仓库 `INSTALL.md`。

## HFT 关联

- 交易机最小依赖部署：优先发行版包；源码构建需带全套 LLVM/Clang，只留在构建机做，产物拷贝部署（注意 glibc/内核 ABI 匹配）。
- 部署前核对 `uname -r` 与五个 CONFIG 选项（与 BCC 相同套路）。
- 部署检查单（拿到一台新交易机时）：

```bash
uname -r                                   # 1. 内核版本 ≥ 4.9（舒服线 5.10+）
grep -cE "CONFIG_(BPF|BPF_SYSCALL|BPF_JIT|HAVE_EBPF_JIT|BPF_EVENTS)=y" \
  /boot/config-$(uname -r)                 # 2. 期望输出 5
bpftrace --version                         # 3. 版本决定语法上限
sudo bpftrace -e 'kprobe:do_nanosleep { printf("%s\n", comm); }'
                                           # 4. 端到端冒烟（探针+验证器+输出全链路）
bpftrace -l 'kprobe:vfs_*' | head          # 5. 探针可见性 + BTF/头文件解析
```

## 陷阱

- ⚠️ 老书时期的包很少——现在主流发行版都有 `bpftrace` 包，但版本差异大（语言特性如 `curtask`、`unroll` 上限等随版本变），脚本要注明最低版本。
- ⚠️ `make install` 的工具路径不在 PATH 中时，脚本 shebang `#!/usr/local/bin/bpftrace` 找不到解释器。
- ⚠️ 源码构建的 bpftrace 绑定构建机的 libbcc/libbpf——拷贝部署到目标机时，目标机的 BCC 库版本不匹配会出现"编译期正常、运行期探针挂载失败"这类跨机问题。runbook 里注明构建机与目标机的配对关系。

<details>
<summary>自测题</summary>

1. bpftrace 推荐的最低内核版本？
   <details><summary>答案</summary>Linux 4.9。</details>

2. 构建后如何快速验证安装成功？
   <details><summary>答案</summary>跑 `tests/bpftrace_test`，或执行一条 `kprobe:do_nanosleep` 单行程序看输出。</details>

3. `CONFIG_BPF_JIT` 关闭时 bpftrace 还能用吗？后果是什么？
   <details><summary>答案</summary>能加载能跑，但 BPF 程序解释执行，每事件开销放大 1~2 个数量级——观测本身就成了性能问题，生产环境视为不可用。</details>
</details>
