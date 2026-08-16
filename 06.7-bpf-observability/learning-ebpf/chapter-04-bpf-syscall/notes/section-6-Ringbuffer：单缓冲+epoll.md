# Ring buffer：单缓冲 + epoll

`BPF_MAP_CREATE` 一条搞定（`key_size=0, value_size=0, max_entries=4096`），无每核 setup。优点：性能更好 + **跨核提交保序**（perf buffer 各核独立、顺序无保证）。

等待机制从 ppoll 升级为 epoll：

```
epoll_create1(EPOLL_CLOEXEC)                       = 8   # 内核里建一个 epoll 实例
epoll_ctl(8, EPOLL_CTL_ADD, 4, {EPOLLIN})          = 0   # 把 ringbuf fd 4 加入集合
epoll_pwait(8, ...)                                 # 阻塞等数据
```

ppoll 每次返回都要重新传整个 fd 集合；epoll 的 fd 集合由**内核对象**持有，注册一次即可。（与 03.5/04-cpp 网络编程模块里 poll vs epoll 的对比完全同源。）
