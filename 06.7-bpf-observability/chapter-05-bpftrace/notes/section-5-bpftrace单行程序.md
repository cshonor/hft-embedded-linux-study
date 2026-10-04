# 5.5 bpftrace 单行程序

> 底本：《BPF之巅》第 5 章 bpftrace（印刷 p137–190），5.5 节（印刷 p145–146）

## 内容详解

单行程序既有用，又是展示 bpftrace 能力的最佳教材。注意：**许多单行在内核内存中统计数据，Ctrl-C 后才打印摘要**。

### 书中单行清单（`-e` 执行，12 条全）

```bash
# 1. 谁在执行什么命令（execve 入参）
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { printf("%s -> %s\n", comm, str(args->filename)); }'

# 2. 新进程创建及参数（execve 的 argv 逐个打印）
bpftrace -e 'tracepoint:syscalls:sys_enter_execve { join(args->argv); }'
# join() 按 8 字节步长解引用 argv 数组直到 NULL，空格拼接——argv 是 char**，
# str() 只能读一层，数组要靠 join()

# 3. openat() 打开的文件，按进程打印
bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'

# 4. 按程序统计系统调用次数
bpftrace -e 'tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }'

# 5. 按系统调用探针名计数（一个通配符看全部 syscall 的分布）
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[probe] = count(); }'

# 6. 按进程统计系统调用数量
bpftrace -e 'tracepoint:syscalls:sys_enter_* { @[comm, pid] = count(); }'

# 7. 按进程统计 read 总字节数
bpftrace -e 'tracepoint:syscalls:sys_exit_read /args->ret > 0/ { @[comm] = sum(args->ret); }'

# 8. 按进程展示 read 返回值分布
bpftrace -e 'tracepoint:syscalls:sys_exit_read { @[comm] = hist(args->ret); }'

# 9. 进程的磁盘 I/O 尺寸
bpftrace -e 'tracepoint:block:block_rq_issue { printf("%d %s %d\n", pid, comm, args->bytes); }'

# 10. 按进程统计页换入（major faults）
bpftrace -e 'software:major-faults:1 { @[comm] = count(); }'

# 11. 按进程统计缺页中断
bpftrace -e 'software:faults:1 { @[comm] = count(); }'

# 12. 对 PID 189 以 49Hz 抓取用户态调用栈
bpftrace -e 'profile:hz:49 /pid == 189/ { @[ustack] = count(); }'
```

### 每条单行在"看什么"（读法）

| # | 单行 | 回答的问题 | 输出形态 |
|---|------|-----------|---------|
| 1/2/3 | execve/openat 逐事件打印 | **谁**在**碰什么**（审计向） | 每事件一行 |
| 4/5/6 | 按 comm/pid/probe 计数 | **负载画像**：哪个进程/哪类调用最热 | Ctrl-C 摘要（计数表） |
| 7/8 | read 的 sum 与 hist | **吞吐 vs 分布**：总量大的未必单次大 | Ctrl-C 摘要 |
| 9 | block_rq_issue 的 bytes | I/O 尺寸画像（大 I/O 换吞吐、小 I/O 换延迟） | 每事件一行 |
| 10/11 | 软件事件计数 | **内存行为异常**：缺页/换页集中到谁 | Ctrl-C 摘要 |
| 12 | profile 采样 ustack | 该进程的 CPU 时间去哪了（火焰图原料） | Ctrl-C 摘要（栈计数） |

### 共性套路

| 套路 | 模式 | 开销档位 |
|------|------|---------|
| 看细节 | `printf("%s %s\n", comm, str(args->xxx))` | 高（逐事件出内核） |
| 计数 | `@[comm] = count()`（键可为 comm/pid/probe） | 低（内核态聚合） |
| 求和 | `@[comm] = sum(args->ret)`（配过滤滤掉负值错误码） | 低 |
| 分布 | `@[comm] = hist(...)` | 低 |
| 软件事件采样 | `software:faults:1`（每 1 次触发） | 取决于事件频率 |
| 采样归因 | `profile:hz:N /pid==X/ { @[ustack] = count(); }` | 固定税（N Hz × 栈抓取） |

