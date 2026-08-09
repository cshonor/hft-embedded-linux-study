# 条款 29：编写异常安全的代码

## 本节讲什么

满足三点：不泄漏资源、不破坏数据状态；分基础保证、强保证、不抛异常三个安全等级。

## 示例

```cpp
class MutexGuard {
    std::mutex &m;
public:
    MutexGuard(std::mutex &mu) : m(mu) { m.lock(); }
    ~MutexGuard() { m.unlock(); }  // 异常时也能解锁
};
```

---

## 代码自测

**题目 1：** 下面的函数提供什么级别的异常安全保证？
```cpp
void Menu::changeBg(const string& path) {
    lock(mutex);
    delete bg;
    bg = new Background(path);
    unlock(mutex);
}
```

<details>
<summary>参考答案</summary>

不满足任何异常安全保证。如果 `new Background(path)` 抛异常：1) `bg` 已被 delete（资源泄漏的相反问题——悬空指针）；2) mutex 未 unlock（死锁）。基本异常安全版：
```cpp
void Menu::changeBg(const string& path) {
    LockGuard lk(mutex);  // RAII 锁
    Background* p = new Background(path);  // 先 new
    delete bg;
    bg = p;
}
```
这样即使 `new` 抛异常，mutex 仍由 RAII 释放，`bg` 不变。

</details>
