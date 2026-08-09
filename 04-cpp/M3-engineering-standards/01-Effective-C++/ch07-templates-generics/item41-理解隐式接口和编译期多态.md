# 条款 41：理解隐式接口和编译期多态

## 本节讲什么

模板靠表达式合法与否判定接口（隐式），运行期虚函数是显式接口 + 运行期多态。

## 示例

```cpp
template<typename Iter>
void doAdvance(Iter &it, int n) {
    it += n;  // 隐式接口：Iter 必须支持 +=
}
```

---

## 代码自测

**题目 1：** 模板的「隐式接口」和 OOP 的「显式接口」有什么区别？
```cpp
template<typename T>
void f(T& w) {
    if (w.size() > 10 && w != someWidget) {
        w.doSomething();
    }
}
```

<details>
<summary>参考答案</summary>

OOP 显式接口：类型必须继承自某个基类或实现某个接口，编译期检查签名。
模板隐式接口：`T` 必须支持 `size()` 返回可比较的值、`operator!=`、`doSomething()`——这些是表达式层面的要求，不要求具体签名。只要这些表达式能通过编译，任何类型都可用。隐式接口更灵活但也更难诊断错误。

</details>
