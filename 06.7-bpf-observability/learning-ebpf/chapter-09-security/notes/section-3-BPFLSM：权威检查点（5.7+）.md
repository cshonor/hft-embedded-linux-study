# BPF LSM：权威检查点（5.7+）

- LSM（Linux Security Module）接口提供**数百个钩子**，每个都在"内核即将对内核数据结构操作之前"触发——此时参数已拷入内核内存，**不存在 TOCTOU**
- 钩子与 syscall 无一一映射，但任何安全敏感 syscall 都会触发一个或多个钩子
- `BPF_LSM` 程序类型让 eBPF 挂上这些钩子（第 7 章：返回非零 = **拒绝操作**）：

```c
SEC("lsm/path_chmod")
int BPF_PROG(path_chmod, const struct path *path, umode_t mode)
{
    bpf_printk("Change mode of file name %s\n", path->dentry->d_iname);
    return 0;   // 非零 → 拒绝本次 chmod
}
```

- 参数是内核数据结构（`path->dentry->d_iname` 直接是文件名）——策略判断**完全在内核内完成，高性能**
- 限制：需要 5.7+ 内核且开启 `CONFIG_BPF_LSM`（boot 参数 `lsm=bpf`），成书时多数发行版还没普及
