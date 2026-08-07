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


### 常见陷阱

1. **演示后忘了清理 probe** — bpftrace Ctrl-C 退出会自动 detach，但 BCC 工具异常退出可能残留 attached probe；用 `bpftool prog list` 检查是否有残留
2. **忽视追 open 的过滤条件** — 全系统追 open 会产生海量事件；应按 PID 或 comm 过滤，否则输出被噪音淹没
3. **只看 open 不看 openat** — 现代 glibc 的 fopen/open 常走 openat 系统调用，仅追 open 会漏掉大量文件访问

<details>
<summary>📝 自测题（点击展开）</summary>

1. **用 bpftrace 追踪 open 系统调用的基本命令是什么？**

   <details>
   <summary>参考答案</summary>

   `sudo bpftrace -e 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\n", comm, str(args->filename)); }'`。注意现代系统用 openat 而非 open，需追 sys_enter_openat。

   </details>

2. **BCC 的 opensnoop 和 bpftrace one-liner 各有什么优劣？**

   <details>
   <summary>参考答案</summary>

   opensnoop 是成熟工具，输出格式固定、有 PID/UID/返回值/文件名，适合标准化排障；bpftrace one-liner 更灵活，可自定义过滤和输出字段，适合快速验证假设。团队标准化用 opensnoop，临时排查用 bpftrace。

   </details>

3. **追踪 open/openat 时如何避免被海量事件淹没？**

   <details>
   <summary>参考答案</summary>

   (1) 按 PID 过滤：`/pid == 12345/`；(2) 按 comm 过滤：`/comm == "myapp"/`；(3) 只看失败：`/args->ret < 0/`；(4) 用 Map 聚合而非逐行打印：`@[comm] = count()` 看谁打开了最多文件。

   </details>

</details>

---
