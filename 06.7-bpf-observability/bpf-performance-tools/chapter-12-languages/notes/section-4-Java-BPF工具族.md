# 4. Java：BPF 工具族（12.3.7–12.3.16）

> 底本：《BPF之巅》第 12 章 编程语言，12.3 节（印刷 p579–601）

前置条件（12.3.7 节）：`-XX:+PreserveFramePointer` + perf-map-agent/jmaps 符号文件 + libjvm 符号表。

## 4.1 profile + CPU 火焰图（12.3.7）

```bash
jmaps; profile -afp 16914 10 > out.profile01.txt
flamegraph.pl --color-java --hash < out.profile01.txt > out.profile02.svg
```

- --color-java：Java 绿、C++ 黄、原生红、内核橘黄。
- 案例：**55% CPU 花在 C2 编译器**（C++ 栈最宽塔），Java 本体仅 29%。可调 -XX:CompileThreshold/MaxInlineSize/InlineSmallCode/FreqInlineSize，或 -Xcomp 实验。
- 长剖析（>2 分钟）符号会过期（C2 移方法）→ 无意义代码路径即翻译错误的信号。

## 4.2 offcputime + off-CPU 火焰图（12.3.8）

```bash
jmaps; offcputime -fp 16914 10 > out.offcpu01.txt
flamegraph.pl --countname=us --title="Off-cpu..." > out.offcpu01.svg
```

- 案例发现：freecol 阻塞在 unlink() 磁盘 I/O——bpftrace 跟 sys_enter_unlink 揭示是**自动存档删除**。
- 等待线程会淹没输出：火焰图搜索"freecol"高亮（14.6%），或 `grep freecol | flamegraph.pl` 预过滤折叠栈。
- libpthread 默认 -fomit-frame-pointer → pthread_cond_timedwait 处断栈；自编译带帧指针版可修复。
- Interpreter 帧多 = 方法未达编译阈值。

## 4.3 stackcount（12.3.9）

```bash
stackcount -p 16914 t:exceptions:page_fault_user   # 用户态缺页栈 = 主存增长画像
```

- 绿色背景火焰图、--countname=pages；量化每条应用路径的内存增长。
- bpftrace 版：`@[kstack,ustack,comm] = count(); END { system("jmaps"); }`。

## 4.4 USDT 工具族（12.3.10–12.3.15）

| 工具 | 探针 | 输出 | 开销 |
|---|---|---|---|
| javastat | ustat 封装 | 每秒 METHOD/GC/OBJNEW/CLOAD/EXC/THR 计数表 | METHOD/OBJNEW 列为 0，除非 ExtendedDTraceProbes |
| javathreads | hotspot:thread__start/stop | `=> / <=` 线程创建/结束事件流（Reference Handler、C2 CompilerThread…） | 可忽略 |
| javacalls | hotspot:method__entry | 方法调用计数 top（String.coder 126 万次） | **Extended 探针，>10x 减速** |
| javaflow | method entry/return | 缩进代码流（ReentrantLock.unlock 嵌套链）；BPF 跟不上时"丢失 9 个采样"=丢事件保护 | 同上 |
| javagc | gc 探针 | GC 起始时刻+时长（微秒） | 标准 USDT，可用 |
| javaobjnew | object__alloc | 对象分配计数（HashMap$KeyIterator 90 万）；BYTES 列 Java 不支持 | Extended 探针 |

- 多数封装自 Sasha Goldhtein 的 ustat/ucalls/uflow/ugc/uobjnew（BCC tools/lib，多语言通用）。

## 4.5 Java 单行（12.3.16）

```bash
funccount '/.../libjvm.so:jni_Call*'                     # JNI 计数
funccount -p $(pidof java) 'u:...:method*'               # 方法事件计数
profile -p $(pidof java) -U -F 49                        # 49Hz 采样带线程名
bpftrace: usdt:...:method__compile__begin → str(arg4,arg5)  # 跟踪编译
bpftrace: usdt:...:class__loaded → str(arg0,arg1)           # 类加载
```

## HFT 关联

- Java 服务性能分析首选**低开销三件**：profile（CPU 火焰图）、offcputime（阻塞火焰图）、stackcount（事件栈），全部采样/事件型，生产可用。
- javacalls/javaflow/javaobjnew 仅限测试环境复现问题。

<details>
<summary>自测题</summary>

1. Java profile 火焰图三色各代表什么？C2 塔过宽怎么办？
2. offcputime 输出被什么淹没？两种过滤方法？
3. 哪些 javastat 列需要 ExtendedDTraceProbes？代价？
4. "Possibly lost N samples" 说明什么？
</details>
