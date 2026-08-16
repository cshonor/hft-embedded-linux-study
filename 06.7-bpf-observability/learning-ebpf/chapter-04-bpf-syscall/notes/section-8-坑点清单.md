# 坑点清单

1. fd 是进程私有的——跨进程传 fd 数字没有意义（同一 map 两个进程 fd 值不同）
2. bpffs 是内存伪文件系统：**重启后 pin 的程序全部消失**，开机自启要靠 systemd/skeleton 重载
3. BCC 程序 Ctrl+C 程序即卸载；想持久必须 pin 程序（或用 libbpf 的 link）
4. strace 里 `expected_attach_type` 出现网络类值不代表程序是网络程序（默认 0 占位）
5. perf buffer 每核一缓冲，事件跨核乱序；需要时间序就用 ring buffer（5.8+）
6. max_entries=10240 这种"魔法数字"是 BCC 默认，生产要显式设定
