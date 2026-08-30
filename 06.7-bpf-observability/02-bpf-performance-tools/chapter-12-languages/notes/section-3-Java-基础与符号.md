# 3. Java：基础与符号（12.3.1–12.3.6）

> 底本：《BPF之巅》第 12 章 编程语言，12.3 节（印刷 p560–579）

Java 是最难跟踪的目标之一。JVM = C++ 的 libjvm（可直接插桩）+ 解释器 + JIT（动态）。示例程序：开源游戏 freecol（代替 Netflix 生产代码）。

## 3.1 跟踪 libjvm（12.3.1）

```bash
funccount '/usr/lib/jvm/.../libjvm.so:jni*'   # 235 个 JNI 函数计数
# 最高频：jni_Get/ReleasePrimitiveArrayCritical 各 3787 次
```

libjvm.so 通常 **stripped**（file(1) 可验证）。修复：源码编译不 strip / 装 debuginfo 包 / debuginfo + eu-unstrip 合并符号回 libjvm.so / 未来 BTF。

## 3.2 jnistacks（12.3.2，本书 2019-02-08）

- 对 `jni_NewObject*` 调用的用户栈计数 → 揭示哪些 Java 代码路径创建 JNI 对象。
- 源码：uprobe libjvm.so:jni_NewObject* + `@[ustack] = count()`；END 从句 `system("jmaps")`（--unsafe）在打印前一刻转储符号，**最小化符号快照与栈打印的时间差**。

## 3.3 Java 线程名字（12.3.3）

- 过滤 comm=="java" 会一无所获——线程名各不相同（C2 CompilerThre、AWT-EventQueue-、VMPeriodicTas…被截断为 15 字符）。
- comm 返回**线程（task）名**而非父进程名（top 显示 java）——更多上下文但易困惑。
- 查看：`/proc/PID/task/TID/comm`。
- **后续例子按 PID 匹配而非名字**：另一个原因是带信号量的 USDT 探针需要 PID 才能置位。

## 3.4 Java 方法符号（12.3.4）

- **perf-map-agent** 生成 `/tmp/perf-PID.map`（格式 `START SIZE 符号名`，如 `Lsun/misc/...;::getMantissa`）。
- 转储开销大（大型应用 >1s CPU）；快照很快失效——重编译不断发生，**超过 60 秒的符号表基本不可信**。
- 自动化：jmaps 找全部 java 进程并转储（174GB 内存的繁忙 JVM 约 10.5 秒、116736 个符号）。BCC 用法 `./jmaps; trace -U ...`；bpftrace 用法放 BEGIN（printf 类）或 END（汇总类）。
- 未来方向：按需的时间采样符号日志（jvmti）、失效符号标记、async-profiler（AsyncGetCallTrace，无需帧指针）、内核级符号翻译、JVM 内建符号转储。

## 3.5 Java 调用栈（12.3.5）

- Java 默认**不遵守帧指针约定**（编译器把 RBP 用作局部变量）→ 帧指针栈回溯在第一个地址就断（输出 1-2 行十六进制乱码）；纯 C++ 栈（未进 Java 方法）正常。
- **修复：`-XX:+PreserveFramePointer`**（Java 8u60+）。作者发起补丁，Oracle Zoltan Majo 重写后进入官方 JDK。
- 三步齐活：PreserveFramePointer（栈完整）+ jmaps（符号翻译）→ 输出完整 Java 方法栈。
- 残留问题：进 libc（如 read()）的栈仍断——libc 无帧指针。
- **内联**：JVM 内联激进（超 2/3 方法被内联），火焰图会出现源码中不存在的调用关系。`jmaps -u` 转储含内联符号的表（9078 → 75000+ 符号）可反解。

## 3.6 Java USDT 探针（12.3.6）

- 探针域：VM 生命周期/线程/类加载/GC/方法编译/监控器/方法调用/对象分配。
- **前提：JDK 以 --enable-dtrace 编译**——多数 Linux 发行版没开，需重编译或催包维护者。
- 探针定义在 hotspot.d → 编译为 HOTSPOT_GC_BEGIN 宏 → 嵌入 JVM 代码（如 VM_GC_Operation::notify_gc_begin）。
- `tplist -p $(pidof java)` 列出 500+ 探针；`readelf -n libjvm.so` 看 .note.stapsdt。
- 使用示例：
  - `trace 'u:...:gc__begin' '"%d", arg1'`——0=部分 GC，1=完全 GC。
  - `trace 'u:...:method__compile__begin' '"%s", arg5'`——方法名是第 5 参数。
- **字符串陷阱**：JVM 探针字符串**不以 NULL 结尾**，长度是单独参数。BCC trace 会报 UnicodeDecodeError；bpftrace 用 `str(arg4, arg5)` 指定长度即可。
- **扩展探针**：method__entry/return、object__alloc、monitor 探针默认禁用（仅启用就 >10% 开销），需 `-XX:+ExtendedDTraceProbes`。实测：method entry/return 各 2600 万次，游戏从 2 秒启动变 22 秒、输入卡顿 3 秒——**实验室诊断用，生产禁用**。

## HFT 关联

- Java 风控/行情转发服务观测三件套：`-XX:+PreserveFramePointer` + jmaps + profile（CPU 火焰图）。
- 扩展探针（方法级）在生产 = 禁区；用采样替代。

<details>
<summary>自测题</summary>

1. 符号快照为什么 60 秒后不可信？如何最小化时间差？
2. PreserveFramePointer 解决什么？谁实现的？
3. JVM USDT 字符串参数的正确读法？
4. ExtendedDTraceProbes 的开销量级？
</details>
