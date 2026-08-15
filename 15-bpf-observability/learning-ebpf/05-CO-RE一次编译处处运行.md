# Learning eBPF · 第 5 章：CO-RE——一次编译，处处运行

> 底本：`../LEARNING-EBPF-BILINGUAL.pdf`。全书最长的一章，讲清楚 eBPF 程序如何跨内核版本可移植：BTF 记录类型布局 → Clang 生成 CO-RE 重定位 → libbpf 加载时按目标内核改写指令。也是作者明确表态"BCC 不适合生产分发"的一章。

## 本章目标

1. 理解跨内核可移植性问题为什么存在，BCC 运行时编译方案的五大痛点
2. 掌握 CO-RE 五要素：BTF、内核头文件、编译器支持、重定位库、（可选）BPF skeleton
3. 能读懂 BTF 类型转储、`vmlinux.h` 生成、`bpf_core_relo` 重定位日志
4. 会写 libbpf 风格的内核侧 `.bpf.c` 与用户侧骨架代码

## 1. BCC 的老方案为什么不行

BCC 的思路：**在目标机器上运行时现场编译** eBPF 源码。痛点（原文列举）：

1. 每台目标机都要装**编译工具链 + 内核头文件**（头文件默认往往不存在）
2. 每次启动工具都要等编译，**数秒延迟**
3. 大规模同构机器集群上逐台重复编译，**浪费算力**
4. 打进容器镜像能解决分发，但解决不了内核头文件缺失，多个 BCC 容器还会互相重复
5. **嵌入式设备内存不够**跑编译

> 作者原话：如果你要开发一个正经的新 eBPF 项目、尤其是要分发给别人用，**不推荐**传统 BCC 方式。BCC 适合学习和快速原型（Python 用户态代码简洁）；BCC 仓库自己都把工具迁移到了 `libbpf-tools/` 目录（C + libbpf + CO-RE 版本）。

## 2. CO-RE 五要素

| 要素 | 作用 |
|---|---|
| **BTF** | 描述数据结构与函数签名的格式；用来对比"编译时布局"与"运行时布局"的差异。5.4 起内核自带（需 `CONFIG_DEBUG_INFO_BTF`） |
| **内核头文件** | 不再逐个 include 内核头文件，用 `bpftool` 生成一个 `vmlinux.h` 全量搞定 |
| **编译器支持** | Clang 加 `-g` 编译时生成 CO-RE 重定位信息（GCC 12 起也支持 BPF 目标的 CO-RE） |
| **重定位库** | 加载时改写字节码适配目标内核：C 用 libbpf，Go 用 cilium/ebpf，Rust 用 Aya |
| **BPF skeleton**（可选） | `bpftool gen skeleton` 自动生成的生命周期管理代码，比裸调库方便 |

必读材料（作者反复点名 Andrii Nakryiko）：CO-RE 博客、《BPF CO-RE Reference Guide》、libbpf-bootstrap 教程、BCC→libbpf 迁移指南。

## 3. BTF 深入

### 3.1 BTF 不只为 CO-RE 服务

- **漂亮打印**：bpftool 用 BTF 把 map 里的字节按类型还原成人话（第 4 章已见）
- **源码交错**：`bpftool prog dump` 里 C 源码与指令交错、第 6 章验证器日志带源码，都靠 BTF 的行/函数信息
- **BPF 自旋锁**（5.1 起）：`struct bpf_spin_lock` 必须内嵌在 map value 结构里，内核需要 BTF 才知道锁字段在哪。限制：只能用于 hash/array map，不能用于 tracing 和 socket filter 程序

### 3.2 bpftool 查看 BTF

```
bpftool btf list          # 所有已加载 BTF：第 1 项 vmlinux（约 5.8MB）
bpftool btf dump id 149   # 某个 BTF blob 的全部类型定义
bpftool btf dump map name config    # 只看某个 map 相关联的类型
bpftool btf dump prog <id>          # 只看某个程序的
```

`btf list` 里每行可见：BTF id、大小、关联的 prog_ids / map_ids / pids。注意 **perf event buffer map 不使用 BTF**，所以 map_ids 列表里看不到它。

### 3.3 读懂 BTF 类型定义（书上手把手例子）

源码里 `BPF_HASH(config, u32, struct user_msg_t)`，`user_msg_t` 含 12 字节 message。BTF dump：

```
[1] TYPEDEF 'u32' type_id=2
[2] TYPEDEF '__u32' type_id=3
[3] INT 'unsigned int' size=4 bits_offset=0 nr_bits=32 encoding=(none)
[4] STRUCT 'user_msg_t' size=12 vlen=1
        'message' type_id=6 bits_offset=0
[5] INT 'char' size=1 bits_offset=0 nr_bits=8 encoding=(none)
[6] ARRAY '(anon)' type_id=5 index_type_id=7 nr_elems=12
[8] STRUCT '____btf_map_config' size=16 vlen=2      ← BCC 自动生成的 key+value 包装结构
        'key' type_id=1 bits_offset=0
        'value' type_id=4 bits_offset=32
```

