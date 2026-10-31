// 6.5 函数指针 —— 基础写法 / 回调 / qsort / 运行时选择 / 设备表
//
// 编译运行：
//   clang++ -std=c++17 -Wall 6.5-1-callback-basics.cpp -o /tmp/a && /tmp/a
//
// 关键实测（Apple clang 23.1.0, arm64）：
//   sizeof(void(*)(int))      = 8     —— 就是一个地址
//   sizeof(void(S::*)())      = 16    —— 成员函数指针是两倍大（含 this 调整/虚表偏移）
//   p(100) 与 (*p)(100)       完全等价
//   p == Hello                为 true（函数名自动退化为指针）

#include <cstdio>
#include <cstdlib>
#include <cstring>

// ============ ① 基础写法 ============
static void Hello(int x) { printf("  Hello: %d\n", x); }

static void (*g_p)(int);          // 定义：指向「参数 int、返回 void」的函数

static void basics() {
    printf("===== ① 基础写法 =====\n");
    g_p = Hello;                  // 函数名自动退化为地址，写 &Hello 也行
    printf("  g_p == Hello ? %s   sizeof(g_p)=%zu\n",
           g_p == Hello ? "true" : "false", sizeof(g_p));
    g_p(100);                     // 通过指针调用
    (*g_p)(200);                  // 显式解引用，完全等价
}

// ============ ② 回调：把函数当参数传进去 ============
// 遍历数组，对每个元素调用 fn —— apply 不知道 fn 做什么，只负责「在合适时机回头调」
static void apply(int* a, int n, int (*fn)(int)) {
    for (int i = 0; i < n; ++i) a[i] = fn(a[i]);
}
static int twice(int x) { return x * 2; }
static int neg(int x)   { return -x; }

static void callback() {
    printf("\n===== ② 回调（callback）=====\n");
    int a[] = {1, 2, 3, 4, 5};
    apply(a, 5, twice);
    printf("  apply(twice): %d %d %d %d %d\n", a[0], a[1], a[2], a[3], a[4]);
    apply(a, 5, neg);
    printf("  apply(neg)  : %d %d %d %d %d\n", a[0], a[1], a[2], a[3], a[4]);
    // 无捕获 lambda 可以隐式转成函数指针（有捕获的不行）
    apply(a, 5, [](int x) { return x + 100; });
    printf("  apply(lambda): %d %d %d %d %d\n", a[0], a[1], a[2], a[3], a[4]);
}

// ============ ③ qsort：C 标准库最经典的函数指针回调 ============
static int cmp_asc(const void* a, const void* b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y) - (x < y);          // 升序；改成 (x < y) - (x > y) 即降序
}
static int cmp_desc(const void* a, const void* b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x < y) - (x > y);
}

static void qsort_demo() {
    printf("\n===== ③ qsort：换比较器 = 换排序规则 =====\n");
    int a[] = {5, 2, 9, 1, 7};
    qsort(a, 5, sizeof(int), cmp_asc);
    printf("  升序: %d %d %d %d %d\n", a[0], a[1], a[2], a[3], a[4]);
    qsort(a, 5, sizeof(int), cmp_desc);
    printf("  降序: %d %d %d %d %d\n", a[0], a[1], a[2], a[3], a[4]);
    printf("  ↑ qsort 自己只管排序，比较逻辑是调用方传进来的\n");
}

// ============ ④ 运行时动态选择 ============
static void Add(int a, int b) { printf("  %d + %d = %d\n", a, b, a + b); }
static void Sub(int a, int b) { printf("  %d - %d = %d\n", a, b, a - b); }
static void Mul(int a, int b) { printf("  %d * %d = %d\n", a, b, a * b); }

