# 条款 30：代理类（Proxy Class）设计模式，解决运算符重载、容器下标等语法痛点

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Proxy {
    std::vector<int> &vec;
    std::size_t idx;
public:
    Proxy(std::vector<int> &v, std::size_t i) : vec(v), idx(i) {}
    int &operator*() { return vec[idx]; }
};
```
