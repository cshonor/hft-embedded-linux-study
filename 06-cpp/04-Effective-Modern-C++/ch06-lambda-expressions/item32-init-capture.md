# Item 32：用初始化捕获将对象移入闭包（C++14）

> 第 6 章 · Item 32 · 上一节：[Item 31 避免默认捕获](item31-avoid-default-capture.md)

## 这节讲什么

初始化捕获（init capture）能在捕获时执行表达式并命名——彻底解决"想捕获移动语义"的需求，C++11 做不到。

---

## 核心用法

```cpp
auto pw = std::make_unique<Widget>();
auto cb = [up = std::move(pw)]{ up->doSomething(); };
// up = std::move(pw) 在闭包里创建 up（按值，即移动），pw 被掏空
```

`[up = std::move(pw)]` 的含义：在闭包里创建 `up`，用 `std::move(pw)` 初始化它。`up` 是闭包成员，类型推导为 `unique_ptr<Widget>`。

C++11 的变通是 `std::bind`，但更绕。

---

## 新手要点

- **C++14 独有**：C++11 没有 init capture，只能用 `bind` 变通。新代码用 C++14 init capture。
- **能 move 进闭包**：这是 init capture 的核心价值——C++11 只能拷贝捕获，不能移动捕获。

---

## HFT 关联

- **移动资源进闭包**：策略对象 `move` 进闭包，避免拷贝大对象。

---

## 自测题

1. `[up = std::move(pw)]` 的语义是什么？
2. 初始化捕获解决了 C++11 的什么限制？
3. C++11 没有 init capture 时怎么变通？

---

## 参考与延伸

- 下一节：[Item 33 泛型 lambda](item33-generic-lambda.md)
- 回到：[第 6 章](README.md)
