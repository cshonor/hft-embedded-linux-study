# 第 22 章 · Local Variables（局部变量） · 局部 vs 全局（小结）

← [本章目录](./README.md) · 上一节：[04-another-scope-edge-case.md](./04-another-scope-edge-case.md) · 下一节：---

```text
全局:  名字 ──intern──► OP_*_GLOBAL ──hash──► vm.globals
局部:  名字 ──compile──► slot index ──O(1)──► stack[slot]
```

---
