# perf buffer 初始化：为什么是每核一个

每颗 CPU 核执行一轮：

```
perf_event_open({PERF_TYPE_SOFTWARE, config=PERF_COUNT_SW_BPF_OUTPUT, ...}, -1, X, ...)
                                        # pid=-1, cpu=X → 测量该核上所有进程
ioctl(Y, PERF_EVENT_IOC_ENABLE)
bpf(BPF_MAP_UPDATE_ELEM, {map_fd=4, ...})   # map 第 X 项指向该核的 perf 缓冲
```

四核机器 = 4 轮 = `PERF_EVENT_ARRAY` 里 4 个条目——"array" 就是每核一缓冲的数组。之后用户态用 `ppoll()` 同时等 4 个 fd，哪个核触发读哪个。
