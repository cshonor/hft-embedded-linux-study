# 条款 9：利用析构函数杜绝资源泄漏（RAII 核心思想）

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Guard {
    Resource *r;
public:
    ~Guard() { delete r; }  // RAII：析构释放资源
};
```

---

## 代码自测

**题目 1：** 以下代码在异常发生时如何保证资源释放？
```cpp
void f() {
    auto* p = new Widget;
    doSomething();  // 可能抛异常
    delete p;
}
```

<details>
<summary>参考答案</summary>

如果 `doSomething()` 抛异常，`delete p` 不会执行——泄漏。RAII 解法：用智能指针。
```cpp
void f() {
    auto p = std::make_unique<Widget>();
    doSomething();  // 抛异常时 p 析构自动 delete
}
```
核心思想：把资源放进对象中，利用析构函数保证释放——即使异常传播也能正确清理。

</details>
