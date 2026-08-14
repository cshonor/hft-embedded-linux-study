# Item 33 · 逻辑脉络

← [Item 33 目录](./README.md)

```text
库兼容 no_std → 嵌入式 / 内核用户也能用
         ↓
常只需 std:: → core:: 替换
         ↓
无 OS：无 HashMap（随机种子）、无 Mutex（OS 原语）
         ↓
用 BTreeMap、spin 等替代
         ↓
Feature "std" / "alloc" 加法门控（Item 26）
         ↓
CI --target thumbv6m-none-eabi 守卫（Item 32）
```

---
