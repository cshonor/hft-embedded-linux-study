# 第 28 章 · Methods and Initializers（方法与初始化器） · 方法调用路径（小结）

← [本章目录](./README.md) · 上一节：[05-optimized-invocations.md](./05-optimized-invocations.md) · 下一节：---

```text
obj.m(a):
  优化: OP_INVOKE "m" 1
  朴素: GET_PROPERTY → ObjBoundMethod → OP_CALL

方法体:
  slot0 = this
  OP_GET/SET_LOCAL / fields / upvalues ...
  OP_RETURN
```

---
