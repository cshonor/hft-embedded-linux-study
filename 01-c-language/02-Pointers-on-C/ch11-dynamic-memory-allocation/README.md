# 第 11 章 动态内存分配

**Dynamic Memory Allocation**

## 本章讲什么

**堆 heap** 上 **malloc / calloc / realloc / free** 的生命周期与故障模式。栈/静态段无法承载变长、海量数据；DPDK mempool 是本章堆模型的工业优化版。

## 学习重点

- 栈 / 静态 / 堆三分区与选型
- **malloc** 不清零；**calloc** 清零；**realloc** 用临时指针
- 分配判 **NULL**；free 原始地址；**free 后 = NULL**
- 四大故障：泄漏、悬垂、双重 free、越界
- 碎片 → 内存池、固定块（**rte_mempool**）
- **posix_memalign**；多线程 malloc 锁竞争

## 场景价值

| 方向 | 本章技能 |
|------|----------|
| DPDK | mempool 预分配、无锁、大页对齐 |
| 内核 | kmalloc / slab 对照 |
| HFT | 热路径禁频繁 malloc；订单/行情池 |

## 线上陷阱（汇总）

1. 未判 NULL  
2. realloc 直接赋回原指针  
3. 双重 free  
4. free 后继续使用  
5. 堆越界  
6. 异常分支泄漏  
7. 热路径 libc malloc 锁抖动  

## 实操（建议完成）

1. malloc vs calloc 脏数据  
2. realloc 丢指针 vs 安全写法  
3. 双重 free → SIGABRT  
4. 堆链表完整 free  
5. 堆越界观察损坏  
6. **posix_memalign** 64B  
7. valgrind 查泄漏  

## 前后章节

| 方向 | 章节 |
|------|------|
| 前置 | ch06 void*；ch08 VLA；ch10 堆 struct |
| 后序 | ch12 指针链表；ch17 ADT；ch18 mmap/brk |
| 配套 | 《C陷阱与缺陷》ch03、ch05 |

## 小节

- [11.1 为什么使用动态内存分配](./11.1-为什么使用动态内存分配.md)
- [11.2 malloc 和 free](./11.2-malloc和free.md)
- [11.3 calloc 和 realloc](./11.3-calloc和realloc.md)
- [11.4 使用动态分配的内存](./11.4-使用动态分配的内存.md)
- [11.5 常见错误](./11.5-常见错误.md)
- [11.6 内存分配实例](./11.6-内存分配实例.md)


---

## 章节自测

> 看代码 → 想答案 → 点开验证。

### Q1: malloc vs calloc

```c
int *a = malloc(100 * sizeof(int));
int *b = calloc(100, sizeof(int));

// a 和 b 指向的内存初始值是什么？
```

<details>
<summary>答案与复习指引</summary>

**答案：** `a` 指向的内存**不确定**（可能含旧数据）。`b` 指向的内存**全零**（`calloc` 保证清零）。

**教训：** 需要清零用 `calloc`，或 `malloc` 后 `memset(p, 0, n)`。调试模式（glibc）下 `malloc` 可能也返回清零内存，但**不能依赖**。

**复习：** → [11.1 Memory Allocation](11.1-为什么使用动态内存分配.md)

</details>

### Q2: realloc 临时指针

```c
int *buf = malloc(10 * sizeof(int));
// ... buf 被使用 ...

// 写法 A (危险)
buf = realloc(buf, 20 * sizeof(int));

// 写法 B (安全)
int *tmp = realloc(buf, 20 * sizeof(int));
if (tmp) buf = tmp;
```

> 写法 A 有什么风险？

<details>
<summary>答案与复习指引</summary>

**答案：** 写法 A 中如果 `realloc` 失败返回 `NULL`，原 `buf` 指针被覆盖 → **内存泄漏 + 悬垂指针**。

写法 B 用临时指针 `tmp` 接收结果：成功再赋值给 `buf`，失败保留原指针不变。

