# 条款 32：面向未来做程序设计（兼容扩展、向后兼容设计思想）

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Plugin {
public:
    virtual ~Plugin() = default;
    virtual int version() const = 0;  // 预留扩展点
};
```
