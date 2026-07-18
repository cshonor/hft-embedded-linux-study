# 条款 10：构造函数抛出异常时，如何防止内存/资源泄漏

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Widget {
    int *data;
public:
    Widget() : data(new int[100]) {
        if (fail()) throw std::runtime_error("ctor failed");
        // 已分配内存需在 catch 或成员析构中清理
    }
    ~Widget() { delete[] data; }
};
```
