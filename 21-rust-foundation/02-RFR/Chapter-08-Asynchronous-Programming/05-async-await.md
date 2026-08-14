# 2.1 async/await

> 所属：**Ergonomic Futures** · [← 章索引](./README.md)

手写 `Future` = 手写 **`enum` 状态机** + 跨 `.await` 保存局部 — 易错。

## 语法糖

- **`async fn` / `.await`**：编译期降阶为实现 `Future` 的匿名类型。
- **让出点**：子 `Future` 未就绪 → 返回 `Pending`；局部变量打包进状态结构体字段。

## 性能注意

嵌套 `.await` 使状态结构体**变大**（内嵌子 `Future`）→ 栈压力 / memcpy。

**缓解**：`Box::pin` 把大 `Future` 放堆并固定地址（与 [06 Pin/Unpin](./06-pin-unpin.md) 配合）。
