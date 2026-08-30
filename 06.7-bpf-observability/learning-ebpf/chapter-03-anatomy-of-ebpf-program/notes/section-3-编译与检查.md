# 编译与检查

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

### 真机案例：ARM64 上交叉编译（树莓派 5 实测，2026-08）

书上这条 Makefile 在 x86_64 直接能跑，但在 **aarch64** 上第一次就编译失败：

```
clang -target bpf -O2 -g -c hello.bpf.c -o hello.bpf.o
fatal error: 'asm/types.h' file not found
```

**原因**：`-target bpf` 让 clang 把目标定为 BPF 虚拟机，它不再知道宿主机的架构，也就不知道该去哪个架构目录找 `asm/` 头文件（`asm/types.h` 是被 `<linux/bpf.h>` 间接包含的）。x86_64 之所以"碰巧能用"，是因为书里的 `-I/usr/include/x86_64-linux-gnu` 手动指了路径——**这行不是可选装饰，是架构相关的必需项**。

**修复**（aarch64）：

```make
CLANG = clang
ARCH_INCLUDES = -D__TARGET_ARCH_arm64 -I/usr/include/aarch64-linux-gnu

hello.bpf.o: hello.bpf.c
	$(CLANG) -target bpf $(ARCH_INCLUDES) -O2 -g -c $< -o $@
```

- `-D__TARGET_ARCH_arm64`：`bpf_tracing.h` 里 `PT_REGS_PARMx` / `BPF_KPROBE` 系宏靠它选对寄存器映射，漏传则编译失败或在 kprobe 里取错参数
- 通用写法：`-D__TARGET_ARCH_$(shell uname -m | sed s/aarch64/arm64/)`，`-I/usr/include/$(shell uname -m)-linux-gnu`

> 完整可复现实验：[ebpf-gate/labs/02-kprobe](https://github.com/cshonor/ebpf-gate)（Pi 5 / 6.18 内核 / clang 19 实测通过）
