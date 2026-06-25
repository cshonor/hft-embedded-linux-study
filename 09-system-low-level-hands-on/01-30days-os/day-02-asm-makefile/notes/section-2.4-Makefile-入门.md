## ④ Makefile 入门

步骤变多：**NASM 编 `ipl.bin`** → **拼进 1.44 MB 映像** → **QEMU 启动** … 若每一步都手敲一长串命令，或堆一堆 **`.bat`**，目录又乱又容易漏。

**Makefile** 把「目标、依赖、命令」写进 **一个纯文本文件**；终端里只敲 **`make`**，由系统里的 **make 工具** 按规则自动执行（只重做改过的步骤）。

先从 **适配自制 OS 第一天的极简版** 上手；拼 1.44 MB 软盘、QEMU 启动见下文 **进阶**。

---

### 第一天极简 Makefile（复制即用）

在 VS Code 里 **新建 → 保存为 `Makefile`**（**M 大写**，无 `.txt`），与引导扇区汇编 **同一目录**，粘贴：

```makefile
# 定义用的编译器和参数
NASM = nasm
NASMFLAGS = -f bin

# 目标：生成的镜像文件
all: os-image.bin

# 把 helloos.nas 编译成二进制（源码后缀不用改）
os-image.bin: helloos.nas
	$(NASM) $(NASMFLAGS) -o $@ $<

# 清理生成的文件
clean:
	rm -f os-image.bin
```

| 行 | 含义 |
|----|------|
| **`NASM` / `NASMFLAGS`** | 用哪个汇编器、`-f bin` 出 **纯二进制**（不是 `.obj`） |
| **`all: os-image.bin`** | 默认目标；终端只敲 **`make`** 就会编出 `os-image.bin` |
| **`os-image.bin: helloos.nas`** | **依赖**：`helloos.nas` 改了才重新汇编 |
| **`$(NASM) … -o $@ $<`** | 等价于 **`nasm -f bin helloos.nas -o os-image.bin`**（`$@`=目标，`$<`=第一个依赖） |
| **`clean`** | **`make clean`** 删掉生成物，方便重来 |

