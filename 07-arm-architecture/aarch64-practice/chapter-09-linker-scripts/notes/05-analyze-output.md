# 9.5 分析链接结果

> 来源：§9.5 · 精读 · [章总览](section-0-本章完整概述.md)

## 本节讲什么

用 `readelf`、`objdump`、`nm`、`size` 等工具分析链接后的 ELF 文件，验证段布局、符号地址、段大小。

## 分析工具速查表

| 工具 | 用途 | 常用选项 |
|------|------|---------|
| `readelf -l` | Program Headers（段布局/加载视图） | `-l` 段头, `-S` 节头, `-s` 符号 |
| `nm` | 符号表 | `-n` 按地址排序, `--size-sort` |
| `objdump -d` | 反汇编代码段 | `-d` 反汇编, `-h` 段头 |
| `size` | 段大小汇总 | `-A` 按段 |
| `strings` | 可打印字符串 | 查找硬编码字符串 |

## ELF 文件的两种视图

```
┌──────────────────────────────────────────────────┐
│ ELF Header                                        │
├──────────────────────────────────────────────────┤
│                                                    │
│  ┌─────────────────┐    ┌─────────────────┐      │
│  │ Section Headers  │    │ Program Headers │      │
│  │ (链接视图)        │    │ (加载视图)       │      │
│  │                  │    │                 │      │
│  │ .text .data .bss │    │ LOAD段1 LOAD段2 │      │
│  │ .rodata .symtab  │    │ (PT_LOAD)       │      │
│  └─────────────────┘    └─────────────────┘      │
│                                                    │
│  Section contents (实际数据)                       │
│                                                    │
└──────────────────────────────────────────────────┘

编译器/链接器 → 看 Section（节）
加载器/OS     → 看 Segment（段，多个节合并）
```

### 链接视图 vs 加载视图

| 视角 | 包含 | 用途 |
|------|------|------|
| Section（节） | .text .data .bss .rodata .symtab .strtab ... | 链接器合并、调试 |
| Segment（段） | 可加载段（PT_LOAD），每段含多个 Section | OS 加载器映射到内存 |

## readelf 常用命令

```bash
# === ELF 文件头 ===
readelf -h vmlinux        # 魔数、入口地址、位数

# === Section Headers（节头表，链接视图）===
readelf -S vmlinux        # 所有节的地址、大小、属性
readelf -SW vmlinux       # 宽格式（不截断）

# === Program Headers（段头表，加载视图）===
readelf -l vmlinux        # LOAD 段的 VMA/LMA/权限
readelf -lW vmlinux       # 宽格式

# === 符号表 ===
readelf -s vmlinux        # .symtab 符号表
readelf -sW vmlinux       # 宽格式

# === 重定位 ===
readelf -r a.o            # 重定位条目（链接前）

# === 全部信息 ===
readelf -a vmlinux        # 一次性输出所有信息
```

### readelf -l 输出解读

```
Elf file type is EXEC (Executable file)
Entry point address: 0x80000              ← 程序入口

Program Headers:
  Type           Offset   VirtAddr   PhysAddr   FileSiz  MemSiz   Flg Align
  LOAD           0x000000 0x00080000 0x00080000 0x001000 0x001000 R E 0x1000
  LOAD           0x001000 0x00081000 0x00081000 0x000100 0x000200 RW  0x1000

#  Flg: R=读 E=可执行 W=可写
#  VirtAddr = VMA,  PhysAddr = LMA
#  FileSiz = 文件中占用大小,  MemSiz = 内存中占用大小
#  MemSiz > FileSiz → 有 .bss（文件中不存，内存中零填充）
```

### readelf -S 输出解读

```
Section Headers:
  [Nr] Name              Type            Address          Off    Size   ES Flg
  [ 0]                   NULL            0000000000000000 000000 000000 00
  [ 1] .text             PROGBITS       0000000000080000 001000 001000 00 AX
  [ 2] .data             PROGBITS       0000000000081000 002000 000100 00 WA
  [ 3] .bss              NOBITS         0000000000081100 002100 000100 00 WA
  [ 4] .symtab           SYMTAB         0000000000000000 002100 000200 18

#  Flg: A=Alloc(可分配) X=Execute W=Write
#  NOBITS = .bss，文件中不占空间
```

## nm 符号表分析

```bash
nm -n vmlinary           # 按地址排序
nm --size-sort vmlinux   # 按大小排序（找最大符号）
```

