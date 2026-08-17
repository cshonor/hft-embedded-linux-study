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

---

## 代码自测

**题目 1：** 「面向未来编程」是什么意思？以下代码有什么未来隐患？
```cpp
class Stack {
    int data[100];  // 固定大小
    int top = 0;
public:
    void push(int x) { data[top++] = x; }  // 不检查越界
};
```

<details>
<summary>参考答案</summary>

面向未来编程：代码应易于扩展、不因环境变化而崩溃。隐患：1) 固定大小——未来数据量增大时溢出；2) 无越界检查——未来调用方可能 push 超过 100 个元素；3) 类型固定为 int——未来需要存其他类型需重写。改进：用 `std::vector`（动态扩展）、加边界检查或 `assert`、考虑模板化。未来兼容也包括：不要硬编码魔法数字、提供扩展点（虚函数/策略）、写好文档。

</details>
