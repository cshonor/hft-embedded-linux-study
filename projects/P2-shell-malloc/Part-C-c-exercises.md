# Part C — C 语言特性练手（轻量）

> 在 Part A/B 的代码上直接加，不单独建项目。每个练习 30 分钟内完成。
> 目标：把 01-c-language 书 02（指针）+ 书 04（GNU C）的技能点变成肌肉记忆，为 P2.5 铺路。

## 练习总览

| # | 练什么 | 在哪段代码上改 | 对应书 |
|---|--------|---------------|--------|
| 1 | 函数指针命令分发表 | shell 内置命令 if-else | K&R ch5 函数指针 |
| 2 | 调试分配器宏 | malloc 调用处 | K&R ch4 宏 |
| 3 | union 多态值类型 | shell 变量存储 | K&R ch6 联合 |
| 4 | offsetof 验证对齐 | malloc 头块 | C和指针 ch14 |
| 4 | likely/unlikely 热路径 | shell 解析器 | 嵌入式自我修养 ch06 |

---

## 练习 1：函数指针命令分发表

### 现状（Part A 代码）

```c
// builtin.c — 用 if-else 链判断内置命令
int try_builtin(char **argv, int argc) {
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    if (strcmp(argv[0], "cd") == 0) {
        // ...
    }
    if (strcmp(argv[0], "pwd") == 0) {
        // ...
    }
    if (strcmp(argv[0], "echo") == 0) {
        // ...
    }
    // 每加一个命令就加一个 if...
    return 0;
}
```

问题：命令一多就变成一长串 if-else，难维护，O(n) 查找。

### 改成函数指针表

```c
// builtin.c — 函数指针分发表

// 定义函数指针类型：内置命令函数的签名
typedef void (*builtin_fn)(char **argv, int argc);

// 命令表：名字 → 函数
struct builtin_entry {
    const char *name;
    builtin_fn fn;
};

// 各内置命令的实现
static void do_exit(char **argv, int argc)   { exit(0); }
static void do_cd(char **argv, int argc)     { /* chdir ... */ }
static void do_pwd(char **argv, int argc)    { /* getcwd ... */ }
static void do_echo(char **argv, int argc)   { /* printf ... */ }

// 分发表
static struct builtin_entry builtin_table[] = {
    { "exit", do_exit },
    { "cd",   do_cd   },
    { "pwd",  do_pwd  },
    { "echo", do_echo },
};
static const int builtin_count = sizeof(builtin_table) / sizeof(builtin_table[0]);

// 查找 + 执行
int try_builtin(char **argv, int argc) {
    for (int i = 0; i < builtin_count; i++) {
        if (strcmp(argv[0], builtin_table[i].name) == 0) {
            builtin_table[i].fn(argv, argc);
            return 1;
        }
    }
    return 0;
}
```

### 为什么这样做

1. **加命令只需加一行表项**——不用改 try_builtin 逻辑
2. **这就是 vtable 的雏形**——P2.5 的练习 5 会把它扩展成完整的虚函数表模式
3. **函数指针是 C 的多态**——Linux 内核的 `file_operations` 就是这个模式

### 练手的点

- 试着加一个 `export` 命令（设置环境变量），只改表项不改逻辑
- 把分发表按名字排序，用 `bsearch` 做 O(log n) 查找
- 理解 `builtin_fn` 是类型、`do_exit` 是值——函数名就是地址

### 卡住翻哪篇笔记

- K&R ch5.11 函数指针
- C和指针 ch13 高级指针话题（函数指针）

---

## 练习 2：调试分配器宏

### 现状

Part B 的 malloc 实现里直接调 `mymalloc(size)`。如果内存泄漏了，你不知道是谁分配的。

### 用宏包裹

```c
// debug_alloc.h
#ifndef DEBUG_ALLOC_H
#define DEBUG_ALLOC_H

#include <stdio.h>

// 正常版本
#ifdef DEBUG_ALLOC

// 调试版本：记录调用位置
#define mymalloc(sz)  debug_malloc((sz), __FILE__, __LINE__, __func__)
#define myfree(ptr)   debug_free((ptr), __FILE__, __LINE__, __func__)

void *debug_malloc(size_t sz, const char *file, int line, const char *func) {
    void *ptr = __real_malloc(sz);  // 调用真正的 malloc
    fprintf(stderr, "[ALLOC] %s:%d %s() → %p (size=%zu)\n",
            file, line, func, ptr, sz);
    return ptr;
}

void debug_free(void *ptr, const char *file, int line, const char *func) {
    fprintf(stderr, "[FREE]  %s:%d %s() ← %p\n",
            file, line, func, ptr);
    __real_free(ptr);
}

#endif // DEBUG_ALLOC

#endif
```

