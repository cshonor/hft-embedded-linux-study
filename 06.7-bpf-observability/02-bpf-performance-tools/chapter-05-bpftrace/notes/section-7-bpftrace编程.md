# 5.7 bpftrace 编程

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.7 节（印刷 p146–155）

## 内容详解

本节格式仿照最初的 awk 文章（6 页讲完 awk）；语言设计灵感来自 **awk 和 C**，兼收 DTrace、SystemTap 特色。

### 核心示例：对 vfs_read() 计时

```awk
#!/usr/local/bin/bpftrace
// this program times vfs_read()
kprobe:vfs_read
{
    @start[tid] = nsecs;
}
kretprobe:vfs_read
/@start[tid]/
{
    $duration_us = (nsecs - @start[tid]) / 1000;
    @us = hist($duration_us);
    delete(@start[tid]);
}
```

这是 bpftrace 最经典的**双探针计时模式**（kprobe 存时间戳 → kretprobe 求差 → 直方图）。

### 5.7.1 用法

- `bpftrace -e 'program'`：单行程序，Ctrl-C 或 `exit()` 结束；
- `bpftrace file.bt`：脚本文件（`.bt` 后缀非必需）；
- 文件首行 shebang `#!/usr/local/bin/bpftrace` + `chmod a+x` 后可直接 `./file.bt`；
- `#!/usr/bin/env bpftrace` 也可，但 env(1) 自身有问题，BCC 仓库已撤销该写法；
- **必须 root 运行**（bpftrace 检查 UID==0；未来可能细化为特定权限）。

### 5.7.2 程序结构

```
probes /filter/ { actions }
```

- 探针激活→执行动作；过滤表达式为真才执行（类 awk 的 `/pattern/ { action }`）；
- 多个动作块按任意顺序书写，谁匹配谁触发。

### 5.7.3 注释

- 单行：`// comment`；多行与行内：`/* ... */`（同 C）。

### 5.7.4 探针格式

```
type:identifier1[:identifier2[...]]
```

如 `kprobe:vfs_read`（一个标识符）、`uprobe:/bin/bash:readline`（路径+函数）。逗号并列多探针共享动作：`probe1,probe2 { ... }`。特殊探针 **BEGIN / END** 无需标识符（同 awk）。

### 5.7.5 探针通配符

- `kprobe:vfs*` 插桩所有 vfs 开头内核函数；
- **防失控**：环境变量 `BPFTRACE_MAXPROBES`（默认 **512**）限制同时开启探针数；
- **先 `-l` 预览再上**：

```bash
# bpftrace -l 'kprobe:vfs*'    # 列出匹配
# bpftrace -l 'kprobe:vfs*' | wc -l   # 56 个
```

探针名放单引号中防 shell 展开。（512 上限同时会让启停变慢——插桩逐个进行，未来内核批处理插桩后可上调。）

### 5.7.6 过滤器

- `/pid == 123/`：布尔表达式决定动作是否执行；
- `/pid/` 等价 `/pid != 0/`（非零即真）；
- 可组合：`/pid > 100 && pid < 1000/`。

### 5.7.7 动作

- 单语句或多语句（分号分隔）：`{ action_one; action_two; }`；
- 语句类 C，可操作变量与调用函数：`{ $x = 42; printf("$x is %d", $x); }`。

### 5.7.8 Hello, World!

```bash
# bpftrace -e 'BEGIN { printf("Hello, World!\n"); }'
```

### 5.7.9 函数

- `exit()`：退出；`str(char*)`：指针→字符串；`system(fmt, args)`：执行 shell 命令；
- 例：`{ printf("got: %llx %s\n", $x, str($x)); exit(); }`——十六进制打印变量并按字符串打印。

### 5.7.10 变量（三类，详见 5.12）

- **内置变量**：预定义只读（pid、comm、nsecs、curtask…）；
- **临时变量** `$x`：类型首次赋值确定，仅在本动作块内有效；引用未声明变量会**报错**（防拼写错误）；
- **映射表变量** `@a`：BPF 映射存储，可跨动作块/探针传数据；支持单键与复合键：`@path[pid, fd] = str(arg0);`。

### 5.7.11 映射表函数（详见 5.14）

- `@x = count()`（每 CPU 独立、特殊 count 类型）vs `@x++`（全局整数映射表，并发更新可能有微小误差）；
- `@y = sum($x)`、`@z = hist($x)`；
- `print(@x)` 不常用——**所有映射表在程序退出时自动打印**；
- `delete(@start[tid])` 删除键值对。

### 5.7.12 对 vfs_read() 计时（完整案例）

核心示例（上文）逐行解释：

1. `kprobe:vfs_read` 入口：`@start[tid] = nsecs` 以线程 ID 为键存时间戳（**多线程不互相覆盖**）；
2. `kretprobe:vfs_read` 返回：过滤器 `/@start[tid]/` **确保只统计记录过开始时间的调用**——否则 `now - 0` 产生离群假值（插桩前已在执行的调用、丢失入口的调用）；
3. 直方图命名 `@us` 表明单位（微秒）——**用有含义的名字**（bytes、latency_ns）让输出可读。

定制：改 `@us[pid, comm] = hist($duration_us);` → 每进程分别出直方图。**对比传统工具（iostat/vmstat 固定格式），bpftrace 可以把指标随意打散组合直到解决问题**——这是它最有用的能力。

## HFT 关联

- 双探针计时模式是测量**任意内核函数延迟**的通用模板：把 `vfs_read` 换成 `tcp_sendmsg`/`mutex_lock` 即可；
- `/@start[tid]/` 过滤缺失是新人最常犯的错——超长离群点几乎都来自"入口未记录"。

## 陷阱

- ⚠️ 通配符先 `-l` 列表再执行；超 512 探针会被拒（BPFTRACE_MAXPROBES）。
- ⚠️ `@x++` 与 `count()` 并非完全等价：前者全局表有并发更新误差，需要精确计数用 `count()`。
- ⚠️ 临时变量引用前必须已赋值（跨块引用未声明变量报错）。

<details>
<summary>自测题</summary>

1. 双探针计时中 `/@start[tid]/` 过滤器防的是什么？
   <details><summary>答案</summary>防止只命中返回探针（入口未记录）的调用算出 `nsecs-0` 的假离群值。</details>

2. `@x++` 与 `@x = count()` 的区别？
   <details><summary>答案</summary>`count()` 是每 CPU 独立映射表的 count 类型（精确）；`@x++` 是全局整数映射表（并发更新可能有误差）。</details>

3. 默认最多能同时挂多少个探针？哪个环境变量控制？
   <details><summary>答案</summary>512；`BPFTRACE_MAXPROBES`。</details>
</details>
