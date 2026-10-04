# 5.8 bpftrace 的帮助信息

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.8 节（印刷 p155–156）

## 内容详解

`bpftrace`（不带参数）或 `-h` 打印帮助：重要选项、环境变量、单行示例。

### 重要选项（书中 v0.9-232，2019-06-15）

| 选项 | 作用 | 典型组合 |
|------|------|---------|
| `-e 'program'` | 执行单行程序 | 一切排障的入口 |
| `-l [search]` | 列出探针 | 配通配符先看探针名再写脚本（防挂错） |
| `-p PID` | 对 PID 启用 USDT 探针 | 探已运行进程 |
| `-c 'CMD'` | 运行命令并对其进程启用 USDT | 探"从出生到退出"的短命进程 |
| `-d` / `-dd` | 调试信息 dry run（AST+LLVM IR / 更详细） | 编译期排障（见 5.17.2） |
| `-v` | 详情模式（运行时打印 BPF 字节码等） | 运行期排障（见 5.17.3） |
| `-o file` | 输出重定向到文件 | 自动化收集（绕开 tee 的缓冲问题） |
| `-I DIR` / `--include FILE` | 头文件搜索路径 / 预处理前注入 #include | 读自编译内核的结构体时指路 |
| `-B MODE` | 输出缓冲模式（line/full/none） | 管道场景要 line，否则看不到实时输出 |
| `--unsafe` | 允许不安全内建函数（如 system()） | 慎用——见下 |
| `-V` | 版本 | runbook 第一行 |

`-l` 的正确用法（先侦察后开枪）：

```bash
bpftrace -l 'kprobe:vfs_*'          # 确认函数名/通配符展开符合预期
bpftrace -l 'tracepoint:syscalls:'  # 看 syscall 跟踪点的真实命名
bpftrace -l 'usdt:/path/to/binary:' # 看二进制里有哪些 USDT 探针
# 然后才写 -e 脚本——探针名拼错是新手第一报错来源
```

### 重要环境变量

| 变量 | 默认 | 含义 | 超限的后果 |
|------|------|------|-----------|
| `BPFTRACE_STRLEN` | 64 | 每个 str() 的 BPF 栈字节数 | 字符串静默截断（长路径只看到前 63 字符） |
| `BPFTRACE_NO_CPP_DEMANGLE` | 0 | 禁用 C++ 符号 demangle | — |
| `BPFTRACE_MAP_KEYS_MAX` | 4096 | 映射表最大键数 | **静默丢新键**（统计不全且无告警） |
| `BPFTRACE_CAT_BYTES_MAX` | 10K | cat() 最大读取字节 | cat 输出截断 |
| `BPFTRACE_MAXPROBES` | 512 | 最大探针数 | 通配符挂太多直接拒绝启动 |

两个"静默坑"的机理展开：

- **STRLEN 与 512B 栈的预算关系**：一次探针执行里所有 str()/局部变量共享 BPF 的 512 字节栈。默认 64B 意味着同一次执行最多 ~7 个 str() 就逼近上限（还有别的局部变量）——验证器报 "stack depth" 错误时，先数脚本里有几个 str()，而不是盲目调小 STRLEN。反过来，把 STRLEN 调到 256 再来两个 str() 同样爆栈：**变量（单个变大）和表达式（个数变多）两条路都会撞墙**。
- **MAP_KEYS_MAX 的静默丢失**：map 是哈希表，键数到上限后新键**插入失败但不报错**——按"IP 五元组 × 时间分桶"这类大键空间聚合时，表会停在 4096 行，你以为看到了全貌，其实只看到了前 4096 个流。发现"条目数恰好是 4096"这个特征值就该怀疑它。

### 示例（帮助内置）

```bash
bpftrace -l '*sleep*'                 # 列出含 sleep 的探针
bpftrace -e 'kprobe:do_nanosleep { printf("PID %d sleeping...\n", pid); }'
```

版本演进后帮助信息可能分长短两版，以当前版本输出为准。

## HFT 关联

- `BPFTRACE_MAP_KEYS_MAX`（默认 4096）是隐秘的坑：按海量键聚合（如按 IP 五元组）时会**静默丢数据/截断**——大键空间场景要显式调大；
- `BPFTRACE_STRLEN`（64B）限制路径/消息截断——抓长路径时调大（当前实现上限受 BPF 512B 栈限制）；
- 交易机固化脚本建议显式 `export` 这两个变量（即使值就是默认值）——把"隐式默认"变成"显式声明"，升级 bpftrace 版本时默认值变化也伤不到你；
- `-B line` 在 `| tee` / 管道进日志采集器时必加，否则全缓冲导致日志实时性归零（你以为没输出，其实堆在缓冲区）。

## 陷阱

- ⚠️ `-d`/`-v` 输出的字节码主要给 bpftrace 开发者；排障常规手段还是 printf（见 5.17）。
- ⚠️ 帮助信息随版本增长很快，别背——记住 `-l`、`-e`、`-c`、`--unsafe` 四个高频项即可。
- ⚠️ `--unsafe` 解锁的 system() 是**内核态事件直接触发用户态命令**——命令本身以 root 跑，探针高频触发时能 fork 炸掉机器。生产环境只在探针频率极低且有明确理由时使用。

<details>
<summary>自测题</summary>

1. str() 默认最大字符串长度？受什么限制？
   <details><summary>答案</summary>64 字节（BPFTRACE_STRLEN）；受 BPF 栈 512 字节上限限制。</details>

2. 运行一条命令并对其启用 USDT 用哪个选项？
   <details><summary>答案</summary>`-c 'CMD'`（对已运行进程则 `-p PID`）。</details>

3. 直方图聚合跑了十分钟，输出恰好 4096 行且不再增长，怀疑什么？
   <details><summary>答案</summary>BPFTRACE_MAP_KEYS_MAX（默认 4096）到达上限后新键静默丢弃——4096 这个特征值就是指纹；显式调大该环境变量重跑。</details>

4. 为什么脚本里三个 str() 各 64B 没爆栈，改成两个 str() 各 256B 反而验证器拒绝？
   <details><summary>答案</summary>栈预算按每次探针执行的所有 str() + 局部变量总和算：3×64=192B 可以，2×256=512B 单独就吃满 512B 栈上限，验证器直接拒绝。</details>
</details>
