# bpf() 总览

```c
int bpf(int cmd, union bpf_attr *attr, unsigned int size);
```

- `cmd`：要执行的命令（BPF_MAP_CREATE、BPF_PROG_LOAD…全量清单在 `linux/bpf.h`，内核源码是最好文档）
- `attr`：命令参数（联合体，各命令用不同字段）
- `size`：attr 字节数

**关键分层**：内核里的 eBPF 程序访问 map **不走 syscall**（用 helper 函数）；syscall 接口只属于用户态。库（BCC/libbpf）的抽象与这些命令几乎一一对应。
