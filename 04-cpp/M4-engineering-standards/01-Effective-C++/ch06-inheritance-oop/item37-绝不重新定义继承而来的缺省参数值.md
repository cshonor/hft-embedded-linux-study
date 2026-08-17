# 条款 37：绝不重新定义继承而来的缺省参数值

## 本节讲什么

编译期绑定参数，运行期绑定函数，极易出现逻辑诡异 bug。

## 示例

```cpp
class Base { public: virtual void print(int x = 10) = 0; };
class Derived : public Base {
public:
    void print(int x = 20) override;  // 不要改缺省参数
};
```

---

## 代码自测

**题目 1：** 以下代码为什么危险？
```cpp
class Base {
public:
    virtual void process(int mode = 0) { /* ... */ }
};
class Derived : public Base {
public:
    void process(int mode = 1) override { /* ... */ }
};
Base* p = new Derived;
p->process();  // mode 的值是？
```

<details>
<summary>参考答案</summary>

`mode` 的值是 `0`（Base 的默认值），不是 `1`。因为默认参数是静态绑定（基于指针的静态类型 Base*），而虚函数调用是动态绑定（调用 Derived::process）。这导致 Derived 的代码假设 `mode=1`，实际收到 `mode=0`——行为不一致。规则：派生类绝不重新定义基类的默认参数值。

</details>
