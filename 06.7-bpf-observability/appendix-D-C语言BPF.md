# 附录 D C 语言 BPF

> 底本：《BPF之巅》附录 D（印刷 p793–811）。以 C 语言实现 BPF 工具的示例——可作为已编译的 C 程序或通过 perf(1) 执行，是第 2 章 BPF 部分的后续、可选学习材料。bpftrace（第 5 章）和 BCC（附录 C）在多数情况下是**首选**；本附录面向想深入了解 BPF 工作原理的读者。

## 为什么用 C 语言编程

回到 2014 年只有 C；然后有了 BCC（内核 BPF 程序用改进版 C + 多语言前端）；现在有 bpftrace（整个程序一种高级语言）。继续用 C 开发跟踪工具的理由及反面观点：

| 理由 | 说明 | 反面观点 |
|---|---|---|
| **降低启动开销** | 笔者系统中 bpftrace 启动约 40ms CPU 时间、BCC 约 160ms，独立 C 二进制可消除 | 可通过**一次性编译 BPF 目标文件按需重发**降低（Cilium、Cloudflare 有用 BPF 目标模板按需改写数据的编排系统）。自问：启动 BPF 程序的频率如何？ |
| **无庞大编译器依赖** | BCC/bpftrace 用 LLVM+Clang，为文件系统增加 80MB+，嵌入式等环境不允许 | LLVM 频繁的 API 变化（5.0/6.0/7/8）增加维护负担；ply(1)、SystemTap BPF 后端等轻量编译器项目正在改进 |
| **降低运行时开销** | 前端最终在内核跑相同字节码，内核内摘要不耗用户 CPU，重写无益 | **例外场景**：事件极多、每秒打印数千条时，前端用户态处理可见于 top(1)，C 重写有收益。未采用的优化：每 CPU 绑定消费者线程读各自环形缓冲区 |
| **BPF hacking** | C 可写出任何校验器接受的代码 | BCC 已接受任意 C 代码，很难想象需要 hacking 的场景 |
| **配合 perf(1)** | perf 支持 BPF 增强 record/trace 子命令；需向二进制输出文件高效记录大量事件的场景 perf 已优化 | 见本附录"perf C"一节 |

**结论**：很多 BPF 网络项目（如 Cilium）用 C；对于跟踪，预计 bpftrace 和 BCC 够用。

## 5 个技巧（开发 C 工具前必知）

1. **BPF C 是受限的**：不能无界循环、不能内核函数调用。只能用 bpf_* 辅助函数、BPF 尾调用、BPF 到 BPF 的函数调用、部分编译器内置函数
2. **所有内存必须通过 bpf_probe_read() 读取**（做必要检查）。目标通常是栈内存，大对象用 BPF 映射。（BCC 有基于 Clang 的重写器把 `a->b->c` 自动扩展为 bpf_probe_read() 调用，C 程序需显式调用）
3. **三种内核态→用户态输出**：
   - `bpf_perf_event_output()`（BPF_FUNC_perf_event_output）：自定义结构体逐事件发送的**首选**
   - `BPF_MAP_TYPE_*` 及映射辅助函数（如 bpf_map_update_elem()）：键值哈希可构建更高级数据结构，用于摘要统计/直方图，定期从用户态读取（高效）
   - `bpf_trace_printk()`：仅调试，写 trace pipe 可能与其他程序冲突
4. **尽量用静态插桩（跟踪点、USDT）而非动态插桩（kprobes、uprobes）**：静态接口更稳定
5. **卡住时在 BCC/bpftrace 里重写并看调试输出**：如 BCC 的 DEBUG_PREPROCESSOR 模式显示预处理后的 C 代码，可能暴露缺失的步骤

**_(P) 宏包装器**（部分工具使用）：

```c
#define _(P) ({ typeof(P) val; bpf_probe_read(&val, sizeof(val), &P); val; })
```

代码中 `_(skb->dev)` 即自动展开为对应的 bpf_probe_read() 调用。

## C 程序与库

