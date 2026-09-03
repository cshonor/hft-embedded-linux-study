# 2.2 断点与观察点（break / 条件断点 / watchpoint）

> 🔴 精读 · 用户态正确性调试

## 本节要点

断点（breakpoint）让程序在**特定位置**停下，观察点（watchpoint）让程序在**某个变量被读写**时停下。两者是定位 bug 的两把刀：断点回答「程序走到这行时状态对不对」，观察点回答「这个变量是谁、在哪一行改坏的」。本节覆盖断点的所有形态 + 条件断点 + 硬件断点/观察点，并用 2.1 的 `orderbook` 把段错误精确钉到某一行。

## 断点的三种指定方式

```gdb
(gdb) break main               # 按函数名
(gdb) break 42                 # 按行号（当前文件第 42 行）
(gdb) break orderbook.c:21     # 按 文件:行号
(gdb) break add_order          # 停在 add_order 入口（进入函数体第一行）
(gdb) info breakpoints         # 查看所有断点（简写 i b）
```

以 `orderbook` 为例，我们想知道 `*p = 42` 是不是真的把程序搞崩了：

```bash
gcc -g -O0 -o orderbook orderbook.c
gdb ./orderbook
(gdb) break main
Breakpoint 1 at 0x11e1: file orderbook.c, line 21.
(gdb) run
Breakpoint 1, main () at orderbook.c:21
21      int main(void) {
(gdb) list
21      int main(void) {
22          add_order(1, 100.5, 100);
...
```

### 断点命中后的常用动作

| 命令 | 作用 |
|------|------|
| `continue`（`c`） | 继续跑到下一个断点或结束 |
| `next`（`n`） | 单步，**不进入**函数内部 |
| `step`（`s`） | 单步，**进入**被调函数内部 |
| `finish` | 跑完当前函数，回到调用者 |
| `until <行号>` | 跑到指定行（跳过中间的循环） |
| `delete 1` | 删除 1 号断点 |
| `disable 1` / `enable 1` | 停用 / 启用 1 号断点 |

## 条件断点：只在满足条件时停

交易系统里循环可能跑几百万次，无条件断点会把你淹没。条件断点只在表达式为真时停：

```gdb
(gdb) break print_orders if p->qty < 0
Breakpoint 2 at 0x122f: file orderbook.c, line 17.
(gdb) run
Breakpoint 2, print_orders () at orderbook.c:17
17          printf("id=%d price=%.2f qty=%ld\n", p->id, p->price, p->qty);
(gdb) print p->qty
$1 = -1        # ← 第 3 笔订单的 qty=-1 被当场抓住
```

> ⚠️ 条件断点每次命中都要让 gdb 停下、求值表达式、再继续，**循环热点里会拖慢一个数量级**。若条件极罕见，优先用「普通断点 + `ignore` 跳过前 N 次」或硬件断点，见下文。

### ignore：跳过前 N 次命中

```gdb
(gdb) ignore 1 1000    # 1 号断点前 1000 次命中不暂停
```

## 硬件断点 vs 软件断点

| 维度 | 软件断点（`break`） | 硬件断点（`hbreak`） |
|------|---------------------|----------------------|
| 实现 | 把目标地址首字节替换成 `int3`（0xCC）陷阱指令 | 用 CPU 调试寄存器（DR0–DR3） |
| 数量 | 不限 | 最多 **4 个**（x86，由硬件决定） |
| 适用 | 内存中的代码 | **只读代码**（ROM/Flash）、**自修改代码** |
| 副作用 | 会改动代码字节 | 无（不碰内存） |

嵌入式场景下，代码可能跑在只读 Flash 里，软件断点写不进 `int3`，必须用 `hbreak`：

```gdb
(gdb) hbreak add_order   # 硬件断点，不修改代码字节
```

## 观察点（watchpoint）：变量被改时停下

断点管「位置」，观察点管「数据」。定位「`head` 这个指针是在哪一步被写坏的」这类问题，观察点是最强武器：

```gdb
(gdb) watch head         # head 被写时停下（写观察点）
(gdb) rwatch *p          # *p 被读时停下（读观察点）
(gdb) awatch qty         # qty 被读或写都停下（读写观察点）
```

```bash
gdb ./orderbook
(gdb) watch head
Hardware watchpoint 3: head
(gdb) run
Hardware watchpoint 3: head
Old value = (order_t *) 0x0
New value = (order_t *) 0x5555555592a0
add_order (id=1, price=100.5, qty=100) at orderbook.c:13
13          o->next = head; head = o;
```

> 观察点默认走**硬件调试寄存器**，所以最多 4 个。超了 gdb 会退化成「软件观察点」（单步执行逐指令比对），慢到难以忍受。调试少量关键变量时好用，别撒网式 `watch` 一堆。

### 观察点的核心局限

