# 条款 11：杜绝资源管理函数内部发生异常造成泄漏

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
void acquire() {
    Resource *r = getResource();
    try {
        doWork(r);
    } catch (...) {
        release(r);
        throw;
    }
    release(r);
}
```
