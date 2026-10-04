# 2. BPF 工具：进程与线程（execsnoop / threadsnoop，13.2.1–13.2.2）

> 底本：《BPF之巅》第 13 章 应用程序，13.2.1–13.2.2 节（印刷 p625–629）

## 13.2.1 execsnoop

第 6 章介绍过的 BCC/bpftrace 工具，跟踪**新进程**，识别应用程序是否使用**短命进程**。空闲服务器示例：

```
# execsnoop
PID    PCOMM    RET  ARGS
17788  sh       0    /bin/sh -a /usr/lib/sysstat/sa11 -sALL
17789  sa1      0    /usr/lib/sysstat/sa11 -sALL
17789  sadc     0    /usr/lib/sysstat/sadc -F -L -S DISK1 -S ALL /var/log/sysstat
```

- 服务器并不真"空闲"——execsnoop 抓到了系统活动记录进程
- 典型场景：应用程序调 **shell 脚本**执行功能（正常编码前的临时方案），导致进程低效使用

## 13.2.2 threadsnoop

通过 **pthread_create()** 库调用跟踪**线程创建**（作者 2019-02-15 为本书开发，灵感来自 execsnoop）。MySQL 启动时：

```
# threadsnoop.bt
TIME(ms)  PID    COMM    FUNC
2049      14456  mysqld  timer_notify_thread_func
2234      14460  mysqld  pfs_spawn_thread
2243      14460  mysqld  io_handler_thread
2243      14460  mysqld  io_handler_thread
...（共 10 个 io_handler_thread）
2274      14460  mysqld  buf_flush_page_cleaner_coordinator
2296      14460  mysqld  lock_wait_timeout_thread
2296      14460  mysqld  srv_error_monitor_thread
2296      14460  mysqld  srv_monitor_thread
2296      14460  mysqld  srv_master_thread
2297      14460  mysqld  srv_purge_coordinator_thread
2297      14460  mysqld  srv_worker_thread（×3）
...
```

三列信息：创建速度（TIME(ms)）、谁创建的（PID/COMM）、**新线程入口函数（FUNC）**——MySQL 的线程角色从入口函数名一目了然。

源代码（核心就一行 uprobe）：

```bash
#!/usr/local/bin/bpftrace
BEGIN { printf("%-10s %-6s %-16s %s\n", "TIME(ms)", "PID", "COMM", "FUNC"); }

uprobe:/lib/x86_64-linux-gnu/libpthread.so.0:pthread_create
{ printf("%-10u %-6d %-16s %s\n", elapsed/1000000, pid, comm, usym(arg2)); }
```

- **arg2 = 线程入口函数指针**，usym() 翻译成符号
- 可能需调整 libpthread 路径
- 增强：加 `ustack` 输出**导致线程创建的调用栈**——不是所有应用的入口函数名都像 MySQL 那样自解释：

```
1739  14981 mysqld  io_handler_thread
    pthread_create+21
    innobase_start_or_create_for_mysql()+6648
    innobase_init(void*)+3044
    ha_initialize_handlerton(st_plugin_int*)+79
    plugin_register_builtin_and_init_core_se(int*, char**)+485
    init_server_components()+960
    mysqld_main(int, char**)+1941
    libc_start_main+231
```

**开销**：线程创建是相对少见的事件，可忽略。

## HFT 关联

- 策略进程里 `std::thread`/线程池初始化是否在交易时段内发生？threadsnoop 的 TIME(ms) 列可以直接看出**运行中还在建线程**（如动态扩池）导致的抖动
- execsnoop 抓 shell 脚本临时方案——交易系统里常见的 `popen()` 调外部命令（取行情、写日志）就是同类反模式，每次 fork+exec 的代价在微秒级链路里不可接受

<details>
<summary>自测题</summary>

1. threadsnoop 从哪个参数取线程入口函数？
   <details><summary>答</summary>pthread_create 的第三个参数 arg2（start_routine），用 usym(arg2) 翻译为符号。</details>

2. 如何用 threadsnoop 定位"新线程是干什么的"但入口函数名不自解释的情况？
   <details><summary>答</summary>printf 中追加 ustack，输出创建线程的用户态调用栈。</details>

3. 这两个工具的开销如何？
   <details><summary>答</summary>都可忽略——进程/线程创建事件频率低。</details>
</details>
