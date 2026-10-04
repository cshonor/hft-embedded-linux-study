# 7. BPF 工具：MySQL 专用（mysqld_qslower / mysqld_clat，13.2.10–13.2.11）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.10–13.2.11 节（印刷 p645–652）

## 13.2.10 mysqld_qslower

跟踪服务器端**慢于阈值的 MySQL 查询**，显示**应用程序上下文**（查询字符串）。BCC 版输出：

```
# mysqld_qslower $(pgrep mysqld)
Tracing MySQL server queries for PID 9908. slower than 1 ms.
TIME(s)     PID    MS       QUERY
1.962227    9962   205.787  SELECT * FROM words WHERE word REGEXP 'bpf.tools*'
9.043242    9962   95.276   SELECT COUNT(*) FROM words
30.343233   9962   181.494  SELECT * FROM words WHERE word REGEXP ... ORDER BY word
```

- 用法：`mysqld_qslower PID [min_ms]`，默认阈值 1ms；**0 = 打印所有查询**
- 相比 MySQL 慢查询日志：BPF 版可**定制加入日志中没有的细节**，如磁盘 I/O 和该查询的其他资源使用
- 实现：USDT 探针 **mysql:query__start / mysql:query__done**；查询率低 → 开销可忽略

### bpftrace 版

```bash
#!/usr/local/bin/bpftrace
BEGIN { ...; printf("%-10s %-6s %6s %s\n", "TIME(ms)", "PID", "MS", "QUERY"); }

usdt:/usr/sbin/mysqld:mysql:query__start
{
    @query[tid] = str(arg0);
    @start[tid] = nsecs;
}

usdt:/usr/sbin/mysqld:mysql:query__done
/@start[tid]/
{
    $dur = (nsecs - @start[tid]) / 1000000;
    if ($dur > $1) {
        printf("%-10u %-6d %6d %s\n", elapsed/1000000, pid, $dur, @query[tid]);
    }
    delete(@query[tid]);
    delete(@start[tid]);
}
```

- `$1` 为阈值（毫秒），缺省 0 打印全部
- bpftrace 需 **-p PID** 启用 USDT 探针：`mysqld_qslower.bt -p PID [min_ms]`
- **tid 关联法**（13.1.4）：服务线程池同一线程处理整个请求 → tid 做唯一键，@query/@start 存查询串和开始时间戳

### uprobes 备胎版（mysqld_qslower-uprobes.bt）

mysqld 没编译 USDT 时，用 profile 找到的内部方法 `dispatch_command()`：

```bash
uprobe:/usr/sbin/mysqld:*dispatch_command*
{
    $COM_QUERY = 3;              // 见 include/my_command.h
    if (arg2 == $COM_QUERY) {
        @query[tid] = str(*arg1);   // COM_DATA 首成员是查询字符串
        @start[tid] = nsecs;
    }
}

uretprobe:/usr/sbin/mysqld:*dispatch_command*
/@start[tid]/
{ ...同上打印与清理... }
```

- dispatch_command() 还处理其他命令，须判断 **arg2 == COM_QUERY(3)**
- 查询串从 **COM_DATA** 参数取：`*arg1` 解引用——字符串是结构体第一个成员
- **警告**：函数名、参数、逻辑都依赖 MySQL 版本（本书跟踪的是 5.7），换版本可能失效——**这就是为什么 USDT 是更好的选择**

## 13.2.11 mysqld_clat

作者 2019-02-15 开发（改进自 2013 年 mysqld_command.d）。跟踪 **MySQL 命令延迟**，按**每种命令类型**显示直方图：

```
# mysqld_clat.bt
@us[COM_QUERY]:
[8, 16)       33
[16, 32)      185
[32, 64)      1128
[64, 128)     300

@us[COM_STMT_EXECUTE]:
[16, 32)      1410
[32, 64)      1654
[64, 128)     11212
[128, 256)    8899
[256, 512)    5000
[512, 1K)     1478
[1K, 2K)      ...
[4K, 8K)      141
```

查询延迟 8~256us；语句执行呈**双峰模式**。命令执行频率通常 <1000/s → 开销可忽略。

源代码要点（USDT 版）：

```bash
BEGIN {
    // 查询表：include/my_command.h 的命令 ID → 名称
    @com[0]="COM_SLEEP"; @com[1]="COM_QUIT"; @com[2]="COM_INIT_DB";
    @com[3]="COM_QUERY"; ... @com[22]="COM_STMT_PREPARE";
    @com[23]="COM_STMT_EXECUTE"; ... @com[30]="COM_BINLOG_DUMP_GTID";
}

usdt:/usr/sbin/mysqld:mysql:command__start
{
    @command[tid] = arg1;          // 命令类型
    @start[tid] = nsecs;
}

usdt:/usr/sbin/mysqld:mysql:command__done
/@start[tid]/
{
    @us[@com[@command[tid]]] = hist(nsecs - @start[tid]);
    delete(@command[tid]); delete(@start[tid]);
}
```

**uprobes 改写只需 3 行 diff**：

```diff
< usdt:/usr/sbin/mysqld:mysql:command__start
> uprobe:/usr/sbin/mysqld:*dispatch_command*
<     @command[tid] = arg1;
>     @command[tid] = arg2;
< usdt:/usr/sbin/mysqld:mysql:command__done
> uretprobe:/usr/sbin/mysqld:*dispatch_command*
```

其余代码不变。

## HFT 关联

- 这一对工具是**"给资源指标加上业务上下文"的模板**：qslower 展示"哪条 SQL 慢"，clat 展示"命令延迟分布"——同样模式可套到交易系统：以 tid 为键存**订单 ID/策略名**，任何后续 I/O、锁、调度事件都能关联回业务请求
- 双峰直方图读法：clat 中 COM_STMT_EXECUTE 的双峰往往对应缓存命中 vs 磁盘回表，交易系统里对应"内存行情 vs 触发风控查询"两类路径

<details>
<summary>自测题</summary>

1. mysqld_qslower 的 uprobes 版如何判断进入的是查询命令？查询串从哪取？
   <details><summary>答</summary>dispatch_command 的 arg2 == COM_QUERY(3)；查询串是 COM_DATA（arg1）的第一个结构体成员，str(*arg1) 取。</details>

2. 为什么作者说 USDT 优于 uprobes？
   <details><summary>答</summary>uprobes 依赖具体版本的函数名、参数布局与内部逻辑（本书针对 5.7），版本一变就可能失效；USDT 是稳定接口。</details>

3. mysqld_clat 用哪两个 USDT 探针？命令名从哪来？
   <details><summary>答</summary>mysql:command__start / command__done；源码里手写 @com[0..30] 查询表，来自 include/my_command.h。</details>
</details>
