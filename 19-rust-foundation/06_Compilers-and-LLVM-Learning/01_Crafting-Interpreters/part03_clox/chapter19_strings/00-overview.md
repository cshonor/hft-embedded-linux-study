# 第 19 章 · Strings（字符串） · 本章定位

← [本章目录](./README.md) · 下一节：[01-values-and-objects.md](./01-values-and-objects.md)

---

bool / number / nil 可 **按值** 塞进 `Value`；**字符串长度可变、堆分配** → 引入 **Obj 对象系统**，为 GC（ch26）与哈希表键（ch20）铺路。

| 对比 | 立即数 Value | Obj |
|------|--------------|-----|
| 存储 | 栈 / 常量池内联 | **堆** |
| 大小 | 固定 | **可变长** |
| 生命周期 | 编译期 / 栈帧 | 需 **追踪与释放** |

| 主题 | 要点 |
|------|------|
| **Obj / 结构体继承** | 首字段 **`Obj`** · 统一转换 |
| **ObjString** | 字符数组 + length + hash |
| **内存管理** | **`vm.objects` 侵入式链表 · 退出时 `freeObjects`** |

---
