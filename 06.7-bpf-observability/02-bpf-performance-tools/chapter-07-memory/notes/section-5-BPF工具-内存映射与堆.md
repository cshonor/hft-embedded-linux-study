# 7.3 BPF 工具（三）：内存映射与堆 — mmapsnoop / brkstack / shmsnoop

> 底本：《BPF之巅》第 7 章 内存，7.3.3–7.3.5 节（印刷 p274–277）。三个低频系统调用跟踪工具，开销全部可忽略。

## 7.3.3 mmapsnoop — 全系统 mmap(2) 跟踪

打印每次 mmap 请求：PID/COMM/保护标识/映射标识/OFFSET/SIZE/FILE。

```
PID    COMM      PROT  MAP   OFFS(KB) SIZE(KB) FILE
6315   java      R-E-  -P--  2222     2222     libjava.so
6315   java      RW--  -PF-  168      168      libjava.so
6315   java      R-E-  -P--  2081     2081     libnss_compat-2.23.so
6015   mmapsnoop RW-   S--  260      260      [perf event]
```

- 第一行常见输出是 BCC 自己的 perf event 环形缓冲区映射
- **PROT**：R=READ W=WRITE E=EXEC；**MAP**：S=SHARED P=PRIVATE F=FIXED A=ANON
- 用途：审计"谁在映射什么文件/多大" — java 启动时的库加载序列一目了然
- 实现：`syscall:sys_enter_mmap` 跟踪点；新映射低频 → 开销≈0；`-T` 加时间戳
- 文件 mmap 的深入分析（mmapfiles/fimapfaults）在第 8 章

## 7.3.4 brkstack — 堆扩展调用栈

brk(2) 扩堆是低频事件，但每次都意味着**堆增长** — 跟踪它的**用户态调用栈**是开销极低的内存增长分析方法（对比：直接跟踪 malloc 开销大）。

可用多种工具实现，最直接的是 bpftrace 版 brkstack：

```bash
#!/usr/local/bin/bpftrace
tracepoint:syscalls:sys_enter_brk
{
    @[ustack, comm] = count();
}
```

BCC 等价（trace/stackcount）：

```bash
trace -U t:syscalls:sys_enter_brk          # 逐事件
stackcount -P -u t:syscalls:sys_enter_brk  # 按进程聚合栈
```

书例输出（java 进程）：`JLI_List_new → JLI_MemAlloc → libc_malloc → sysmalloc → default_morecore → sbrk → brk` — 某个 List 对象扩展触发了堆扩展。栈完整需要启用帧指针的 libc（13.2.9）。

注意：brk 栈只说明"谁触发了这次扩展"，可能是一次超大分配，也可能是 1 字节小分配恰好越界 — 需结合代码分析。

## 7.3.5 shmsnoop — SysV 共享内存跟踪

BCC 工具，跟踪 `shmget(2)` / `shmat(2)` / `shmdt(2)` / `shmctl(2)` 及参数：

```
PID     COMM   SYS     RET        ARGS
12520   java   SHMGET  58c000a    key:0x0,size:65536,shmflg:0x380(IPC_CREAT|0600)
12520   java   SHMAT   7fde9c...  shmid:0x58c000a,shmaddr:0x0,shmflg:0x0
1863    Xorg   SHMAT   7f98cd...  shmid:0x58c000a,shmflg:0x1000(SHM_RDONLY)
1863    Xorg   SHMDT   0          shmaddr:0x7f98cd3b9000
```

解读：java shmget 创建 64KB 段（shmid 0x58c000a），java 与 Xorg 先后 shmat 同一 shmid → **两个进程在共享内存**。Xorg 以只读挂载（XSHM 图像传输的典型模式）。

- 选项：`-T` 时间戳、`-p PID`
- 调用低频 → 开销≈0

## 三工具对比

| | mmapsnoop | brkstack | shmsnoop |
|---|-----------|----------|----------|
| 跟踪 | mmap(2) | brk(2) | shm*(2) |
| 输出 | 映射参数+文件 | 用户态调用栈 | 调用+参数 |
| 回答 | 映射了什么 | 堆因谁而涨 | 谁在和谁共享 |

## HFT 关联

- 行情/订单共享内存（SysV shm 或 memfd）部署验证：shmsnoop 确认读写双方挂载同一 shmid、权限标志正确（读端 SHM_RDONLY）
- 策略进程内存审计：mmapsnoop 记录启动时全部文件映射（库/数据文件），运行期新增 mmap 都该有解释
- brkstack 定位"堆缓慢增长"：比 memleak 便宜得多，先跑 brkstack 看扩展栈再决定是否深入

## 常见陷阱

1. **brk 栈≠分配栈** — brk 只在堆顶部扩不动时发生，它显示的是"压垮骆驼的最后一根稻草"的路径，不是高频分配路径本身
2. **sbrk 不是系统调用** — Linux 中 sbrk(3) 是 libc 库函数，内部走 brk(2)；跟踪 brk 跟踪点即可覆盖
3. **mmapsnoop 输出的 [perf event] 误当应用行为** — 那是 BCC 自己的缓冲区映射；同理各种工具自身也有 mmap，注意排除

<details>
<summary>📝 自测题（点击展开）</summary>

1. **为什么说 brkstack 是"开销极低的内存增长分析"？对比跟踪 malloc 有何优劣？**

   <details>
   <summary>参考答案</summary>

   brk(2) 只在堆需要扩展时调用（低频），调用栈开销可忽略；malloc/free 每秒百万次，跟踪可致 1/10 减速。brkstack 劣势：只看到触发堆扩展的那一次分配路径，看不到常规分配（它们从空闲列表满足，不触发系统调用），且无法看到分配大小分布。
   </details>

2. **shmsnoop 输出里如何判断两个进程在共享内存？**

   <details>
   <summary>参考答案</summary>

   看同一 shmid 被 shmid 不同的 PID 先后 shmat — 书例 java shmget 得到 0x58c000a，java 和 Xorg 都 shmat 这个 id，即共享同一物理段。再对比 shmflg 可见权限差异（Xorg 只读）。
   </details>

</details>