- 新 BPF 功能的补丁集通常同时提供**示例 C 程序**（`samples/bpf/`）和**内核自测**（`tools/testing/selftests/bpf/`）；社区鼓励新开发人员添加自测而非示例
- 两种指定 BPF 程序的方法：① BPF 指令数组嵌入 C 传给 bpf(2)；② **可编译为 BPF 的 C 程序（首选）**。LLVM 有 BPF 目标，C 程序可像 x86 一样编译为 ELF，指令存在按程序类型命名的节中（如 `kprobe/`）
- **API 更迭警告**：2018-12 至 2019-08 本附录因 BPF C 库 API 变化重写两次。使用 **libbpf**（内核源 `tools/lib/bpf`，与内核同步开发，被 BCC/bpftrace 使用）和 **libbcc**（iovisor BCC，`src/cc/libbpf.h`）；Linux 4.x 的旧 `samples/bpf/bpf_load.*` 已弃用，别自造库（功能和修复滞后且阻碍 BPF 推广）
- 本附录工具由 Andrii Nakryiko 用 Linux 5.4 的最新 API 重写（早期 Linux 4.5 版本在本书工具仓库）

## 编译步骤（Ubuntu 18.04 示例）

```bash
apt-get update
apt-get install bc libssl-dev llvm-9 clang libelf-dev
ln -s /usr/bin/clang-9 /usr/bin/clang      # 让 make 找到 clang
cd /usr/src && wget https://git.kernel.org/torvalds/t/linux-5.4.tar.gz
cd linux-5.4
make olddefconfig
make $(getconf _NPROCESSORS_ONLN)           # 编译内核
make modules_install && make install && make headers_install
reboot
make samples/bpf/                           # 编译 BPF 示例（含本书工具）
```

- **警告**：先在测试系统尝试（缺虚拟化环境必要 CONFIG 选项等错误可能导致系统无法启动）
- llvm-9 或更新版本是 BPF 支持所必需；打包的 LLVM 有问题时需从源码构建（cmake + llvm/clang git 仓库，限制构建目标于 x86 和 BPF）

## 工具 1：hello_world.c（指令级编程）

把附录 C 的 hello_world.py 重写为 C。加进 samples/bpf/Makefile 后从 samples/bpf 编译：

```text
# ./hello_world
[007] svscan-1991  ...2582253.708941: 0: Hello, World!
[008] cron-983     ...2582254.363956: 0: Hello, World!
```

核心结构（节选）：

```c
struct bpf_insn prog[] = {
    BPF_MOV64_IMM(BPF_REG_1, 0xa21),           /* "!\n" */
    BPF_STX_MEM(BPF_H, BPF_REG_10, BPF_REG_1, -4),   /* 栈 -4，半字 */
    BPF_MOV64_IMM(BPF_REG_1, 0x646c726f),      /* "orld" */
    BPF_STX_MEM(BPF_W, BPF_REG_10, BPF_REG_1, -8),   /* 栈 -8，字 */
    BPF_MOV64_IMM(BPF_REG_1, 0x57202c6f),      /* "o, W" */
    BPF_STX_MEM(BPF_W, BPF_REG_10, BPF_REG_1, -12),
    BPF_MOV64_IMM(BPF_REG_1, 0x6c6c6548),      /* "Hell" */
    BPF_STX_MEM(BPF_W, BPF_REG_10, BPF_REG_1, -16),
    BPF_MOV64_IMM(BPF_REG_1, 0),
    BPF_STX_MEM(BPF_B, BPF_REG_10, BPF_REG_1, -2),   /* \0 结尾，字节 */
    BPF_MOV64_REG(BPF_REG_1, BPF_REG_10),
    BPF_ALU64_IMM(BPF_ADD, BPF_REG_1, -16),    /* r1 = 栈顶 - 16 */
    BPF_MOV64_IMM(BPF_REG_2, 15),              /* 长度 */
    BPF_RAW_INSN(BPF_JMP | BPF_CALL, 0, 0, 0, BPF_FUNC_trace_printk),
    BPF_MOV64_IMM(BPF_REG_0, 0),
    BPF_EXIT_INSN(),
};
prog_fd = bpf_load_program(BPF_PROG_TYPE_KPROBE, prog, insns_cnt,
    "GPL", LINUX_VERSION_CODE, bpf_log_buf, BPF_LOG_BUF_SIZE);  /* libbpf */
probe_fd = bpf_attach_kprobe(prog_fd, BPF_PROBE_ENTRY,
    "hello_world", "do_nanosleep", 0, 0);                       /* libbcc */
system("cat " DEBUGFS "/trace_pipe");
```