### 练的宏技巧

| 技巧 | 代码 | 说明 |
|------|------|------|
| `#` 字符串化 | `#sz` → `"sz"` | 把参数变成字符串字面量 |
| `##` 拼接 | `a##b` → `ab` | 拼接两个 token |
| 预定义宏 | `__FILE__` `__LINE__` `__func__` | 编译器自动填入调用位置 |
| 条件编译 | `#ifdef DEBUG_ALLOC` | 编译时决定是否启用调试 |

### 额外练习：字符串化

```c
#define STR(x)  #x
#define XSTR(x) STR(x)    // 两层：先展开 x 再字符串化

int value = 42;
printf("%s = %d\n", XSTR(value), value);
// 输出: value = 42

printf("%s\n", STR(value));
// 输出: value （不会展开，直接字符串化参数名）
```

理解为什么需要两层宏才能正确展开——这是宏编程的经典坑。

### 卡住翻哪篇笔记

- K&R ch4.11.2 宏替换
- K&R ch4.11.3 条件包含
- 嵌入式自我修养 ch06.6 宏

---

## 练习 3：union 多态值类型

### 背景

shell 可能需要支持变量：`x=42`、`y=3.14`、`z=hello`。一个变量可能是 int、float 或 string，C 没有 variant 类型。

### 用 union + tag 实现

```c
// variant.h

typedef enum {
    VAL_INT,
    VAL_FLOAT,
    VAL_STR,
} val_type;

typedef struct {
    val_type tag;          // 标记当前是什么类型
    union {
        int    ival;
        float  fval;
        char  *sval;
    };                      // 匿名 union（C11），直接用 .ival 访问
} varval;

// 设置
varval make_int(int v)    { return (varval){ .tag = VAL_INT,   .ival = v }; }
varval make_float(float v){ return (varval){ .tag = VAL_FLOAT, .fval = v }; }
varval make_str(char *v)  { return (varval){ .tag = VAL_STR,   .sval = v }; }

// 读取（带类型检查）
void print_varval(varval v) {
    switch (v.tag) {
        case VAL_INT:   printf("%d\n", v.ival); break;
        case VAL_FLOAT: printf("%f\n", v.fval); break;
        case VAL_STR:   printf("%s\n", v.sval); break;
        default:        printf("unknown type\n");
    }
}
```

### 练的点

1. **union 的内存布局**——所有成员共享同一块内存，`sizeof(union)` = 最大成员的大小
2. **tag 是必须的**——union 本身不知道当前存的是哪种类型，必须用外部标记
3. **这跟 C++ 的 variant 一样**——C++17 的 `std::variant` 本质上就是这个模式

### 验证内存布局

```c
printf("sizeof(varval) = %zu\n", sizeof(varval));
// 通常是 16 字节：4(tag) + 4(padding) + 8(union, 最大成员是 char* = 8)

varval v = make_int(42);
printf("tag=%d ival=%d fval=%f sval=%p\n",
       v.tag, v.ival, v.fval, (void*)v.sval);
// 你会看到 ival=42，fval 是乱码（42 的 float 解释），sval 是一个荒谬的地址
// 这就是 union 的本质：同一块内存，不同类型解释
```

### 卡住翻哪篇笔记

- K&R ch6.8 联合
- C和指针 ch10 结构体和联合

---

## 练习 4：offsetof 验证 malloc 头块对齐

### 背景

Part B 的 malloc 每个 block 有 Header（4 字节），payload 指针必须 8 字节对齐。用 `offsetof` 可以打印结构体成员的偏移量，验证布局是否符合预期。

### 代码

```c
#include <stddef.h>
#include <stdio.h>

// 模拟 malloc 的 block 结构
// 注意：这不是 malloc 的真实实现，是教学用的简化模型
typedef struct {
    unsigned int header;    // size + alloc flag (4 bytes)
    // 隐含 4 bytes padding（为了 payload 对齐到 8）
    char payload[8];        // 用户数据起始处
} block_meta;

// 真实的 block 没有 struct 定义（是裸内存操作）
// 但我们可以用 offsetof 验证手算的偏移是否正确

int main() {
    printf("offsetof(header)  = %zu\n", offsetof(block_meta, header));
    printf("offsetof(payload) = %zu\n", offsetof(block_meta, payload));
    printf("sizeof(block_meta)= %zu\n", sizeof(block_meta));
    printf("alignof(payload)  = %zu\n", _Alignof(block_meta));

    // 验证 Part B 的宏是否正确
    // HDRP(bp) = bp - 4  →  header 在 payload 前面 4 字节
    // 所以 payload 的偏移量应该 = 4（如果 header 是 4 字节）
    // 但由于对齐，实际偏移可能是 8

    // 如果 offsetof(payload) == 8，说明有 4 字节 padding
    // 那 HDRP(bp) = bp - 8，不是 bp - 4！
    // 这就是为什么 Part B 用 WSIZE=4 但实际布局可能有 padding

    return 0;
}
```

