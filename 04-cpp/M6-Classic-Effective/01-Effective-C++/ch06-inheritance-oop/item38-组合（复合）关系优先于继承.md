# 条款 38：组合（复合）关系优先于继承

## 本节讲什么

继承强耦合；组合弱耦合，复用实现而非接口，优先 has-a 组合，少用 is-a 继承。

## 示例

```cpp
class Engine { /* ... */ };
class Car {
    Engine engine_;  // 组合优于继承
public:
    void start() { engine_.ignite(); }
};
```

---

## 代码自测

**题目 1：** 以下关系应该用继承还是组合？
```cpp
// 场景：一个 OrderBook 内部使用 std::list 来存储订单
class OrderBook : public std::list<Order> {};  // 继承
class OrderBook {
    std::list<Order> orders;  // 组合
};
```

<details>
<summary>参考答案</summary>

应该用组合。OrderBook 和 list 不是 is-a 关系（OrderBook 不是一种 list），而是 has-a / is-implemented-in-terms-of 关系。组合意味着 OrderBook 复用 list 的实现但不受其接口约束。用继承会暴露 list 的全部 public 接口，破坏封装。

</details>
