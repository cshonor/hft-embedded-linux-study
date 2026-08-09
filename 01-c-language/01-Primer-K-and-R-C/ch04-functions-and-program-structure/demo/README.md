# ch04 Demo

```bash
make -C demo_rpn && ./demo_rpn/calc
```

Windows：

```bash
cd demo_rpn
gcc -Wall -Wextra -std=c11 -o calc main.c stack.c getop.c
```

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