**`helloos.nas` 是什么？** 就是 Day 1 [§1.4](../day-01-boot-asm/notes/section-1.4-加工润色.md) 的 **引导扇区汇编**（带 `TIMES` / `0xAA55`）。**原书就叫 `.nas`，用 NASM 时也不用改后缀** — 只把 nask 命令换成 **`nasm -f bin helloos.nas -o …`**（见 [§1.3 · nask→NASM 对照](../day-01-boot-asm/notes/section-1.3-初次体验汇编程序.md#只换-nask--nasm命令对照后缀照旧)）。

**产出多大？** `nasm -f bin` 此时通常得到 **512 B** 引导扇区（不是整盘 1.44 MB）。下文把同样产物记作 **`ipl.bin`**；`os-image.bin` 只是第一天起的文件名。

**怎么用：**

```bash
make          # 生成 os-image.bin（≈512 B）
make clean    # 删除 os-image.bin
```

仓库里有一份同款副本：[code/Makefile](../code/Makefile)。

---

### Makefile 硬规则：配方行必须用 Tab

```makefile
os-image.bin: helloos.nas
	$(NASM) $(NASMFLAGS) -o $@ $<
```

上面 **`$(NASM) …` 那一行开头必须是 Tab 键**，**不能**用空格顶格。这是 Make 的语法规定 — 用空格会报 `missing separator` 之类错误，编不过。

VS Code 技巧：**查看 → 渲染空白字符**，或装 **Makefile Tools** 扩展，Tab/空格一眼能分清。

---

### 不用专门软件 — VS Code 就能写

| 要点 | 说明 |
|------|------|
| **格式** | **纯文本**，没有特殊二进制格式 |
| **文件名** | 必须叫 **`Makefile`**（**M 大写**，无 `.txt` 后缀） |
| **编辑器** | **VS Code** / Cursor / 记事本均可 — 与写 `helloos.nas` 一样 **新建 → 保存** |
| **常见坑** | 若存成 `Makefile.txt`，`make` **找不到**；配方行必须用 **Tab**（见上一节） |

在 VS Code 中：

1. 工程目录（与 `helloos.nas` 同级）→ **新建文件** → 保存为 **`Makefile`**
2. 写入下面的规则
3. **终端**（VS Code `` Ctrl+` ``）→ `cd` 到该目录 → 输入 **`make`**

> **make 本身** 是已安装的工具（[SETUP §3.5](../../SETUP.md#35-确认-make)：`make --version`）。**Makefile** 只是告诉 make「要做什么」的说明书。

---

### 进阶：拼 1.44 MB 软盘 + QEMU（Day 2 完整链）

极简版只解决 **「汇编 → 512 B 二进制」**。要把 [Day 1](../day-01-boot-asm/) 的 **`hello, world`** 在 QEMU 里跑起来，还要把引导扇区 **嵌进 1,474,560 B 软盘**（见 [§2.3](./section-2.3-先制作启动区.md)）。在极简 Makefile 上 **追加** 下面目标即可：

### 三条规则在干什么

```makefile
# 目标: 依赖
# 	命令（行首必须是 Tab）

ipl.bin: helloos.nas
	nasm -f bin $< -o $@ -l helloos.lst

helloos.img: ipl.bin
	dd if=/dev/zero of=$@ bs=512 count=2880
	dd if=ipl.bin of=$@ conv=notrunc

run: helloos.img
	qemu-system-i386 -fda helloos.img -boot a
```

| 目标 | 依赖 | 做什么 |
|------|------|--------|
| **`ipl.bin`** | `helloos.nas` | **NASM** 汇编 → **512 B** 引导扇区（见 [Day 1 §1.4](../day-01-boot-asm/notes/section-1.4-加工润色.md)） |
| **`helloos.img`** | `ipl.bin` | 生成 **1,474,560 B** 软盘，把 `ipl.bin` **写到偏移 0** |
| **`run`** | `helloos.img` | 调 **QEMU** 启动 |

**Make 变量简写（知道即可）：**

| 符号 | 含义 |
|------|------|
| **`$<`** | 本条规则的第一个依赖（如 `helloos.nas`） |
| **`$@`** | 本条规则的目标（如 `ipl.bin`） |

原书：`nask helloos.nas helloos.lst ipl.bin` → 本仓库：**`nasm -f bin helloos.nas -o ipl.bin -l helloos.lst`**（`.nas` 后缀不用改）。

---

### 拼 1.44 MB 映像：和 Day 1 的关系

- **`nasm` 只产出 512 B 的 `ipl.bin`**（引导扇区）。
- **昨天 HxD** 是直接编辑 **整盘 1.44 MB**；**今天** 可以：
  - **Makefile 里用 `dd`**（**MSYS2 / Git Bash** 终端里 `make` — `dd` 在这类环境可用），或
  - 用 HxD 打开模板 [helloos.img](../day-01-boot-asm/code/helloos.img)，把 `ipl.bin` **覆盖到文件开头**（与 [1.4 做法 A](../day-01-boot-asm/notes/section-1.4-加工润色.md) 相同）

`dd` 两行含义：

```text
dd if=/dev/zero of=helloos.img bs=512 count=2880   # 2880×512 = 1,474,560 B 全零底稿
dd if=ipl.bin of=helloos.img conv=notrunc          # 只覆盖开头 512B，不截断文件
```

---

### 怎么用

在 **含 `Makefile` 的目录** 打开终端：

```bash
make              # 默认构建第一个目标，或构建 helloos.img 的依赖链
make helloos.img  # 只构建映像（ipl.bin 已最新则跳过 nasm）
make run          # 构建后启动 QEMU（需写好 run 目标）
make clean        # 若定义了 clean，删除中间文件
```

**增量构建：** 只改注释、没动 `helloos.nas` 时，再敲 `make` 可能 **跳过 nasm**（因为 `ipl.bin` 比 `.nas` 新）— 这就是 Makefile 相对手敲命令的好处。

---

### 和 `.bat`、手敲命令对比

| | 手敲 / `.bat` 堆叠 | **`Makefile` + `make`** |
|---|-------------------|-------------------------|
| 命令长度 | 每次复制一长串 | 平时只打 **`make`** |
| 依赖顺序 | 自己记、易漏拼盘 | **目标–依赖** 自动排序 |
| 改了一处 | 可能全量重跑 | **只重建过期的目标** |
| 文件 | 多个脚本 | **一个 `Makefile`** |
| 行业习惯 | Windows 批处理 | **Linux 内核 / HFT** 与 CMake/Ninja 同族思想 |

**HFT：** 策略、网关工程里 **CMake / Make / Ninja** — 同一套路：**声明依赖，工具链执行**。

---

### Windows 与 `make clean`

极简版里 **`rm -f`** 在 **Git Bash / MSYS2** 终端可用（与 [SETUP](../../SETUP.md) 里跑 `make` 的环境一致）。若在 **纯 cmd** 里 `clean` 报错，可暂时手删 `os-image.bin`，或把 `clean` 改成：

```makefile
clean:
	del /f os-image.bin
```

---

### 自检

- [ ] 工程根目录有 **`Makefile`**（不是 `makefile.txt`）
- [ ] 配方行用 **Tab** 缩进（不是空格）
- [ ] **`make`** 能从 **`helloos.nas`** 生成 **`os-image.bin` / `ipl.bin`（512 B）**
- [ ] `make` 能生成 **`helloos.img`（1,474,560 B）**，QEMU 仍出 **`hello, world`**
- [ ] 说清：**Makefile 是文本**；**make 是读它的程序**

---

← [2.3 先制作启动区](./section-2.3-先制作启动区.md) · 下一日 [Day 3](../../day-03-32bit-c/)
