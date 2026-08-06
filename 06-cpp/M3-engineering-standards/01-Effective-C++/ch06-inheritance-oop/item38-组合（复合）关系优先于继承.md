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
