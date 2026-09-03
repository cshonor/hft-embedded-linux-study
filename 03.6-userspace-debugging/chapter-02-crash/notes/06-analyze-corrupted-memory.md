# 2.6 深入内存分析（x 看内存 / 多线程 core / 反汇编 / 与 rr 互补）

> 🔴 精读 · 从「崩了」到「为什么崩」

## 本节要点

`bt full` 能定位「崩在哪一行」，但「为什么这里的数据坏了」往往要**直接看内存**——被溢出的缓冲区、被写坏的链表指针、被踩掉的结构体字段。本节用 `x` 命令直接检查内存、用反汇编钉死崩溃指令，并讲清 core 的边界：它是「崩溃瞬间快照」，缺「崩溃前历史」，这点 rr 正好互补。

## x 命令：直接看内存

`x`（examine）按指定格式把内存内容打印出来，是尸检的核心工具：

```
x/NFU <addr>
 N = 数量   F = 格式(x=hex d=dec s=string i=指令 a=addr)   U = 单位(b=1 w=4 g=8)
```

```gdb
(gdb) x/4gx &head           # 4 个 8 字节，十六进制，看 head 指针附近的原始字节
(gdb) x/s p->name           # 以字符串读 p->name 指向的内容
(gdb) x/16bx p->buf         # 16 个单字节，看缓冲区实际内容
(gdb) x/i main+40           # 反汇编一条指令
```

## 实战：缓冲区溢出写坏链表

下面这个程序 `buf[8]` 太小，`strcpy` 写了 20 字节，**溢出覆盖了紧邻的 `next` 指针**，遍历到一半崩溃：

```c
// corrupt_list.c —— 缓冲区溢出破坏链表
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct node {
    int  id;
    char buf[8];        // 太小！
    struct node *next;
} node_t;

int main(void) {
    node_t *a = malloc(sizeof(node_t));
    node_t *b = malloc(sizeof(node_t));
    node_t *c = malloc(sizeof(node_t));
    a->id = 1; a->next = b;
    b->id = 2; b->next = c;
    c->id = 3; c->next = NULL;

    strcpy(a->buf, "0123456789ABCDEFGHIJ");   // 20 字节，溢出覆盖 a->next

    for (node_t *p = a; p; p = p->next)       // 遍历：a->next 已被破坏
        printf("id=%d\n", p->id);
    return 0;
}
```

```bash
gcc -g -O0 -o corrupt_list corrupt_list.c
ulimit -c unlimited && ./corrupt_list
# id=1
# Segmentation fault (core dumped)
gdb ./corrupt_list core
```

```gdb
(gdb) bt
#0  main () at corrupt_list.c:23
23          for (node_t *p = a; p; p = p->next)

(gdb) info locals
a = 0x5555555592a0
b = 0x5555555592c0
c = 0x5555555592e0
p = 0x3433323130393837        # ← p 的值是 ASCII 字符！0x37='7' 0x38='8'... = "7890..."
```

`p` 的值 `0x3433323130393837` 一看就是 ASCII——这正是被 `strcpy` 写进 `a->next` 的字符串字节（`"7890"` 段）。用 `x` 坐实：

```gdb
(gdb) p a
$1 = (node_t *) 0x5555555592a0
(gdb) x/4gx a                  # 看 a 节点原始内存布局
0x5555555592a0: 0x0000000000000001      0x3433323130393837
                 # ↑ id=1                    ↑ next 字段 = ASCII "78901234" ← 被覆盖！
0x5555555592b0: 0x4544434241393837      0x0000000000000000
                 # ↑ 溢出的后续字节
(gdb) x/s a->buf
0x5555555592a4: "0123456789ABCDEFGHIJ"
```

铁证链完整：`strcpy` 往 `buf`（8 字节）写了 20 字节，多出的字节覆盖了 `next` 字段，`next` 变成了字符串 `"78901234"` 的 ASCII 值，遍历时 `p = p->next` 跳到非法地址 → 段错误。

### 关键：struct 内存布局决定「溢出打到谁」

```gdb
(gdb) ptype /o node_t         # /o 显示字段偏移
type = struct node {
/*    0      */    int id;        // offset 0
/*    4      */    char buf[8];   // offset 4
/*   12      */    int :0;        // padding 到 8 对齐（next 需 8 字节对齐）
/*   16      */    struct node *next;  // offset 16
}          /* total size 24 */
```

`buf` 在 offset 4、`next` 在 offset 16。写进 `buf` 的第 13～20 字节正好落在 `next` 上——这就是「溢出覆盖 next」的布局依据。`ptype /o` 能精确告诉你溢出会打到哪个字段。

## 多线程 core：所有线程的崩溃现场

core 不只含崩溃线程，**所有线程的栈和寄存器都在**：

