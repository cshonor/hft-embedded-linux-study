# 条款 21：按需把函数声明为虚函数，不要盲目虚函数

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Base {
public:
    virtual void interface() = 0;
    void helper() { /* 非虚即可 */ }
};
```

---

## 代码自测

**题目 1：** 什么情况下应该用虚函数？什么情况下不应该？
```cpp
// 场景A：日志输出格式可扩展
class Logger {
    virtual void format(string& msg);  // 派生类可覆盖
};
// 场景B：简单的数据容器
class Point {
    virtual int getX() const { return x; }  // 需要虚吗？
};
```

<details>
<summary>参考答案</summary>

场景A：合理用虚函数——格式化逻辑需要运行时扩展。场景B：不需要虚函数——getter 只是访问内部数据，不会被覆盖。虚函数的开销（vtable + 间接调用 + 阻止内联）对小函数影响大。判断标准：行为是否真的需要在运行时根据实际类型变化？如果不，用非虚函数或模板。

</details>
