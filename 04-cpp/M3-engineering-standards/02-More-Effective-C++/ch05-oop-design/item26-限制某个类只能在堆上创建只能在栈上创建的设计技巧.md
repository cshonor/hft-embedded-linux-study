# 条款 26：限制某个类只能在堆上创建 / 只能在栈上创建的设计技巧

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class HeapOnly {
public:
    static HeapOnly *create() { return new HeapOnly; }
    void destroy() { delete this; }
private:
    HeapOnly() = default;
    ~HeapOnly() = default;
};
```

---

## 代码自测

**题目 1：** 如何限制一个类只能在堆上创建？如何限制只能在栈上创建？
```cpp
// 只能堆上
class HeapOnly {
public:
    static HeapOnly* create() { return new HeapOnly; }
private:
    HeapOnly() = default;  // 构造私有
};
// 只能栈上
class StackOnly {
public:
    StackOnly() = default;
private:
    static void* operator new(size_t) = delete;  // 禁止 new
};
```

<details>
<summary>参考答案</summary>

只能堆上：将构造函数设为 private，提供静态工厂方法返回 `new` 出的对象。这样栈上声明 `HeapOnly h;` 编译错误。只能栈上：将 `operator new` 设为 delete 或 private，禁止 `new` 表达式。注意：只能堆上的类要处理析构——如果析构也设为 private，需要在工厂方法中返回智能指针。

</details>
