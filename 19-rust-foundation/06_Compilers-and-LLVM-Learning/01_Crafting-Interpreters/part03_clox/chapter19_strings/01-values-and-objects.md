# 第 19 章 · Strings（字符串） · 对象与结构体继承（Values and Objects）

← [本章目录](./README.md) · 上一节：[00-overview.md](./00-overview.md) · 下一节：[02-memory-management.md](./02-memory-management.md)

---

**堆上值 = 对象 (Obj)**：

```c
struct Obj {
  ObjType type;
  struct Obj* next;   // 链表节点
};
```

**具体类型**（结构体继承技巧）：

```c
struct ObjString {
  Obj obj;      // 必须是第一个字段
  int length;
  char* chars;
  uint32_t hash;
};
```

| 技巧 | 说明 |
|------|------|
| **`Obj` 为首字段** | `(ObjString*)` ↔ `(Obj*)` **安全 upcast/downcast** |
| **`ObjType` 枚举** | `OBJ_STRING` 等，运行时判别 |
| **模拟 OOP** | C 无 class · **组合 + 首字段布局** |

**`Value` 扩展**：增加 **`OBJ_VAL(obj*)`** · **`IS_OBJ`** · **`AS_STRING`** 等。

**编译字符串字面量**：

- Scanner 已识别 **`TOKEN_STRING`** lexeme。
- Compiler **`copyString`** → 堆上 **`ObjString*`** → **`emitConstant(OBJ_VAL(string))`**。

---
