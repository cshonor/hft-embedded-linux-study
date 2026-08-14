# 8 · 不透明结构体 (Opaque structs)

← [本章目录](./README.md) · 上一节：[07-unwind.md](./07-unwind.md) · 下一节：---

C 只给指针、不公开布局 → Rust 用 **`#[repr(C)]` struct + 私有字段 + `PhantomData`**，优于空枚举或裸 `c_void`（类型更安全）。

→ 源码：[src/opaque.rs](./src/opaque.rs)

---

利用 **`repr(C)`、裸指针、`Option` niche、`catch_unwind`** 等机制，在获得底层控制力的同时尽量维护安全边界。

→ 下一章：[10 NoStd](../10_NoStd/README.md)
