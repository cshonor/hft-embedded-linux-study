# 2.2 Pin and Unpin

> 所属：**Ergonomic Futures** · [← 章索引](./README.md)

## 自引用

`async` 体内可能出现「某字段引用另一字段」；`.await` 后布局须跨 `poll` 有效。

## 移动 = 灾难

若 `Future` 整体 **move**，内嵌指针仍指向**旧地址** → UB。

## `Pin<P<Pointer<T>>>`

对 **`!Unpin`** 的 `T`：约束「`poll` 期间不非法移动 `T`」（精确规则见官方文档）。

## `Unpin`

类型**不依赖**地址不变 → 自动/可标为 `Unpin`；`Pin` 约束大多无负担。

## 常见报错

`Future` 不是 `Unpin` → 需 **`Box::pin`** 或运行时提供的 pinned spawn API。

→ [第 9 章 · Unpin 与 unsafe](../Chapter-09-Unsafe-Code/04-unsafe-traits.md)
