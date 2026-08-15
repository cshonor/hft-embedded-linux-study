# 5.8 bpftrace 的帮助信息

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.8 节（印刷 p155–156）

## 内容详解

`bpftrace`（不带参数）或 `-h` 打印帮助：重要选项、环境变量、单行示例。

### 重要选项（书中 v0.9-232，2019-06-15）

| 选项 | 作用 |
|------|------|
| `-e 'program'` | 执行单行程序 |
| `-l [search]` | 列出探针 |
| `-p PID` | 对 PID 启用 USDT 探针 |
| `-c 'CMD'` | 运行命令并对其进程启用 USDT |
| `-d` / `-dd` | 调试信息 dry run（AST+LLVM IR / 更详细） |
| `-v` | 详情模式（运行时打印 BPF 字节码等） |
| `-o file` | 输出重定向到文件 |
| `-I DIR` / `--include FILE` | 头文件搜索路径 / 预处理前注入 #include |
| `-B MODE` | 输出缓冲模式（line/full/none） |
| `--unsafe` | 允许不安全内建函数（如 system()） |
| `-V` | 版本 |

### 重要环境变量

| 变量 | 默认 | 含义 |
|------|------|------|
| `BPFTRACE_STRLEN` | 64 | 每个 str() 的 BPF 栈字节数 |
| `BPFTRACE_NO_CPP_DEMANGLE` | 0 | 禁用 C++ 符号 demangle |
| `BPFTRACE_MAP_KEYS_MAX` | 4096 | 映射表最大键数 |
| `BPFTRACE_CAT_BYTES_MAX` | 10K | cat() 最大读取字节 |
| `BPFTRACE_MAXPROBES` | 512 | 最大探针数 |

### 示例（帮助内置）

```bash
bpftrace -l '*sleep*'                 # 列出含 sleep 的探针
bpftrace -e 'kprobe:do_nanosleep { printf("PID %d sleeping...\n", pid); }'
```

版本演进后帮助信息可能分长短两版，以当前版本输出为准。

## HFT 关联

- `BPFTRACE_MAP_KEYS_MAX`（默认 4096）是隐秘的坑：按海量键聚合（如按 IP 五元组）时会**静默丢数据/截断**——大键空间场景要显式调大；
- `BPFTRACE_STRLEN`（64B）限制路径/消息截断——抓长路径时调大（当前实现上限受 BPF 512B 栈限制）。

## 陷阱

- ⚠️ `-d`/`-v` 输出的字节码主要给 bpftrace 开发者；排障常规手段还是 printf（见 5.17）。
- ⚠️ 帮助信息随版本增长很快，别背——记住 `-l`、`-e`、`-c`、`--unsafe` 四个高频项即可。

<details>
<summary>自测题</summary>

1. str() 默认最大字符串长度？受什么限制？
   <details><summary>答案</summary>64 字节（BPFTRACE_STRLEN）；受 BPF 栈 512 字节上限限制。</details>

2. 运行一条命令并对其启用 USDT 用哪个选项？
   <details><summary>答案</summary>`-c 'CMD'`（对已运行进程则 `-p PID`）。</details>
</details>
