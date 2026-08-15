# Learning eBPF · 第 3 章：eBPF 程序解剖

> 底本：`../LEARNING-EBPF-BILINGUAL.pdf`。抛弃 BCC 的黑盒，用纯 C + libbpf + bpftool 走完源码 → 字节码 → 机器码 → 加载 → 挂载全流程。

## 本章目标

1. 理解 eBPF 虚拟机：10 个软件寄存器 + 8 字节定长指令
2. 亲手编译（clang -target bpf）、检查（llvm-objdump/bpftool）、加载、附加、卸载
3. 全局变量 = map 语义（.bss/.rodata）
4. BPF to BPF 函数调用在字节码层的形态

## 1. eBPF 虚拟机

- 字节码早期在内核里**解释执行**，现已被 **JIT 编译**取代——既为性能，也为规避 eBPF 解释器的 Spectre 类侧信道漏洞
- 指令集/寄存器模型设计成与主流 CPU 架构**一一映射**，使 JIT/解释都简单直接

**寄存器**（软件实现，`include/uapi/linux/bpf.h` 中 `BPF_REG_0`~`BPF_REG_10`）：
- R0-R9 通用；R10 = 栈帧指针（只读）
- 程序入参 ctx 放 **R1**；返回值放 **R0**
- 调用函数时参数放 R1-R5（最多 5 个参数）

**指令** `struct bpf_insn`（64 位 / 8 字节）：

```c
struct bpf_insn {
    __u8 code;          /* 操作码 */
    __u8 dst_reg:4;     /* 目标寄存器 */
    __u8 src_reg:4;     /* 源寄存器 */
    __s16 off;          /* 有符号偏移（跳转/访存） */
    __s32 imm;          /* 有符号立即数 */
};
```

- 需要装载 64 位立即数时放不下 → **宽指令编码 16 字节**（两条槽）
- 指令大类：装载（立即数/内存/寄存器）、存储、算术、条件跳转
- 5.12 加入原子指令（ADD/AND/OR/XOR 由 imm 指定）——与 04-cpp 学的原子操作直接对应

## 2. XDP 版 Hello World（纯 C）

```c
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
int counter = 0;                       // 全局变量
SEC("xdp")                             // ELF 段名 = 程序类型标记
int hello(void *ctx) {
    bpf_printk("Hello World %d", counter);
    counter++;
    return XDP_PASS;                   // 裁决：正常继续处理
}
char LICENSE[] SEC("license") = "Dual BSD/GPL";
```

**要点：**
- 文件名约定 `*.bpf.c` 区分内核态代码与用户态代码
- `SEC("license")` 是**硬性要求**：部分 helper 是 "GPL only"，声明不兼容时验证器直接拒绝；LSM 类程序必须 GPL 兼容
- `bpf_printk`（libbpf 名）/`bpf_trace_printk`（BCC 名）是同一内核函数的封装
- XDP 在包**到达网卡入口**的瞬间触发，可改包内容并给裁决（PASS/DROP/REDIRECT…）
- 部分网卡支持 **XDP 卸载**，程序直接跑在网卡上（DDoS 防护、防火墙、负载均衡利器）

## 3. 编译与检查

```make
hello.bpf.o: %.o: %.c
	clang -target bpf -I/usr/include/$(shell uname -m)-linux-gnu -g -O2 -c $< -o $@
```

- `file hello.bpf.o` → `ELF 64-bit LSB relocatable, eBPF`
- `llvm-objdump -S hello.bpf.o` 看字节码（`-g` 让源码与指令并排显示）

反汇编解读（学会读这个，验证器报错就不可怕了）：

```
 0: 18 06 ... r6 = 0 ll          # 宽指令16字节：r6 指向全局变量所在的 map（.bss）
 2: 61 63 ... r3 = *(u32 *)(r6+0)  # 取 counter
 5: b7 02 00 00 0f 00 00 00 r2 = 15  # 操作码0xb7 = dst=imm，r2=15（字符串长度）
 6: 85 00 00 00 06 00 00 00 call 6   # 操作码0x85 = 函数调用，imm=6 → helper #6 (bpf_trace_printk)
10: b7 00 00 00 02 00 00 00 r0 = 2   # 返回值放 r0；XDP_PASS == 2
11: 95 ... exit
```

注意宽指令占 2 个偏移槽（offset 0 的下一条在 2）。

## 4. 加载、检查、附加、卸载（bpftool）

```
bpftool prog load hello.bpf.o /sys/fs/bpf/hello   # 加载 + pin 到 bpffs
bpftool prog list                                  # 540: xdp name hello tag d35b... gpl
bpftool prog show id 540 --pretty                  # JSON 全字段
bpftool net attach xdp id 540 dev eth0             # 挂到网卡
bpftool net detach xdp dev eth0                    # 摘除（程序仍在内核）
rm /sys/fs/bpf/hello                               # 删 pin 文件 = 卸载
```