**逐段解读**：

- **字符串入栈（前 10 条指令）**：为提高效率，四个字符一组声明为 32 位整数（`BPF_W`）存入，而不是一次存一个字符；最后两个字节用 16 位（`BPF_H`）、\0 用 8 位（`BPF_B`）
- **调 trace_printk**：BPF_RAW_INSN + BPF_FUNC_trace_printk 把字符串写入共享跟踪缓冲区
- **bpf_load_program()**（libbpf）：加载程序；**bpf_attach_kprobe()**（libbcc）：附加到 do_nanosleep() 入口，事件名 "hello_world" 会显示在 `/sys/kernel/debug/tracing/kprobe_events`，利于调试；失败时库自己打印错误
- **清理**：close 探针 fd → bpf_detach_kprobe → close 程序 fd。不做的话**旧内核**会一直保留并启用探针（无用户态消费者也有开销）——用 `cat .../kprobe_events` 或 `bpftool prog show` 检查，用 BCC 的 reset-trace(8) 清除。**Linux 5.2 起切换为基于文件描述符的探针，程序退出自动关闭**
- trace_printk + system() 只为示例简短（共享 trace_pipe 无保护机制）；生产应换 BPF_FUNC_perf_event_output（见工具 2）
- **宏速查见附录 E**；虽然指令级编程可行，但不建议用于跟踪工具

**Makefile diff**（Linux 5.3，加 3 行）：`hostprogs-y += hello_world`、`hello_world-objs := hello_world.o`、`HOSTLDLIBS_hello_world += $(LIBBPF) -lbcc -lelf`。

## 工具 2：bigreads（C 声明 BPF 程序）

跟踪 vfs_read() 返回值，打印大于 1MB 的读。等价单行：

```bash
bpftrace -e 'kr:vfs_read /retval > 1024*1024/ { printf("READ: %d bytes\n", retval); }'
```

**kern/user 双文件结构**：内核组件单独用 BPF 目标架构编译，用户组件读取该文件并把 BPF 指令发到内核。

**bigreads_kern.c**：

```c
#define MIN_BYTES (1024 * 1024)
SEC("kretprobe/vfs_read")
int bpf_myprog(struct pt_regs *ctx)
{
    char fmt[] = "READ: %d bytes\n";
    int bytes = PT_REGS_RC(ctx);        /* x86 上映射为 ctx->rax */
    if (bytes >= MIN_BYTES)
        bpf_trace_printk(fmt, sizeof(fmt), bytes, 0, 0);
    return 0;
}
u32 _version SEC("version") = LINUX_VERSION_CODE;
```

- `SEC("kretprobe/vfs_read")` 声明 ELF 节：可在最终 ELF 二进制中看到，某些加载器用它决定附加位置；bitehist 的加载器没用它，但节标题调试时仍有用
- `struct pt_regs *ctx`：寄存器状态 + BPF 上下文，函数参数/返回值从寄存器读取，也是多个 BPF 辅助函数的必需参数
- `PT_REGS_RC(ctx)` 宏取返回值（kretprobe）；trace_printk 仍是为简短，有共享缓冲区冲突的警告

**bigreads_user.c**（libbpf 新 API 流程）：

```c
setrlimit(RLIMIT_MEMLOCK, &lim);                  /* 无穷大，避免 BPF 内存分配问题 */
obj = bpf_object_open(filename);                  /* 引用 kern.o，可含多个程序和映射 */
prog = bpf_program__next(NULL, obj);              /* 匹配节标题" kretprobe/vfs_read" */
bpf_program__set_type(prog, BPF_PROG_TYPE_KPROBE);
bpf_object_load(obj);                             /* 加载全部映射和程序到内核 */
link = bpf_program__attach_kprobe(prog, true /*retprobe*/, "vfs_read");
system("cat " DEBUGFS "/trace_pipe");
bpf_link__destroy(link);                          /* 分离 */
bpf_object__close(obj);                           /* 卸载并释放资源 */
```

**编译**：Makefile 加 `hostprogs-y += bigreads`、`bigreads-objs := bigreads_user.o`、`always += bigreads_kern.o`。用 `objdump -h bigreads_kern.o` 可验证 `kretprobe/vfs_read` 节确实存在。

