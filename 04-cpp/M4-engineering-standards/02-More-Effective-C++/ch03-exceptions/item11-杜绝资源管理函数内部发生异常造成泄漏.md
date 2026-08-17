# 条款 11：杜绝资源管理函数内部发生异常造成泄漏

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void acquire() {
    Resource *r = getResource();
    try {
        doWork(r);
    } catch (...) {
        release(r);
        throw;
    }
    release(r);
}
```

---

## 代码自测

**题目 1：** 以下代码如果 `processB()` 抛异常，`p` 会泄漏吗？
```cpp
void f() {
    auto p = std::make_unique<Widget>();
    processB(p.get());  // 可能抛异常
}
```

<details>
<summary>参考答案</summary>

不会泄漏。`p` 是 `unique_ptr`，函数栈展开时 `p` 析构自动释放。但如果 `processB` 内部接管了 `p.get()` 的所有权（比如存入容器），`p` 析构后那块内存仍会被使用——双重释放或悬空指针。核心原则：资源管理函数内部如果抛异常，不能让部分资源处于「已转移但未完成」状态。

</details>
