# 7.2 异常处理实现

> 第 7 章 · 上一节：[7.1 模板实例化](01-template-instantiation.md) · 下一节：[7.3 模板+异常交互](03-template-exception.md)

## 这节讲什么

现代 C++ 用 table-based EH：编译器为每个函数生成异常表（.gcc_except_table），记录哪些区间可被哪些 catch 捕获。正常路径零开销，异常路径昂贵。

---

## Table-based EH

```
正常执行：无额外指令（异常表在只读段）
    ↓ 抛异常
查 .gcc_except_table → 找到当前 PC 对应的 catch handler
    ↓
栈展开：逐帧析构局部对象 → 跳转到 handler
```

异常表记录：函数的哪些 PC 区间可被哪些 `catch` 捕获、需要析构哪些局部对象。这是"零开销"的原理——正常路径不执行任何异常相关代码。

---

## 新手要点

- **setjmp/longjmp vs table-based**：C 用 `setjmp/longjmp` 做非局部跳转（不析构对象）。C++ 的 table-based EH 更先进——正常路径零开销，异常路径自动析构。

---

## 自测题

1. table-based 异常处理如何做到正常路径零开销？
2. 异常表存在哪个段？记录什么信息？
3. 抛异常时的栈展开过程是什么？

---

## 参考与延伸

- 下一节：[7.3 模板+异常交互](03-template-exception.md)
- 回到：[第 7 章](README.md)
