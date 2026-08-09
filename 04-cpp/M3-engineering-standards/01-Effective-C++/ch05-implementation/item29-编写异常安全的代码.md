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