三条隐藏纪律，比套路本身更重要：

1. **负值过滤纪律**：所有 `sum()/avg()` 前先问"返回值可能是 -errno 吗？"——read/write 族必须 `/args->ret > 0/`；
2. **采样频率选素数**：49Hz 而非 50Hz——避免与系统中 10/50Hz 周期活动锁相（永远采到同一相位），这是 perf 99Hz、bpftrace 示例 49Hz 的共同原因；
3. **双探针计时纪律**（书中 5.17 的经典 bug 泛化）：凡 kprobe 存时间戳 → kretprobe 求差的模板，**kretprobe 一律加 `/@start[tid]/`**：

```bash
# 通用双探针计时模板（第 6-10 章 runqlat/biolatency/tcplatency 全族的骨架）
bpftrace -e '
kprobe:某函数   { @start[tid] = nsecs; }
kretprobe:某函数 /@start[tid]/ {
    @ns = hist(nsecs - @start[tid]);   # 有 /过滤/ 才不会把"入口未记录"的
    delete(@start[tid]);               # 半截调用算成天文数字
}'
```

更多单行见**附录 A**（完整单行宝典）与附录 B 备忘单。

## HFT 关联

- 这 12 条就是**分钟级健康检查**的雏形：read 分布看 I/O 尺寸、faults 看内存行为、按 comm 计数看进程活跃度——不用装任何监控 Agent；
- 把单行固化到 runbook 时，务必加**运行时长控制**（`interval:s:N { exit(); }`），避免无限跑：

```bash
# runbook 固化形态：限时 60s 自动退出 + exit() 触发摘要打印
sudo timeout 70 bpftrace -e '
tracepoint:syscalls:sys_exit_read /args->ret > 0/ { @[comm] = hist(args->ret); }
interval:s:60 { exit(); }'
```

- 单行 → 工具的升级路径：反复用的单行存成 `.bt` 文件（加 shebang + 注释 + man 式头），就完成了从"即兴"到"货架"的转化——这正是 tools/ 目录里每个工具的由来。

## 陷阱

- ⚠️ `sum(args->ret)` 必须加 `/args->ret > 0/` 过滤——read 的负返回值是错误码，混入求和结果完全失真；
- ⚠️ 单行默认**不带时长限制**，Ctrl-C 才出摘要——自动化脚本里要配 interval+exit。
- ⚠️ `join(args->argv)` 读的是**用户态指针数组**，进程执行完 execve 后 argv 内存可能已变——看到个别行乱码属正常，统计意义不受影响。
- ⚠️ 12 号的 `profile:hz:49` 是**全 CPU 每 CPU 49Hz**，不是全局 49Hz——多核机上总采样率 = 49 × 核数，读栈计数时注意口径。

<details>
<summary>自测题</summary>

1. 统计 read 总字节数时为什么要过滤 `args->ret > 0`？
   <details><summary>答案</summary>read(2) 负返回值是 -errno 错误码，不是字节数。</details>

2. 单行程序的结果什么时候打印？
   <details><summary>答案</summary>多数在 Ctrl-C 退出时统一打印摘要（内核映射表统计）。</details>

3. 书里采样用 49Hz 而不是 50Hz，为什么？
   <details><summary>答案</summary>选素数频率避免与系统周期性活动（10/50Hz）锁相——整数倍频率会永远采到同一相位，看不到完整周期面貌。</details>

4. `str(args->filename)` 能读 argv 吗？该用什么？
   <details><summary>答案</summary>不能——argv 是 char**（指针数组），str() 只做单层字符串解引用；读数组要用 join()。</details>

5. 双探针计时模板里 kretprobe 侧的 `/@start[tid]/` 过滤器防的是什么 bug？
   <details><summary>答案</summary>工具启动前已在执行中的函数（或中途进来的线程）在 kretprobe 触发时 @start[tid] 不存在（按 0 处理），nsecs-0 产生巨大假值直方图离群点（5.17 的 vfs_read 经典案例）。</details>
</details>
