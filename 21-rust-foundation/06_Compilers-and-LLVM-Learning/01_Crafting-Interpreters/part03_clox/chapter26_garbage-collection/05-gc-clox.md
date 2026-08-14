# 第 26 章 · Garbage Collection（垃圾回收） · GC 与 clox 子系统

← [本章目录](./README.md) · 上一节：[04-when-to-collect.md](./04-when-to-collect.md) · 下一节：---

```text
         Roots
    stack · globals · frames · upvalues
              │
              ▼ mark (tri-color / work stack)
         所有可达 Obj 变黑
              │
    sweep vm.objects ──► free 白对象
              │
    清理 vm.strings 弱引用条目
              │
         重置 nextGC
```

---
