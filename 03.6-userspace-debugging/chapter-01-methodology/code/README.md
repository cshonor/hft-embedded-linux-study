# Ch1 demos — 排错方法论

本章四篇笔记（1.1 分类学 / 1.2 症状→工具 / 1.3 最小复现 / 1.4 调试元流程）原本全是表格和口诀。
这个目录把「方法论」落成**拿在手里能跑的东西**：四个 demo，每个都在 Compiler Explorer
（gcc 13.3.0 / clang 18.1.0，x86-64，Ubuntu 24.04 容器）上真编译、真跑过，
输出原样抄在对应笔记的「实测输出」块里。

> ⚠️ **CE 的执行器只跑单文件程序**，所以每个 demo 都是自包含的单个 `.c`；
> 输入数据用 `stdin` 喂（`ticks_*.txt`），不依赖额外文件路径。

## 四个 demo

| 文件 | 对应节 | 演示什么 | 需要什么 |
|------|--------|----------|----------|
| `c1_1_three_bugs.c` | 1.1 / 1.2 / 1.4 | **一份程序装三类雷**：`1`=空指针解引用（崩溃类）、`2`=只借不还（内存类）、`3`=两个线程抢一个计数器（并发类）。同一份「病人」在不同工具下呈现不同症状，是症状→工具决策树的实体道具 | case 3 要 `-pthread`；case 2 要有 ASan 才看得见；case 3 只有 **clang** 的 TSan 跑得起来（见下） |
| `c1_2_shrink_demo.c` | 1.3 | **最小复现的起点**：读 N 条行情算均价，第 7 条 `volume=0` → 除零 SIGFPE。输入走 stdin，于是「砍输入」是改文件而不是改代码 | 无 |
| `c1_3_auto_bisect.c` | 1.3 | **把「数据二分」写成程序**：16 条记录、4 次判定自动收敛到那条坏记录。顺带演示 `sigsetjmp/siglongjmp` 把「崩了」变成可返回的布尔值 | 无 |
| `c1_4_hypothesis_div.c` | 1.4 / 3.3 | **一个被证伪的假设**：三种除零写法各跑一次，`1 / volatile-zero`（常量分子）**不崩、打出 0、退出 0**。用来演示「实测打脸直觉时该信数据」以及「UB ≠ 一定崩」 | 无 |
| `c1_5_pipeline.c` | 1.5 | **全链路标本**：测 `-g` 前后体积、`dsymutil` 生成 `.dSYM`、`llvm-dwarfdump --debug-line` 摊开行号表（第 20 行 → `0x100003efc`）。⚠️ 本 demo 实测环境是**本机 macOS / clang 23.1.0（Mach-O）**，不是 CE 的 Linux ELF——正因如此它才能演示「macOS 的 DWARF 不在可执行文件里」这个平台差异 | 本机 `clang` + `dsymutil` + `llvm-dwarfdump`；编链接需保留 `.o`（见 1.5 笔记实测说明） |

## 输入数据（喂给 `c1_2_shrink_demo`）

格式：`<ts_ms> <volume> <turnover>`，一行一条。

| 文件 | 内容 | 用它验证什么 |
|------|------|-------------|
| `ticks_10.txt` | 10 条，**第 7 条 `volume=0`** | 整批跑 → 崩在第 7 条（缩小前的「完整场景」） |
| `ticks_first6.txt` | 前 6 条（都是好的） | 砍掉后半 → 不崩，证明「bug 在后 4 条里」 |
| `ticks_only_bad.txt` | 只剩那条坏的 | 缩到头 → 1 条记录就能稳定复现 |

## 编译

