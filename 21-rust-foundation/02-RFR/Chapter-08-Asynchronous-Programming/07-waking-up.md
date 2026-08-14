# 3.1 Waking Up（唤醒）

> 所属：**Going to Sleep** · [← 章索引](./README.md)

`Pending` 后若**忙轮询**，CPU 烧干。

## Reactor + Waker

- I/O 层（`epoll` / `kqueue` / IOCP）监听就绪事件。
- 就绪时调用 **`Waker::wake()`**，通知执行器「某任务可再 `poll`」。

## `Context`

`poll(cx, …)` 中的 `cx` 提供注册 waker 的路径；子 `Future` 把 waker 传给底层 I/O。

→ [09 唤醒的误称](./09-waking-misnomer.md)
