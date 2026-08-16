# 程序类型决定什么

```
程序类型 ──→ 可附加的事件
        ──→ 上下文参数的结构（每个 eBPF 程序都收到一个 ctx 指针，指向什么取决于事件）
        ──→ 可用的 helper 函数集合（验证器按此检查，第 6 章）
        ──→ 返回码的含义（XDP：放行/丢包/重定向；追踪类：被忽略；LSM：非零=拒绝操作）
```

- 附加类型（attachment type）更细粒度指定挂点；多数程序类型可从程序类型推断附加类型，少数（如 CGROUP_SOCK）必须显式指定（内核 `bpf_prog_load_check_attach`、`bpf/syscall.c` 可查全集）
- helper 函数属于 **UAPI 稳定接口**：一经定义不再变化（即使内核内部函数/结构可以变）
- **kfunc**：把内核内部函数注册给 BPF 子系统供调用。与 helper 不同，**无兼容性保证**，跨内核版本要自己担风险；现有"核心" kfuncs 主要是获取/释放 task 和 cgroup 引用
- `bpftool feature` 可列出当前内核每种程序类型支持的 helper
- 权限：追踪类需要 `CAP_PERFMON + CAP_BPF` 或 `CAP_SYS_ADMIN`；网络类需要 `CAP_NET_ADMIN + CAP_BPF` 或 `CAP_SYS_ADMIN`
