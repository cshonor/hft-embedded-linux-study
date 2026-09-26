# CH4 · Linux / GNU：TLPI 的主场

> **这一章解决什么困惑：** 三个平台里只有 Linux 从头到尾没有官方 IDE，
> 而且工具的名字和其他两边完全对不上（`gdb` 是什么？为什么到处都在说 binutils？）。
>
> 原因很简单：**Linux 的工具链从来没被包装过**，你看见的就是零件本身。
> 坏事是入门陡，好事是学完你真正知道自己写的代码怎么变成进程。

## 本章地图

| 节 | 标题 | 一句话 | 状态 |
|----|------|--------|------|
| [4.1](./4.1-GNU工具链地图.md) | **没有 IDE 的那个世界** | gcc 其实是「司机」不是编译器：cc1 → as → collect2 → ld 四段接力；binutils/glibc 全家福 | ✅ Pi 实测 |
| [4.2](./4.2-三道防线全部沉默.md) | **三道防线全部沉默** | `-Wall -Wextra` 沉默、gcc 14 的 `-fanalyzer` 也沉默；真正救场的是 ASan 与双编译器 | ✅ Pi 实测 |
| [4.3](./4.3-实测gdb.md) | **一次完整的 gdb 会话** | 断点、条件断点、`info locals`、`x/10w` 裸内存、`bt` 调用栈；与 lldb 命令并列对照 | ✅ Pi 实测 |
| [4.4](./4.4-ELF与DWARF.md) | **调试信息就住在 ELF 里** | `-g` 之后文件真的变大；`.debug_info` / `.debug_line` 各管什么；ELF、动态链接器、glibc 三个角色 | ✅ Pi 实测 |
| [4.5](./4.5-Linux上写C用什么.md) | **没有 VS 的日子** | VS Code / CLion / Neovim+clangd / 纯命令行四选一；为什么现阶段建议先用命令行 | 建议型 |

## 为什么要单独给 Linux 一整章

因为**你的三条主线里，有两条的主战场在这里**：

| 主线 | 在哪跑 | 理由 |
|---|---|---|
| TLPI（`03-linux-userspace-api`） | Linux 独占 | 书里讲的每一个系统调用都是 Linux ABI |
| LKD3rd（内核侧） | Linux 源码 | v6.6 源码与你的 Pi 6.18 内核可以对得上 |
| eBPF | Pi 上的 Linux | libbpf / ring buffer 全是 Linux 内核设施 |

反过来，Windows 和 macOS 这两章的定位都是**"认清它们不是 Linux"**——
不是贬低，而是知道边界在哪（Windows 那边的边界写在 [CH5](../05-boundary/README.md)）。

## 你的可用 Linux 环境

| 环境 | 具体规格 | 用途 |
|---|---|---|
| **Raspberry Pi 5**（`wzp@192.168.31.109`） | Debian 13 trixie，kernel 6.18.39 aarch64，gcc 14.2.0 / clang 19.1.7 / gdb 16.3 / cmake 3.31.6 / glibc 2.41 | **主战场**。本章所有实测输出都出自这里 |
| WSL（可选） | Windows 上的 Linux 子系统 | 若你日后回到 Windows 机器，这是最接近原生 Linux 的路径 |

> Pi 上有 **gcc 和 clang 两个编译器并存**，这一点比 Mac 还方便：
> 同一份代码两个都编译一遍，是最便宜的一道 UB 检验（见 [4.2](./4.2-三道防线全部沉默.md)）。

---

> Linux 这一支的价值不在于"它有 gcc"，而在于**它没有隐藏任何东西**。
> 习惯了 `gcc -save-temps` 能看见 `.i / .s / .o`，你就再也不会把 IDE 的
> 「运行」按钮当成魔法 —— 那正是读 TLPI 需要的心态。
