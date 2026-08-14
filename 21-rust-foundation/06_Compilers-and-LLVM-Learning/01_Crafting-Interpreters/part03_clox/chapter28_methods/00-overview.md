# 第 28 章 · Methods and Initializers（方法与初始化器） · 本章定位

← [本章目录](./README.md) · 下一节：[01-method-declarations.md](./01-method-declarations.md)

---

为 **OOP 注入可调用行为**：类方法表 · **`this` 作 slot 0** · **`init`** · **`OP_INVOKE` 超指令** 避免 BoundMethod 堆分配。

| 对比 | jlox ch12 | clox ch28 |
|------|-----------|-----------|
| 方法查找 | 实例 fields 优先 → 类方法 | 同 · **`ObjClass.methods` Table** |
| 绑定 | **`LoxBoundMethod`** 常分配 | 一般路径 BoundMethod · **invoke 优化** |
| **`this`** | 方法环境 **`define("this")`** | **隐藏局部变量 slot 0** |
| **`init`** | 自动调用 · return 限制 | 同语义 · 编译期约束 |

| 小节 | 主题 |
|------|------|
| **§28.1** | 方法声明 · **`methods` 哈希表** |
| **§28.2** | **`ObjBoundMethod`** |
| **§28.3** | **`this` = slot 0** |
| **§28.4** | **`init()`** 初始化器 |
| **§28.5** | **`OP_INVOKE`** 超指令 |

---