**教训：** `realloc` 永远用临时指针接收。

**复习：** → [11.3 realloc](11.3-calloc和realloc.md)

</details>


### Q4: 内存对齐与 malloc

```c
struct Aligned {
    double d;    // 8 bytes, 要求 8 字节对齐
    int i;       // 4 bytes
};

struct Aligned *p = malloc(sizeof(struct Aligned));
// malloc 返回的地址是否一定满足 8 字节对齐？

char *buf = malloc(1);
// *buf 后面紧跟的内存属于谁？
```

> `malloc` 返回的指针是否保证对齐？越界写 `buf[1]` 会怎样？

<details>
<summary>答案与复习指引</summary>

**答案：** `malloc` 返回的指针保证对齐到 `max_align_t`（通常是 16 字节），满足任何标准类型的要求——`struct Aligned` 的 8 字节对齐没问题。

`buf[1]` 越界写——属于**堆元数据或下一个分配的块**。可能不立即崩溃，但破坏堆管理结构，导致后续 `malloc`/`free` 崩溃。这是**堆溢出**（heap overflow），安全漏洞的常见来源。

**规则：** 永远不越界访问 `malloc` 分配的内存；用 `valgrind` 或 ASan 检测。

**复习：** → [11.5 常见错误](./11.5-常见错误.md) · [11.1 动态内存分配](11.1-为什么使用动态内存分配.md)

</details>

### Q5: use-after-free 检测

```c
char *p = malloc(100);
strcpy(p, "hello");
free(p);

// 忘记 p = NULL;
char *q = p;          // 悬垂指针
printf("%s\n", q);    // 可能输出 "hello"，可能崩溃
```

> 为什么 `printf` 可能输出 "hello"？这段代码有什么隐患？

<details>
<summary>答案与复习指引</summary>

**答案：** `free` 释放内存但**不清零内容**——原来的 `"hello"` 可能还在那块内存中。`printf` 读悬垂指针——如果该内存还没被重新分配，可能"正常"输出 `"hello"`；如果已被重新分配覆盖，输出垃圾或崩溃。

**这是 use-after-free（UAF）**——最危险的内存错误之一：
- **不可预测**：取决于堆分配器的内部状态
- **安全漏洞**：攻击者可利用 UAF 读取敏感数据或控制执行流

**防护：** `free(p); p = NULL;` — 释放后立即置空。访问 NULL 会立即崩溃（容易发现），比 UAF 静默错误好得多。

**复习：** → [11.5 常见错误](./11.5-常见错误.md)

</details>


### Q3: 四大内存故障

```c
// 指出每种故障：
int *p = malloc(4);
free(p);
free(p);              // (1) 什么故障？

int *q = malloc(4);
// ... 忘记 free(q) ...  // (2) 什么故障？

int *r = malloc(4);
r[10] = 0;            // (3) 什么故障？

free(p);
*p = 42;              // (4) 什么故障？
```

<details>
<summary>答案与复习指引</summary>

**答案：**
1. **双重 free（double free）** → 堆元数据损坏
2. **内存泄漏（leak）** → 内存不释放，长时间运行 OOM
3. **堆越界（heap buffer overflow）** → 写到相邻堆块，可能被攻击利用
4. **use-after-free（UAF）** → 访问已释放内存

**工具：** valgrind / AddressSanitizer (ASan) 可检测以上所有问题。

**复习：** → [11.5 常见错误](./11.5-常见错误.md)

---

## 代码自测

**题目 1：** 以下代码有什么问题？
```c
int *p = malloc(10 * sizeof(int));
if (p == NULL) exit(1);
// 使用 p...
// 没有 free(p)
```

<details>
<summary>参考答案</summary>

内存泄漏。malloc 分配的内存必须用 free 释放，否则程序运行期间这块内存不会被回收。虽然程序退出时操作系统会回收所有内存，但长时间运行的程序（如服务器）中泄漏会累积导致 OOM。正确做法：使用完后 free(p); p = NULL;。

</details>