### nm 输出格式

```
00080000 T _start          ← 地址  类型  符号名
00080040 T main
00081000 D global_var
00081100 B bss_var
         U printf          ← U = 未定义（需要动态链接）
```

### nm 符号类型速查

| 大写 | 含义 | 小写 | 含义 |
|------|------|------|------|
| T | Text（代码段，全局） | t | Text（局部 static） |
| D | Data（已初始化数据，全局） | d | Data（局部） |
| B | BSS（未初始化，全局） | b | BSS（局部） |
| R | Read-only data | r | Read-only（局部） |
| W | Weak symbol | w | Weak（局部） |
| U | Undefined（未定义） | - | - |

## objdump 反汇编

```bash
# 反汇编所有代码段
objdump -d vmlinux

# 只反汇编 .text 段
objdump -d -j .text vmlinux

# 带源码（需 -g 编译）
objdump -S vmlinux

# 显示段头
objdump -h vmlinux

# 显示所有重定位
objdump -r a.o
```

### objdump -h 输出

```
Sections:
Idx Name          Size      VMA            LMA            File off  Algn
  0 .text         00001000  0000000000080000 0000000000080000 00001000 2**0
  1 .data         00000100  0000000000081000 0000000000081000 00002000 2**3
  2 .bss          00000100  0000000000081100 0000000000081100 00002100 2**3
```

## size 段大小分析

```bash
size vmlinux
#    text    data     bss     dec     hex filename
#   65536     256     100   65892   10164 vmlinux

# 按段详细列出
size -A vmlinux
# section          size          addr
# .text           65536    0x80000
# .data             256    0x81000
# .bss              100    0x81100
# Total            65892
```

| 列 | 含义 |
|------|------|
| text | 代码段（.text + .rodata） |
| data | 数据段（.data，已初始化） |
| bss | BSS 段（未初始化，文件中不占空间） |
| dec | 十进制总大小 |
| hex | 十六进制总大小 |

## 综合分析流程

```bash
# 1. 看入口地址和段布局
readelf -h app.elf | grep "Entry"
readelf -l app.elf

# 2. 看符号地址
nm -n app.elf

# 3. 反汇编验证
objdump -d app.elf | head -50

# 4. 段大小
size -A app.elf

# 5. 检查 --gc-sections 效果
nm app.elf | grep dead_function  # 应该找不到
```

## 自测题

1. `nm` 输出中 `T`、`D`、`B`、`U` 分别代表什么？小写字母呢？
<details><summary>答案</summary>
T=Text（代码段，全局可见），D=Data（已初始化数据，全局），B=BSS（未初始化数据，全局），U=Undefined（未定义，需外部解析）。小写 t/d/b 表示局部符号（static 限定作用域）。大写=全局，小写=局部。
</details>

2. `readelf -l` 和 `readelf -S` 的区别？
<details><summary>答案</summary>
`-l` 显示 Program Headers（加载视图，OS 加载器看的角度，PT_LOAD 段含 VMA/LMA/权限）。`-S` 显示 Section Headers（链接视图，链接器/调试器看的角度，每个 .text/.data/.bss 段的地址和大小）。一个 ELF 文件可以同时有两种视图。
</details>

3. 如何确认某函数被 `--gc-sections` 删除了？
<details><summary>答案</summary>
（1）`nm app.elf | grep dead_func` 找不到该符号（2）`-Map` 文件中标记为 "discarded"（3）`objdump -d` 反汇编中找不到该函数（4）`size` 对比 GC 前后 text 段变小。
</details>

4. ELF 文件中 `.bss` 段为什么 FileSiz < MemSiz？
<details><summary>答案</summary>
.bss 是未初始化数据，文件中不存储具体值（全零），只记录大小。加载器映射内存后由 OS 零填充。所以文件中 FileSiz=0（或很小），但内存中 MemSiz=实际大小。这节省了磁盘空间。
</details>

5. 如何用 `objdump` 只反汇编 `.text` 段？为什么要限定段？
<details><summary>答案</summary>
`objdump -d -j .text vmlinux`。限定段是因为大 ELF 文件（如内核）有几十 MB，全反汇编很慢且输出巨大。只看 .text 段可以快速定位代码问题。
</details>

## 参考与延伸

- 原书 §9.5
- [9.6 内核链接脚本分析](06-kernel-link-script.md)
- `man readelf` / `man objdump` / `man nm`
