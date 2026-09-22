# 3.0′ 第一次内存错误：ASan 报告逐行读

> 🟢 零起点 · 接续 [1.0 学前篇](../../chapter-01-methodology/notes/00-gcc-first-steps.md)
>
> 内存类 bug 最气人的地方：**程序不崩，退出码 0，结果是错的**（或者今天不错明天错）。
> 这类 bug 靠眼睛看不出来，但有件武器能让它现形——**AddressSanitizer（ASan）**。
> ASan 的报告第一次看像天书：十几行地址、数字、十六进制块。
> 这一篇把一份真实报告的**每一行**拆开读完——读完会发现它其实只说了四件事。
> 主角：`code/c3_1_mem_bugs.c` 的 case 1（堆越界写）。

---

## 先看「不报错」有多危险

同一个程序，裸编译跑一遍：

```bash
gcc -g -O0 -Wall -Wextra -o c3_1_plain c3_1_mem_bugs.c
./c3_1_plain 1
```

```text
case 1: 写了 buf[8]（越界 1 字节）—— 裸编译下「成功」跑完
case 1 跑完了 main
$ echo $?
0
```

**代码里明明越界写了（`buf[8]`，buf 只有 8 字节），程序却一切正常**。
物理上那 1 字节确实写进去了（踩进了堆分配器自己的地盘），没触发任何保护。
这就是内存 bug 的教学第一课：**「不崩」和「没错」是两件事**。
它会在三天后、在另一个函数里、以完全无关的姿势崩——那时谁也想不到凶手在这里。

## 同一份代码，加一个 flag 就现形

```bash
gcc -g -O0 -fsanitize=address -o c3_1_asan c3_1_mem_bugs.c
#        └────────┬────────┘
#        唯一的区别：请编译器在每次内存访问前后插入「这块地能不能碰」的检查
./c3_1_asan 1
```

`-fsanitize=address` 一个选项做两件事：编译时**插桩**（每次读写内存先查岗）、
链接时带上**运行时库**（管账本、报错、打印报告）。

## 报告逐行读：其实只说了四件事

跑 `./c3_1_asan 1` 打出的报告（实测，已精简）：

```text
=================================================================
==1==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x...   ①
WRITE of size 1 at 0x... thread T0                                        ②
    #0 crash_heap_overflow .../c3_1_mem_bugs.c:38                        ③
    #1 main .../c3_1_mem_bugs.c:96

0x... is located 0 bytes after 8-byte region [0x...,0x...)               ④
allocated by thread T0 here:
    #0 malloc ...
    #1 crash_heap_overflow .../c3_1_mem_bugs.c:34                        ⑤

Shadow bytes around the buggy address:
=>0x...: fa fa 00[fa]fa fa ...                                          ⑥
SUMMARY: AddressSanitizer: heap-buffer-overflow c3_1_mem_bugs.c:38      ⑦
ABORTING
```

逐条拆：

| 行 | 它说了什么 | 人话 |
|----|-----------|------|
| ① | **错误类型** `heap-buffer-overflow` + 出事地址 | 「堆上越界」——越界的种类直接写在第一行（heap/stack/use-after-free/double-free/leak 五种，c3_1 正好一种一例） |
| ② | **动作**：WRITE，长度 1 字节 | 是「写」闯的祸（也可能是 READ），一次只越了 1 字节 |
| ③ | **犯罪现场**：调用栈，`#0` 是动手的行 | `c3_1_mem_bugs.c:38`——打开文件第 38 行，就是 `buf[8] = 1` |
| ④ | **受害者**：8 字节的地盘，你越到了它后面第 0 字节 | buf 只有 8 字节，你写了第 9 个 |
| ⑤ | **这块地是谁分的**：malloc 的调用栈，行号 `:34` | 越界事故经常「分的时候就算错大小」——所以 ASan 把分配点也给你 |
| ⑥ | **影子内存图**：`00`=合法，`fa`=红区（禁地），`[fa]`=你踩的那格 | ASan 在每块合法内存周围撒「红区」，踩红区即报警——这就是它物理上怎么抓到你的 |
| ⑦ | **一句话总结**（SUMMARY） | 抄给同事/贴 issue 就用这行 |

**四件事：什么错（①）→ 谁干的（③）→ 地是谁的（④⑤）→ 总结（⑦）。**
任何 ASan 报告都用这个顺序读，30 秒读完。

## 第二种形态：泄漏（连错误现场都没有）

`./c3_1_asan 5`（100 次 malloc 只借不还）的报告头部不一样：

```text
=================================================================
==1==ERROR: LeakSanitizer: detected memory leaks
Direct leak of 40000 byte(s) in 100 object(s) allocated from:
    #0 malloc ...
    #1 crash_leak .../c3_1_mem_bugs.c:84
SUMMARY: AddressSanitizer: 40000 byte(s) leaked in 100 allocation(s).
```

