## 4. 计算机底层结构与数据表示

---

### 一、计算机基本结构

```
        ┌─────────┐
        │   CPU   │  数字电路 — 执行二进制指令
        └────┬────┘
             │
    ┌────────┼────────┐
    ▼        ▼        ▼
 主存储器   存储器    I/O 设备
 (RAM)   (磁盘/U盘)  (键盘/显示/网卡…)
```

| 组件 | 角色 |
|------|------|
| **CPU** | 取指、译码、执行 — **只处理二进制** |
| **主存储器** | 运行中程序与数据 — **字节寻址** |
| **存储器** | 持久保存 — 本章 U 盘上的 **BOOTX64.EFI** |
| **I/O** | 与外部世界交互 — Hello World 最终到 **显示设备** |

→ 与 [02-Hennessy 体系结构](../../../03-Computer-Architecture-6th/) · [CSAPP Ch6 存储器](../../../01-CSAPP-3rd/chapter-06-memory-hierarchy/) 同构

---

### 二、二进制与十六进制

CPU 处理 **二进制（0/1）**；人类编辑时常用 **十六进制（0–9, A–F）** — 每 1 个 hex 位 = 4 bit，2 个 hex 字符 = 1 字节。

| 十进制 | 二进制 | 十六进制 |
|--------|--------|----------|
| 10 | 00001010 | **0x0A** |
| 72 ('H') | 01001000 | **0x48** |

**编辑器里改的一个字节** = CPU 可能执行的一条指令或一个数据常量。

---

### 三、字符编码（ASCII · Unicode · UCS-2 · UTF-8/16）

> **写 OS、EFI 打印必懂：** 内存里永远是 **数字（字节）**；「字符」是 **编码规则** 的解释。  
> **UEFI 控制台认 UTF-16LE** — 所以 `hello.c` 里 `L"Hello, world!"` 和 `CHAR16` 不是装饰。

---

#### 1. ASCII — 最基础，单字节

| 项 | 说明 |
|----|------|
| **规则** | 1 字节 = 8 bit，常用 **低 7 bit** → **0~127** 共 128 个字符 |
| **0~31** | 控制字符（换行 `\n`、回车 `\r`、退格等） |
| **32~126** | 数字、大小写英文、符号 `!@#$%` 等 |
| **例子** | `'A'` → **65**（`0x41`），**1 字节** 存下 |

**缺陷：** 无中文、日文、emoji — **只能舒适处理英文**。

→ 完整表 [附录 F ASCII](../../appendix-F-ascii-table/) · [CSAPP §2.1.4 字符串](../../../01-CSAPP-3rd/chapter-02-representing-information/notes/section-2.1.4-2.1.9-字符串布尔与C位操作.md)

---

#### 2. Unicode — 全球文字的统一编号（不是存储格式）

给全世界每个字符分配 **唯一数字**，叫 **码点（code point）**。

| 字符 | 码点 |
|------|------|
| `A` | **U+0041** |
| `中` | **U+4E2D** |

**重点：** Unicode 只规定「**几号数字代表哪个字**」，**没规定** 这个数字在内存/磁盘里占几个字节、字节怎么排 — **存储方案** 由 UTF / UCS 系列实现。

---

#### 3. UCS-2 — 早期定长 2 字节（已基本淘汰）

| 项 | 说明 |
|----|------|
| **规则** | 固定 **2 字节（16 bit）** 存一个码点 |
| **范围** | 仅 **U+0000 ~ U+FFFF** |
| **问题 ①** | 纯英文也 **2 字节/字** — 体积翻倍 |
| **问题 ②** | **U+FFFF 以上**（很多 emoji、极生僻字）**存不下** |

现在被 **UTF-16** 取代；文档里仍常见「UCS-2 宽字符」说法，UEFI 语境实际按 **UTF-16 码元** 处理。

---

#### 4. UTF-16 — Windows / **UEFI 默认**（UCS-2 改良）

