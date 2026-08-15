# 8. BPF 工具：锁分析（pmlock / pmheld，13.2.14）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.14 节（印刷 p655–660）

## 两个互补的锁工具（作者 2019-02-17 开发，灵感来自 Solaris lockstat(1M)）

| 工具 | 测什么 | 回答什么问题 |
|------|--------|--------------|
| **pmlock(8)** | pthread 互斥锁**获取延迟**（等待锁的时间） | **哪里有锁争用** |
| **pmheld(8)** | 互斥锁**被持有**的时间 | **原因：哪条代码路径持有锁太久** |

均以直方图 + 用户态栈记录，插桩 libpthread 的 `pthread_mutex_lock()`/`unlock()`（uprobes + uretprobes）。

## pmlock 输出示例（MySQL）

```
# pmlock.bt $(pgrep mysqld)
@lock_latency_ns[0x7f3728001a50,
    pthread_mutex_lock+36
    THD::Query_plan::set_query_plan(enum sql_command, LEX*, bool)+121
    mysql_execute_command(THD*, bool)+15991
    Prepared_statement::execute(String*, bool)+1410
    ...
    mysqld]:
[2K, 4K)    1203
[4K, 8K)    6576
[8K, 16K)   2077            ← 8~16us 档为主

@lock_latency_ns[0x7f37280019f0,       ← 同一把锁的另一条争用路径
    pthread_mutex_lock+36
    THD::set_query(st_mysql const*, lex_string const&)+94
    dispatch_command(THD*, COM_DATA const*, enum server_command)+1045
    do_command(THD*)+544
    handle_connection+680
    mysqld]:
[2K, 4K)    1198
[4K, 8K)    5283
```

锁地址（第一键，usym(arg0)）+ 争用栈：同一锁 0x...19f0 出现在多条路径上，延迟 4~16us。

## pmheld 输出示例（找原因）

```
# pmheld.bt $(pgrep mysqld)
@held_time_ns[0x7f37280019c0,
    pthread_mutex_unlock+0
    close_thread_table(THD*, TABLE**)+169
    ...
    mysqld]:
[2K, 4K)    3311
[4K, 8K)    4523            ← 这条路径持有 4~8us

@held_time_ns[0x7f37280019f0,
    pthread_mutex_unlock+0
    THD::set_query(...)+147
    dispatch_command(...)+1045
    mysqld]:
[2K, 4K)    3848
[4K, 8K)    5038
```

显示**哪些路径持有了同一把锁、持有多久**。据此可采取措施：**调整线程池大小**减少争用；开发人员**优化持锁代码路径**降低持锁时间。

建议输出重定向到文件备查：

```
# pmlock.bt PID > out.pmlock01.txt
# pmheld.bt PID > out.pmheld01.txt
```

## 源代码

### pmlock

```bash
uprobe:.../libpthread.so.0:pthread_mutex_lock
/($1 == 0 || pid == $1)/
{
    @lock_start[tid] = nsecs;
    @lock_addr[tid] = arg0;
}

uretprobe:.../libpthread.so.0:pthread_mutex_lock
/($1 == 0 || pid == $1) && @lock_start[tid]/
{
    @lock_latency_ns[usym(@lock_addr[tid]), ustack(5), comm]
        = hist(nsecs - @lock_start[tid]);
    delete(@lock_start[tid]); delete(@lock_addr[tid]);
}
```

- 入口记时间戳 + 锁地址（arg0），返回时算**获取延迟**
- ustack(5) 可调大；栈**需要帧指针**才能工作（libpthread 没帧指针也许可行——跟踪的是库入口，寄存器可能尚未被重用）
- 不跟踪 trylock（本来就快，可用 funclatency(8) 验证）

### pmheld

```bash
uprobe:...:pthread_mutex_lock,
uprobe:...:pthread_mutex_trylock
{ @lock_addr[tid] = arg0; }

uretprobe:...:pthread_mutex_lock
/... && @lock_addr[tid]/
{
    @held_start[pid, @lock_addr[tid]] = nsecs;   // 拿到锁时开始计时
    delete(@lock_addr[tid]);
}

uretprobe:...:pthread_mutex_trylock
/retval == 0 && ... && @lock_addr[tid]/         // trylock 成功才算持有
{ @held_start[pid, @lock_addr[tid]] = nsecs; ... }

uprobe:...:pthread_mutex_unlock
/... && @held_start[pid, arg0]/
{
    @held_time_ns[usym(arg0), ustack(5), comm]
        = hist(nsecs - @held_start[pid, arg0]);
    delete(@held_start[pid, arg0]);
}
```

- 计时从 lock/trylock **返回**（调用者已持锁）开始，到 unlock 结束
- **@held_start 以 [pid, 锁地址] 为键**——因为不同线程可同时持有不同锁
- libpthread 也有 USDT 探针，工具可改写成 USDT 版

## 开销警告

锁事件频率极高。funccount 实测 1 秒：

```
# funccount -d 1 /lib/x86_64-linux-gnu/libpthread.so.0:pthread_mutex*lock*
pthread_mutex_trylock   4525
pthread_mutex_lock     44726
pthread_mutex_unlock   49132
```

每秒近 10 万次锁调用，每次哪怕多一点开销，加起来也很大——**pmlock/pmheld 是高开销工具，短时间运行并指定 PID**（不提供 PID 则记录全系统所有 pthread 锁事件）。

## HFT 关联

- 订单簿/持仓表的自旋锁或互斥锁是策略延迟常见的隐形来源：pmlock 的延迟直方图直接回答"等锁等了多久"，pmheld 回答"谁拿着不放"
- 每秒 10 万次锁调用 × uprobe 开销 ≈ 百毫秒级/秒的额外消耗——**绝不能在交易时段全量跑**；先 funccount 估频率，再决定采样窗口
- @held_start 的 [pid, 锁地址] 复合键写法是多线程同键冲突的标准解法，写自己的锁探针时照抄

<details>
<summary>自测题</summary>

1. pmlock 和 pmheld 分别回答什么问题？
   <details><summary>答</summary>pmlock 测锁获取（等待）延迟——定位争用在哪；pmheld 测持有时间——定位哪条路径持有太久导致争用。</details>

2. pmheld 为什么以 [pid, 锁地址] 作为 held_start 的键？
   <details><summary>答</summary>不同线程可同时各自持有不同的锁，复合键避免多线程互相覆盖时间戳。</details>

3. 为什么 pmheld 的 trylock 探针要加 retval == 0 过滤？
   <details><summary>答</summary>trylock 失败（非 0）时调用者没有获得锁，不能开始计持有时间。</details>

4. 如何事先评估这两个工具的开销？
   <details><summary>答</summary>funccount -d 1 统计 pthread_mutex_* 每秒调用次数（示例近 10 万/秒），频率越高开销越大。</details>
</details>
