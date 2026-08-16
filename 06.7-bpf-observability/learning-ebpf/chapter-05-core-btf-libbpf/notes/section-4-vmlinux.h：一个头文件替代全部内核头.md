# vmlinux.h：一个头文件替代全部内核头

```
bpftool btf dump file /sys/kernel/btf/vmlinux format c > vmlinux.h
```

- 包含**当前运行内核**的全部数据类型定义，`.bpf.c` 里 include 它即可，不用再翻内核源码找头文件
- **不含 `#define` 常量**！比如以太网协议号 `0x0800`（IP）/`0x0806`（ARP）在 `if_ether.h` 里，vmlinux.h 没有这些值，要么自己抄要么单独 include（第 8 章会踩到）
- 5.4+ 内核自带 `/sys/kernel/btf/vmlinux`；老内核可从 **BTFHub** 拿到各发行版预生成的 BTF 文件