**可靠化改造**（替代 trace_printk）：通过每 CPU perf 环形缓冲区映射发记录：

```c
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(int));
    __uint(value_size, sizeof(u32));
} mymap SEC(".maps");                              /* 基于 BTF 的新声明风格 */
bpf_perf_event_output(ctx, &mymap, 0, &bytes, sizeof(bytes));
```

用户态删掉 system()，添加映射输出事件处理函数并注册到 perf 事件轮询器——参考示例 `samples/bpf/trace_output_user.c`。（较早内核需把 max_entries 设为 NR_CPUS，现在已是 PERF_EVENT_ARRAY 的默认）

## 工具 3：bitehist（BPF 映射直方图）

基于附录 C 的 BCC bitehist.py 的 C 版，演示**通过 BPF 映射输出**块设备 I/O 大小直方图。

**bitehist_kern.c** 片段：

```c
struct hist_key { u32 index; };
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, struct hist_key);
    __type(value, long);
} hist_map SEC(".maps");                           /* BTF 风格，键=桶索引，值=计数 */

SEC("kprobe/blk_account_io_completion")
int bpf_prog1(struct pt_regs *ctx)
{
    long init_val = 1;
    long *value;
    struct hist_key key = {};
    key.index = log2l(PT_REGS_PARM2(ctx) / 1024);  /* 第2参数→log2桶索引 */
    value = bpf_map_lookup_elem(&hist_map, &key);
    if (value)
        __sync_fetch_and_add(value, 1);            /* 原子递增 */
    else
        bpf_map_update_elem(&hist_map, &key, &init_val, BPF_ANY);
    return 0;
}
```

**bitehist_user.c** 要点：

- main() 与 bigreads 同套路：setrlimit → `bpf_object_open` → `bpf_object__find_program_by_title(obj, "kprobe/blk_account_io_completion")` → set_type → `bpf_object_load` → `bpf_program__attach_kprobe(prog, false, ...)`（入口探针）
- `bpf_object__find_map_by_name()` 取映射对象存为全局变量，供退出时打印
- **int_exit() 是 SIGINT（Ctrl+C）信号处理程序**：`signal(SIGINT, int_exit)` 注册后 main 进入 sleep(-1)；按 Ctrl+C 时调用 print_log2_hist() 打印直方图、销毁 link、关闭对象、exit(0)
- print_log2_hist() 用 `bpf_map_get_next_key()` 循环 + `bpf_map_lookup_elem()` 迭代读取每个桶值

## perf C

perf(1) 能在两类事件接口上跑 BPF 程序：**perf record**（自定义过滤器并向 perf.data 发额外记录）和 **perf trace**（BPF 过滤并增强跟踪输出，如显示系统调用上的文件名字符串而非指针）。perf 的 BPF 能力增长快但缺文档——最好的资料是用 "perf" + "BPF" 搜内核邮件列表归档。

**perf 版 bigreads**：

```bash
perf record -e bpf-output/no-inherit,name=evt/ \
    -e ./bigreads.c/map:channel.event=evt/ -a
perf script
```

```text
dd 31049 [009] 2652091.826549: evt: ffffffff...  kretprobe_trampoline+0x0
    BPF output: 0000: 00002000 00000000 0008: 00000000
```

- perf.data 只含大于 1MB 的读 + 包含读大小的 BPF 输出事件；`000020` 是 2MB（0x200000）的**小端序**（x86）表示
- bigreads.c：`SEC("func=vfs_read")`、`ctx->rdx`（x86_64 返回值寄存器）、旧式 `struct bpf_map_def` 声明 `channel`（PERF_EVENT_ARRAY），大于 MIN_BYTES 时调 `perf_event_output(ctx, &channel, BPF_F_CURRENT_CPU, &bytes, sizeof(bytes))`
- perf 接口正获得更多能力（如 `perf record -e program.c` 直接运行 BPF 程序），关注最新发展

## 更多信息

- Linux 源码 `Documentation/networking/filter.txt`

## 相关章节

- 上一章：[appendix-C-BCC工具开发.md](./appendix-C-BCC工具开发.md)
- 下一章：[appendix-E-BPF指令.md](./appendix-E-BPF指令.md)
