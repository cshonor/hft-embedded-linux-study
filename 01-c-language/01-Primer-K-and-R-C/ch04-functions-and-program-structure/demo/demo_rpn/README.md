# demo_rpn — K&R 4.8 逆波兰计算器（可编译）

多文件示范：`calc.h` / `stack.h` + `stack.c` / `getop.c` / `main.c`。

## 编译

```bash
make
```

## 运行

```bash
./calc
```

输入示例（逆波兰，行末回车出结果）：

```
3 4 +
```
→ `结果：7`

```
10 2 /
```
→ `结果：5`

Windows（无 `make` 时）：

```bash
gcc -Wall -Wextra -std=c11 -o calc main.c stack.c getop.c
calc.exe
```

## 知识点对照

| 语法 | 代码位置 |
|------|----------|
| **`static` 模块私有** | `stack.c` 里 `static int stack_buf[]` |
| **`extern` 全局** | `calc.h` 声明 `stack_ptr`；`stack.c` 定义 |
| **头文件保护** | `CALC_H` / `STACK_H` |
| **增量编译** | `Makefile` 各 `.c` → `.o` 再链接 |

详见 [4.8 程序块结构](../../4.8-程序块结构.md)。

---

## 代码自测

**题目 1：** 以下 RPN（逆波兰）计算器的栈操作体现了什么数据结构设计？
```c
#define MAXVAL 100
int sp = 0;
double val[MAXVAL];
void push(double f) { val[sp++] = f; }
double pop(void) { return val[--sp]; }
```

<details>
<summary>参考答案</summary>

用数组实现的栈。sp 是栈顶指针（指向下一个空位）。push 先写入 val[sp] 再 sp++，pop 先 sp-- 再读取 val[sp]。栈空：sp == 0。栈满：sp >= MAXVAL。这个实现缺少边界检查——push 不检查栈满，pop 不检查栈空。K&R 的代码简洁但不够健壮，教学中常用来展示核心逻辑。

</details>
