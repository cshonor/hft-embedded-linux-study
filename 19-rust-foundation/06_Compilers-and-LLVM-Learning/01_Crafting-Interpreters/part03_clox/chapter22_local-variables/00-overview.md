# 第 22 章 · Local Variables（局部变量） · 本章定位

← [本章目录](./README.md) · 下一节：[01-representing-local-variables.md](./01-representing-local-variables.md)

---

ch21 全局变量走 **哈希表按名查找**；ch22 引入 **块作用域 + 局部变量**，变量直接住在 **Value Stack 槽位** 上，读写 **O(1) 按索引**。

| 对比 | 全局 ch21 | 局部 ch22 |
|------|-----------|-----------|
| 存储 | **`vm.globals` 表** | **栈槽 slot** |
| 指令 | **`OP_GET/SET_GLOBAL`** | **`OP_GET/SET_LOCAL`** |
| 编译期 | 名字 → 常量池 | **`Compiler.locals[]`** 记录 slot + depth |
| 查找 | 运行时 hash | **编译期定 index** |

| 小节 | 主题 |
|------|------|
| **§22.1** | **`Local`** · 栈偏移 · `locals` 数组 |
| **§22.2～§22.3** | **`beginScope` / `endScope`** · shadowing |
| **§22.4** | **`OP_GET/SET_LOCAL`** |
| **§22.5** | **`var a = a;`** 两阶段初始化 |

---
