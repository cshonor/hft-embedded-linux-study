# Ch10 完整总结 · GCC 内嵌汇编代码

> **《ARM64体系结构编程与实践》** · 奔跑吧Linux社区 · **精读**  
> [章导读](../README.md) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md) · [Pi5 适配](../../PI5-ADAPT.md)

---

## 本章定位

在 C 代码中嵌入汇编——内核驱动中大量使用的写法。学会约束、clobber list、goto 模板后，能读懂和编写内核工具函数。

---

## 10.1 基本语法

```c
asm volatile (
    "汇编指令\n\t"
    : 输出操作数        // 可选
    : 输入操作数        // 可选
    : clobber 列表      // 可选
    : goto 标签         // 可选（goto 模板）
);
```

| 部分 | 作用 |
|------|------|
| `asm` / `__asm__` | 关键字 |
| `volatile` / `__volatile__` | 禁止编译器优化/重排 |
| 输出操作数 | 汇编结果写回的 C 变量 |
| 输入操作数 | C 变量传入汇编 |
| clobber | 汇编修改了哪些寄存器/内存（编译器需要知道） |

---

## 10.2 约束字符 ⭐

| 约束 | 含义 | AArch64 示例 |
|------|------|-------------|
| `"r"` | 通用寄存器 | `x0-x30` |
| `"w"` | 浮点/SIMD 寄存器 | `v0-v31` |
| `"i"` | 立即数 | `#42` |
| `"I"` | 立即数（ADD/SUB 范围） | `0-4095` |
| `"J"` | 立即数（0，特殊） | `0` |
| `"L"` | 逻辑立即数 | `0xFF` 等 |
| `"m"` | 内存操作数 | `[x0]` |
| `"=&r"` | early-clobber 输出 | 避免与输入寄存器冲突 |

```c
// 简单加法
int add(int a, int b) {
    int result;
    asm volatile (
        "add %0, %1, %2"
        : "=r" (result)    // 输出：%0
        : "r" (a), "r" (b) // 输入：%1, %2
    );
    return result;
}
```

> `%0`、`%1`、`%2` 是操作数编号：输出在前，输入在后。

---

## 10.3 常用实战示例

### 读写系统寄存器宏

```c
#define read_sysreg(reg) ({                          \
    u64 val;                                         \
    asm volatile("mrs %0, " #reg : "=r"(val));       \
    val;                                             \
})

#define write_sysreg(val, reg) ({                     \
    asm volatile("msr " #reg ", %0" : : "r"(val));    \
})

// 使用
u64 el = read_sysreg(CurrentEL);
write_sysreg(0x0, DAIFSet);
```

### memset 实现

```c
void my_memset(void *dst, int c, size_t n) {
    asm volatile (
        "1: strb %w1, [%0], #1\n\t"   // 写 1 字节，后变基
        "subs %2, %2, #1\n\t"          // n--
        "b.ne 1b"                       // n≠0 继续
        : "+r"(dst)                    // 输入+输出：dst 会被修改
        : "r"(c), "r"(n)
        : "memory"                     // 修改了内存
    );
}
```

### 原子比较交换（CAS）

```c
bool cas(volatile u64 *addr, u64 old, u64 new) {
    u64 tmp;
    asm volatile (
        "1: ldxr %0, [%2]\n\t"
        "   cmp  %0, %3\n\t"
        "   b.ne 2f\n\t"
        "   stxr %w0, %4, [%2]\n\t"  // 复用 w0
        "   cbnz %w0, 1b\n\t"
        "2:"
        : "=&r"(tmp)
        : "r"(old), "r"(addr), "r"(old), "r"(new)
        : "memory", "cc"
    );
    return tmp == old;
}
```

---

## 10.4 clobber 列表 ⭐

告诉编译器：汇编代码除了输出操作数外，还修改了什么。

| clobber | 含义 |
|---------|------|
| `"memory"` | 修改了内存（编译器不能缓存内存读） |
| `"cc"` | 修改了条件标志（NZCV） |
| `"x0"` ... `"x30"` | 修改了指定寄存器 |

```c
// 汇编中用了 CMP，必须声明 "cc"
asm volatile (
    "cmp %0, %1"
    : 
    : "r"(a), "r"(b)
    : "cc"
);

// 汇编中写了内存，必须声明 "memory"
asm volatile (
    "str %1, [%0]"
    : 
    : "r"(ptr), "r"(val)
    : "memory"
);
```

> **忘加 clobber** 是内联汇编最危险的 bug：编译器可能把寄存器/内存值缓存到其他地方，导致数据不一致。

---

## 10.5 goto 模板

C 语言 `goto` 跳转 + 内联汇编，内核中用于分支预测优化。

```c
bool likely_true(int x) {
    bool result = true;
    asm goto (
        "cmp %w0, #0\n\t"
        "b.eq %l[zero]"         // 跳到 C 标签
        :
        : "r"(x)
        : "cc"
        : zero                   // C 标签名
    );
    return result;
zero:
    return false;
}
```

> `%l[label]` 在汇编中展开为对应的跳转标签地址。

---

## 10.6 实验要点

| 实验 | 内容 | 平台 |
|------|------|------|
| 10-1 | 实现简单的 memcpy 函数 | QEMU |
| 10-2 | 使用汇编符号名编写内嵌汇编 | QEMU |
| 10-3 | 完善 __memset_16bytes | QEMU |
| 10-4 | 内嵌汇编代码与宏 | QEMU |
| 10-5 | 读写系统寄存器的宏 | QEMU |
| 10-6 | goto 模板的内嵌汇编 | QEMU |

---

## 10.7 易错点清单

1. **忘加 `volatile`** → 编译器可能优化掉汇编代码（如果输出未被使用）。
2. **忘加 `"memory"` clobber** → 编译器缓存旧内存值，数据不一致。
3. **忘加 `"cc"` clobber** → 条件标志被改但编译器不知道，后续分支可能错误。
4. **输入和输出用了同一个寄存器** → 用 `"=&r"`（early-clobber）避免。
5. **约束选错** → `"r"` 是通用寄存器；SIMD 操作要 `"w"`。

---

## 书中思考题（自测）

1. `asm volatile` 中的 `volatile` 作用是什么？
2. `%0`、`%1` 编号规则是什么？输出和输入的编号顺序？
3. `"memory"` clobber 的含义？什么时候需要加？
4. 为什么要加 `"cc"` clobber？不加有什么后果？
5. goto 模板中 `%l[label]` 是什么意思？

**参考答案：**

1. 禁止编译器**优化或重排**这条汇编指令。  
2. **输出在前，输入在后**，从 0 开始编号。  
3. 告诉编译器汇编**修改了内存**，不能缓存旧值。写内存操作必须加。  
4. 汇编执行了 CMP/ADDS 等改 NZCV 的指令时必须加；不加 → 编译器可能误用旧标志值。  
5. 展开为 C 代码中对应 `goto` 标签的**跳转地址**。

---

上一章 [Ch9 链接器](../../chapter-09-linker-scripts/) · 下一章 [Ch11 异常处理](../../chapter-11-exception-handling/) · [OUTLINE](../../OUTLINE.md) · [全书总结](../../BOOK-SUMMARY.md)