解读要点：
- 每行 `[N]` 是类型 id，类型间用 `type_id=` 链式引用（u32 → __u32 → unsigned int 三层 typedef）
- `vlen` = 结构体字段数；`bits_offset` = 字段在结构内的位偏移
- **对齐坑**：`{char letter; u64 number;}` 中 letter 后面有 7 字节填充（64 位对齐），所以不能假设字段紧挨着；`____btf_map_config` 里 value 从 32 位处开始正是因为 key 占前 32 位
- 函数也有 BTF：`FUNC_PROTO`（返回类型 + 参数）+ `FUNC`；`PTR type_id=0` = void 指针

### 3.4 map 创建时如何携带 BTF

`bpf(BPF_MAP_CREATE)` 的 attr 里有 `btf_fd / btf_key_type_id / btf_value_type_id` 三个字段。BTF 之前，内核只知道 key/value 各占多少字节（`key_size/value_size`），不知道内部结构。注意 key 和 value 是**分开传两个 type_id**，`____btf_map_config` 只是 BCC 用户态的产物，内核不用它。

## 4. vmlinux.h：一个头文件替代全部内核头

```
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

- 包含**当前运行内核**的全部数据类型定义，`.bpf.c` 里 include 它即可，不用再翻内核源码找头文件
- **不含 `#define` 常量**！比如以太网协议号 `0x0800`（IP）/`0x0806`（ARP）在 `if_ether.h` 里，vmlinux.h 没有这些值，要么自己抄要么单独 include（第 8 章会踩到）
- 5.4+ 内核自带 `/sys/kernel/btf/vmlinux`；老内核可从 **BTFHub** 拿到各发行版预生成的 BTF 文件

## 5. 内核侧代码（hello-buffer-config.bpf.c）

### 5.1 头文件组合（libbpf 项目标准范式）

```c
#include "vmlinux.h"            // 内核全部类型
#include <bpf/bpf_helpers.h>    // helper 函数与 map 宏
#include <bpf/bpf_tracing.h>    // BPF_KPROBE_SYSCALL 等
#include <bpf/bpf_core_read.h>  // CO-RE 读内存封装
#include "hello-buffer-config.h"// 自己写的、用户态/内核态共享的结构
```

libbpf 的微妙之处：它**不只是用户态库**，内核侧 C 代码同样要 include 它的头。

共享结构放独立头文件（如 `data_t`），因为用户态 `.c` 也要用；BCC 时代两边代码在同一文件、BCC 幕后打通。

### 5.2 map 定义：BCC 宏 → 手写 BTF 风格

BCC 一行：`BPF_HASH(config, u64, struct user_msg_t);`

libbpf 要写全：

```c
struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(u32));
    __uint(value_size, sizeof(u32));
} output SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, u32);
    __type(value, struct user_msg_t);
} my_config SEC(".maps");
```

`__uint/__type/__array` 是 `bpf_helpers_def.h` 里的宏（`int (*name)[val]` 这类指针技巧）。坑：`config` 与 vmlinux.h 里的定义撞名，改名 `my_config`。

### 5.3 SEC() 节名：程序类型 + 自动附加点

```c
SEC("kprobe")                        // 只声明类型，附加点留给用户态
SEC("kprobe/__arm64_sys_execve")     // 指定具体函数（架构相关！）
SEC("ksyscall/execve")               // libbpf 自动解析架构相关 syscall 函数名
```

- ELF 节名 → libbpf 据此决定按什么程序类型加载、往哪附加
- libbpf 1.0 之后节名要求严格化，老代码里的野节名会报错

### 5.4 程序本体：BPF_KPROBE_SYSCALL 宏

```c
SEC("ksyscall/execve")
int BPF_KPROBE_SYSCALL(hello, const char *pathname)
{
    struct data_t data = {};
    struct user_msg_t *p;
    data.pid = bpf_get_current_pid_tgid() >> 32;
    data.uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    bpf_get_current_comm(&data.command, sizeof(data.command));
    bpf_probe_read_user_str(&data.path, sizeof(data.path), pathname);
    p = bpf_map_lookup_elem(&my_config, &data.uid);
    if (p != 0)
        bpf_probe_read_kernel(&data.message, sizeof(data.message), p->message);
    else
        bpf_probe_read_kernel(&data.message, sizeof(data.message), message);
    bpf_perf_event_output(ctx, &output, BPF_F_CURRENT_CPU, &data, sizeof(data));
    return 0;
}
```

与 BCC 版的关键差异：

