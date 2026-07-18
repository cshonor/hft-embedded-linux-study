# 条款 39：谨慎使用私有继承

## 本节讲什么

私有继承代表「复用实现」，不是 is-a；能用组合就不用私有继承。

## 示例

```cpp
class Engine { void tune(); };
class Car : private Engine {};  // 谨慎：实现继承，不是 is-a
```
