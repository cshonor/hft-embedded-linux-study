# 第 19 章 · Strings（字符串）

> **Crafting Interpreters** · [Part III · clox](../README.md) · [本书目录](../../本书目录.md)  
> 在线：[craftinginterpreters.com](https://craftinginterpreters.com/strings.html) · [中文在线](https://craftinginterpreters.com/strings.html)

## 状态

- [x] 已读（笔记整理）

---

## 一句话

bool / number / nil 可 按值 塞进 `Value`；字符串长度可变、堆分配 → 引入 Obj 对象系统，为 GC（ch26）与哈希表键（ch20）铺路。

---

## 专项笔记（一小节一文件）

| 小节 | 主题 | 阅读 |
|------|------|------|
| — | 本章定位 | [00-overview.md](./00-overview.md) |
| ·2 | 对象与结构体继承（Values and Objects） | [01-values-and-objects.md](./01-values-and-objects.md) |
| ·3 | 内存管理基础（Memory Management） | [02-memory-management.md](./02-memory-management.md) |
| ·4 | ObjString 要点 | [03-objstring.md](./03-objstring.md) |
| — | 速记 · 自测 · 进度 |

---

## 逻辑脉络


---

## 速记

## 本章速记

```text
Obj       type + next · 堆对象基类
继承      ObjString.obj 第一字段 · 指针转换
Value     OBJ_VAL · 字符串走常量池
内存      vm.objects 链表 · freeObjects 退出清理
```

---

---

## 读后下一步

| 章 | 目录 | 内容 |
|:--:|------|------|
| **20** | [chapter20 · Hash Tables](../chapter20_hash-tables/) | 开放寻址 · **字符串 intern** |
| **21** | Global Variables | 变量名 = **字符串键** |
| **26** | Garbage Collection | 替换 **freeObjects** |

---

---

## 自测

1. 为什么 `Obj` 必须是 `ObjString` 的第一个字段？
2. 侵入式链表相比「全局指针数组存所有 Obj」有何取舍？
3. 字符串 hash 为什么在创建时就算好？

---

---

## 阅读进度

- [x] Obj / ObjString / 链表内存 结构梳理（本章笔记）
- [ ] 追踪 `copyString` → `emitConstant` 路径
- [ ] 本章 Challenges

