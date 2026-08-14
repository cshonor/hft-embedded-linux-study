# 第 24 章 · Calling and Closures（调用与函数） · Call 路径总览

← [本章目录](./README.md) · 上一节：[06-native-functions.md](./06-native-functions.md) · 下一节：---

```text
compile call:
  args... → callee → OP_CALL n

run OP_CALL:
  校验 arity · 建 CallFrame
  slots 指向 callee · args 已在 slots[1..n]
  新 ip 跑 function chunk

OP_RETURN:
  pop frame · push result · 恢复 caller ip
```

---
