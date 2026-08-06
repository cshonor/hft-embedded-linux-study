# 条款 22：成员变量一律声明为 private

## 本节讲什么

封装隔离，控制读写逻辑，方便后续维护、校验、加日志；`protected` 依然破坏封装。

## 示例

```cpp
class AccessDemo {
public:
    void pub_api();
private:
    int data_;  // 数据成员一律 private
};
```
