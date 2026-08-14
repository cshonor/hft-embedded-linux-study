# Rustonomicon 读书笔记

> 配套 **[The Rustonomicon](https://doc.rust-lang.org/nomicon/)** — *The Dark Arts of Unsafe Rust*  
> 各章笔记与源码在对应目录下（BYOC 式：一章一夹）。

编译环境：`rustup run nightly`（按需）· Edition **2018**

---

## 全书进度

| 章 | 官方主题 | 笔记 | 源码 |
|:--:|----------|------|------|
| **01** | Meet Safe and Unsafe | [00-overview.md](./01_Safe_Unsafe/00-overview.md) | [01_Safe_Unsafe/](./01_Safe_Unsafe/) |
| **02** | Data Representation | [00-overview.md](./02_Data_Layout/00-overview.md) | [02_Data_Layout/](./02_Data_Layout/) |
| **03** | Ownership and Lifetimes | [00-overview.md](./03_Lifetime_Variance/00-overview.md) | [03_Lifetime_Variance/](./03_Lifetime_Variance/) |
| **04** | Type Conversions | [00-overview.md](./04_Type_Cast/00-overview.md) | [04_Type_Cast/](./04_Type_Cast/) |
| **06** | Uninitialized Memory | [00-overview.md](./05_Uninit_Mem/00-overview.md) | [05_Uninit_Mem/](./05_Uninit_Mem/) |
| **06** | The Perils Of OBRM | [00-overview.md](./06_OBRM_RAII/00-overview.md) · [00.1 RAII/OBRM](./06_OBRM_RAII/00-1-RAII与OBRM辨析.md) | [06_OBRM_RAII/](./06_OBRM_RAII/) |
| **07** | Concurrency and Parallelism | [00-overview.md](./07_Concurrency_Atomic/00-overview.md) | [07_Concurrency_Atomic/](./07_Concurrency_Atomic/) |
| **08** | Implementing Vec / Arc | [00-overview.md](./08_Impl_Vec_Arc/00-overview.md) | [08_Impl_Vec_Arc/](./08_Impl_Vec_Arc/) |
| **09** | FFI | [00-overview.md](./09_FFI/00-overview.md) | [09_FFI/](./09_FFI/) |
| **10** | Beneath std | [00-overview.md](./10_NoStd/00-overview.md) | [10_NoStd/](./10_NoStd/) |