| 项 | 说明 |
|----|------|
| **常用字**（≤ U+FFFF） | **2 字节** |
| **超大码点**（emoji 等） | **代理对（surrogate pair）** → **4 字节** |
| **字节序** | PC/UEFI 上多为 **UTF-16LE**（低字节在前）— 与 [§四 小端](#四小端little-endian) 一致 |

**与写 EFI 强相关：**

- UEFI 固件、**EFI 控制台（ConOut）**、Windows 内核字符串 API 默认 **UTF-16LE**
- 代码里 **`CHAR16`**、`L"..."` 宽字符串 → 底层即 **UTF-16 码元**
- **`OutputString` 传 ASCII 单字节 C 串** → 极易 **乱码/崩溃**；须 **`L"..."`** 或 UTF-16 缓冲

→ [hello.c 的 EfiMain](../code/01-clang-minimal/hello.c) · [§2 ConOut](./section-2-二进制编辑器与BOOTX64.md)

---

#### 5. UTF-8 — Linux / 网页 / 现代 C 程序主流

**变长编码：1~4 字节**

| 内容 | 典型长度 |
|------|----------|
| 英文（ASCII 范围） | **1 字节** — 与 ASCII **完全兼容** |
| 常用汉字 | **3 字节** |
| 极生僻字 / 部分 emoji | **4 字节** |

| 优点 | 缺点 |
|------|------|
| 英文文件 **体积最小** | 随机访问第 N 个字符需 **扫描变长** |
| **无字节序问题**，跨平台 | 底层解析比 UTF-16 **稍复杂** |

**OS 分工直觉：** UEFI 阶段 **UTF-16**；日后 Linux 风格内核终端、日志、配置文件多为 **UTF-8**。

---

#### 6. 层级关系（一张表记牢）

```
ASCII          仅英文，单字节老旧标准（Unicode 子集）
    ↓
Unicode        全球码点编号标准（不负责怎么存）
    ↓
UCS-2          固定 2B 存码点 — 有缺陷，淘汰
UTF-16         改良 UCS-2 — Windows / UEFI
UTF-8          变长 1~4B — Linux / 通用程序
```

| # | 标准 | 角色 |
|---|------|------|
| 1 | **ASCII** | 英文单字节 |
| 2 | **Unicode** | 码点编号 |
| 3 | **UCS-2** | 老旧 2B 定长存储（废弃） |
| 4 | **UTF-16** | **MikanOS UEFI / ConOut** |
| 5 | **UTF-8** | **日后内核终端 / Linux 主线** |

**ASCII 与 Unicode：** 前 128 个码点 **一致** — `'A'` 在两者中都是 **U+0041 / 0x41**。

---

#### 7. 直观字节例子（小端主机）

**字符 `A`**

| 编码 | 内存/文件字节 |
|------|---------------|
| ASCII / UTF-8 | `0x41`（1 字节） |
| Unicode 码点 | U+0041 |
| UTF-16LE | `0x41 0x00`（2 字节，**低字节在前**） |

**字符 `中`**

| 编码 | 字节 |
|------|------|
| Unicode 码点 | U+4E2D |
| UTF-8 | `0xE4 0xB8 0xAD`（3 字节） |
| UTF-16LE | `0x2D 0x4E`（2 字节） |

**字符串 `Hi`（两个 ASCII 字母）在内存里（UTF-16LE + `\0` 结束）：**

```
地址 →  48 00  69 00  00 00
        H  ·   i  ·   NUL
        └─ U+0048 ─┘ └ U+0069 ┘
```

**字符串 `Hello, world!\n` — [hello.c](../code/01-clang-minimal/hello.c) 里 `L"..."` 数据段大致形态：**

```
每个可见字符 2 字节（ASCII 区仍扩展为 16 bit）+ 最后 00 00 结束
'H' → 48 00
'e' → 65 00
…
'\n' → 0A 00
结束 → 00 00
```

用十六进制编辑器打开编好的 `BOOTX64.EFI`，搜 **`48 00 65 00 6C 00 6C 00 6F 00`** 可找到 `"Hello"` 的 UTF-16LE 嵌入。

---

#### 9. 代码例子（对 / 错 / 手写 UTF-16）

**✅ 正确 — UEFI ConOut（本章 [hello.c](../code/01-clang-minimal/hello.c)）**

```c
// CHAR16 = 16 位宽字符；L 前缀 → 编译器生成 UTF-16LE 字面量
SystemTable->ConOut->OutputString(SystemTable->ConOut, L"Hello, world!\n");
```

**❌ 错误 — 把 ASCII C 串直接传给要 CHAR16* 的 API**

```c
// 错：每个 char 1 字节，固件按 2 字节读 → 乱码或越界
SystemTable->ConOut->OutputString(SystemTable->ConOut, "Hello, world!\n");
```

**✅ 手写 UTF-16LE 数组（等价于 `L"Hi"`）**

```c
static CHAR16 msg[] = { 0x0048, 0x0069, 0x0000 };  // 'H' 'i' NUL
SystemTable->ConOut->OutputString(SystemTable->ConOut, msg);
```

**对比：同一句「Hi」若用 C 窄串（UTF-8/ASCII）**

```c
char ascii[] = { 0x48, 0x69, 0x00 };  // 仅 3 字节 — UEFI 控制台不能直接用
// Linux 内核 printk 日后常见 UTF-8 窄串或 char* — 与 EFI 阶段不同
```

**emoji 例子（UTF-16 代理对 · 4 字节一个码点）**

| 字符 | 码点 | UTF-16LE 字节（小端） |
|------|------|------------------------|
| 🥳 | U+1F973 | `73 D8 97 DF`（**2 个 CHAR16 码元**） |

基本多文面（BMP）内的 `中` 仍只需 **1 个 CHAR16**（2 字节）；超出 BMP 才要代理对。

---

#### 10. 用 PowerShell 自己验字节（Windows）

```powershell
# UTF-16LE 字节
[System.Text.Encoding]::Unicode.GetBytes("A")     # 41 00
[System.Text.Encoding]::Unicode.GetBytes("中")    # 2D 4E

# UTF-8 字节
[System.Text.Encoding]::UTF8.GetBytes("A")        # 41
[System.Text.Encoding]::UTF8.GetBytes("中")      # E4 B8 AD
```

与 [§7 表](#7-直观字节例子小端主机) 对照，建立「码点 → 字节序列」直觉。

---

#### 8. 贴合 MikanOS / UEFI 的三条结论

1. **`CHAR16` + `L"..."` = UTF-16** — UEFI 控制台 **只认这套**；Windows 编译时加 **`-fshort-wchar`**（见 [SETUP.md](../../SETUP.md)）。
2. **内核终端若学 Linux** — 日后 printf/串口多半走 **UTF-8**；与 EFI 阶段 **类型不同**，切换层要转码或约定统一。
3. **ASCII 是 Unicode 子集** — 英文字母编号相同；差别在 **存储宽度**（1B vs 2B）和 **API 类型**。

---

### 口述巩固 · 自测

1. **Unicode 是文件格式吗？** — **否**，只是码点编号；**UTF-8/16** 才是存储格式。
2. **UEFI 为什么用 `L"Hello"`？** — **ConOut** 要 **UTF-16LE**，不是 ASCII C 串。
3. **`中` 的 UTF-8 几字节？UTF-16LE 几字节？** — **3 字节** / **2 字节**（上表）。
5. **在 `.efi` 里怎么找到 Hello 字符串？** — 搜 hex **`48 00 65 00 6C 00 6C 00 6F 00`**（`Hello` 的 UTF-16LE）。

---

### 四、小端（Little-Endian）

**x86-64** 采用 **小端模式：低字节在前**。

例：32 位整数 `0x12345678` 在内存中地址递增顺序为：

```
地址:  +0   +1   +2   +3
字节:  78   56   34   12
```

| 影响 | 说明 |
|------|------|
| **手写 hex** | 多字节常数 **字节序写反** → 程序逻辑错误或崩溃 |
| **跨平台** | 网络字节序（大端）vs 主机序 — UEFI/x64 主机为 **小端** |

→ [CSAPP Ch3 数据格式](../../../01-CSAPP-3rd/chapter-03-machine-level-programs/) · [02 附录 A 指令集](../../../03-Computer-Architecture-6th/appendix-A-指令集原理.md)

---

← [3. 真机与 QEMU](./section-3-真机与QEMU测试.md) · 下一节 [5. UEFI 启动](./section-5-UEFI启动流程.md)
