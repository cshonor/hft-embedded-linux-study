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

---

## 代码自测

**题目 1：** 为什么不应该把成员变量声明为 public？
```cpp
class Point {
public:
    int x, y;  // public 成员
};
```

<details>
<summary>参考答案</summary>

public 成员破坏封装：1) 无法在将来加读写约束（如范围检查）而不影响调用方；2) 无法在不改接口的前提下改实现（如从 int 改为 double）；3) 不一致——函数接口可以改，但数据成员暴露后修改影响面大。应声明为 private，通过 getter/setter 访问。

</details>