| BCC | libbpf |
|---|---|
| `my_config.lookup(&uid)` | `bpf_map_lookup_elem(&my_config, &uid)`（BCC 编译前重写，libbpf 不重写，直接写 helper 全名） |
| `output.perf_submit(ctx, ...)` | `bpf_perf_event_output(ctx, &output, BPF_F_CURRENT_CPU, ...)` |
| 不支持全局变量 | `char message[12] = "Hello World";` 可作全局变量 |
| `int hello(void *ctx)` 手动从 pt_regs 取参 | `BPF_KPROBE_SYSCALL(hello, const char *pathname)` 按名字拿 syscall 参数 |

`ctx` 在源码里看不见——藏在 `bpf_tracing.h` 的宏定义内部，但可以直接用（往 perf buffer 提交时需要它）。

### 5.5 CO-RE 读内存：bpf_core_read / BPF_CORE_READ

验证器（第 6 章）通常不允许 `x = p->y` 直接解引用（tp_btf/fentry/fexit 等 BTF-enabled 类型例外）。追踪类程序必须走 `bpf_probe_read_*()` 家族；`bpf_probe_write_user()` 官方口径"仅供实验"。

CO-RE 封装：

```c
#define bpf_core_read(dst, sz, src) \
    bpf_probe_read_kernel(dst, sz, (const void *)__builtin_preserve_access_index(src))
```

`__builtin_preserve_access_index()` 是 Clang 扩展：让编译器为这条访存指令生成 CO-RE 重定位条目（这也是部分 C 编译器至今无法产出 eBPF 字节码的原因）。

链式读取三连 `a->b->c->d` 压缩成一行：

```c
d = BPF_CORE_READ(a, b, c, d);
```

### 5.6 编译三要素（Makefile 背后的为什么）

```make
clang -target bpf \
      -D __TARGET_ARCH_$(ARCH) \
      -I/usr/include/$(shell uname -m)-linux-gnu \
      -Wall -O2 -g -c $< -o $@
llvm-strip -g $@
```

- **`-g`**：生成 BTF 必需；但顺带产出的 DWARF 调试信息 eBPF 用不到，`llvm-strip -g` 剥掉以减小体积
- **`-O2`（或更高）**：不是优化偏好而是硬性要求——Clang 默认会用 `callx <reg>`（从寄存器取地址调用），eBPF 不支持；-O2 起才会生成验证器能接受的字节码
- **`-D __TARGET_ARCH_$(ARCH)`**：`BPF_KPROBE_SYSCALL` 等宏展开依赖架构相关的 `pt_regs`，必须告知目标架构。作者吐槽：所以准确说是 "compile once **per architecture**, run everywhere"

### 5.7 目标文件里的 BTF 与重定位

`readelf -S xxx.bpf.o | grep BTF` 可见 `.BTF`（数据+字符串）与 `.BTF.ext`（函数+行信息）两个节。

重定位的载体是 `linux/bpf.h` 里的：

```c
struct bpf_core_relo {
    __u32 insn_off;        // 哪条指令
    __u32 type_id;         // 结构体的 BTF 类型
    __u32 access_str_off;  // 字段访问路径（如 0:1:2）
    enum bpf_core_relo_kind kind;
};
```

谁触发 Clang 生成重定位？`vmlinux.h` 开头的一行：

```
#pragma clang attribute push (__attribute__((preserve_access_index)), ...)
```

`attribute push/pop` 把 `preserve_access_index` 应用到文件内**所有**类型定义。

加载时观察重定位（`bpftool -d prog load ...`）：

```
libbpf: CO-RE relocating [24] struct user_pt_regs: found target candidate [205] struct user_pt_regs in [vmlinux]
libbpf: prog 'hello': relo #0: <byte_off> [24] struct user_pt_regs.regs[0] (0:0:0 @ offset 0)
libbpf: prog 'hello': relo #0: matching candidate #0 ...
libbpf: prog 'hello': relo #0: patched insn #1 (LDX/ST/STX) off 0 -> 0
```

流程：程序 BTF 里的 `user_pt_regs`（id 24）按名字匹配 vmlinux BTF 里的同名结构（id 205）→ 算出字段偏移差异 → **patch 指令的 offset 字段**。本例编译加载同机所以 0→0；跨内核时就会真正改写。目标内核里根本没有该字段/结构时，程序不可移植（加载报错）。

## 6. 用户侧：libbpf + BPF Skeleton

### 6.1 生成与本质

```
bpftool gen skeleton hello-buffer-config.bpf.o > hello-buffer-config.skel.h
```

骨架头文件里有：程序/map 的结构定义、一整套 `hello_buffer_config_bpf__*` 生命周期函数、以及末尾的 `__elf_bytes()` 函数——**ELF 字节被嵌进骨架**，生成后 .o 文件可删，可执行文件自带字节码（也可用 `bpf_object__open_file()` 直接从 ELF 文件加载，二选一）。

