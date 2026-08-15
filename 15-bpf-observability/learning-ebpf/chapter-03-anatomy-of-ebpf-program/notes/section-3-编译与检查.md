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
