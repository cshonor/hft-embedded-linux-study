# ch03 code · libbpf 风格程序

> 《Learning eBPF》第 3 章示例：从 BCC（用户态现场编译）转向 libbpf（离线编译 ELF）。
> 笔记中 `bpf_insn` 结构体（3.1 块1）为教学引用，不落盘。

## 程序

| 文件 | 出处 | 内容 | 验证（2026-10 Ubuntu 24.04，clang 18.1） |
|------|------|------|------|
| [hello-xdp.bpf.c](./hello-xdp.bpf.c) | 3.1 块2 | XDP 版 Hello World：全局变量（=.data map）+ bpf_printk + XDP_PASS 裁决 | `clang -target bpf` 编译 ✓ 生成 .o |
| [hello-bpf2bpf.bpf.c](./hello-bpf2bpf.bpf.c) | 3.3 块3 | `noinline` 生成真·BPF-to-BPF 调用（内核 4.16+） | 编译 ✓ |

## scripts/

| 脚本 | 内容 |
|------|------|
| [compile-bpf.sh](./scripts/compile-bpf.sh) | `compile`（全部 .bpf.c → .o）/ `load <nic>`（挂 XDP，lo 自动用 xdpgeneric）/ `unload <nic>` |

## 编译要点（实测踩坑）

```bash
clang -O2 -g -target bpf -D__TARGET_ARCH_x86 \
      -I/usr/include/x86_64-linux-gnu \    # ← 必须：asm/types.h 在 arch 目录
      -c hello-xdp.bpf.c -o hello-xdp.bpf.o
```

- 不加 `-I/usr/include/x86_64-linux-gnu` 会报 `'asm/types.h' file not found`
- 本例只 include `linux/bpf.h` + `bpf_helpers.h`，**不需要 vmlinux.h**
  （涉及内核结构体字段的 CO-RE 程序才需要，见 ch05）
- 加载与 attach 需 root：`sudo ip link set dev lo xdpgeneric obj hello-xdp.bpf.o sec xdp`
