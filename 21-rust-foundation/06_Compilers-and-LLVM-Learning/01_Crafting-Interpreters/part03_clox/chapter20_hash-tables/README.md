# 第 20 章 · Hash Tables（哈希表）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/hash-tables.html) · [中文在线](https://craftinginterpreters.com/hash-tables.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

动态语言的核心基础设施：全局变量、属性、方法表 都依赖 O(1) 均摊 查找。clox 手写 C 哈希表，并为 字符串驻留 (interning) 服务。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 开放寻址与线性探测（Open Addressing and Linear Probing） | [01-open-addressing-and-linear-probing.md](./01-open-addressing-and-linear-probing.md) |
| ·3 | 哈希函数与负载因子（Hash Functions and Load Factor） | [02-hash-functions-and-load-factor.md](./02-hash-functions-and-load-factor.md) |
| ·4 | 字符串驻留（String Interning） | [03-string-interning.md](./03-string-interning.md) |
| ·5 | Table API（概念） | [04-hash-tables-table-api.md](./04-hash-tables-table-api.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
结构    开放寻址 · 线性探测 · Entry 数组
冲突    探测下一槽 · 75% 扩容 rehash
删除    Tombstone 保链
Intern  vm.strings · 同文同址 · 指针相等
Hash    FNV-1a · ObjString.hash 缓存
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **21** | [chapter21 · Global Variables](../chapter21_global-variables/) | **`vm.globals`** · DEFINE/GET/SET |
| **22** | Local Variables | 栈 slot · 非全局表 |
| **25** | Objects | 实例字段表 |

---

---

## 自测

1. 线性探测聚集（primary clustering）是什么？本书为何仍选它？
2. 负载因子超过 75% 不扩容会怎样？
3. 没有 intern，`OP_EQUAL` 对两个内容相同的字符串要做什么？

---

---

## 阅读进度

- [x] 开放寻址 / 墓碑 / intern 结构梳理（本章笔记）
- [ ] 手工模拟插入冲突与墓碑删除
- [ ] 本章 Challenges

