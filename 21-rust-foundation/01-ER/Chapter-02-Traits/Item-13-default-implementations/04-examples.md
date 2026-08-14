# Item 13 · 案例与代码

← [Item 13 目录](./README.md)

### `ExactSizeIterator::is_empty`

```rust
trait ExactSizeIterator: Iterator {
    fn len(&self) -> usize; // 必需

    fn is_empty(&self) -> bool {
        self.len() == 0     // 默认实现
    }
}
```

### `Iterator`：只写 `next()`

```rust
trait Iterator {
    type Item;
    fn next(&mut self) -> Option<Self::Item>; // 唯一必需

    // map, filter, fold, collect, ... 均有 default
}
```

### 带 bound 的默认方法：`cloned`

```rust
fn cloned<'a, T>(self) -> Cloned<Self>
where
    T: 'a + Clone,
    Self: Sized + Iterator<Item = &'a T>,
{
    Cloned::new(self)
}
```

仅当 `Item` 为 `&T` 且 `T: Clone` 时，调用方才能用 `.cloned()`。

---
