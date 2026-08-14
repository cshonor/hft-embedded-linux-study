# 09 · Foreign Function Interface · 本章定位

← [本章目录](./README.md) · [全书笔记](../notes.md) · 上一章：[08 Vec/Arc](../08_Impl_Vec_Arc/README.md) · 下一章：[10 NoStd](../10_NoStd/README.md)

---

官方标题 **Foreign Function Interface (FFI)**。探讨 Rust 与外部环境（主要为 **C**）如何安全、高效互调。C 是系统编程的「通用语」，本章是打破语言壁垒的**避坑指南**。

| 对照 | 路径 |
|------|------|
| RFR FFI 章 | [Ch11 FFI](../../02-RFR/Chapter-11-Foreign-Function-Interfaces/README.md) |
| ER Item 34 | [ffi-boundaries](../../01-ER/Chapter-06-Beyond-Standard-Rust/Item-34-ffi-boundaries/README.md) |
| repr(C) | [02_Data_Layout](../02_Data_Layout/00-overview.md) |
| catch_unwind | [Book 9.1.3](../../00-Book/09-error-handling/9.1.3-catch_unwind与panic模式.md) |

**读完应能回答**：FFI 边界为何 unsafe、panic 为何不能跨界、如何用 `repr(C)`/`CString`/`Option` niche 安全互操作。

---

## 小节路线图

```text
01  Rust → C（extern + link）
  ↓
02  C → Rust（cdylib + no_mangle）
  ↓
03  回调与 *mut 状态
  ↓
04  repr(C) / CString
  ↓
05  外部 static mut
  ↓
06  Option niche
  ↓
07  panic / catch_unwind
  ↓
08  opaque struct
  ↓
10 no_std
```

| 节 | 主题 | 阅读 |
|:--:|------|------|
| — | 本章定位 | 本页 |
| 1 | 从 Rust 调用 C | [01-call-c.md](./01-call-c.md) |
| 2 | 从 C 调用 Rust | [02-export-to-c.md](./02-export-to-c.md) |
| 3 | 回调 | [03-callbacks.md](./03-callbacks.md) |
| 4 | 互操作与数据表示 | [04-interop.md](./04-interop.md) |
| 5 | 外部全局变量 | [05-globals.md](./05-globals.md) |
| 6 | 可空指针优化 | [06-nullable.md](./06-nullable.md) |
| 7 | 异常与栈展开 | [07-unwind.md](./07-unwind.md) |
| 8 | 不透明结构体 | [08-opaque.md](./08-opaque.md) |
| — | 速记 · 自测 |

---

## 一句话

**FFI 避坑章** — Rust↔C 互调、`repr(C)`/`CString`、回调与全局变量风险、Option niche、panic 不可跨界、`catch_unwind`、opaque struct。

→ 从 [01-call-c.md](./01-call-c.md) 起读。
