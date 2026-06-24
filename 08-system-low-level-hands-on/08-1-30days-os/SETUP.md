# Day 0 · Windows + QEMU 环境部署

> **目标：** 在不使用光驱/U 盘的前提下，用原书 **tolset** 工具链 + **QEMU** 跑通 Day 1 的 `helloos.img`。

---

## 1. 你需要什么

| 组件 | 作用 | 典型文件名 |
|------|------|------------|
| **nask** | 书专用汇编器（非 NASM） | `nask.exe` |
| **bcc** | 书配套 C 编译器 | `bcc32.exe` 等 |
| **映像工具** | 把 `ipl.bin` 写入软盘映像 | `edimg` / `imgtool` / 书内脚本 |
| **QEMU** | 加载 `.img` 模拟 x86 PC 启动 | `qemu-system-i386.exe` |
| **Make** | 驱动 Makefile | `make.exe`（tolset 常自带或需单独装） |
| **每日源码** | Day 1–30 工程 | `z_tools/`、`z_proj/` 等目录 |

**获取方式：** 原书随书光盘或出版社/社区提供的 **tolset 完整资源包**（解压即用）。本仓库 **不托管** 版权资源，请将 tolset 放在本机独立目录，勿提交进 git。

---

## 2. 目录规划（强制规范）

```
C:\dev\haribote\          ← 推荐：纯英文、无空格
├── tolset\               ← 解压的整套工具（PATH 或脚本引用）
│   ├── nask.exe
│   ├── bcc32.exe
│   └── ...
├── qemu\                 ← 若 tolset 未带 QEMU，可单独安装
│   └── qemu-system-i386.exe
└── proj\                 ← 每日工程（对应本仓库 ../code/）
    ├── day-01\
    ├── day-02\
    └── ...
```

**禁止：** 路径含 **中文**、**空格**（如 `C:\Users\张三\Desktop\我的 OS`）— tolset 批处理常直接失败。

**本仓库建议：** 实验代码同步到 [../code/](../code/)，路径同样遵守上述规则。

---

## 3. 配置步骤

### 3.1 解压 tolset

1. 将资源包解压到 `C:\dev\haribote\tolset\`（路径按你本机调整）。
2. 确认存在 `nask.exe` 与 Day 1 所需 `helloos.nas` 或等价源码目录。

### 3.2 安装 / 确认 QEMU

- 若 tolset 已含 QEMU：在 `tolset` 或 `z_tools` 下找到 `qemu-system-i386.exe`。
- 否则从 [QEMU 官网](https://www.qemu.org/download/#windows) 安装，记下安装路径。

**验证：**

```cmd
qemu-system-i386 --version
```

### 3.3 配置 PATH（二选一）

**A. 临时（当前 cmd 窗口）：**

```cmd
set PATH=C:\dev\haribote\tolset;C:\dev\haribote\qemu;%PATH%
```

**B. 永久：** 系统环境变量 `Path` 追加上述目录（改完后新开终端）。

### 3.4 确认 Make

在 Day 1 工程目录执行：

```cmd
make --version
```

若无 make：安装 [GnuWin32 Make](http://gnuwin32.sourceforge.net/packages/make.htm) 或使用 tolset 自带 `make.exe`，并加入 PATH。

---

## 4. Day 1 首次构建与运行

进入 Day 1 工程目录（含 `Makefile` 与 `helloos.nas`）：

```cmd
cd C:\dev\haribote\proj\day-01
make
```

成功应生成 **`helloos.img`**（约 **1,474,560 字节** = 1.44 MB 软盘）。

### 4.1 用 QEMU 启动（软盘 A:）

```cmd
qemu-system-i386 -fda helloos.img
```

或若 Makefile 已定义 `run` 目标：

```cmd
make run
```

**预期：** 虚拟机全屏或窗口内出现 **`hello, world`**（与 [day-01 section 1.1](./day-01-boot-asm/notes/section-1.1-先动手操作.md) 一致）。

### 4.2 常见问题

| 现象 | 处理 |
|------|------|
| `'nask' 不是内部或外部命令` | PATH 未含 tolset；或应在工程目录用 `..\tolset\nask.exe` |
| 路径乱码 / 找不到文件 | 工程移到纯英文路径 |
| QEMU 黑屏无字 | 确认 `-fda` 指向的 img 大小为 1440KB；重新 `make` |
| make 报语法错误 | 勿用 WSL 的 make 编 Windows tolset 工程；用 **cmd** + Windows 版 make |

---

## 5. 与后续 Day 的衔接

| Day | 额外注意 |
|-----|----------|
| **2** | Makefile 多目标；只编 `ipl.bin` 再拼盘 — [day-02](./day-02-asm-makefile/) |
| **3** | 引入 `bootpack.c`、`naskfunc.nas`；IPL 读盘 — [day-03](./day-03-32bit-c/) |
| **6+** | 多 `.c` / `.nas` 分割编译；**不要跳天合并工程** |

每日完成后，将 `proj\day-NN\` 复制一份归档，便于回滚对照。

---

## 6. Day 0 自检清单

- [ ] tolset 解压完成，`nask.exe` 可执行
- [ ] QEMU 可启动（`--version` 正常）
- [ ] 工程路径 **无中文、无空格**
- [ ] `make` 生成 `helloos.img`（1,474,560 B）
- [ ] `qemu-system-i386 -fda helloos.img` 出现 `hello, world`
- [ ] 阅读 [LEARNING_PLAN.md](./LEARNING_PLAN.md) 与 [day-01-boot-asm/](./day-01-boot-asm/)

---

## 相关

- [LEARNING_PLAN.md](./LEARNING_PLAN.md) — 三阶段总方案
- [README.md](./README.md) — 模块导读
- [day-01-boot-asm/](./day-01-boot-asm/) — Day 1 笔记
