# 5.3 bpftrace 的安装

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.3 节（印刷 p141–143）

## 内容详解

### 5.3.1 内核版本要求

- 推荐 **Linux 4.9（2016-12）或更新**；主要 BPF 组件在 4.1–4.9 间加入，**越新的内核越好**（BCC 文档有各版本 BPF 特性列表）；
- 内核配置（多数发行版默认开启）：`CONFIG_BPF=y`、`CONFIG_BPF_SYSCALL=y`、`CONFIG_BPF_JIT=y`、`CONFIG_HAVE_EBPF_JIT=y`、`CONFIG_BPF_EVENTS=y`。

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

### 5.3.5 其他发行版

先查有无安装包；详见仓库 `INSTALL.md`。

## HFT 关联

- 交易机最小依赖部署：优先发行版包；源码构建需带全套 LLVM/Clang，只留在构建机做，产物拷贝部署（注意 glibc/内核 ABI 匹配）。
- 部署前核对 `uname -r` 与五个 CONFIG 选项（与 BCC 相同套路）。

## 陷阱

- ⚠️ 老书时期的包很少——现在主流发行版都有 `bpftrace` 包，但版本差异大（语言特性如 `curtask`、`unroll` 上限等随版本变），脚本要注明最低版本。
- ⚠️ `make install` 的工具路径不在 PATH 中时，脚本 shebang `#!/usr/local/bin/bpftrace` 找不到解释器。

<details>
<summary>自测题</summary>

1. bpftrace 推荐的最低内核版本？
   <details><summary>答案</summary>Linux 4.9。</details>

2. 构建后如何快速验证安装成功？
   <details><summary>答案</summary>跑 `tests/bpftrace_test`，或执行一条 `kprobe:do_nanosleep` 单行程序看输出。</details>
</details>
