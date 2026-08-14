# 第 28 章 · Methods and Initializers（方法与初始化器） · §28.2 绑定方法（Bound Methods）

← [本章目录](./README.md) · 上一节：[01-method-declarations.md](./01-method-declarations.md) · 下一节：[03-this.md](./03-this.md)

---

**朴素路径**：`instance.method` **仅 Get** → 查 **`methods` 表** → 包装 **`ObjBoundMethod { receiver, method }`** → push。

| 对象 | 作用 |
|------|------|
| **`ObjBoundMethod`** | **胶囊**：**receiver 实例** + **方法闭包** |
| **`OP_CALL`** | 对 BoundMethod：绑定 **`this`**（slot 0）再进 chunk |

**代价**：每次 **Get 属性** 若只为调用 → **堆分配 BoundMethod** · GC 压力 · 慢。

**§28.5** 用 **`OP_INVOKE`** 规避（见下）。

**对照 jlox [ch12 §12.4](../../part02_jlox/chapter12_classes/README.md)**：语义相同 · clox 后加 **invoke 融合优化**。

---