### 6.2 生命周期主流程

```c
skel = hello_buffer_config_bpf__open_and_load();  // open: 解析 ELF；load: 装入内核 + CO-RE 修复
err  = hello_buffer_config_bpf__attach(skel);     // 按 SEC() 自动附加
pb   = perf_buffer__new(bpf_map__fd(skel->maps.output), 8,
                        handle_event, lost_event, NULL, NULL);
while (true) err = perf_buffer__poll(pb, 100);    // 100ms 超时轮询
perf_buffer__free(pb);
hello_buffer_config_bpf__destroy(skel);
```

细节：
- **open 与 load 可拆开**：`__open()` → 改配置（如 `skel->data->c = 10` 初始化全局变量）→ `__load()`。加载之后再改 `skel->data->c` **无效**——骨架对象只是 ELF 信息的用户态副本
- 复用已有 map：`bpf_map__set_autocreate()` 关掉自动创建，`bpf_obj_get("/sys/fs/bpf/xxx")` 按 pin 路径拿 fd（典型场景：两个 eBPF 程序共享一个 map，只允许一方建）
- SEC() 没写全附加点时，用 `bpf_program__attach_kprobe / attach_xdp / ...` 手工附加
- `libbpf_set_print()` 注册 libbpf 日志回调

## 7. 坑点清单

1. **vmlinux.h 没有 `#define` 常量**——协议号、标志位得自己补（第 8 章实例）
2. **vmlinux.h 撞名**——自定义 map/变量名可能和内核类型重名（config → my_config）
3. **`-O2` 不是可选项**——默认优化级别会产出 `callx`，验证器直接拒
4. **`-D __TARGET_ARCH_$(ARCH)` 忘了传**——用了 BPF_KPROBE 系宏就编译失败或取错寄存器
5. **加载后改 skel->data 无效**——配置必须在 open/load 之间做
6. **同机编译加载时重定位日志全是 0→0**——别误以为重定位没生效；跨内核才能看到真 patch
7. **libbpf 1.0 起节名严格**——老教程里的自由格式 SEC 名会加载失败
8. **perf buffer map 不带 BTF**——btf list 里看不到它属正常

## 8. HFT 关联

- **低延迟交易系统必须先低抖动**：BCC 每次启动编译数秒只是小事，真正的风险是"每台机器各自编译"导致同构集群行为 subtly 不一致。CO-RE 单一二进制 + 加载时确定性重定位，符合交易系统"构建产物可审计、运行时可复现"的合规直觉
- **行情/风控探针的部署形态**：把 eBPF 字节码嵌进骨架、编译成单个静态可执行文件，无 Python、无工具链依赖，适合推送到交易机房的精简镜像环境（内存受限场景 BCC 编译可能直接 OOM）
- **内核结构漂移**：交易机内核升级（如 5.4 → 6.x）后，旧对象文件里 task_struct 字段偏移已变；CO-RE 加载时自动 patch，避免"升级后探针静默读错字段"这类极难排查的数据污染
- `BPF_CORE_READ(a, b, c, d)` 链式读在追踪热路径（如 `execve` 风暴、上下文切换统计）中每层都是显式 helper 调用，读多深要在"信息量"与"指令数（验证器限额）"之间权衡

## 9. 自测题

1. BCC 运行时编译方案的五个问题是什么？哪一条对嵌入式是致命的？
2. `bpftool btf dump map name config` 输出里 `bits_offset=32` 的 value 字段为什么从 32 位处开始？
3. `struct {char c; u64 n;}` 在 BTF 里 size 是多少？为什么？
4. vmlinux.h 是从哪个文件生成的？它缺哪类信息需要手工补？
5. `bpf_core_read()` 与 `bpf_probe_read_kernel()` 的唯一差别是什么？哪个 Clang 内建函数触发了重定位条目的生成？
6. 为什么编译 eBPF 必须 `-O2`？为什么还要 `-D __TARGET_ARCH_$(ARCH)`？
7. `struct bpf_core_relo` 四个字段各是什么含义？libbpf 拿到它之后做了什么？
8. `__open()` 和 `__load()` 拆开用的典型场景是什么？加载后修改 `skel->data` 结果如何？
9. 两个 eBPF 程序要共享一个 map，怎么避免 map 被创建两次？

## 10. 交叉引用

- 前置：`04-bpf系统调用.md`（BPF_BTF_LOAD、btf_fd 字段）、`03-eBPF程序解析.md`（clang -target bpf、bpftool load）
- 后续：`06-验证器.md`（为什么不能直接 `p->y`）、`07-程序类型.md`（SEC 名与程序类型全集）、`08-eBPF网络.md`（vmlinux.h 缺协议常量的实际案例）、`10-eBPF编程.md`（cilium/ebpf、libbpfgo、Aya）