### 练的点

1. **`offsetof(type, member)`** 返回成员在结构体中的字节偏移
2. **结构体对齐规则**——编译器会在成员之间插入 padding，保证每个成员对齐到自身大小
3. **这正是 `container_of` 的基础**——P2.5 练习 1 会用 `offsetof` 从成员指针反推宿主结构体指针

### 思考题

```c
struct s1 { char a; int b; char c; };
struct s2 { int b; char a; char c; };
// sizeof(s1) = ? sizeof(s2) = ?
// 用 offsetof 验证你的答案
```

### 卡住翻哪篇笔记

- C和指针 ch14 结构体对齐
- 嵌入式自我修养 ch06.7 aligned / packed

---

## 练习 5：likely/unlikely 热路径标注

### 背景

shell 解析器的主循环里，有些分支几乎每次都走（比如"成功读取到 token"），有些极少走（比如"输入行太长，截断"）。用 `likely()`/`unlikely()` 告诉编译器哪个分支更可能执行，编译器会重新排列指令，让 CPU 分支预测更准。

### 代码

```c
// likely.h
#ifndef LIKELY_H
#define LIKELY_H

// GNU C 扩展：__builtin_expect 告诉编译器分支预测倾向
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)

#endif
```

### 在 shell 解析器里用

```c
// parser.c — 标注热路径

char *read_line(void) {
    static char buffer[MAXLINE];

    if (unlikely(fgets(buffer, MAXLINE, stdin) == NULL)) {
        // EOF — 极少发生
        return NULL;
    }

    size_t len = strlen(buffer);
    if (unlikely(len == MAXLINE - 1 && buffer[len-1] != '\n')) {
        // 行太长被截断 — 极少发生
        fprintf(stderr, "warning: line too long, truncated\n");
    }

    if (likely(len > 0 && buffer[len-1] == '\n')) {
        // 正常情况：有换行符
        buffer[len-1] = '\0';
    }

    return buffer;
}
```

### 验证编译器确实在优化

```bash
# 不加 likely/unlikely
gcc -O2 -S shell.c -o without.s

# 加了 likely/unlikely
gcc -O2 -S shell.c -o with.s

# 对比汇编
diff without.s with.s
# 你会看到分支跳转方向变了
```

### 练的点

1. **`__builtin_expect` 是 GCC 扩展**——标准 C 没有，但 Linux 内核到处用
2. **`!!(x)` 把任何值转成 0 或 1**——`!!NULL` = 0, `!!42` = 1
3. **这不是银弹**——只有热路径（循环里跑几百万次的分支）才有效果，冷代码标注没意义
4. **P2.5 的 ring buffer 会大量用到**——SPSC 队列的 push/pop 是最热的路径

### 卡住翻哪篇笔记

- 嵌入式自我修养 ch06.11.6 likely/unlikely
- CSAPP ch03 控制流（分支预测的基本原理）

---

## 完成后的衔接

做完这 5 个练习，你手上有了：

| 技能 | P2.5 哪里会用到 |
|------|----------------|
| 函数指针表 | 练习 5 vtable 模式——把表扩展成 `struct ops` + weak 符号 |
| 宏编程（`#`/`##`/`__LINE__`） | 练习 4 编译期宏——`BUILD_BUG_ON` / `ARRAY_SIZE` |
| union + tag | 练习 5 vtable 的多态——不同"驱动"共用接口 |
| offsetof + 对齐 | 练习 1 `container_of`——直接用 offsetof 算偏移 |
| likely/unlikely | 练习 3 ring buffer——SPSC 热路径 |

P2.5 就是把这些"在 P2 代码上贴的小补丁"变成**独立的、可复用的库**。

## 完成检查清单

- [ ] 练习 1：shell 内置命令改成函数指针表，加新命令只改表项
- [ ] 练习 2：`DEBUG_ALLOC` 宏开关，能打印每次 alloc/free 的调用位置
- [ ] 练习 3：shell 变量用 union 存储 int/float/string，tag 分发
- [ ] 练习 4：用 offsetof 打印 malloc 头块布局，验证对齐
- [ ] 练习 5：shell 解析器热路径加 likely/unlikely，对比汇编差异
