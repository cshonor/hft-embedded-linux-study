# 附录 B 标准库

K&R 附录 B 各标准头文件速查；与 ch01–ch08 示例中的 `#include` 对应。

## 小节

- [B.1 输入与输出 `<stdio.h>`](./B.1-输入与输出stdio.h.md) — `printf`、`stdin/stdout`、`FILE`、`EOF`
- [B.2 字符类别测试ctype.h](./B.2-字符类别测试ctype.h.md)
- [B.3 字符串函数string.h](./B.3-字符串函数string.h.md)
- [B.4 数学函数math.h](./B.4-数学函数math.h.md)
- [B.5 实用函数stdlib.h](./B.5-实用函数stdlib.h.md)
- [B.6 诊断assert.h](./B.6-诊断assert.h.md)
- [B.7 可变参数表stdarg.h](./B.7-可变参数表stdarg.h.md)
- [B.8 非局部跳转setjmp.h](./B.8-非局部跳转setjmp.h.md)
- [B.9 信号signal.h](./B.9-信号signal.h.md)
- [B.10 日期与时间函数time.h](./B.10-日期与时间函数time.h.md)
- [B.11 与具体实现相关的限制](./B.11-与具体实现相关的限制.md)

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
