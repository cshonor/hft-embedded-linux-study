# P2.5 — C 系统编程工具箱（GNU C 桥梁）

> 用纯 C 实现内核/HFT 代码里天天用、但标准 C 教材不讲的宏和数据结构。
> 做完这个，读内核源码不再被 `container_of` / `list_head` / `__attribute__` 卡住。

## 项目目标

把 01-c-language 书 02（C 和指针）+ 书 04（嵌入式自我修养 GNU C）的技能点变成可复用代码库。
P2 练标准 C 够了，这个项目专门补 **GNU C 扩展 + 内核级数据结构模式**——P4 内核模块直接复用。

## 交付物

### 1. container_of 宏

- [ ] 用 `typeof` + 语句表达式 `({ ... })` 实现 `container_of(ptr, type, member)`
- [ ] 用 `offsetof` 验证正确性
- [ ] 写测试：从成员指针反推宿主结构体指针

### 2. 侵入式双向链表

- [ ] `struct list_head { struct list_head *next, *prev; }`（仿 Linux `list.h`）
- [ ] `list_add` / `list_del` / `list_for_each` / `list_entry`（用 container_of）
- [ ] 不用 container_of 的版本（直接算偏移），对比理解

### 3. SPSC 无锁环缓冲

- [ ] `struct ringbuf` + `head`/`tail` 原子索引
- [ ] `__attribute__((aligned(64)))` 缓存行对齐（防伪共享）
- [ ] `ringbuf_push` / `ringbuf_pop`（单生产者单消费者，无锁）
- [ ] 可变容量（运行时 `mmap` + `mlock`）

### 4. 编译期工具宏

- [ ] `BUILD_BUG_ON(cond)` — 编译期断言（变长数组负下标 trick）
- [ ] `ARRAY_SIZE(arr)` — 安全数组大小（`sizeof(arr)/sizeof(arr[0])` + 类型检查）
- [ ] `container_of` 的 `__same_type` 类型检查（`__builtin_types_compatible_p`）

### 5. 函数指针 vtable 模式

- [ ] `struct ops { int (*init)(void*); int (*read)(void*, char*, size_t); void (*close)(void*); }`
- [ ] 用 vtable 实现多态：两个"驱动"（file-backed + memory-backed）共用接口
- [ ] 弱符号 `__attribute__((weak))` 做默认实现覆盖

### 6. 结构体布局控制

- [ ] `__attribute__((packed))` vs 默认对齐对比（`sizeof` 差异）
- [ ] `__attribute__((aligned(N)))` 强制对齐
- [ ] `__attribute__((section("custom")))` 自定义 ELF 段 + 链接脚本读取
- [ ] flexible array member（`struct s { int n; int data[]; };`）

### 7. X-Macro 代码生成

- [ ] 用 X-macro 技术一次定义 enum + 字符串数组 + 打印函数
- [ ] 对比手写三遍的维护成本

### 8.（选做）内嵌汇编

- [ ] `asm volatile` 实现内存屏障（`mfence` / `dmb`）
- [ ] `asm volatile` 读取 CPU 时间戳（`rdtsc` / `cntvct_el0`）
- [ ] `asm goto`（GCC 扩展，内核 fast path 用）

## 覆盖模块

| 模块 | 用到什么 |
|------|----------|
| [`01` c-language](../../01-c-language/) | 指针运算、结构体布局、宏、函数指针、ABI |
| [`01`/04-Kernel-Prep](../../01-c-language/04-Kernel-Prep-Embedded-C-Self-Cultivation/) | GNU C 扩展：`typeof` / `__attribute__` / `container_of` / 内嵌汇编 / ELF 段 |

## 前置

[P2](../P2-shell-malloc/)（基本 C 指针 + 堆内存过关）。

## 学习目标

- `container_of` 不再是黑魔法——能从零写出来并解释每一行
- 侵入式链表 vs 非侵入式链表的取舍（内存、类型安全、复用性）
- 缓存行对齐为什么影响性能（`aligned(64)` + false sharing）
- 宏不是文本替换——`typeof` / 语句表达式 / `__builtin_*` 让宏接近类型安全
- 读内核 `list.h` / `container_of.h` / `#define` 不再卡壳

## 里程碑

1. **M1** container_of + intrusive list 跑通测试
2. **M2** ring buffer SPSC 无锁版跑通 + 缓存行对齐验证
3. **M3** 编译期宏 + vtable 模式跑通
4. **M4** 结构体布局实验 + X-macro 跑通
5. **M5**（选做）内嵌汇编屏障 + 时间戳读取

## 参考模块

- [01-c-language/02-Pointers-on-C/](../../01-c-language/02-Pointers-on-C/) — 指针 / 内存模型 / ABI
- [01-c-language/04-Kernel-Prep-Embedded-C-Self-Cultivation/](../../01-c-language/04-Kernel-Prep-Embedded-C-Self-Cultivation/) — GNU C 扩展（container_of / __attribute__ / 内嵌汇编 / ELF）
- [07-linux-kernel/](../../07-linux-kernel/) — 对照真实内核 `list.h` / `container_of.h`

## 环境

- WSL Ubuntu 24.04（gcc 13.3 + make）
- 编译：`gcc -std=gnu11 -Wall -Wextra -g`
- 验证对齐：`pahole` / `__alignof__` / `offsetof`
