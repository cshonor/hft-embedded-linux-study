# 5. bash shell 与其他语言（12.4–12.5）

> 底本：《BPF之巅》第 12 章 编程语言，12.4–12.5 节（印刷 p601–619）

## 5.1 bash：方法论演示场（12.4）

解释型语言慢且少作性能调优对象，但**排障需求真实存在**。本章展示"对未知解释器开展工作"的完整方法论（可迁移到其他语言）。

**准备**：`CFLAGS=-fno-omit-frame-pointer ./configure && make`（帧指针+本地符号表）；样例 welcome.sh 调用 welcome 函数 7 次、每次 3 个 echo（已知量验证法）。

### 5.1.1 函数计数（12.4.1）

- 猜测法：`funccount 'p:bash:*func*'` → execute_function 恰 7 次、restore_funcarray_state 7 次。
- 扩展到 execute*：execute_builtin 21 次（=echo 数）、execute_command 23 次。

### 5.1.2 bashfunc.bt：函数参数跟踪（12.4.2）

- 读源码：execute_function(var, ...) 第一参数是 SHELL_VAR *，首成员 `char *name`。
- 两种结构体获取方式：#include variables.h（报 stdio.h 缺失警告但可用）；**"部分结构体"**——只声明需要的首成员（不依赖源码树）。
- 输出 `function: welcome` ×7。

### 5.1.3 bashfunclat.bt：函数时长（12.4.3）

- welcome 加 sleep 0.3 → funclatency 验证 execute_function 延迟落在 256-511ms 桶。
- 工具化：uprobe 存 `@name[tid]/@start[tid]`，uretprobe 出 `@ms[函数名] = hist(ms)`。

### 5.1.4 /bin/bash 的现实困难（12.4.4）

自编译版一切顺利，切到发行版 /bin/bash 全部失效：**stripped**（execute_function 是本地符号被剥）。破局过程：

1. 幸存线索：restore_funcarray_state 仍可见（7 次=已知负载）。
2. stackcount 证实它在 execute_function 之下（自编译版栈完整可见层级）。
3. 尝试从其 struct func_array_state 参数找函数名——失败。
4. **可行方案**：find_function（首参数即函数名）缓存 → restore_funcarray_state 时打印。可工作但**依赖特定版本实现**（uprobes 非稳定接口）。

### 5.1.5 bash USDT 展望（12.4.5）

正确做法是给 bash 加 USDT（如 `bash:execute_function_entry(name, args, file, linenum)`）。Solaris Bourne shell 已有先例（provider sh：function-entry/return、builtin、command、script-start/done、subshell、line、variable-set/unset）。

## 5.2 JavaScript / Node.js（12.5.1）

- v8 运行时（解释+JIT+GC），跟踪方式与 Java 类似。
- USDT：需 `./configure --with-dtrace` 源码编译；探针含 gc__start/done、http__server/client__request/response、net__*。
- 符号（v10.x+ 两法）：① `--perf-basic-prof` 滚动日志（不可禁用，会涨到 GB 级且旧映射仍被使用→需后处理只留最新）；② **linux-perf 模块**（perf-map-agent 式持续写入，推荐）。
- `--interpreted-frames-native-stack`：让解释帧显示真名而非 "Interpreter"。
- 无函数级 USDT（v8 架构难加，加了开销 >10x）；函数上下文靠采样等内核事件获得。

## 5.3 C++（12.5.2）

与 C 几乎相同，差异：

- 符号名 mangling：`_ZN11ClassLoader10initializeEv`（BCC/bpftrace 打印时自动 demangle；跟踪时可用 `*ClassLoader*initialize*` 通配）。
- 参数可能不遵守 ABI（this/self 指针；字符串是 C++ 对象需在 BPF 里定义结构体解析）。
- 通配符匹配是实用技巧；BTF 成熟后对象成员定位会更容易。

## 5.4 Golang（12.5.3）

- Go1.7+ gc 与 gccgo **默认保留帧指针且含符号** → 调用栈/采样开箱即用。
- gc 静态链接（函数在二进制里）；gccgo 动态链接（函数在 libgo.so，路径要写对）。
- **参数**：gccgo 用标准 ABI（arg0..N 可用）；gc 用 Plan9 栈传参——需 `reg("sp")` 手工读栈偏移（需 `-gcflags="-N -l"` 防内联）。
- **uretprobes 在 Go 上不安全**：goroutine 栈可移动，蹦床恢复会破坏数据（崩溃算幸运，带坏数据继续跑更悲惨）。替代：Gianluca Borello 反汇编定位返回点放 uprobes。
- **goroutine 跨 OS 线程迁移**：以 tid 为键测时长不可靠。
- 动态 USDT：libstapsdt（mattn）。

## HFT 关联

- **Go 服务（行情网关常见）：严禁 uretprobe；时长测量改用入口/出口双 uprobe 或采样**。
- C++ 参数解析（this 指针、std::string 对象）是交易系统 uprobe 工具开发的日常；部分结构体法（5.1.2）是最省事的手筋。
- bash 方法论（计数猜函数→栈验证层级→参数找数据）适用于任何闭源解释器的逆向观测。

<details>
<summary>自测题</summary>

1. "部分结构体"技巧是什么？为什么不需要完整源码头文件？
2. /bin/bash stripped 后的破局路径？
3. Go 的三个跟踪陷阱？
4. Node.js 两种符号文件方案的取舍？
</details>
