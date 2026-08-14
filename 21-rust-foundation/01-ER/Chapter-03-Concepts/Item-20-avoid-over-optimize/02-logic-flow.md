# Item 20 · 逻辑脉络

← [Item 20 目录](./README.md)

```text
零拷贝：struct Tlv<'a> { value: &'a [u8] }  → 解析快
         ↓
放进长生命周期状态：NetworkServer 缓存 Tlv<'a>
         ↓
数据来源是循环里临时 Vec → data does not live long enough
         ↓
破局：struct Tlv { value: Vec<u8> } + .to_vec()
         ↓
一次分配换掉<'a>，代码恢复灵活
```

**生命周期约束会传染** —— 引用型字段把 `'a` 绑进整个类型图；长期状态机往往装不下「借自临时 buffer」的数据。

---
