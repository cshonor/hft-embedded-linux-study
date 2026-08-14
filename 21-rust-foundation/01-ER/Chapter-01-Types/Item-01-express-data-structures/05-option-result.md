# Item 1 · 05 Option 与 Result

← [Item 1 目录](./README.md)

## 内建枚举

| 类型 | 变体 | 语义 | 替代什么 |
|------|------|------|----------|
| **`Option<T>`** | `Some(T)` / `None` | 值可能存在或缺失 | **null**、空指针 |
| **`Result<T, E>`** | `Ok(T)` / `Err(E)` | 成功或失败 | **异常**（可恢复错误） |

## 四条实用原则

1. **缺失值用 `Option<T>`**——不用 `-1`、`nullptr` 等哨兵。
2. **可失败操作用 `Result<T, E>`**——不用魔法返回值或全局错误码。
3. **库 API 有诊断价值时保留 `Result`**——别偷换成 `Option`（Item 3 展开）。
4. **与 `?`、转换方法配合**——见 [Item 3](../Item-03-option-result-transforms/README.md)、[Item 4](../Item-04-idiomatic-error-types/README.md)。

---

## `Result` 和 `Option` 怎么分工？

两者**不是二选一**，而是回答**两个不同的问题**：

| 类型 | 问的问题 | 两个答案 |
|------|----------|----------|
| **`Result<T, E>`** | 这次操作**成不成功**？ | `Ok(T)` 成功 · `Err(E)` 失败（可恢复错误） |
| **`Option<T>`** | 这个值**在不在**？ | `Some(T)` 在 · `None` 不在（不是错误，只是缺失） |

### ⚠️ 先纠正一个极易说反的误区

有人会把 `Option` 说成「报文回没回来」——**这是错的**：

| ❌ 错误说法 | ✅ 正确归属 |
|------------|------------|
| `None` = 报文没回来、超时、断连 | **`Result::Err`** —— 通信 / 解析失败 |
| `Some` = 报文回来了 | **`Result::Ok`** —— 已成功拿到并解析合法响应 |
| `None` = 报文回来了但没有某字段 | **`Ok(None)`** —— 在**成功**前提下，字段不存在 |
| `Some(vec![])` = 报文回来了、payload 空数组 | **`Ok(Some(vec![]))`** —— 字段存在，内容为空 |

**记忆口诀**：

```text
Result  outer → 「链路通不通 / 解析过不过」
Option  inner → 「成功之后，这个字段在不在」（仅当需要区分存在性时）
Vec     inside → 「在的话，有几条」
```

### 报文例子：到底用 `Option` 还是 `Result`？

**都要用**——但包在不同层：

```rust
// 拉取盘口档位
Result<Option<Vec<OrderBookLevel>>, NetworkError>
```

**处理顺序**（HFT 里建议严格按层 `match`，别混）：

```rust
match fetch_order_book(symbol) {
    Err(e) => {
        // ① Result 层：网络 / 解析 / 权限级错误 → 重连、告警、熔断
        log::error!("request failed: {e}");
    }
    Ok(None) => {
        // ② Option 层：请求成功，但该品种/用户「无此数据字段」
        show_no_data_for_symbol();
    }
    Ok(Some(levels)) if levels.is_empty() => {
        // ③ Vec 层：字段在，但当前 0 档挂单
        show_empty_book();
    }
    Ok(Some(levels)) => {
        // ③ Vec 层：有有效档位
        render_book(levels);
    }
}
```

| 只用 `Result` 够吗？ | 只用 `Option` 够吗？ |
|---------------------|---------------------|
| 若 API **永远**返回列表字段（最多 `[]`）→ `Result<Vec<T>, E>` 即可 | **不够**——`Option` 无法携带错误原因，超时 / 断网只能变 `None`，和「字段缺失」混在一起 |
| 若还要区分「字段缺失」vs「字段为空」→ 内层加 `Option` | 网络失败必须用 **`Result::Err`** |

### 和「Option 区分空跟无」的关系

- **`Option`**：在**已经成功**的前提下，区分 **无（不存在）** vs **有（Some）**；有之后若类型是 `Vec`，再区分 **空集合** vs **非空**。
- **`Result`**：区分 **对（Ok）** vs **错（Err）**——错的是整个操作（没拿到可用响应），不是「某个字段缺了」。

所以：**`Result` 管成败，`Option` 管存在性**——报文场景里，`Err` 才是「报文没回来」，`Ok(None)` 是「报文回来了，但没有你要的那个字段」。

---

## `Result` 与 `Error` trait（预告）

`Result<T, E>` 里的 **`E` 不强制**实现 `std::error::Error`，但自定义错误类型**惯例**应实现它，以便：

