# 第 29 章 · Superclasses（超类与继承） · 继承 + super 数据流

← [本章目录](./README.md) · 上一节：[03-29-4-super.md](./03-29-4-super.md) · 下一节：---

```text
声明时:
  OP_INHERIT  →  superclass.methods 拷贝进 subclass.methods

调用 super.cook():
  OP_SUPER_INVOKE "cook" 0
    super 类表 → cook 闭包
    slot0 = this 实例
    执行超类方法体
```

---