```gdb
(gdb) info threads
  Id   Target Id                          Frame
* 1    Thread ... (LWP 12345)  consumer () at orderbook_mt.c:24
  2    Thread ... (LWP 12346)  producer () at orderbook_mt.c:16

(gdb) thread apply all bt        # 每个线程崩溃瞬间在干嘛
Thread 1 (LWP 12345):
#0  consumer () at orderbook_mt.c:24   # ← 崩溃线程，遍历到野指针
Thread 2 (LWP 12346):
#0  producer () at orderbook_mt.c:16   # ← 另一个线程还在插单
```

多线程崩溃的关键洞察：**崩溃线程往往只是受害者**，真正的破坏者（并发写坏数据的线程）在别的线程栈里能看到。配合 Ch2 的数据竞争分析，core 提供的是「崩溃瞬间的全线程定格」。

## core 的边界 vs rr 的互补

core 是强大但**静态**的：

| 能力 | core | rr |
|------|------|-----|
| 崩溃瞬间快照 | ✅ 完整内存+寄存器+全线程 | ✅（也能导出） |
| 崩溃前执行历史 | ❌ 没有 | ✅ 完整可倒带 |
| 反向追「谁改坏了我」 | ❌ 只能靠猜/加日志重跑 | ✅ `reverse-continue`+`watch` |
| 复现成本 | 零（事后分析） | 需 record 阶段开销 |
| 生产环境友好度 | ✅ 低开销、天然可用 | ⚠️ record 开销 + perf 权限 |

**结论**：先 core 定位「崩在哪、什么坏了」，若需追「怎么坏的过程」，再上 rr 复现倒带。两者是「快照」与「录像」的互补，不是替代。

## HFT 关联

1. **`x` 命令识别内存破坏形态**：`p`/`next` 指针值出现 ASCII 可打印字符（如 `0x34333231`）→ 几乎必是字符串溢出；出现 `0x41414141`（"AAAA"）→ 恶意/测试填充；出现 `0xdeadbeef` → 常见哨兵值。一眼定性破坏来源。
2. **`ptype /o` 看字段偏移**：判断「溢出会打到哪个字段」，是定位内存越界的关键一步；交易系统里结构体打包（`__attribute__((packed))`）后偏移更刁钻，必须实查不猜。
3. **多线程 core 找「破坏者」**：崩溃线程是受害者，`thread apply all bt` 找并发写共享数据的线程，还原竞态。
4. **core + rr 双保险**：生产开 core（低开销兜底），疑难偶发崩溃再上 rr 复现倒带。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 崩溃时局部变量 `p` 的值是 `0x3433323130393837`，这能说明什么？

> 这个十六进制值转成 ASCII 是 `"78901234"`——说明 `p` 这个指针字段被一段字符串覆盖了。几乎可以断定是**缓冲区溢出**（某个相邻的 `char[]` 写超了，把紧邻的指针字段冲掉）。指针值出现可打印 ASCII 是溢出破坏的经典签名。

**Q2:** `ptype /o` 的作用是什么？为什么分析溢出要用它？

> `ptype /o` 打印结构体的**字段偏移和总大小**。溢出分析要靠它确定「越界写的字节会落到哪个字段」——本例 `buf` 在 offset 4、`next` 在 offset 16，写进 buf 的第 13~20 字节正好覆盖 next。不查偏移，就说不清「为什么溢出打到的是 next 而不是别的」。

**Q3:** 多线程 core 里，崩溃线程一定是「根因线程」吗？

> 不一定。崩溃线程往往是踩到坏数据的**受害者**，真正的破坏者（并发写坏共享数据/链表的线程）可能还在别的线程栈里正常执行。所以多线程 core 要用 `thread apply all bt` 看**所有**线程，结合 Ch2 的数据竞争分析找并发写入者。

**Q4:** core 和 rr 的核心区别？什么场景必须上 rr？

> core 是崩溃**瞬间的静态快照**（内存+寄存器+全线程），没有崩溃前的执行历史；rr 记录了完整执行轨迹，可前进可倒带。需要追「这个指针是什么时候、被哪一行改坏的」这类**过程性**问题，core 只能靠猜或加日志重跑，必须上 rr 的 `reverse-continue`+`watch` 反向追。

**Q5:** 指针值出现 `0xdeadbeef` 或 `0x41414141` 通常意味着什么？

> `0xdeadbeef` 是程序员常用的「哨兵值」（标记已释放/未初始化内存，如 glibc 的 `MALLOC_PERTURB_` 也常见）；`0x41414141` 是 ASCII `"AAAA"`，常见于测试填充或缓冲区溢出攻击载荷。识别这些「魔法值」能快速定性内存是被谁、以什么方式污染的。

</details>

## 交叉引用

- [2.4 core 文件生成配置](04-core-dump-config.md)
- [2.5 加载 core 回溯](05-load-core-backtrace.md)
- [4.2 rr 可逆调试](../../chapter-04-concurrency/notes/02-rr-reversible-debugging.md)
- [03.6 模块导读](../../README.md)
