# Item 28 · 逻辑脉络

← [Item 28 目录](./README.md)

```text
函数：     同类型不同值
泛型+trait：不同类型
宏：         同「语法角色」的不同程序片段（ident / expr / ty…）
         ↓
Rust 宏：Token / AST 级（非 C 文本替换）
         ↓
代价：难读、难调试、rustfmt/IDE 黑盒、编译期执行（Item 25）
         ↓
原则：函数/泛型不够再用；优先 derive
```

---
