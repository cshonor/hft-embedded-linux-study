# 5.12 bpftrace 的变量

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.12 节（印刷 p165–170）

## 内容详解

三类变量：内置变量、临时变量（`$`）、映射表变量（`@`）。

### 5.12.1 内置变量（表 5-5）

| 类型 | 变量 | 描述 |
|------|------|------|
| integer | `pid` | 进程 ID（内核中的 tgid） |
| integer | `tid` | 线程 ID（内核中的 pid） |
| integer | `uid` | 用户 ID |
| string | `username` | 用户名 |
| integer | `nsecs` | 时间戳（纳秒） |
| integer | `elapsed` | 自 bpftrace 启动的纳秒时间戳 |
| integer | `cpu` | 处理器 ID |
| string | `comm` | 进程名 |
| string | `kstack` / `ustack` | 内核/用户态调用栈 |
| integer | `arg0`…`argN` | 探针参数（k/uprobe） |
| struct | `args` | 探针参数（tracepoint/usdt 结构体） |
| integer | `retval` | 返回值（kret/uretprobe） |
| string | `func` | 被跟踪函数名 |
| string | `probe` | 当前探针全名 |
| integer | `curtask` | 内核 task_struct 地址（u64，可强转） |
| integer | `cgroup` | Cgroup ID |
| int/char* | `$1`…`$N` | 位置参数 |

**注意：pid/tid 命名反直觉**——bpftrace 的 `pid` = 内核 tgid（进程），`tid` = 内核 pid（线程）。所有整数目前均为 64 位无符号。

### 5.12.2 pid、comm 和 uid

```bash
# bpftrace -e 't:syscalls:sys_enter_setuid { printf("setuid by PID %d (%s), UID %d\n", pid, comm, uid); }'
setuid by PID 3907 (sudo), UID 1000
setuid by PID 14593 (evil), UID 33
```

只看调用不够——成功与否要看**出口跟踪点**：

```bash
# bpftrace -e 'tracepoint:syscalls:sys_exit_setuid { printf("setuid by %s returned %d\n", comm, args->ret); }'
setuid by sudo returned 0
setuid by evil returned -1     # ← 失败！
```

### 5.12.3 kstack 和 ustack

- 返回多行字符串调用栈，**最大深度 127**；`kstack(n)`/`ustack(n)` 函数可指定深度（最大 1024）；
- 栈顺序：先子函数后父函数，每帧带函数名+偏移；
- **栈可作映射表键**——按栈聚合计数比逐次打印高效（内核上下文完成计数；BPF 把每栈转为唯一栈 ID，bpftrace 读频次再查栈内容）：

```bash
bpftrace -e 't:block:block_rq_insert { @[kstack] = count(); }'
```

### 5.12.4 位置参数

- `$1`、`$2`…来自命令行（同 shell 位置参数）：`./watchconn.bt 181`；
- 默认整数类型；字符串参数须 `str($1)` 访问；
- 未传递的参数有默认值：整数 0、字符串 ""。

### 5.12.5 临时变量

- `$name`，仅动作块内有效；类型首次赋值确定（整数/字符串/结构体指针/结构体）。

### 5.12.6 映射表变量

- `@name`、`@name[key]`、`@name[key1,key2,...]`——BPF 映射表（哈希/关联数组）存储；
- **键/值类型必须前后一致**，类型由首次赋值决定（含特殊统计函数类型）：

```awk
@start = nsecs;              // 整数
@last[tid]  = nsecs;         // 键整数，值整数
@bytes      = hist(retval);  // 特殊类型：2 的幂直方图
@who[pid,comm] = count();    // 复合键（int,string）→ count 统计
```

## HFT 关联

- `pid` vs `tid` 语义（tgid vs 内核 pid）在多线程交易引擎上**必须搞清**——按线程统计用 `tid`，按进程用 `pid`；
- `elapsed` 适合测量工具自身运行窗口内的事件速率变化；
- 按栈计数（`@[kstack] = count()`）是"哪条路径触发热点"最低成本的答法。

## 陷阱

- ⚠️ `pid`/`tid` 反直觉映射（pid=tgid、tid=内核 pid），跨工具（BCC/proc 文件）比对时注意口径。
- ⚠️ 映射表首次赋值后类型固定——同一 `@` 名后续赋不同类型值会编译错。
- ⚠️ kstack 变量最大 127 帧、kstack() 函数最大 1024 帧，两个上限不同。

<details>
<summary>自测题</summary>

1. bpftrace 的 pid 和 tid 分别对应内核的什么？
   <details><summary>答案</summary>pid = tgid（进程）；tid = 内核 pid（线程）。</details>

2. 怎么知道 setuid 调用是否成功？
   <details><summary>答案</summary>跟踪出口 `t:syscalls:sys_exit_setuid`，看 args->ret（0 成功，-errno 失败）。</details>

3. 按调用路径统计块 I/O 的低成本写法？
   <details><summary>答案</summary>`t:block:block_rq_insert { @[kstack] = count(); }`（栈作键内核态聚合）。</details>
</details>
