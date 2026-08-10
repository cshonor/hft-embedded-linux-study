# §21.4 0 号进程与 do_fork ⭐

> **来源：** [Ch21 完整概述](section-0-本章完整概述.md) · [章导读](../README.md)

## 本节讲什么

0 号进程（idle process）是内核启动后的第一个"进程"，当没有其他进程可运行时回到它。do_fork 创建新进程：分配 PCB、分配栈、设置初始 SP 和 PC、加入调度队列。

## 核心要点

### 0 号进程（idle process）

```c
// 0 号进程：内核启动后第一个"进程"
struct task_struct *task0;
task0->pid = 0;
task0->state = RUNNING;
task0->pc = (uint64_t)cpu_idle;  // 死循环

void cpu_idle(void) {
    while (1) {
        wfe;  // 低功耗等待事件
    }
}
```

> 0 号进程永不退出，没有可运行进程时调度器切回它。WFE 让 CPU 进入低功耗状态等待中断唤醒。

### do_fork 流程

```c
int do_fork(void (*fn)(void)) {
    struct task_struct *new = alloc_task();
    new->pid = next_pid++;
    new->state = RUNNING;

    // 设置栈（给新进程分配栈空间）
    uint64_t *sp = alloc_stack();
    *sp-- = (uint64_t)fn;     // 栈顶放 fn 地址（ret 后跳到 fn）
    new->sp = (uint64_t)sp;

    // 加入调度队列
    enqueue_task(new);
    return new->pid;
}
```

### 新进程的"巧妙"栈设计

| 步骤 | 内容 | 目的 |
|------|------|------|
| 1 | 分配新栈 | 每个进程独立栈空间 |
| 2 | 栈顶放 fn 地址 | switch_to 的 RET 跳到 fn |
| 3 | SP 指向栈顶-8 | 模拟"函数调用后准备返回"的状态 |
| 4 | PCB 的 LR = fn | 等价于"上次从 fn 返回" |

```
新进程栈初始状态：
高地址
┌──────────────┐
│ fn 地址      │ ← SP 指向这里
│ (未使用)     │
│ (未使用)     │
└──────────────┘
低地址

switch_to 加载 LR = fn → RET → 跳到 fn 开始执行
```

> 新进程从未执行过，没有"上次被切走的位置"。把 fn 放在栈顶模拟"LR = fn"，switch_to 恢复 LR 后 RET 自然跳到 fn。

### do_fork 完整初始化代码

```c
int do_fork(void (*fn)(void *), void *arg) {
    struct task_struct *new = alloc_task();
    if (!new) return -1;

    new->pid = next_pid++;
    new->state = TASK_RUNNING;

    // 分配栈（4KB 对齐）
    uint8_t *stack = alloc_pages(1);  // 1 page = 4KB
    if (!stack) {
        free_task(new);
        return -1;
    }

    // 栈从高地址增长，SP 指向栈顶
    uint64_t *sp = (uint64_t *)(stack + PAGE_SIZE);

    // 模拟"刚被 schedule() 调用准备返回"的状态
    // 这样 switch_to RET 后会跳到 ret_from_fork
    *--sp = (uint64_t)arg;          // fn 的参数（X0）
    *--sp = (uint64_t)ret_from_fork; // 返回地址（LR）

    new->sp = (uint64_t)sp;
    new->pc = (uint64_t)ret_from_fork;

    // callee-saved 全部清零（新进程没有历史值）
    memset(new->cpu_context, 0, sizeof(new->cpu_context));

    enqueue_task(new);
    return new->pid;
}

// fork 后的入口包装
void ret_from_fork(void) {
    // 从栈上恢复 fn 和 arg
    // 调用 fn(arg)
    // fn 返回后调用 exit()
    schedule();  // fn 返回 → 永不返回
}
```

### BenOS do_fork vs Linux copy_process

| 维度 | BenOS do_fork | Linux copy_process |
|------|---------------|-------------------|
| 地址空间 | 共享（单一地址空间） | COW 复制（copy_mm） |
| 文件描述符 | 无 | 复制 files_struct |
| 信号处理 | 无 | 复制 sighand_struct |
| 页表 | 无 | COW 页表（copy_pte_range） |
| 栈 | 全新分配 | COW 复制父进程栈 |
| 调度信息 | 直接 RUNNING | 继承 nice/优先级，vruntime 调整 |
| 用途 | 创建内核线程 | 创建用户进程/线程 |

### 协程类比

