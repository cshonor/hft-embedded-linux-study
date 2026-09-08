# 第 3 章 控制流

**Control Flow**

## 本章讲什么

程序的分支、循环与跳转：**纯流程语法**，不涉及内存布局与指针。写内核调度、行情循环、飞控主循环，都建立在本章之上。

## 学习重点

- **`else` 就近匹配**、**`switch` 穿透** —— C 笔试与读遗留代码的高频坑
- **`while` / `for` 等价转换** —— 遍历与计数两种写法互换
- **`break` / `continue` / `goto`** —— 尤其内核里 `goto` 错误处理与多层循环退出
- 本章难度低于第 2、5 章，可快速过一遍，但上述考点需记牢

## 小节

- [3.1 语句与程序块](./3.1-语句与程序块.md) — 复合语句、`{}` 打包、块作用域
- [3.2 if-else 语句](./3.2-if-else语句.md) — 非 0 即真、指针判空、`else` 就近匹配
- [3.3 else-if 多分支结构](./3.3-else-if语句.md)
- [3.4 switch 语句](./3.4-switch语句.md) — `case` 仅整型常量、字符转码值、字符串/浮点不行、与其他语言对比、穿透与 break
- **`while` 条件里的 `i++`** —— 先判断、副作用立刻发生；与 `++i`、`for` 步进不等价（见 3.5）
- [3.5 while 循环与 for 循环](./3.5-while循环与for循环.md) — 执行顺序、`i++`/`++i` 在条件中的差异、与 `for` 等价写法
- [3.6 do-while 循环](./3.6-do-while循环.md) — 先体后判、至少一次；菜单、输入重试；宏 `do{}while(0)`
- [3.7 break 与 continue](./3.7-break语句与continue语句.md) — 嵌套循环只作用于最近一层；`for` 中 continue 仍步进
- [3.8 goto 语句与标号](./3.8-goto语句与标号.md)

---

## 章节自测

> 每题对应一个小节。看代码 → 想答案 → 点开验证。

### Q1: if-else 就近匹配

```c
int x = 5;

if (x > 0)
    if (x > 10)
        printf("A\n");
else
    printf("B\n");
```

> 输出 A、B 还是不输出？`else` 配哪个 `if`？

<details>
<summary>答案与复习指引</summary>

**输出：** 不输出任何内容（无 A 也无 B）

**解析：** `else` 遵循**就近匹配**，配的是内层 `if (x > 10)`，不是外层 `if (x > 0)`。`x = 5` 通过外层 `if`，但内层 `if (x > 10)` 为假，走到 `else`——但 `else` 配的是内层 `if`，所以 `else` 也不执行... 

等等，让我重新分析：`x = 5`，外层 `if (x > 0)` 为真 → 进入内层 `if (x > 10)` 为假 → 执行 `else printf("B")`。所以输出 **B**。

缩进骗人——缩进看着像配外层 `if`，但 C 语法上 `else` 配最近的 `if`。

**教训：** 多层 `if` 必须用 `{}` 明确括住。

**复习：** → [3.2 if-else语句](./3.2-if-else语句.md) — else 就近匹配

</details>

### Q2: switch 穿透

```c
int n = 2;

switch (n) {
    case 1: printf("one ");
    case 2: printf("two ");
    case 3: printf("three ");
    default: printf("default");
}
printf("\n");
```

> 输出什么？如果每个 case 都加 `break`，输出又是什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `two three default`

**解析：** `n = 2` 匹配 `case 2`，之后**穿透**执行 `case 3` 和 `default` 的代码，直到 `switch` 结束。加 `break` 后只输出 `two`。

**教训：** 除非刻意利用穿透，每个 `case` 必须加 `break`。`default` 也应该加 `break`（即使放最后）。

**复习：** → [3.4 switch语句](./3.4-switch语句.md) — 穿透与 break

</details>

### Q3: while 与 for 等价

```c
// 写法 A
int i = 0;
while (i < 5) {
    printf("%d ", i);
    i++;
}

// 写法 B
for (int j = 0; j < 5; j++) {
    printf("%d ", j);
}
```

> 两种写法输出一样吗？`while` 和 `for` 可以互相替换吗？

<details>
<summary>答案与复习指引</summary>

**输出：** 都是 `0 1 2 3 4 `

**解析：** `for (init; cond; step)` 等价于 `init; while (cond) { body; step; }`。任何 `for` 都能改写成 `while`，反之亦然。选择取决于可读性：计数用 `for`，条件驱动用 `while`。

**注意：** K&R C89 中 `for (int j=0; ...)` 的变量声明在 C99 才合法。C89 需在块外声明 `int j`。

**复习：** → [3.5 while循环与for循环](./3.5-while循环与for循环.md)

</details>

### Q4: break 与 continue

```c
for (int i = 0; i < 10; i++) {
    if (i == 3)
        continue;
    if (i == 7)
        break;
    printf("%d ", i);
}
printf("\n");
```

> 输出什么？`continue` 和 `break` 分别跳出什么？

<details>
<summary>答案与复习指引</summary>

**输出：** `0 1 2 4 5 6 `

**解析：** `i=3` 时 `continue` 跳过 `printf`，但 `i++` **仍然执行**（`for` 的步进不受 `continue` 影响）。`i=7` 时 `break` 跳出整个循环。`continue` 只跳过当次循环体，`break` 跳出整个循环。嵌套循环中两者都只作用于**最近一层**。

**复习：** → [3.7 break语句与continue语句](./3.7-break语句与continue语句.md)

</details>

### Q5: goto 错误清理

```c
int do_work(void) {
    void *buf1 = malloc(100);
    if (!buf1) goto fail;

    void *buf2 = malloc(200);
    if (!buf2) goto fail_buf1;

    // ... use buffers ...
    free(buf2);
    free(buf1);
    return 0;

fail_buf1:
    free(buf1);
fail:
    return -1;
}
```

> 这段代码用了什么模式？为什么用 `goto` 而不是直接 `return`？

<details>
<summary>答案与复习指引</summary>

**答案：** Linux 内核经典的 **goto 错误清理链**模式。

**解析：** 多步分配时，如果中间步骤失败，需要按**逆序释放**之前分配的资源。`goto` 跳到对应的清理标签，依次释放。直接 `return` 会导致内存泄漏（没 `free` 已分配的部分）。

内核代码大量使用此模式（`out:` / `err_:` 标签），不是坏实践。

**复习：** → [3.8 goto语句与标号](./3.8-goto语句与标号.md)

</details>

---

## 代码自测

**题目 1：** 以下代码中 if-else 的配对关系是怎样的？
```c
if (a > 0)
    if (b > 0)
        x = 1;
else
    x = 2;
```

<details>
<summary>参考答案</summary>

else 与最近的 if 配对（dangling else 问题）——else 属于 if (b > 0)，不是 if (a > 0)。当 a <= 0 时，x 不被赋值。如果想 else 属于外层 if，需要用花括号：if (a > 0) { if (b > 0) x = 1; } else x = 2;。K&R 建议始终用花括号避免歧义。

</details>
