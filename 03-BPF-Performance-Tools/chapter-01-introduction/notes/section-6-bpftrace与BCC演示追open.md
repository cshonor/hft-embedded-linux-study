# 6. bpftrace 与 BCC 演示 · 追 `open()`

### bpftrace — 单行

```bash
sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'
```

**特点：** 语法短，适合 **5 分钟验证假设**。

### BCC — opensnoop

```bash
sudo opensnoop-bpfcc
```

**特点：** 列格式化输出、过滤、错误码 — **可脚本化、可给 SRE runbook**。

**共同目标：** 捕获 **文件打开** — 查配置读失败、权限、错误路径（「软件行为异常但无 crash」类问题）。

**HFT：** 查策略是否误读大文件、NFS 配置、证书路径 — 与 strace 相比 **生产开销更可控**。

---
