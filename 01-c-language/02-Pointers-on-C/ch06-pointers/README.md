# 第 6 章 指针

**Pointers**

## 本章讲什么

全书**核心灵魂**：`&`/`*`、指针算术、数组等价、**const**/**void***、野指针/悬垂指针、指针数组。读懂 DPDK mbuf、内核链表、HFT 零拷贝的分水岭。

## 学习重点

- 指针存**地址**；解引用读写对象
- **`int *a, b`** 陷阱；局部指针 **= NULL**
- **`arr[i]` ≡ `*(arr+i)`**；`sizeof(arr)` vs 退化
- **指针数组** `*pkts[32]` vs **数组指针** `(*row)[32]`
- **const** 四式；**void\*** + cast
- 算术/比较**同数组**；越界 **UB**
- 只读 **`"literal"`** vs **`char buf[]`**
- **free 后 p = NULL**

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | mbuf*、burst 数组、void* 池 |
| 内核 | 链表、device*、private_data |
| HFT | 零拷贝解析、restrict、空指针过滤 |

## 实操（建议完成）

1. 打印地址/解引用  
2. 改字面量段错误  
3. 模拟 burst 指针遍历  
4. 野/悬垂指针实验  
5. const 四层测试  
6. void* 转报文 struct  
7. 指针数组 vs 数组指针  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch03 const/void；ch05 &/* |
| 后序 | ch08 数组；ch10 ->；ch11 malloc；ch12/13 高阶 |
| 配套 | 《C陷阱与缺陷》ch03、ch07 |

## 小节

- [6.0 星号两场景（入门速记）](./6.0-星号两场景.md) ← 声明 `*` vs 表达式 `*`
- [6.1 内存和地址](./6.1-内存和地址.md)
- [6.2 值和类型](./6.2-值和类型.md)
- [6.3 指针变量的内容](./6.3-指针变量的内容.md)
- [6.4 间接访问操作符](./6.4-间接访问操作符.md)
- [6.5 未初始化和非法的指针](./6.5-未初始化和非法的指针.md)
- [6.6 NULL 指针](./6.6-NULL指针.md)
- [6.7 指针和间接访问](./6.7-指针和间接访问.md)
- [6.8 指针和变量](./6.8-指针和变量.md)
- [6.9 指针常量](./6.9-指针常量.md)
- [6.10 指针的指针](./6.10-指针的指针.md)
- [6.11 指针表达式](./6.11-指针表达式.md)
- [6.12 实例](./6.12-实例.md)
- [6.13 指针运算](./6.13-pointer-arithmetic/6.13-pointer-arithmetic.md)
  - [6.13.1 指针的算术运算](./6.13-pointer-arithmetic/6.13.1-指针的算术运算.md)
  - [6.13.2 指针的关系运算](./6.13-pointer-arithmetic/6.13.2-指针的关系运算.md)


---

## 章节自测

> 指针是 C 的灵魂。看代码 → 想答案 → 点开验证。

### Q1: int *a, b 陷阱

```c
int *a, b;

a = malloc(sizeof(int));
*a = 10;
b = 20;

// a 和 b 的类型分别是什么？
```

<details>
<summary>答案与复习指引</summary>

**答案：** `a` 是 `int *`（指针），`b` 是 `int`（不是指针！）。

**解析：** `*` 只修饰紧跟的变量名 `a`，不修饰 `b`。要两个都是指针需写 `int *a, *b;`。

**教训：** 一行声明一个变量，避免混淆。

**复习：** → [6.1 Pointer Variables](./6.1-pointer-variables/6.1-pointer-variables.md)

</details>

### Q2: 数组退化与 sizeof

```c
int arr[10];
int *p = arr;

printf("%zu\n", sizeof(arr));   // (1)
printf("%zu\n", sizeof(p));     // (2)
printf("%zu\n", sizeof(*p));    // (3)

void func(int a[]) {
    printf("%zu\n", sizeof(a)); // (4)
}
```

> 四个 sizeof 各输出多少（64 位）？

<details>
<summary>答案与复习指引</summary>

**输出：** `(1)` = 40，`(2)` = 8，`(3)` = 4，`(4)` = 8

**解析：**
- `sizeof(arr)` — 数组大小 = 10 × 4 = 40
- `sizeof(p)` — 指针大小 = 8（64 位）
- `sizeof(*p)` — `int` 大小 = 4
- `sizeof(a)` — **函数参数中数组退化为指针**，所以 = 8

**教训：** 函数内 `sizeof(参数[])` 得到的是指针大小，不是数组大小。传参时必须额外传长度。

**复习：** → [6.4 Pointers to Arrays](./6.4-pointers-to-arrays/6.4-pointers-to-arrays.md) — 退化

</details>

### Q3: 野指针与 free 后置 NULL

```c
int *p = malloc(sizeof(int));
*p = 42;
free(p);

// p 现在指向什么？
// *p = 99;  // 会怎样？
// free(p);  // 会怎样？

p = NULL;
// free(p);  // 现在 free(p) 安全吗？
```

<details>
<summary>答案与复习指引</summary>

**答案：** `free(p)` 后 `p` 仍指向原地址（**悬垂指针**）。
- `*p = 99` → UB（可能段错误，可能写到已回收的堆块）
- `free(p)` → **双重 free** → 堆损坏，可能崩溃

`p = NULL` 后 `free(NULL)` 是**安全的**（标准保证什么都不做）。

**教训：** `free` 后立即 `p = NULL`，防止悬垂指针和双重 free。

**复习：** → [6.9 Pointers to Pointers](./6.9-pointers-to-pointers/6.9-pointers-to-pointers.md) — 悬垂指针

</details>

### Q4: void* 通用指针

```c
void *vp = malloc(100);
int *ip = vp;           // (1) 合法吗？
// int x = *vp;         // (2) 合法吗？
// vp++;                // (3) 合法吗？
// vp = vp + 1;        // (4) 合法吗？
char *cp = vp;
cp++;                   // (5) 合法吗？
```

> 哪些合法，哪些不合法？

<details>
<summary>答案与复习指引</summary>

**答案：**
- `(1)` 合法 — `void*` 可隐式转为任何类型指针（C 中不需要 cast，C++ 需要）
- `(2)` **不合法** — 不能解引用 `void*`（编译器不知道读几个字节）
- `(3)(4)` **不合法** — 不能对 `void*` 做算术（不知道步长）
- `(5)` 合法 — `char*` 可以 `++`（步长 1 字节）

**教训：** `void*` 用于通用接口（`malloc`、`qsort` 回调），使用前必须 cast 到具体类型。

**复习：** → [6.7 Pointers to void](./6.7-pointers-to-void/6.7-pointers-to-void.md)

</details>