**prog show 关键字段：**
| 字段 | 含义 |
|------|------|
| id | 加载时分配，进程内唯一 |
| tag | 指令的 **SHA 哈希**——同一程序重加载 tag 不变（id 会变） |
| bytes_xlated | 过验证器（可能被内核改写）后的字节码字节数 |
| bytes_jited / jited | JIT 后机器码字节数（例：96B 字节码 → 148B 机器码） |
| memlock | 锁定内存（不会被换出）——eBPF 内存必须常驻 |
| map_ids | 关联的 map（本例源码没有 map 却有 2 个——见全局变量） |
| btf_id | 有 BTF 信息块（`-g` 编译才有） |

**程序引用四方式**：id / name / tag / pinned path。name 和 tag 可重复，id 和 pin 路径唯一。

`bpftool prog dump xlated` 看验证后字节码（与 llvm-objdump 基本一致）；`dump jited` 看机器码（需要 bpftool 编译时带 libbfd）。

**XDP 事件的上下文特殊性**：trace 输出行首是 `<idle>-0`——包到达时**没有任何用户态进程与之关联**（第 2 章 syscall 事件则有 pid/comm）。这解释了为什么网络类程序拿不到 `bpf_get_current_pid_tgid()` 的有效值。

## 5. 全局变量 = map

2019 年起支持全局变量，实现就是 map：
- `hello.bss`（array map，1 项）→ 初始化为 0 的全局变量（counter）
- `hello.rodata`（array map，1 项，frozen）→ 只读数据（格式串 "Hello World %d"）

有 BTF（`-g`）时 `bpftool map dump` 能漂亮打印变量名和值；没有则只能看裸十六进制（`19 01 00 00` = 小端 281）。

**pin 机制**：bpffs（`/sys/fs/bpf/`）上的伪文件持有程序/map 的引用；删除文件即释放（无 `prog unload` 命令时用 rm）。用户态程序退出后，pinned 的程序仍留在内核。

## 6. BPF to BPF 调用（libbpf 才有）

```c
static __attribute((noinline)) int get_opcode(struct bpf_raw_tracepoint_args *ctx) {
   return ctx->args[1];
}
SEC("raw_tp")
int hello(struct bpf_raw_tracepoint_args *ctx) {
   int opcode = get_opcode(ctx);
   bpf_printk("Syscall: %d", opcode);
   return 0;
}
```

字节码里 `call pc+7` 跳到子程序偏移 8 处——**真实函数调用，非内联**。函数调用要把状态压栈以便返回，而栈只有 512 字节 → **嵌套深度受限**。（`noinline` 仅为演示防编译器优化掉；生产代码让编译器自行决定。）

对照第 2 章：尾调用替换栈帧不增长栈，BPF-to-BPF 调用消耗栈但可返回——5.10 起两者可混用。

## 坑点清单

1. **XDP 返回 0 = XDP_ABORTED ≠ 成功**——返回 0 挂到 eth0 会丢掉所有包，SSH 直连机器直接失联。测试 XDP 放容器/虚拟网卡里（参考 lizrice/lb-from-scratch）
2. 忘了 `SEC("license")` 或用了 GPL-only helper 却声明专有许可 → 验证器拒绝
3. 编译不加 `-g`：没 BTF → bpftool 无法漂亮打印、CO-RE 不可用（第 5 章）
4. bpftool 必定 pin；忘删 pin 文件 = 程序滞留内核（重启前一直占内存）
5. `net.core.bpf_jit_enable` sysctl / `CONFIG_BPF_JIT` 控制 JIT——性能测试前先确认开着
6. tag 与 id 的区别：tag 跟着内容走（内容变 tag 变），id 跟着加载实例走

## HFT 关联

- `bpftool prog dump jited` 是核对"最终执行的机器码"的手段——类比反汇编交易热路径确认编译器优化，eBPF 侧要确认验证器改写和 JIT 没引入意外
- memlock 常驻内存：eBPF 程序和数据永不换页——与 HFT 的 mlockall 哲学一致，天然适合延迟敏感路径
- XDP 卸载到网卡 = 在包进主机内存前处理，理论上限低于 DPDK 但零改动内核栈——交易前置的行情过滤器可以用这个层次
- 读字节码是理解验证器日志的前置技能（第 6 章验证失败报的都是指令偏移）

## 自测题

1. eBPF 的 10 个寄存器各司什么职？R1 和 R0 的特殊用途？
2. 什么是宽指令编码？为什么需要它？
3. 一个 XDP 程序源码没有 map，为什么 bpftool 显示关联了 2 个 map？
4. id、name、tag、pinned path 四种引用方式，哪些唯一？
5. BPF-to-BPF 调用和尾调用在字节码层和栈使用上的区别？
6. 为什么 trace 输出显示 `<idle>-0`？这反映 XDP 事件的什么特性？

## 交叉引用

- 加载/附加在 syscall 层的全过程 → `04-bpf系统调用.md`
- BTF 与 CO-RE → `05-CO-RE一次编译处处运行.md`
- XDP 深入 → `08-eBPF网络.md`
- 指令集全景 → 本书附录 E；BPF 之巅附录同主题笔记 `../appendix-E-BPF指令.md`
