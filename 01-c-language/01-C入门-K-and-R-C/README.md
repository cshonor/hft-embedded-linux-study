# 《C 程序设计语言（K&R 第二版）》

**The C Programming Language, 2nd Edition**

## 语言基准（先钉死）

| 书 | 对应标准 |
|----|----------|
| **K&R 第2版（1988，市面通行）** | ✅ **ANSI C89 / ≈ ISO C90** |
| K&R 第1版（1978） | 经典 K&R C（非正式；无函数原型等） |

> **不是 C99，不是 C11。** 原版 C89 **没有** `//` 单行注释（只有 `/* */`）；书中或笔记若出现 `//`，是现代编译器习惯。

与内核串联：[LKD Ch2 §2.4](../../07-linux-kernel/00_Book_3rd_Notes/chapter-02-getting-started/notes/section-2.4-内核开发的特点.md) — 旧内核 `gnu89` = C89+GCC 扩展；现代内核 `gnu11`。读完 K&R 还需补 C99 习惯 + [05 GNU-C](../04-内核基础-Embedded-C-Self-Cultivation/)。

## 定位

阶段 1 · **C89** 标准 C 基底。剔除 C++ 的类、RAII、`new` 等思维，重新习惯 `malloc`/`free`、原生指针、结构体、函数指针。

## 阅读建议

有 C++ 基础可快速过，不必逐行死磕例题；重点是改掉面向对象的写法习惯。

## 章节索引

全书 8 章 + 3 附录。各章目录下已按小节划分占位笔记；路径均为 ASCII，中文标题在文件内。

| 章 | 目录 | 主题 |
|----|------|------|
| 第 1 章 | [ch01-introduction](./ch01-introduction/) | 导言 |
| 第 2 章 | [ch02-types-operators-expressions](./ch02-types-operators-expressions/) | 类型、运算符与表达式 |
| 第 3 章 | [ch03-control-flow](./ch03-control-flow/) | 控制流 |
| 第 4 章 | [ch04-functions-and-program-structure](./ch04-functions-and-program-structure/) | 函数与程序结构 |
| 第 5 章 | [ch05-pointers-and-arrays](./ch05-pointers-and-arrays/) | 指针与数组 |
| 第 6 章 | [ch06-structures](./ch06-structures/) | 结构 |
| 第 7 章 | [ch07-input-and-output](./ch07-input-and-output/) | 输入与输出 |
| 第 8 章 | [ch08-unix-system-interface](./ch08-unix-system-interface/) | UNIX 系统接口 |

### 附录

| 附录 | 目录 | 主题 |
|------|------|------|
| 附录 A | [appendix-a-reference-manual](./appendix-a-reference-manual/) | 参考手册 |
| 附录 B | [appendix-b-standard-library](./appendix-b-standard-library/) | 标准库 |
| 附录 C | [appendix-c-change-summary](./appendix-c-change-summary/) | 变更小结 |

## 学习进度

- [x] 第 1 章 导言
- [x] 第 2 章 类型、运算符与表达式
- [x] 第 3 章 控制流
- [x] 第 4 章 函数与程序结构
- [x] 第 5 章 指针与数组
- [x] 第 6 章 结构
- [x] 第 7 章 输入与输出
- [x] 第 8 章 UNIX 系统接口
- [ ] 附录 A 参考手册
- [ ] 附录 B 标准库
- [ ] 附录 C 变更小结
