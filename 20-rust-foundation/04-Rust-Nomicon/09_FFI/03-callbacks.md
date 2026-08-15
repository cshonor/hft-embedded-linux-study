# 3 · 回调 (Callbacks)

← [本章目录](./README.md) · 上一节：[02-export-to-c.md](./02-export-to-c.md) · 下一节：[04-interop.md](./04-interop.md)

---

| 模式 | 说明 |
|------|------|
| 全局回调 | `extern "C" fn` 直接传给 C |
| 带状态 | `*mut RustObject` 传入，回调内 `unsafe` 转回 |
| 异步/异线程 | **极危险** — 须 `Mutex`/通道，销毁前**注销**回调 |

→ 源码：[src/callbacks.rs](./src/callbacks.rs)
