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
