# 第 13 章 高级指针话题

**Advanced Pointer Topics**

## 本章讲什么

指针高阶：**多级指针**、**函数指针**、**转移表**、复杂声明解读、**回调 + void \***、argv。DPDK 驱动表、内核 file_operations、HFT 报文分发的分水岭章节。

## 学习重点

- **`char **`** 动态串数组；≤ 三级指针
- **typedef** 封装函数指针
- **函数指针数组** 分发 vs if-else
- 回调：**判空** + **void \*priv**
- 声明解读：指针数组 / 数组指针 / 函数指针 / 函数指针数组
- **argv** = `char **` 实例
- 字面量 **`const char *`**

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | PMD ops、报文 dispatch、mempool 钩子 |
| 内核 | file_operations、private_data |
| HFT | 表驱动分发、策略回调、通用 destroy |

## 线上陷阱（汇总）

1. 函数指针数组声明写错  
2. 回调未判空调用  
3. void* 强转类型不匹配  
4. 多级指针逐层 free 遗漏  
5. 不用 typedef 难维护  
6. 不同签名函数赋给同一函数指针  

## 实操（建议完成）

1. typedef + 报文 dispatch 表  
2. 仿 file_operations 挂载 read/write  
3. 5 组复杂声明拆解  
4. event_mgr 注册/触发  
5. 空调回调段错误  
6. pool_destroy + free_cb  
7. if-else vs 转移表对比  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06–ch07；ch12 二级指针 |
| 后序 | ch14 宏；ch17 ADT；ch18 ABI |
| 配套 | 《C陷阱与缺陷》ch03、ch04 |

## 小节

- [13.1 进一步探讨指向指针的指针](./13.1-进一步探讨指向指针的指针.md)
- [13.2 高级声明](./13.2-高级声明.md)
- [13.3 函数指针](13.3-function-pointers/13.3-函数指针.md)
- [13.4 命令行参数](./13.4-命令行参数.md)
- [13.5 字符串常量](./13.5-字符串常量.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: 函数指针数组分发

```c
typedef int (*handler_t)(int);

int add(int x) { return x + 1; }
int sub(int x) { return x - 1; }
int mul(int x) { return x * 2; }

handler_t table[3] = {add, sub, mul};

int dispatch(int op, int val) {
    if (op >= 0 && op < 3)
        return table[op](../../../../../kotlin 01 var  和   val .mp4);
    return -1;
}
```

> 这种模式比 `if-else` / `switch` 有什么优势？

<details>
<summary>答案与复习指引</summary>

**答案：** 函数指针数组 = **O(1) 分发**，直接用索引跳转。`switch` 需要 N 次比较（或跳转表优化后接近 O(1)），`if-else` 是 O(N)。

**用途：** 内核 VFS `file_operations`、DPDK 驱动回调表、中断向量表、HFT 报文分发。

**代价：** 间接调用（`call [rax]`）可能破坏分支预测，热路径上不如 `switch` 编译器跳转表。

**复习：** → [13.3 Function Pointers](13.3-function-pointers/13.3-函数指针.md)

</details>


### Q3: 返回函数指针

```c
typedef int (*MathOp)(int, int);

int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }

MathOp get_op(char op) {
    switch (op) {
        case '+': return add;
        case '-': return sub;
        default:  return NULL;
    }
}

int main() {
    MathOp fn = get_op('+');
    printf("%d\n", fn(3, 4));     // 输出什么？
    return 0;
}
```

> `fn(3, 4)` 输出什么？`get_op` 返回的是什么？`fn` 如果是 NULL 会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：** `fn(3, 4)` = `add(3, 4)` = **7**。`get_op` 返回**函数指针** `MathOp`（指向 `add` 或 `sub` 函数的地址）。

`fn` 如果是 NULL（如 `get_op('*')` 返回 NULL），`fn(3, 4)` 解引用 NULL → **SIGSEGV**。

**防护：** 调用前判空：`if (fn) printf("%d\n", fn(3, 4));`

**用途：** 策略模式、命令分发表、回调注册——C 语言用函数指针实现面向对象的多态。

**复习：** → [13.3 函数指针](13.3-function-pointers/13.3-函数指针.md)

</details>

### Q4: argv 与字符串常量

```c
int main(int argc, char *argv[]) {
    // argv[0] 是程序名
    printf("Program: %s\n", argv[0]);

    // 修改 argv[1] 的内容
    argv[1][0] = 'X';     // A: 合法吗？

    // 修改 argv 指针本身
    argv[1] = "modified"; // B: 合法吗？

    // argc < 2 时访问 argv[1] 会怎样？
    return 0;
}
```

> A 和 B 分别合法吗？argc=1 时访问 argv[1] 会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：**
- A：**合法**（大多数实现）——`argv` 元素指向**可修改**的字符串（C11 §5.1.2.2.1）
- B：**合法**——`argv` 是可修改的指针数组，`argv[1]` 可以重新指向其他字符串
- `argc=1` 时 `argv[1]` 是 **NULL**（标准保证 `argv[argc]` == NULL）——解引用 NULL → 崩溃

**规则：** 访问 `argv[i]` 前必须检查 `i < argc`。

**复习：** → [13.4 命令行参数](./13.4-命令行参数.md)

</details>


### Q2: 回调 + void* priv

```c
void for_each(int *arr, int n,
              void (*cb)(int, void *), void *priv) {
    for (int i = 0; i < n; i++)
        cb(arr[i], priv);
}

void print_sq(int x, void *priv) {
    printf("%d -> %d\n", x, x * x);
}

int data[] = {1, 2, 3};
for_each(data, 3, print_sq, NULL);
```

> `void *priv` 参数做什么？为什么几乎所有回调都有它？

<details>
<summary>答案与复习指引</summary>

**答案：** `priv` 是**上下文指针**，让回调函数能访问外部状态而不依赖全局变量。

**例子：** `for_each` 遍历数组时，回调可能需要往某个 buffer 累加结果——`priv` 指向那个 buffer。

**内核实例：** `file->private_data`、`timer_list->data`、`notifier_block` 回调都有 `void *data`。

**教训：** 设计回调接口时永远预留 `void *priv` 参数，即使当前用不上。

**复习：** → [13.3 Function Pointers](13.3-function-pointers/13.3-函数指针.md)

---

## 代码自测

**题目 1：** 以下代码有什么问题？
```c
char *get_msg(void) {
    char msg[] = "hello";
    return msg;
}
```

<details>
<summary>参考答案</summary>

返回局部数组地址——未定义行为。msg 是局部数组，存储在栈上，函数返回后栈空间被回收。调用方得到的指针指向已失效的内存。正确做法：(1) 用 static char msg[]；(2) 动态分配 malloc；(3) 由调用方传入缓冲区。

</details>