```c
// 协程库（如 Boost.Context）的创建与 do_fork 思路一致：
// 1. 分配栈
// 2. 栈顶放入口函数
// 3. 通过 switch_to 切换

typedef struct {
    void *sp;       // 保存 SP
    void *pc;       // 保存 PC
    // ... callee-saved
} coroutine_t;

coroutine_t *coro_create(void (*fn)(void)) {
    coroutine_t *c = malloc(sizeof(*c));
    void *stack = malloc(STACK_SIZE);
    void *sp = (char *)stack + STACK_SIZE;

    // 模拟 do_fork 的栈设计
    sp -= sizeof(void *);
    *(void **)sp = fn;    // 入口函数
    c->sp = sp;
    c->pc = fn;
    return c;
}
```

## HFT 关联

HFT 系统中 `fork()` 几乎不使用（fork + exec 的 COW 开销太大），而是用 `pthread_create` 或 `clone()` 共享地址空间。但 do_fork 的栈设计思路对理解协程（coroutine）有启发：协程库（如 Boost.Context）创建协程时也是在栈顶放入口函数地址，然后通过 `switch_to` 类似的汇编切换。

```c
// HFT 协程式用户态调度
// 交易线程 A 处理完一个包后 yield → 交给 IO 线程 B
// 不经过内核调度，切换开销 ~50ns（vs 内核切换 ~1μs）

// HFT 线程创建（避免 fork）
void hft_create_thread(void (*fn)(void *)) {
    pthread_t tid;
    pthread_attr_t attr;
    cpu_set_t cpuset;

    pthread_attr_init(&attr);
    // 绑核
    CPU_SET(DEDICATED_CPU, &cpuset);
    pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
    // 设置栈大小
    pthread_attr_setstacksize(&attr, 64 * 1024);  // 64KB 足够
    // 创建
    pthread_create(&tid, &attr, (void *(*)(void *))fn, NULL);
    pthread_attr_destroy(&attr);
}
```

## 自测题

1. **新进程的栈顶为什么放 fn 的地址？**

<details>
<summary>答案</summary>

因为 switch_to 的最后一条指令是 `RET`（跳到 X30/LR）。对于新进程，它从未执行过，没有"上次被切走的返回点"。把 fn 放在栈顶（模拟 LR 的位置），switch_to 从 PCB 恢复 LR 时读到 fn 地址，RET 后自然跳到 fn 开始执行。这是一种"伪造返回地址"的技巧——让新进程的首次调度看起来像是从 switch_to "返回"到 fn。
</details>

2. **0 号进程为什么用 WFE 而不是 while(1)?**

<details>
<summary>答案</summary>

`WFE`（Wait For Event）让 CPU 进入**低功耗等待状态**，直到有事件（如中断）唤醒。纯 `while(1)` 会让 CPU 全速空转，浪费功耗和热量。WFE 在等待中断时几乎不消耗功耗——当定时器中断到来时，CPU 自动唤醒（SEV 由中断触发），调度器可以切换到新就绪的进程。这在嵌入式和服务器场景中都很有意义。
</details>

3. **do_fork 分配的栈和父进程的栈有什么关系？**

<details>
<summary>答案</summary>

BenOS 的 do_fork 分配的是**全新栈**，不复制父进程栈内容。这与 Linux 的 `fork()` 不同——Linux fork 会 COW（Copy-On-Write）复制父进程的整个地址空间（包括栈）。BenOS 的 do_fork 更像 Linux 的 `kernel_thread()`——创建一个内核线程，从指定函数开始执行，没有父进程栈数据的继承。新进程从 fn 开始独立执行。
</details>

4. **如果 do_fork 中忘了初始化 PCB 的 LR/PC 字段会怎样？**

<details>
<summary>答案</summary>

PCB 的 LR 字段未初始化（可能是 0 或垃圾值）。新进程被调度后，switch_to 从 PCB 加载 LR = 0（或垃圾值），然后 RET 跳到地址 0 → 立即触发对齐异常或取指异常 → 系统崩溃。正确做法是把 fn 地址写入 PCB 的 LR 字段（或栈顶），switch_to RET 后跳到 fn 开始执行。alloc_task() 中如果用了 memset 清零，LR 默认为 0，必须显式设置。
</details>

## 参考与延伸

- [§21.3 进程控制块 PCB](03-pcb.md) — PCB 结构定义
- [§21.5 上下文切换](05-context-switch.md) — switch_to 如何恢复新进程
- [§21.6 简易调度器](06-scheduler.md) — do_fork 后如何加入调度队列
