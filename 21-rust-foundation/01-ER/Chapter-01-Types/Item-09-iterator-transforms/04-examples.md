# Item 9 · 案例与代码

← [Item 9 目录](./README.md)

### 前 5 个偶数的平方和

```rust
// 演进：iter → filter → take → map → sum
let even_sum_squares: u64 = values
    .iter()
    .filter(|x| *x % 2 == 0)
    .take(5)
    .map(|x| x * x)
    .sum();
```

### `Option` / `Result` 当迭代器 + `flatten`

```rust
let opts = vec![Some(1), None, Some(3)];
let nums: Vec<_> = opts.into_iter().flatten().collect(); // [1, 3]

// Result 同理：flatten 掉 Err（或配合 filter_map）
```

### `Result` 集合一次 collect

```rust
let parsed: Result<Vec<i32>, _> = strings
    .iter()
    .map(|s| s.parse::<i32>())
    .collect();
let vec = parsed?; // 任一 parse 失败则 ? 返回
```

---