```bash
# 1.1 三个 lambda 的载体（普通构建）
gcc -g -O0 -Wall -Wextra -pthread -o c1_1_three_bugs c1_1_three_bugs.c

# 1.1 case 2（泄漏）要 ASan/LSan 才看得见
gcc -g -O0 -fsanitize=address -pthread -o c1_1_three_bugs_asan c1_1_three_bugs.c

# 1.1 case 3（数据竞争）用 clang 的 TSan —— gcc 的 TSan 在这台评测机上跑不起来（见坑 2）
clang -g -O1 -fsanitize=thread -pthread -o c1_1_three_bugs_tsan c1_1_three_bugs.c

# 1.2 / 1.3
gcc -g -O0 -Wall -Wextra -o c1_2_shrink_demo c1_2_shrink_demo.c
gcc -g -O0 -Wall -Wextra -o c1_3_auto_bisect c1_3_auto_bisect.c

# 1.4
gcc -g -O0 -Wall -Wextra -o c1_4_hypothesis_div c1_4_hypothesis_div.c
```

## 运行

| 命令 | 期望结果 |
|------|----------|
| `./c1_1_three_bugs 1` | **SIGSEGV(11)**，退出码 139，只打出 `case 1: ...` 一行 |
| `./c1_1_three_bugs_asan 2` | LeakSanitizer 报告 `40000 byte(s) ... in 100 object(s)`，退出码 1 |
| `./c1_1_three_bugs_tsan 3` | TSan 报 `data race`，退出码 66；计数**照样等于 200000**（这正是竞态难查的原因） |
| `./c1_2_shrink_demo < ticks_10.txt` | 打到 `record 7` 后 **SIGFPE(8)**，退出码 136 |
| `./c1_2_shrink_demo < ticks_first6.txt` | 6 条全过，退出码 0 |
| `./c1_2_shrink_demo < ticks_only_bad.txt` | 第 1 条就崩 —— 缩到最小了 |
| `./c1_3_auto_bisect` | 4 步二分日志，收敛到第 12 条（下标 11） |
| `./c1_4_hypothesis_div 1 0 A` | `A: 1 / volatile-zero = 0` + `没有崩，正常退出`，退出码 **0** ← 反直觉 |
| `./c1_4_hypothesis_div 1 0 B` | **SIGFPE(8)**，退出码 136 |
| `./c1_4_hypothesis_div 1 0 C` | **SIGFPE(8)**，退出码 136 |

## 四个必须知道的坑

1. **退出码 = 128 + 信号号**：SIGSEGV(11) → 139，SIGFPE(8) → 136。CI 里「程序挂了」和「程序返回非零」能用退出码区分，别把段错误当成普通失败。
2. **gcc 的 TSan 在 CE 上跑不起来**：`FATAL: ThreadSanitizer: unexpected memory mapping`（退出码 66）。原因是 gcc 版 TSan 对地址空间布局有假设，容器的 ASLR 熵不符合。**同一份代码换 clang 就正常**——所以本目录的竞态 demo 指定用 clang。
3. **`-fsanitize=address` 能不能抓到，取决于那颗雷在 `-O1` 下是否还「存在」**：实测本题 `c1_1_three_bugs.c` 的 case 2（泄漏）在 `-O0` 与 `-O1` 下都抓到了（`malloc` 有副作用，删不掉）；但**在 Ch3 的 `c3_1_mem_bugs.c` 里，case 3（UAF）在 `-O1` 下会被整段优化掉、exit 0、零报告**。所以给内存 bug 写 demo 时：**先用 `-O0` 保证现形**，再用 `-O1/-O2` 复跑对照。
4. **SIGFPE 这个名字骗人**：整数除零在 x86-64 上发的是 `SIGFPE`（"floating-point exception"），跟浮点没关系。`c1_2` 的 `long / long` 就是它。另外**「除零必崩」也是错的**——分子是编译期常量时编译器会折成不含 `idiv` 的序列，见 `c1_4_hypothesis_div.c`。
5. **stdout 接管道时是全缓冲的**：`c1_2_shrink_demo.c` 每条记录后都写 `fflush(stdout)`，否则「崩在第 7 条」这一行会留在缓冲区里随进程一起消失，你只看得到 PID 和信号号。这个坑在 Ch3 的 `c3_3_ubsan_ops.c` 里写得更惨（缓冲区丢了 4 行输出）。
