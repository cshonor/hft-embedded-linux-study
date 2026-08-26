# 《C 程序设计语言（K&R 第二版）》

**The C Programming Language, 2nd Edition**

> **第 1 本书** · C89 奠基

## 语言基准（先钉死）

| 书 | 对应标准 |
|----|----------|
| **K&R 第2版（1988，市面通行）** | ✅ **ANSI C89 / ≈ ISO C90** |
| K&R 第1版（1978） | 经典 K&R C（非正式；无函数原型等） |

> **不是 C99，不是 C11。** 原版 C89 **没有** `//` 单行注释（只有 `/* */`）；书中或笔记若出现 `//`，是现代编译器习惯。

与内核串联：[LKD Ch2 §2.4](../../05-linux-kernel/chapter-02-getting-started/notes/section-2.4-内核开发的特点.md) — 旧内核 `gnu89` = C89+GCC 扩展；现代内核 `gnu11`。读完 K&R 还需补 C99 习惯 + [05 GNU-C](../05-Kernel-Prep-Embedded-C-Self-Cultivation/)。

## 定位

阶段 1 · **C89** 标准 C 基底。剔除 C++ 的类、RAII、`new` 等思维，重新习惯 `malloc`/`free`、原生指针、结构体、函数指针。

## 阅读建议

有 C++ 基础可快速过，不必逐行死磕例题；重点是改掉面向对象的写法习惯。

## HTML 阅读版

全书笔记已生成单文件 HTML 阅读版（暗色主题 · 侧边栏目录 · 章内锚点跳转 · 自测折叠答案）：[进入封面页](./html/index.html)，或直达各章：

| 章 | HTML | 章 | HTML |
|----|------|----|------|
| 第 1 章 | [ch01.html](./html/ch01.html) | 第 6 章 | [ch06.html](./html/ch06.html) |
| 第 2 章 | [ch02.html](./html/ch02.html) | 第 7 章 | [ch07.html](./html/ch07.html) |
| 第 3 章 | [ch03.html](./html/ch03.html) | 第 8 章 | [ch08.html](./html/ch08.html) |
| 第 4 章 | [ch04.html](./html/ch04.html) | 附录 A | [appendix-a.html](./html/appendix-a.html) |
| 第 5 章 | [ch05.html](./html/ch05.html) | 附录 B/C | [appendix-b.html](./html/appendix-b.html) · [appendix-c.html](./html/appendix-c.html) |

> 由 `md → HTML` 转换脚本生成，源笔记更新后可重新生成。

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

---

## 代码自测

**题目 1：** K&R 第二版基于哪个 C 标准？以下代码在该标准下合法吗？
```c
// K&R 2nd Edition (1988)
int main(void) {
    // K&R 风格的函数声明
    int f();
    return f(1);
}
int f(int x) { return x * 2; }
```

<details>
<summary>参考答案</summary>

K&R 第二版基于 ANSI C89 标准（1989）。代码中 int f(); 在 C89 中合法——空参数列表表示接受任意参数。C99 起空参数列表被废弃，建议用 int f(void) 明确不接受参数，或 int f(int) 声明原型。K&R 第二版是学习 C 的经典教材，但某些写法（如隐式 int 返回、K&R 函数定义语法）在现代 C 中已不推荐。

</details>
