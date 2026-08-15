# 2. C 语言（12.2）

> 底本：《BPF之巅》第 12 章 编程语言，12.2 节（印刷 p551–560）

C 是最容易跟踪的语言：内核 C 有 kallsyms+发行版默认开帧指针（CONFIG_FRAME_POINTER），kprobes 直达；用户态 C 只要没 strip 符号、保留帧指针，uprobes 直达。例外：内联函数、标记不安全的函数。

## 2.1 函数符号（12.2.1）

- `readelf -s`：.dynsym（动态链接符号，strip 后仍在）+ .symtab（含本地符号，strip 后丢失）。
- 静态编译程序 strip 后可能丢全部符号。修复：① 构建去 strip 重编译；② 用 DWARF 调试信息（-dbg/-dbgsym/-debuginfo 包，perf/BCC/bpftrace 都支持）；③ BTF（未来用户态支持）。
- **轻量级调试信息技巧**：debuginfo 222MB vs libjvm.so 17MB，但其中 .symtab 仅 1.2MB。`objcopy -R .debug_*` 剥离出轻量符号文件（3.3MB），`eu-unstrip` 可合并回二进制（20MB 含全符号 52748 个）。作者 PoC，未生产验证。
- bpftrace 列符号：`bpftrace -l 'uprobe:/bin/bash:*'`（支持通配符）。

## 2.2 调用栈（12.2.2）

- BPF 用户栈默认基于帧指针：gcc 需 `-fno-omit-frame-pointer` 重编译。
- 栈去重机制：`BPF_MAP_TYPE_STACKTRACE` + `bpf_get_stackid()`——重复栈复用 ID，省存储。bpftrace 用 `ustack`/`kstack` 内置变量。
- 实例：默认 bash 栈在 read+16 后是乱码；重编译后完整显示 `read → rl_read_key → readline_internal → ... → yyparse → reader_loop → main`（自顶向下=子→父）。
- 栈底问题：回溯到 libc 即断（libc 无帧指针）。

## 2.3 函数跟踪（12.2.3）

```bash
# 参数（char *readline(char *prompt) 的 arg0）
bpftrace -e 'uprobe:/bin/bash:readline { printf("readline: %s\n", str(arg0)); }'
# 返回值
bpftrace -e 'uretprobe:/bin/bash:readline { printf("readline: %s\n", str(retval)); }'
```

发行版差异：有的 bash 用 libreadline 的 readline()——探针路径换成 `/usr/lib/libreadline.so.8`。共享库同理改路径。

## 2.4 函数偏移量跟踪（12.2.4）

uprobe 可插函数内部任意偏移（BCC attach_uprobe 支持；bpftrace/trace 工具暂未暴露）→ 可看寄存器值即局部变量。**风险**：uprobes 不检查指令边界，插错到多字节指令中间会破坏目标程序；perf 用调试信息做边界检查。

## 2.5 USDT（12.2.5）

C 程序可加 USDT 提供稳定 API。libc 自带：`usdt:libc:setjmp/longjmp/longjmp_target/memory:*`。提供方：systemtap-sdt-dev、Facebook Folly。

## 2.6 单行程序（12.2.6）

```bash
funccount 'attach*'                                    # 内核函数计数
funccount '/bin/bash:a*'                               # 二进制函数计数
funccount '/lib/x86_64-linux-gnu/libc.so.6:a*'         # 库函数计数
trace '/bin/bash:readline' '"%s", arg1'
argdist -C 'r:/lib/.../libc.so.6:fopen():int:@retval'  # 返回值统计
stackcount -u '/bin/bash:readline'                     # 用户栈计数
profile -U -F 49                                       # 49Hz 用户栈采样
```

## HFT 关联

- 交易系统 C++ 二进制发布军规：**不 strip 符号 + `-fno-omit-frame-pointer`**——离线符号包可以另存，但线上能直接 uprobe/采样省掉一切对齐麻烦。代价（寄存器少一个）在 x86-64 上可接受。
- objcopy 轻量符号技巧适合把符号装进生产镜像而不爆体积。

<details>
<summary>自测题</summary>

1. .dynsym 与 .symtab 的区别？strip 各损失什么？
2. bpf_get_stackid 如何省存储？
3. 为什么栈会断在 libc？偏移量跟踪的风险？
</details>
