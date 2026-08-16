# uprobe 钩 SSL：看加密流量的明文

### 4.1 思路

TLS 加密后，tcpdump 看到的全是密文。但任何程序最终都要调用 SSL 库收发明文——**在 `SSL_read`/`SSL_write` 上挂 uprobe，明文自然到手**：

- `SSL_write(SSL *ssl, const void *buf, int num)`：entry 时 buf 已是**待发送的明文**，直接读
- `SSL_read` 不同：**entry 时 buf 还是空的**，明文要等函数返回后才填进去——所以必须 entry + retprobe **配对**：

```c
// 全局 map：暂存 "SSL 指针 → 缓冲区指针"
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __type(key,   __u64);            // SSL*（第一参数，两个探针都能看到）
    __type(value, __u64);            // buf 指针（第二参数 PT_REGS_PARM2）
    __uint(max_entries, 1024);
} ssl_read_context SEC(".maps");

SEC("uprobe/SSL_write")
int BPF_KPROBE(ssl_write, const void *ssl, const void *buf, int num) {
    process_SSL_data(ctx, ssl, false, buf, num);   // entry 直接读明文
    return 0;
}

SEC("uprobe/SSL_read")
int BPF_KPROBE(ssl_read_enter, const void *ssl, void *buf) {
    // 只记下 buf 指针，现在还没数据
    bpf_map_update_elem(&ssl_read_context, &ssl, &buf, 0);
    return 0;
}

SEC("uretprobe/SSL_read")
int BPF_KPROBE(ssl_read_exit) {
    __u64 ret = PT_REGS_RC(ctx);          // 返回值 = 实际读取字节数
    // 以 ssl* 为 key 反查 buf 指针，再从 buf 读 ret 字节 → 明文
    ...
    bpf_map_delete_elem(&ssl_read_context, &ssl);
    return 0;
}
```

`process_SSL_data` 是共用的输出函数：从 `buf` 用 `bpf_probe_read_user_bytes` 拷贝到栈/PERFBUF，发给用户态。

### 4.2 uprobe 的四大坑（第 7 章埋的，这里全踩）

1. **架构相关**：`SSL_write` 的 buf 是第二参数，x86 用 `PT_REGS_PARM2`，ARM 上可能不同——`BPF_KPROBE` 宏 + `bpf_trace_printk` 打印各参数实测确认
2. **库不可控**：目标进程用哪个 libssl.so、路径在哪，取决于发行版/编译方式，SEC 名里的库路径要写对（可用 `ldd` 查）
3. **静态链接**：二进制把 SSL 静态链进去就没有 `.so` 可挂——只能对二进制本身的符号挂 uprobe（要求非 strip）
4. **Go < 1.17 栈传参**：旧版 Go 编译的程序参数不走寄存器走栈，`PT_REGS_PARMx` 拿不到——Go 程序要么升级 1.17+，要么改挂系统调用层

### 4.3 更通用的视角

同一模式适用于一切"函数边界即明文边界"的场景：压缩库（zlib 的 `deflate`/`inflate` 前后）、数据库驱动、行情解码函数——**只要符号可见，uprobe 就能看到进出函数的数据**。
