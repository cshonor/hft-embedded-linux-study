# 条款 9：利用析构函数杜绝资源泄漏（RAII 核心思想）

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Guard {
    Resource *r;
public:
    ~Guard() { delete r; }  // RAII：析构释放资源
};
```
