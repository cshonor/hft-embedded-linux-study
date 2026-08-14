# 08 · Implementing Vec and Arc

> **The Rustonomicon** · [03 Rust Nomicon](../README.md) · [全书笔记](../notes.md)

## 状态

- [x] Vec 已读（笔记 + `MyVec` 实现）
- [x] Arc 已读（笔记 + `MyArc` 实现，无 Weak）
- [ ] Mutex（Nomicon 同章延伸，待建）

---

## 一句话

**Vec + Arc 实战** — stable 徒手 `MyVec` 与简化 `MyArc`：NonNull/PhantomData、原子 refcount、Release/Acquire fence。

---

## 专项笔记

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| 1 | 数据布局 | [01-layout.md](./01-layout.md) |
| 2 | 内存分配 | [02-allocating.md](./02-allocating.md) |
| 3 | Push 与 Pop | [03-push-pop.md](./03-push-pop.md) |
| 4 | 内存释放 | [04-dealloc.md](./04-dealloc.md) |
| 5 | 切片解引用 | [05-deref.md](./05-deref.md) |
| 6 | Insert 与 Remove | [06-insert-remove.md](./06-insert-remove.md) |
| 7 | IntoIter / Drain / RawVec | [07-iterators.md](./07-iterators.md) |
| 8 | 零大小类型 | [08-zst.md](./08-zst.md) |
| — | Arc 实现要点 | [01-arc-overview.md](./01-arc-overview.md) |
| — | 速记 · 自测 |

---

## 示例源码

| 文件 | 演示 |
|------|------|
| [src/my_vec.rs](./src/my_vec.rs) | Vec：push/pop/Deref/… |
| [src/my_arc.rs](./src/my_arc.rs) | Arc：Clone Relaxed / Drop Release+Acquire |
| [src/raw_vec.rs](./src/raw_vec.rs) | RawVec 分配 |
| [src/into_iter.rs](./src/into_iter.rs) | IntoIter |
| [src/drain.rs](./src/drain.rs) | Drain |
| [src/zst.rs](./src/zst.rs) | ZST 辅助 |
| [src/main.rs](./src/main.rs) | 运行入口 |

```bash
cd 04-Rust-Nomicon/08_Impl_Vec_Arc
cargo run
cargo test
```

---

## 与仓库其他部分

| 主题 | 对照 |
|------|------|
| 原子内存序 | [07_Concurrency_Atomic](../07_Concurrency_Atomic/README.md) |
| forget / Rc | [06_OBRM_RAII](../06_OBRM_RAII/README.md) |
| 上一章 | [07_Concurrency_Atomic](../07_Concurrency_Atomic/README.md) |
| 下一章 | [09_FFI](../09_FFI/README.md) |

---

## 逻辑脉络

MyVec 布局 → 分配 → push/pop → dealloc → deref → insert/remove → 迭代器/Drain → ZST → MyArc →（待）Mutex → FFI。

---

## 速记

## 三句背诵

1. **MyVec = NonNull + cap + len；push 用 `ptr::write`，pop 用 `ptr::read`，勿对未初始化槽位 Drop/move。**
2. **Drain 初始化时 `len = 0`；RawVec 复用分配逻辑；ZST 不 alloc、`cap = usize::MAX`。**
3. **MyArc = NonNull + PhantomData；Clone Relaxed；Drop Release + Acquire fence；防 forget 溢出 abort。**

## 自测

- [ ] 能画出 MyVec 三字段并说明为何需 `unsafe impl Send/Sync`
- [ ] 能解释 push 为何不能用 `*ptr = x`
- [ ] 能说明 Drain 为何先把 `len` 置 0
- [ ] 能列出 ZST 的四条特殊规则
- [ ] 能解释 Arc Drop 为何 Release 后还要 Acquire fence
- [ ] 能对照 [src/my_vec.rs](./src/my_vec.rs) 与 [src/my_arc.rs](./src/my_arc.rs) 说出关键 API

## 术语表（本章）

| 术语 | 含义 |
|------|------|
| RawVec | 仅 ptr + cap；Vec/Drain 共享分配逻辑 |
| ptr::write/read | 对未初始化/待销毁槽位的正确读写 |
| Release/Acquire | Drop 侧同步其它线程对 `T` 的访问 |
| ZST | size=0；alloc(0) UB，偏移 no-op |

## 源码索引

| 文件 | 演示 |
|------|------|
| [src/my_vec.rs](./src/my_vec.rs) | Vec 主体 |
| [src/raw_vec.rs](./src/raw_vec.rs) | 分配 / ZST |
| [src/into_iter.rs](./src/into_iter.rs) | IntoIter |
| [src/drain.rs](./src/drain.rs) | Drain |
| [src/zst.rs](./src/zst.rs) | ZST 辅助 |
| [src/my_arc.rs](./src/my_arc.rs) | Arc refcount / 屏障 |

