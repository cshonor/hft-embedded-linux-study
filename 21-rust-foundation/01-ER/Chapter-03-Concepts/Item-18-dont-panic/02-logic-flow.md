# Item 18 · 逻辑脉络

← [Item 18 目录](./README.md)

```text
C++/Java 思维：catch_unwind ≈ try-catch
         ↓
阻碍 1：panic=abort → catch 根本跑不到
阻碍 2：状态撕裂 → exception safety 不成立
阻碍 3：panic 越过 FFI → UB
         ↓
库代码正路：Result + ? → 把决策交给调用者（passing the buck）
```

---
