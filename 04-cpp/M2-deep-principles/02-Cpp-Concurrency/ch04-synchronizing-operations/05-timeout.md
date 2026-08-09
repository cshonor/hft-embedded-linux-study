# 4.5 超时等待

> 第 4 章 · 上一节：[4.4 future 的局限](04-future-limits.md) · 下一节：[4.6 C++20 新同步原语](04-cpp20-primitives.md)

## 这节讲什么

`wait_for`/`wait_until` 让等待有超时——避免无限阻塞。

---

## 核心用法

```cpp
if (cv.wait_for(lk, std::chrono::milliseconds(100), [&]{ return ready; }))
    // 条件满足
else
    // 超时

// future 也有超时
if (f.wait_for(std::chrono::seconds(1)) == std::future_status::ready)
    auto v = f.get();
else
    // 超时
```

---

## 新手要点

- **超时等待返回 bool**：`true` = 条件满足，`false` = 超时。
- **带谓词的超时**：`cv.wait_for(lk, timeout, pred)` 结合了谓词检查和超时——最安全的形式。

---

## HFT 关联

- **超时防死等**：HFT 守护进程等待交易所响应用超时，避免网络故障时无限阻塞。

---

## 自测题

1. `wait_for` 和 `wait_until` 的区别是什么？
2. 超时等待返回什么？如何判断是条件满足还是超时？

---

## 参考与延伸

- 下一节：[4.6 C++20 新同步原语](04-cpp20-primitives.md)
- 回到：[第 4 章](README.md)
