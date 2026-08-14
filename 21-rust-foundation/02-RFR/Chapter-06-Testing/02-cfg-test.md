# 1.2 `#[cfg(test)]`

> 所属：**Rust Testing Mechanisms** · [← 章索引](./README.md)

不仅「不把测试编进 release」，更是**隔离与可观测性**。

## 常见用法

| 用途 | 说明 |
|------|------|
| **Mock / 替身** | 测试下替换网络、DB（可配合 `mockall`） |
| **测试专用 API** | `#[cfg(test)] pub fn inspect_invariant()` 不污染公开 API |
| **测试记账** | 仅测试配置下增加计数字段 |

## 与模块组织

```rust
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn it_works() { ... }
}
```

Book → [11.1.1 cfg test 与模拟输入](../../00-Book/11-testing/11.1.1-cfg-test与模拟输入.md)
