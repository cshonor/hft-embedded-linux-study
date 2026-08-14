# 第 21 章 · Global Variables（全局变量） · 全局变量管线小结

← [本章目录](./README.md) · 上一节：[04-assignment-precedence.md](./04-assignment-precedence.md) · 下一节：---

```text
var x = 1;
  CONSTANT 1
  CONSTANT "x"    // interned name
  DEFINE_GLOBAL

print x;
  GET_GLOBAL "x"
  PRINT

x = 2;
  CONSTANT 2
  SET_GLOBAL "x"
```

---