- 只能观察**全局/静态变量或当前作用域内的变量**；局部变量离开作用域后观察点自动失效。
- 对**指向堆内存**的指针，`watch *p` 观察的是 `p` 当前指向的内容；若 `p` 本身（指针值）被改，要 `watch p`。
- 结构体整体观察很慢，尽量 `watch p->field` 精确到字段。

## commands：断点命中自动执行

把「停在断点 → 打印几个变量 → 继续」固化成脚本，适合在循环里自动采样：

```gdb
(gdb) break print_orders
(gdb) commands 2
> silent            # 命中时不打印 "Breakpoint 2, ..."
> printf "id=%d qty=%ld\n", p->id, p->qty
> continue          # 自动继续，不停下
> end
(gdb) run
id=3 qty=-1
id=2 qty=200
id=1 qty=100
# ... 程序跑完，全程无需人工干预
```

## 用断点钉住 orderbook 的段错误

回到 2.1 的崩溃点 `*p = 42`。段错误发生在 `main` 里，我们断在 `main` 后逐步逼近：

```gdb
(gdb) break main
(gdb) run
Breakpoint 1, main () at orderbook.c:21
(gdb) next          # 连续 next，跳过三笔 add_order
...
(gdb) next          # 越过 print_orders
27          int *p = NULL;
(gdb) next
28          *p = 42;
(gdb) next
Program received signal SIGSEGV, Segmentation fault.
0x00005555555551a7 in main () at orderbook.c:28
28          *p = 42;      # ← 精确钉到第 28 行，解引用 NULL
(gdb) print p
$1 = (int *) 0x0          # ← p 是 NULL，铁证
```

gdb 在收到 `SIGSEGV` 时自动停下，并标出「是哪一行、哪个指令触发」。这就是段错误定位的标准动作——但真正的快速路径是 2.3 的 `bt` + 直接加载 core，不用逐步 `next`。

## HFT 关联

1. **条件断点抓异常值**：订单簿里 `qty < 0`、`price <= 0` 这类脏数据，用 `break ... if` 一停一个准，比加满日志再肉眼扫快得多。
2. **观察点定位竞态写坏**：多线程下某个共享字段被神秘写坏，`watch` 该字段能直接抓到是哪个线程、哪一行写的（配合 Ch2 的 `thread` 命令）。
3. **硬件断点调试只读代码**：嵌入式板端程序烧在 Flash 时，软件断点失效，`hbreak` 是唯一选择。
4. **commands 自动采样**：不想停下来的循环，用 `commands` + `silent` + `printf` + `continue` 做无感采样，输出直接喂给后续分析。

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 软件断点为什么不能用在 ROM/Flash 里的代码上？

> 软件断点的原理是把目标地址第一个字节替换成 `int3`（0xCC）陷阱指令，等 CPU 执行到那里触发 SIGTRAP 再换回来。ROM/Flash 只读，写不进 `int3`，所以只能靠硬件断点——CPU 把地址写进调试寄存器（DR0–DR3），不修改任何代码字节。

**Q2:** 硬件断点和硬件观察点共享同一组寄存器吗？为什么「最多 4 个」？

> 是。x86 的 DR0–DR3 四个地址寄存器同时服务硬件断点和观察点（DR7 控制模式、DR6 存状态），所以两者加起来总共 4 个。这是硬件硬限制，gdb 超了就退化成软件观察点（逐指令单步比对，极慢）。

**Q3:** 条件断点在百万次循环里很慢，为什么？怎么规避？

> 每次执行到断点地址，gdb 都要让目标进程停下、切回 gdb 求值条件表达式，条件不成立再 `continue` 恢复——两次上下文切换 + 一次 ptrace 往返，代价远高于正常执行。规避：① 用 `ignore N` 跳过前 N 次；② 把条件判断写进代码临时加个 `if` 再 `break`；③ 用硬件断点降低每次命中的开销；④ 改用 watchpoint 盯着会变化的关键变量。

**Q4:** `watch` 一个局部变量，函数返回后会怎样？

> 观察点自动失效（gdb 会提示 "Watchpoint N deleted because the program has left the block"）。因为局部变量随栈帧销毁，地址不再有效。要跨函数观察，得观察全局变量、静态变量或堆上的对象。

**Q5:** `*p = 42` 段错误，为什么 gdb 能精确报出行号 28？

> 因为二进制带 `-g` 调试信息，`.debug_line` 段里存了「地址 ↔ 源文件行号」的映射。gdb 收到 SIGSEGV 后，读当前 PC（`rip`）对应的行号，就定位到第 28 行。若没调试信息，gdb 只能报一个裸地址 `0x5555...`，还得自己反汇编去猜。

</details>

## 交叉引用

- [2.1 gdb 入门与调试信息](01-gdb-intro-build.md)
- [2.3 栈帧与回溯](03-stack-backtrace.md)
- [03.6 模块导读](../../README.md)
