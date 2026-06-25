# 4. BPF 相对传统工具的优势

| 传统 | BPF |
|------|-----|
| `ps` 看 RSS 总量 | `memleak` 看 **未释放分配栈** |
| `sar` 看 fault 率 | `faults` 看 **哪段代码 fault** |
| `vmstat` 见 swap | `swapin` 看 **哪个进程** 换入 |
| 猜 kswapd 忙 | `vmscan`、`drsnoop` **量化回收延迟** |

---
