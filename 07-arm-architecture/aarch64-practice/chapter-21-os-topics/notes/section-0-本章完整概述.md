# Ch21 完整总结 · 操作系统相关话题

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

用前面学到的 A64 指令、异常、MMU 知识，在 BenOS 上实现简易进程管理：PCB、fork、调度、上下文切换、自定义 syscall。是从裸机到 OS 的桥梁。

---

## 21.1 AArch64 C 语言陷阱

### 栈对齐

AAPCS64 要求 SP **16 字节对齐**。编译器自动保证，但裸机/汇编中手动操作需注意。

### 函数指针

```c
void (*handler)(void) = irq_handler;  // 函数指针存地址
handler();  // BLR 调用
```

### 可变参数

AArch64 可变参数：前 8 个参数在 X0-X7，超出部分走栈。`va_list` 实现复杂（涉及栈保存区）。

### volatile

裸机/驱动中 `volatile` 必不可少——MMIO 寄存器、中断共享变量。

```c
volatile uint32_t *uart_dr = (uint32_t *)0x09000000;
*uart_dr = 'A';  // 每次都真正写内存，不被优化
```

---

## 21.2 调用约定与栈帧 ⭐

### AAPCS64 栈帧结构

```
高地址
┌──────────────────┐
│ 调用者保存的寄存器  │  (X19-X28, 如果用了)
├──────────────────┤
│ 局部变量           │
├──────────────────┤
│ ...               │
├──────────────────┤
│ FP (X29) →────────├── 帧指针
├──────────────────┤
│ LR (X30)          │  返回地址
├──────────────────┤
│ ...               │
低地址 (SP)
```

```asm
; 典型函数入口/出口
func:
    stp x29, x30, [sp, #-16]!   ; 保存 FP + LR
    mov x29, sp                  ; 设新 FP
    sub sp, sp, #32              ; 局部变量空间
    ; ... 函数体 ...
    add sp, sp, #32              ; 恢复 SP
    ldp x29, x30, [sp], #16     ; 恢复 FP + LR
    ret
```

> `X29`(FP) 和 `X30`(LR) 是栈回溯的关键：从 FP 沿链表回溯可打印完整调用栈。

---

## 21.3 进程控制块（PCB）

```c
struct task_struct {
    uint64_t cpu_context[8];  // 保存 X19-X28（callee-saved）
    uint64_t sp;              // 保存 SP
    uint64_t pc;              // 保存 PC（恢复点）
    int pid;                  // 进程 ID
    int state;                // RUNNING / SLEEPING / ZOMBIE
    struct task_struct *next; // 调度链表
};
```

> 上下文切换只需保存 **callee-saved 寄存器**（X19-X28）+ SP + PC。  
> caller-saved（X0-X18）调用者自己保存，切换时不需要管。

---

## 21.4 0 号进程与 do_fork ⭐

### 0 号进程（idle process）

```c
// 0 号进程：内核启动后第一个"进程"
struct task_struct *task0;
task0->pid = 0;
task0->state = RUNNING;
task0->pc = (uint64_t)cpu_idle;  // 死循环

void cpu_idle(void) {
    while (1) {
        wfe;  // 低功耗等待
    }
}
```

### do_fork

```c
int do_fork(void (*fn)(void)) {
    struct task_struct *new = alloc_task();
    new->pid = next_pid++;
    new->state = RUNNING;

    // 设置栈（给新进程分配栈空间）
    uint64_t *sp = alloc_stack();
    *sp-- = (uint64_t)fn;     // 返回地址（ret 后跳到 fn）
    new->sp = (uint64_t)sp;

    // 加入调度队列
    enqueue_task(new);
    return new->pid;
}
```

---

## 21.5 上下文切换 ⭐

```c
// 从 prev 切换到 next
void switch_to(struct task_struct *prev, struct task_struct *next) {
    // 1. 保存 prev 的 callee-saved 寄存器
    //    （在汇编中做，C 看不到寄存器）
    // 2. 保存 prev 的 SP
    // 3. 加载 next 的 SP
    // 4. 加载 next 的 callee-saved 寄存器
    // 5. RET → 跳到 next 的 PC
}
```

