# 内核侧代码（hello-buffer-config.bpf.c）

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
