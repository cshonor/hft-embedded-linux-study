# 3. BPF 工具：CPU 剖析（profile / threaded，13.2.3–13.2.4）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.3–13.2.4 节（印刷 p629–634）

## 13.2.3 profile

第 6 章的 BCC 工具，定时采样 **on-CPU 调用栈**，廉价但粗粒度地展示哪条代码路径在耗 CPU。剖析 MySQL：

```
# profile -d -p $(pgrep mysqld)
Sampling at 49 Hertz of PID 9908 by user + kernel stack... Hit Ctrl-C to end.
[...]
    my_hash_sort_simple
    hp_rec_hashnr
    hp_write_key
    heap_write
    ha_heap::write_row(unsigned char*)
    handler::ha_write_row(unsigned char*)
    end_write(JOIN*, QEP_TAB*, bool)
    evaluate_join_record(JOIN*, QEP_TAB*)
    sub_select(JOIN*, QEP_TAB*, bool)
    JOIN::exec()
    handle_query(THD*, LEX*, Query_result*, ...)
    execute_sqlcom_select(THD*, TABLE_LIST*)
    mysql_execute_command(THD*, bool)
    Prepared_statement::execute(...)
    mysqld_stmt_execute(...)
    dispatch_command(THD*, COM_DATA const*, enum server_command)
    do_command(THD*)
    handle_connection
    pfs_spawn_thread
    start_thread
    mysqld(9908)
    14
```

- 第一个栈：SQL 语句 → join 操作 → my_hash_sort_simple() 在 CPU 上跑（用户态哈希排序）
- 最后一个栈：在**内核**中发送套接字（vio_write → net_send_ok → sys_sendto）；**-d 选项用分隔符（";"）分割内核态与用户态栈**

### 火焰图

输出有数百个栈，用 **-f 折叠格式**喂给火焰图软件：

```
# profile -p $(pgrep mysqld) -f 30 > out.profile01.txt
# flamegraph.pl --width=800 --title "cpu Flame Graph" < out.profile01.txt > out.profile01.svg
```

图 13-2 的火焰图中：**dispatch_command() 占采样 69%，JOIN::exec() 占 19%**（悬停可见，点击放大）。

火焰图的第二用途：显示哪些函数正在执行 → **都可以直接用 uprobes 插桩**，研究它们的参数和延迟（do_command()、mysqld_stmt_execute()、JOIN::exec()、JOIN::optimize()……）。

> 有效的前提：剖析的是**编译了帧指针的 MySQL**，且 libc 和 libpthread 也有帧指针——否则 BPF 无法正确遍历栈（详见 13.2.9 / 本目录 section-6）。

## 13.2.4 threaded

对指定进程的 **on-CPU 线程**采样（99Hz），验证**多线程的有效性**（作者 2005-07-25 首版 threaded.d，用于性能课程演示线程池锁竞争）。

```
# threaded.bt $(pgrep mysqld)
Sampling PID 2274 threads at 99 Hertz. Ctrl-c to end.
23:47:13
@[mysqld,2317]: 1
@[mysqld,2319]: 2
@[mysqld,2318]: 3
@[mysqld,2316]: 4
@[mysqld,2534]: 55        ← 只有一个线程大量使用 CPU
```

评估多线程应用**线程间分工是否均衡**。Java freecol 案例（第 12 章）——应用会改线程名：

```
@[C2 CompilerThre,32617]: 44   ← JIT 编译线程吃掉大头
@[C2 CompilerThre,32616]: 44
@[C2 CompilerThre,32615]: 48
@[FreeColServer:A,975]: 26
```

源代码：

```bash
#!/usr/local/bin/bpftrace
BEGIN {
    if (!$1) { printf("USAGE: threaded.bt PID\n"); exit(); }
    printf("Sampling PID %d threads at 99 Hertz. Ctrl-c to end.\n", $1);
}
profile:hz:99 /pid == $1/
{ @[comm, tid] = count(); }
interval:s:1
{ time(); print(@); clear(@); }
```

- 需要位置参数 PID（默认 0 时直接退出）
- **局限**：定时采样会**漏掉两次采样间的短暂线程唤醒**
- 99Hz 低频采样对性能几乎无影响

## HFT 关联

- `-d` 分隔内核/用户栈：策略进程的延迟归因必须区分"策略计算耗 CPU"（用户栈）还是"陷在内核收发包"（内核栈）
- threaded 的"单线程占 55/62 采样"输出形态，直接对应交易系统里的**行情解析单线程瓶颈**——加线程池前先跑 threaded 验证瓶颈确实在单线程上
- 火焰图找热点函数→uprobes 测函数延迟，这条路径同样适用于策略回调函数

<details>
<summary>自测题</summary>

1. profile 的 -d 和 -f 选项分别做什么？
   <details><summary>答</summary>-d 用分隔符分割内核与用户调用栈；-f 输出折叠格式供火焰图软件使用。</details>

2. threaded 的采样盲区是什么？
   <details><summary>答</summary>99Hz 定时采样会漏掉两次采样之间短暂唤醒的线程（短于 ~10ms 的 CPU 使用可能不被计入）。</details>

3. 火焰图除了解释 CPU 消耗还能用来干什么？
   <details><summary>答</summary>显示哪些函数正在执行，作为 uprobes 插桩目标（研究参数与延迟）的起点。</details>
</details>
