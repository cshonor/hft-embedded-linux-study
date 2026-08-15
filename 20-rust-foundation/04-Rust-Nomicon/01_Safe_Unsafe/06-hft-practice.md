# 六 · 实操落地规范（Rust + HFT）

← [本章目录](./README.md) · 上一节：[05-faq.md](./05-faq.md) · 下一节：[07-raw-pointers.md](./07-raw-pointers.md)

---

1. **业务逻辑 100% 使用 Safe Rust**，禁止随意引入 unsafe；
2. **所有 unsafe 操作统一收敛在独立底层模块**，上层业务不直接接触裸指针、FFI；
3. **每一个 `unsafe fn`、`unsafe impl` 必须在文档写明完整前置契约**；
4. **使用模块私有性封装内部不安全状态**，杜绝外部篡改不变量；
5. **能封装为 Safe 接口的底层逻辑，绝不对外暴露 unsafe 函数**；
6. **5 种不安全操作分开记忆**，开发时逐条核对，避免遗漏安全校验。

## 与后续章节衔接

| 场景 | 后续 Nomicon 章 |
|------|-----------------|
| FFI / `catch_unwind` | [09_FFI](../09_FFI/README.md) |
| `Send`/`Sync` / 原子 | [07_Concurrency](../07_Concurrency_Atomic/README.md) |
| `repr(C)` / 布局 | [02_Data_Layout](../02_Data_Layout/README.md) |
| 自定义 `Vec`/分配器 | [08_Impl_Vec_Arc](../08_Impl_Vec_Arc/README.md) |
| `no_std` 裸机 | [10_NoStd](../10_NoStd/README.md) |

## 下一章

→ [02 Data Layout](../02_Data_Layout/README.md) · 数据在内存中如何表示