泄漏没有「犯罪现场」（借的时候没错，错在**没还**），所以报告给的是
「**这笔钱在哪借的**」——`:84` 那行 malloc。100 次 × 400 字节 = 40000，
数字对得上账。长跑服务（7×24 的交易进程）里泄漏是慢性自杀：
每天漏 40 KB，一个月后 OOM——LeakSanitizer 就是体检里的「慢性病筛查」。

## 三条必须记住的纪律

1. **ASan 是开发期工具，不上线**：它让程序慢约 2 倍、内存涨约 3 倍。
   正确姿势 = 开发/测试环境每次全量测试都带 `-fsanitize=address` 跑（见 01.5 第 5 章的 dev preset）。
2. **配 `-O0` 先保证现形**：`-O1` 下编译器可能把「出错代码」当死码删掉
   （c3_1 的 case 3 UAF 在 `-O1` 下就检不出——优化改变 bug 的存在性，见 code/README 坑 1）。
3. **报告从头读，不要从中间读**：新手容易被影子图（⑥）吓住，
   其实 ①③⑤⑦ 四行就够定位；影子图是讲原理用的。

## 与 valgrind 的关系（一句话）

valgrind 是上一代工具：**不用重新编译**，把程序放在虚拟 CPU 上跑，
但慢 10–50 倍；ASan 要重编但只慢 2 倍——**能重编就 ASan，拿不到源码/不想重编才 valgrind**。
细节是 [3.1 valgrind](01-valgrind-memcheck.md) 与 [3.2 ASan](02-addresssanitizer.md) 的内容。

## 衔接

- 前置：[1.0 学前篇](../../chapter-01-methodology/notes/00-gcc-first-steps.md)（编译命令逐词、退出码）
- 展开：[3.1 valgrind Memcheck](01-valgrind-memcheck.md)、[3.2 AddressSanitizer](02-addresssanitizer.md)
- demo：`code/c3_1_mem_bugs.c` 五种内存错误各一例（建议 1–5 全跑，对照报告种类）；
  `code/c3_2_uninit_read.c`（ASan 的盲区：未初始化读）
- 工程化：[01.5 第 5 章](../../../01.5-cmake-build/05-hft-compile-options/README.md)（把 ASan 做成 CMake preset）

---

<details>
<summary>自测题（点击展开）</summary>

**Q1:** 裸编译跑 case 1 退出码 0、输出正常，能不能说明代码没问题？为什么？

> 不能。越界写 1 字节踩进了堆分配器的内部区域，物理可写、不触发页错误，
> 所以不崩——但那块地不属于你，里面可能是分配器的元数据或别的对象的内存，
> 后果是延迟爆发的（别处、别的时间崩）。「不崩」只说明「这次没踩到雷管」，
> 内存类 bug 必须用 ASan/valgrind 这类主动检查工具才能现形。

**Q2:** 一份 ASan 报告按什么顺序读？每段各回答什么问题？

> 四段式：①错误类型行（heap-buffer-overflow 等）→ 什么错；
> ③现场调用栈（#0 行号）→ 谁干的；
> ④⑤受害者信息 + 分配点栈 → 地是谁的、当初怎么分的；
> ⑦SUMMARY → 一句话结论。
> 影子内存图（⑥）理解原理即可，定位不必细读。

**Q3:** ASan 为什么能抓到「只差 1 字节」的越界？（影子内存与红区）

> ASan 用影子内存记账：每 8 字节真实内存对应 1 字节影子，
> 值 0 表示全部合法、fa 表示红区（禁地）。分配内存在两块合法区之间插入红区，
> 每次内存访问前插桩代码先查影子——踩红区立即报警。
> 所以哪怕只越 1 字节（踩进红区第一格）也逃不掉。
> 代价是内存约 3 倍（影子 + 红区）、速度约 2 倍——所以不上线。

**Q4:** 泄漏报告为什么没有「犯罪现场」？它给的关键行号是什么？

> 泄漏的错误是「该 free 时没 free」——借钱（malloc）那一刻完全合法，
> 没有一个「出错的动作」可以指认。所以 LeakSanitizer 在进程退出时盘点：
> 哪些块还活着但没有任何指针能找到它们，然后报告**这些块的分配点**（在哪行 malloc 的）。
> 修泄漏就是顺着分配点找到应该配对的 free 该放在哪条路径上。

**Q5:** 为什么 ASan demo 要求先用 `-O0` 构建？`-O1` 会发生什么？

> 优化器做死代码消除时不管「这段代码有没有副作用外的错误」。
> c3_1 case 3 的 UAF：`-O1` 下「往已释放内存写 + 读回来」被整体判定为无外部效果，
> 整段删掉——UAF 在二进制里「不存在」了，ASan 自然零报告。
> 结论：调内存 bug 先 -O0 保证现形，再 -O1/-O2 复跑对照；
> 优化改变的不只是报告措辞，还有 bug 的存在性。

</details>
