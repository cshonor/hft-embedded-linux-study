# 3.5 读写锁 std::shared_mutex（C++17）

> 第 3 章 · 上一节：[3.4 初始化保护](04-init-protection.md) · 下一章：[第 4 章 同步操作](../ch04-synchronizing-operations/README.md)

## 这节讲什么

多读少写场景用 `shared_mutex`：读用 `shared_lock`（多读者并发），写用 `unique_lock`（独占）。

---

## 核心用法

```cpp
std::shared_mutex mtx;

// 读操作：多读者并发
{
    std::shared_lock lk(mtx);   // 共享锁
    // 读数据...
}

// 写操作：独占
{
    std::unique_lock lk(mtx);   // 独占锁
    // 写数据...
}
```

`shared_mutex` 允许多个读者同时持有锁，但写者独占——适合"读多写少"的场景。

---

## 新手要点

- **`shared_mutex` 不是银弹**：读少写多时性能不如普通 `mutex`（管理共享锁有额外开销）。
- **适用场景**：配置缓存（多线程读、偶尔更新）、行情快照（多策略读、单线程写）。

---

## HFT 关联

- **行情快照**：多策略读、单线程写用 `shared_mutex`，读不互斥——策略并行读快照不互相阻塞。

---

## 自测题

1. `shared_mutex` 的读写锁如何工作？
2. 什么场景适合用 `shared_mutex`？什么场景不适合？
3. `shared_lock` 和 `unique_lock` 分别对应什么操作？

---

## 参考与延伸

- 下一章：[第 4 章 同步操作](../ch04-synchronizing-operations/README.md)
- 回到：[第 3 章](README.md)