```asm
; 汇编实现
switch_to:
    ; x0 = prev, x1 = next
    ; 保存 prev 的 callee-saved
    stp x19, x20, [x0, #0]
    stp x21, x22, [x0, #16]
    stp x23, x24, [x0, #32]
    stp x25, x26, [x0, #48]
    stp x27, x28, [x0, #64]
    str x29,      [x0, #80]   ; FP
    str x30,      [x0, #88]   ; LR (= 切换返回点)
    ; 保存 SP
    mov x2, sp
    str x2,       [x0, #96]

    ; 加载 next 的 callee-saved
    ldp x19, x20, [x1, #0]
    ldp x21, x22, [x1, #16]
    ldp x23, x24, [x1, #32]
    ldp x25, x26, [x1, #48]
    ldp x27, x28, [x1, #64]
    ldr x29,      [x1, #80]
    ldr x30,      [x1, #88]   ; RET 后跳回 next 上次停的地方
    ; 加载 SP
    ldr x2,       [x1, #96]
    mov sp, x2

    ret  ; → x30 = next 的 PC
```

> **核心**：切换 SP + callee-saved + LR。RET 后自然跳到 next 上次被切走时的位置。

---

## 21.6 简易调度器

```c
struct task_struct *current;
struct task_struct *run_queue;  // 循环链表

void schedule(void) {
    struct task_struct *prev = current;
    current = current->next;   // 轮转调度
    switch_to(prev, current);
}

// 定时器中断触发调度
void timer_irq_handler(void) {
    // 清中断
    // ...
    schedule();  // 切换到下一个进程
}
```

---

## 21.7 自定义系统调用 ⭐

BenOS 可以定义自己的 SVC 系统调用：

```c
// 用户态调用
int my_malloc(size_t size) {
    // x8 = syscall number, x0 = 参数
    register long x8 asm("x8") = 1;  // SYS_malloc
    register long x0 asm("x0") = size;
    asm volatile("svc #0" : "+r"(x0) : "r"(x8) : "memory");
    return x0;  // 返回值
}
```

```asm
; SVC 异常处理
svc_handler:
    ; 保存现场
    ; ...
    ; 读系统调用号
    ldr x8, [sp, #64]      ; x8 在栈上的位置
    ; 分发
    cmp x8, #1
    b.eq sys_malloc
    cmp x8, #2
    b.eq sys_clone
    ; ...
sys_malloc:
    bl  do_malloc          ; 调用 C 实现
    ; 恢复现场
    ; ...
    eret
```

### malloc 系统调用

```c
long sys_malloc(size_t size) {
    void *p = simple_alloc(size);  // 简单 bump allocator
    return (long)p;
}
```

### clone 系统调用

```c
long sys_clone(void (*fn)(void)) {
    return do_fork(fn);  // 创建新进程
}
```

---

## 21.8 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 21-1 | 观察栈布局（GDB 看栈帧） | QEMU |
| 21-2 | 进程创建（do_fork） | QEMU |
| 21-3 | 进程调度（轮转 + 定时器） | QEMU |
| 21-4 | 新增 malloc 系统调用 | QEMU |
| 21-5 | 新增 clone 系统调用 | QEMU |

---

## 21.9 易错点清单

1. **上下文切换没保存 callee-saved** → 新进程使用 prev 的寄存器值，数据损坏。
2. **SP 没切换** → 两个进程用同一个栈，互相覆盖。
3. **0 号进程的 PC 设错** → 启动就跳飞。
4. **调度器在中断外调用** → BenOS 简化版可以，但真正的 Linux 只在中断/特定点调度。
5. **syscall 返回值没正确传回 x0** → 用户态拿到垃圾值。

---

## 书中思考题（自测）

1. 上下文切换需要保存哪些寄存器？为什么只保存 callee-saved？
2. do_fork 做了什么？新进程的栈怎么设置？
3. 0 号进程的作用是什么？
4. 自定义系统调用的流程是什么？参数和返回值怎么传？
5. 栈帧中 FP(X29) 和 LR(X30) 的作用？

**参考答案：**

1. **X19-X28 + SP + PC(LR)**。caller-saved 由调用者自己保存，切换时不需要管。  
2. 分配 PCB → 分配栈 → 栈顶放函数入口 → 加入调度队列。新进程栈顶 = fn 地址，RET 后跳到 fn。  
3. 内核启动后第一个进程，**idle 循环**（WFE 低功耗），没有其他进程可运行时回到它。  
4. 用户设 X8=调用号、X0-X5=参数 → SVC → 异常处理读 X8 分发 → 调用 C 实现 → 返回值写 X0 → ERET。  
5. FP=帧指针（栈回溯链表）；LR=返回地址（RET 跳回调用者）。

---

上一章 [Ch20 原子操作](../../chapter-20-atomic-operations/) · 下一章 [Ch22 NEON](../../chapter-22-fp-neon/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
