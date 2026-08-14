# 第 20 章 · Hash Tables（哈希表） · 本章定位

← [本章目录](./README.md) · 下一节：[01-open-addressing-and-linear-probing.md](./01-open-addressing-and-linear-probing.md)

---

动态语言的核心基础设施：**全局变量、属性、方法表** 都依赖 **O(1) 均摊** 查找。clox **手写 C 哈希表**，并为 **字符串驻留 (interning)** 服务。

| 用途（本章及后） | 结构 |
|------------------|------|
| **字符串 intern** | 全局 **`vm.strings`** 表 |
| **全局变量 ch21** | **`vm.globals`** |
| 实例字段 / 方法 | ch25+ |

| 主题 | 要点 |
|------|------|
| **开放寻址 + 线性探测** | 无链表 · 缓存友好 |
| **FNV-1a + 负载 75%** | 扩容 **rehash** |
| **Tombstones** | 删除不破坏探测链 |
| **String Interning** | 相等 = **指针比较** |

---
