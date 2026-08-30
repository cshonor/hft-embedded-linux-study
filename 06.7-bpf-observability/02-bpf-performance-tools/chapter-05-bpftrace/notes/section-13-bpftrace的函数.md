# 5.13 bpftrace 的函数

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.13 节（印刷 p170–177）

## 内容详解

### 表 5-6：重要内置函数

| 函数 | 描述 |
|------|------|
| `printf(char *fmt [, ...])` | 按格式打印 |
| `time(char *fmt)` | 格式化打印时间 |
| `join(char *arr[])` | 打印字符串数组（空格分隔） |
| `str(char *s [, int len])` | 指针→字符串 |
| `kstack(int limit)` / `ustack(int limit)` | 限深调用栈（还带 mode 参数） |
| `ksym(void *p)` / `usym(void *p)` | 地址→符号字符串 |
| `kaddr(char *name)` / `uaddr(char *name)` | 符号名→地址 |
| `reg(char *name)` | 读指定寄存器 |
| `ntop([int af,] int addr)` | IP 地址字符串 |
| `system(char *fmt [, ...])` | 执行 shell 命令 |
| `cat(char *filename)` | 打印文件内容 |
| `exit()` | 退出 bpftrace |

**同步/异步**：printf、time、cat、join、system 是**异步**（内核入队、用户态稍后处理）；kstack/ustack/ksym/usym **同步记录地址**、符号解析异步。

### 5.13.1 printf()

- 格式 `%[-]width type`；转义 `\n` `\"` `\\`；
- 类型占位符：`%u/%d`、`%lu/%ld`、`%llu/%lld`、`%hu/%hd`、`%x/%lx/%llx`、`%c`、`%s`；
- 例：`printf("%-16s%-6d\n", comm, pid)`——comm 左对齐 16 字符。

### 5.13.2 join()

- 空格连接字符串数组并打印；典型用法 execve 的 argv：

```bash
bpftrace -e 't:syscalls:sys_enter_execve { join(args->argv); }'
ls -l
df -h
```

- 注意打印的是**所有执行尝试**——成败要看 `sys_exit_execve` 的 args->ret；
- 限制：**16 个参数、每个 ≤1KB**；输出被截断即触上限（未来版本会改为返回字符串、不再异步）。

### 5.13.3 str()

- `str(char *s [, int length])`；例：打印全系统 bash 交互命令：

```bash
# bpftrace -e 'ur:/bin/bash:readline { printf("%s\n", str(retval)); }'
ls -lh
echo hello BPF
```

- 默认 64 字节上限（`BPFTRACE_STRLEN`）；当前不支持超过 **200 字节**（字符串存 BPF 栈、栈上限 512B；计划改用映射表存储以支持 MB 级）。
- 注：假定 readline 在 bash 可执行文件内；有些 bash 用 libreadline，需改探针路径。

### 5.13.4 kstack() 和 ustack()

- `kstack(limit)`、`kstack(mode, limit)`；最大深度 **1024**；
- mode：默认 `bpftrace`，或 `perf`（输出格式与 perf(1) 一致，含 DSO）：

```
7f220f1f2c60 nanosleep+64 (/lib/x86_64-linux-gnu/libpthread-2.27.so)
7f220f653fdd g_timeout_add_full+77 (/usr/lib/.../libglib-2.0.so.0.5600.3)
```

- 例：深度 3 的块 I/O 栈 + 进程名计数 `@[kstack(3), comm] = count()`。

### 5.13.5 ksym() 和 usym()

- 地址→符号。经典案例 hrtimer 回调频率：原始地址 → `ksym()` 转函数名：

```bash
@[args->function] = count()        # @[-1169374160]: 3 ...
@[ksym(args->function)] = count()  # @[tick_sched_timer]: 27092
```

usym() 依赖二进制符号表。

### 5.13.6 kaddr() 和 uaddr()

- 符号名→地址。例：读 bash 的 PS1 提示符内容：

```bash
# bpftrace -e 'uprobe:/bin/bash:readline { printf("PS1: %s\n", str(*uaddr("ps1_prompt"))); }'
PS1:\[\e[34;1m\]\u@\h:\w>\[\e[0m\]
```

### 5.13.7 system()

- 执行 shell 命令；**不安全函数，需 `--unsafe`**；例：在 nanosleep 时运行 ps(1)；
- ⚠️ 高频探针 + system() = 每次命中 fork 新进程，**大量 CPU 消耗**——只在必要时用。

### 5.13.8 exit()

- 结束程序；经典搭配——**固定时长统计**：

```bash
# bpftrace -e 't:syscalls:sys_enter_read { @reads = count(); } interval:s:5 { exit(); }'
@reads: 735        # 5 秒内 735 次 read()
```

退出时全部映射表自动打印。

## HFT 关联

- `interval:s:N { exit(); }` 是自动化取样的标配（限时长防失控）；
- `ntop()` + `system()` 组合可做"异常连接触发抓取"——生产慎用 system()（fork 开销 + --unsafe 风险）；
- str() 64B 截断对长路径（深目录、容器 overlay 路径）经常不够，调 BPFTRACE_STRLEN。

## 陷阱

- ⚠️ printf/join/system 异步——输出顺序可能与事件顺序不一致，别用它做严格时序判断。
- ⚠️ join 16 参数 ×1KB 上限；str 200B 硬上限（老版本）。
- ⚠️ system() 必须 `--unsafe` 且高频探针下是性能炸弹。

<details>
<summary>自测题</summary>

1. 哪些函数是异步的？
   <details><summary>答案</summary>printf、time、cat、join、system（kstack/ustack/ksym/usym 的地址记录同步、符号解析异步）。</details>

2. 固定 5 秒统计怎么做？
   <details><summary>答案</summary>计数探针 + `interval:s:5 { exit(); }`——退出时自动打印映射表。</details>

3. 打印全系统 bash 命令行的单行？
   <details><summary>答案</summary>`ur:/bin/bash:readline { printf("%s\n", str(retval)); }`。</details>
</details>
