# 4.1 条件变量 condition_variable

> 第 4 章 同步操作 · 下一节：[4.2 future/promise](02-future-promise.md)

## 这节讲什么

`condition_variable` 让线程等待某个条件成立。`wait` 必须带谓词——虚假唤醒会让无谓词版本失效。

---

## 核心用法

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

// 等待方
std::unique_lock<std::mutex> lk(m);
cv.wait(lk, [&]{ return ready; });   // 虚假唤醒自动重检 lambda

// 通知方
{
    std::lock_guard<std::mutex> lk(m);
    ready = true;
}
cv.notify_one();   // 或 notify_all()
```

关键点：
- `wait` 必须带**谓词**形式 `cv.wait(lk, pred)`，否则虚假唤醒会让条件判断失效
- 通知方改变共享状态必须在**锁内**完成
- `notify_one` 唤醒一个，`notify_all` 唤醒全部

---

## 新手要点

- **虚假唤醒**：`wait` 可能在条件不满足时被唤醒——这是允许的行为。带谓词版本自动重检，无谓词版本需要手动 while 循环。
- **永远用谓词形式**：`cv.wait(lk, [&]{ return ready; })` 比 `while(!ready) cv.wait(lk);` 简洁且等价。

---

## HFT 关联

- **热路径用自旋替代 cv**：`condition_variable` 会 park 线程（让出 CPU），适合非热路径。HFT 热路径用 `atomic` + 自旋（`yield`/`pause`）避免上下文切换。

---

## 自测题

1. 为什么 `condition_variable::wait` 必须用谓词形式？虚假唤醒是什么？
2. 通知方改变共享状态为什么必须在锁内完成？
3. `notify_one` 和 `notify_all` 的区别是什么？

---

## 参考与延伸

- 下一节：[4.2 future/promise](02-future-promise.md)
- 回到：[第 4 章](README.md)