static void runtime_pick(char c) {
    void (*op)(int, int) = nullptr;
    if      (c == '+') op = Add;
    else if (c == '-') op = Sub;
    else if (c == '*') op = Mul;
    else               op = nullptr;

    if (op) op(3, 5);                        // 判空再调用
    else    printf("  未知运算符 '%c'\n", c);
    // ⚠ 不判空直接 op(3,5) 且 op==nullptr → 实测 SIGSEGV（退出码 139）
}

// ============ ⑤ 设备表：C 语言靠函数指针做「面向对象」 ============
// 这是 Linux 内核 struct file_operations、嵌入式驱动的标准写法
struct Device {
    const char* name;
    int  (*open)(void);
    int  (*read)(char* buf, int n);
    int  (*write)(const char* buf, int n);
};

static int uart_open(void)                { printf("  [uart]  open\n");  return 0; }
static int uart_read(char* b, int n)      { (void)b; (void)n; printf("  [uart]  read\n");  return 0; }
static int uart_write(const char* b, int n){ (void)b; (void)n; printf("  [uart]  write\n"); return 0; }
static int spi_open(void)                 { printf("  [spi]   open\n");  return 0; }
static int spi_read(char* b, int n)       { (void)b; (void)n; printf("  [spi]   read\n");  return 0; }
static int spi_write(const char* b, int n){ (void)b; (void)n; printf("  [spi]   write\n"); return 0; }

static const Device kUart = {"uart", uart_open, uart_read, uart_write};
static const Device kSpi  = {"spi",  spi_open,  spi_read,  spi_write };

static void device_table() {
    printf("\n===== ⑤ 设备表：同一段代码驱动不同设备 =====\n");
    const Device* devs[] = {&kUart, &kSpi};
    for (int i = 0; i < 2; ++i) {          // 上层代码完全不知道具体是哪种设备
        printf("  --- %s ---\n", devs[i]->name);
        devs[i]->open();
        devs[i]->read(nullptr, 0);
        devs[i]->write(nullptr, 0);
    }
    printf("  ↑ 新增设备 = 新增一个 Device 实例，上层代码一行不改（开闭原则）\n");
}

// ============ ⑥ 函数指针数组 = 跳转表 ============
static void jump_table() {
    printf("\n===== ⑥ 函数指针数组（跳转表）=====\n");
    void (*ops[3])(int, int) = {Add, Sub, Mul};
    const char* names[3] = {"Add", "Sub", "Mul"};
    for (int i = 0; i < 3; ++i) {
        printf("  ops[%d] = %s: ", i, names[i]);
        ops[i](10, 4);
    }
    printf("  ↑ ops[k](...) 取代一长串 if/else；实测性能与 if-else 相当，价值在可维护性\n");
}

// ============ ⑦ 一个易混点：static 成员函数 vs 非静态 ============
struct S {
    static void sfn(int x) { printf("  static 成员: %d\n", x); }   // 没有 this
    void        mfn(int x) { printf("  非静态成员: %d\n", x); }    // 隐含 this
};
static void static_member() {
    printf("\n===== ⑦ static 成员函数 = 普通函数 =====\n");
    void (*p)(int) = &S::sfn;      // ✅ 可以：static 成员没有 this，就是普通函数
    p(1);
    // void (*q)(int) = &S::mfn;   // ❌ 编译错误：类型是 void (S::*)(int)，不是 void(*)(int)
    void (S::*pmf)(int) = &S::mfn; // ✅ 成员函数指针
    S obj; (obj.*pmf)(2);          // 必须通过对象调用
    printf("  sizeof(void(*)(int))   = %zu\n", sizeof(void(*)(int)));
    printf("  sizeof(void(S::*)(int)) = %zu  ← 成员函数指针更大\n", sizeof(void(S::*)(int)));
}

int main() {
    basics();
    callback();
    qsort_demo();
    printf("\n===== ④ 运行时选择 =====\n");
    runtime_pick('+');
    runtime_pick('*');
    runtime_pick('?');
    device_table();
    jump_table();
    static_member();
    return 0;
}
