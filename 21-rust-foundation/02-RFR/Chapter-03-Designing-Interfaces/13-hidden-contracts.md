# 4.3 Hidden Contracts（隐藏契约）

> 所属：**Constrained** · [← 章索引](./README.md)

用户未在签名里看到、但 semver 上仍算 **API 承诺** 的隐式约束。

## 重导出 (Re-export)

- 公开 API 中出现第三方类型 → 用户代码可能**依赖该类型路径**。
- 策略：re-export 并文档说明，或 newtype 隐藏 → [ER Item 24](../../01-ER/Chapter-04-Dependencies/Item-24-re-export-api-types/README.md)

## 自动 trait 实现

- **`Send` / `Sync`** 由字段自动推导；改内部字段可能**静默改变**线程语义。
- 用 **`assert_send_sync`** 类测试锁定「公开类型始终 Send + Sync」的意图。

## 布局与 ABI

- 默认 `repr(Rust)` **不保证**布局稳定；FFI 暴露需 `repr(C)` 并文档化 → [RFR 第 2 章 · 布局](../Chapter-02-Types/02-layout.md)

## 测试锁定契约

- 编译期：`const _: fn() = || { fn assert_send<T: Send>() {} assert_send::<MyType>(); };`
- CI 中运行，防止回归破坏隐藏承诺。