- 用 `{}`（`Display`）给用户看、用 `{:?}`（`Debug`）给开发者调试
- 通过 `source()` 关联底层原因（错误链）
- 与 `?`、`From`、日志库、`anyhow` 等生态对齐

| 谁实现了 `Error`？ | 说明 |
|--------------------|------|
| 标准库 | 如 `std::io::Error`、`ParseIntError` 等已默认实现 |
| 自定义类型 | 手写 `impl Error`，或用 **`thiserror`** 自动派生（见 [Item 4](../Item-04-idiomatic-error-types/README.md)） |

高频交易里：行情查询缺数据 → `Option`；连接超时、下单被拒 → `Result<HftError>`，把精力放在**错误怎么处理**上，而不是手写 `Display` / `Error` 样板。

---

## `Option<Vec<T>>`：None 不是「空集合」

### 先纠正一个常见误区

| ❌ 错误理解 | ✅ 正确理解 |
|------------|------------|
| `None` = 空集合 | `None` = **字段 / 集合本身不存在**（未提供、缺失） |
| `Some` 里必须有数据 | `Some(vec![])` = **集合存在，但里面 0 条** |
| `Option` 用来替代空 `Vec` | `Option` 区分 **存在性**；空 `Vec` 表达 **有无元素** |

`None` 在业务上是「**无这个字段 / 无这类数据**」，不是底层「指针为空」的实现细节——业务代码按语义用，不必纠结堆分配。

### 三层分工：`Result` + `Option` + `Vec`

场景：**拉取某合约今日成交列表**。

标准写法：

```rust
Result<Option<Vec<Trade>>, ApiErr>
```

| 层 | 管什么 | 变体 | HFT 含义 |
|----|--------|------|----------|
| **`Result`** | 请求 / 通信成不成功 | `Err(e)` | 断网、超时、JSON 解析失败 → 重试 / 告警 |
| | | `Ok(...)` | 拿到合法响应，进入内层 |
| **`Option`** | 目标字段**存不存在** | `None` | 响应合法，但**根本没有** `trade_list` 字段（或业务码表示「无此数据」） |
| | | `Some(vec)` | 响应里**明确给了**列表字段 |
| **`Vec`** | 列表里**有几条** | `vec![]` | `{"trade_list":[]}` → 拉取成功，今日无成交 |
| | | 非空 | 有成交记录 |

### 四个状态，四个报文例子

**A. 网络 / 请求失败** → `Err(...)`

```json
// 超时、断连，没拿到正常报文
```

**B. 成功，但字段不存在** → `Ok(None)`

```json
{ "code": 0, "msg": "该合约暂无挂单列表" }
// 整个报文里没有 order_list 字段
```

**C. 成功，字段存在、列表为空** → `Ok(Some(vec![]))`

```json
{ "code": 0, "trade_list": [] }
```

**D. 成功，列表有数据** → `Ok(Some(trades))`

```json
{ "code": 0, "trade_list": [{ "price": 100, "qty": 10 }] }
```

B 和 C 在 UI 上可能都显示「当前无行情」，但**业务语义不同**：

- `None`：这类数据**对你不可用 / 未提供**（权限、品种不支持、字段缺失）
- `Some(vec![])`：数据通道正常，就是**今天确实 0 笔**

### 为什么有了 `Result`，还需要 `Option::None`？

**网络成功 ≠ 业务上「存在这个列表」。**

- `Result::Err`：连合法响应都没拿到
- `Ok(None)`：响应拿到了，但**目标字段不存在**
- `Ok(Some(vec![]))`：字段存在，内容为空

三者**不重叠**，各司其职。

### 什么时候只用 `Vec`，什么时候必须套 `Option`？

| API 约定 | 推荐类型 |
|----------|----------|
| 字段**一定会出现**，最多是 `[]` | `Result<Vec<T>, E>` —— 空数组 = 没数据，够用 |
| 字段**可能有、可能完全没有** | `Result<Option<Vec<T>>, E>` —— 必须分开「不存在」和「存在但空」 |

权限错误、业务错误码应走 **`Result::Err`**，不要塞进 `Option::None`——`None` 只表示「成功响应里**没有**这个可选字段」，不是「出错了」。

### 一句话背下来

```text
Result::Err     → 请求炸了
Ok(None)        → 成功，但字段 / 集合不存在
Ok(Some([]))    → 成功，集合存在，里面空的
Ok(Some(有数据)) → 成功，有内容
```

## 相关

- Book 深入 → [9.2 Result](../../../00-Book/09-error-handling/9.2-Result-与可恢复的错误.md)
- `Option` 嵌套陷阱 → [07-pitfalls.md](./07-pitfalls.md)
