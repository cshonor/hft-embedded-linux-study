# 06 — Documentation/networking/filter.rst

> **对应 Rosen:** Ch9（Netfilter）/ Ch1（socket filter）
> **内核源码路径:** `Documentation/networking/filter.rst`

## 文档概述

Linux 包过滤器文档，从经典 BPF（cBPF）到 eBPF 的演进。

## 核心内容

### cBPF（Classic BPF）

- 原始 BSD 包过滤器，用于 tcpdump / SO_ATTACH_FILTER
- 指令集简单（load/store/jump/ret）
- 32 位寄存器

### eBPF（Extended BPF）

- 10 个 64 位寄存器
- JIT 编译为原生指令
- 可调用 helper 函数
- verifier 保证安全性

### cBPF → eBPF 转换

内核内部将 cBPF 程序翻译为 eBPF 再执行：
```c
// tcpdump 编译为 cBPF
// 内核自动翻译为 eBPF
// JIT 编译为原生指令
```

### Socket Filter

```c
// 经典 cBPF socket filter
struct sock_filter code[] = {
    BPF_STMT(BPF_LD | BPF_W | BPF_ABS, 12),   // 加载 ethertype
    BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, 0x0800, 0, 1),  // IPv4?
    BPF_STMT(BPF_RET | BPF_K, 65535),  // 接受
    BPF_STMT(BPF_RET | BPF_K, 0),      // 拒绝
};
struct sock_fprog prog = { .len = 4, .filter = code };
setsockopt(sockfd, SOL_SOCKET, SO_ATTACH_FILTER, &prog, sizeof(prog));
```

## HFT 要点

- socket filter 可在 recvmsg 之前过滤无关包
- 现代 HFT 更多用 XDP-BPF 替代 socket filter（更早过滤）
- tcpdump 使用 cBPF，在协议栈之后
