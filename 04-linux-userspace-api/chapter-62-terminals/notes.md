# TLPI 第 62 章 — Terminals

> 对应目录：`chapter-62-terminals/`  
> 书名原文：**Terminals**（General Interface）  
> ⚠️ **改 termios 后必须恢复**（崩溃也要：`atexit`/信号），否则终端无回显。规范 vs 非规范看 **`ICANON`**；VMIN/VTIME 定非规范 `read`。Ctrl+C/Z → 打向前台进程组。管道/socket **不是** tty（`isatty`=0）。后置按地图是 [Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md)；**PTY 在 [Ch64](../chapter-64-pseudoterminals/notes.md)**（非 Ch63）。

**优先级**：🔴（shell/作业控制/交互程序）  
**前置**：[Ch61 Socket Advanced](../chapter-61-sockets-advanced/notes.md)  
**后置**：[Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md) · [Ch64 PTY](../chapter-64-pseudoterminals/notes.md)

---

## 章节目标

终端类型；`tcgetattr`/`tcsetattr`；规范/非规范/原始；标志与 `c_cc`；会话/控制终端/前台组；终端相关信号。

---

## 62.1 类型

| 类型 | 例 |
|------|-----|
| 串口 | `/dev/ttyS0` |
| 虚拟控制台 | `/dev/tty1`… |
| **PTY** | master `/dev/ptmx`，slave `/dev/pts/N`（ssh、终端模拟器） |

---

## 62.2 `termios`

```c
tcgetattr(fd, &t);
tcsetattr(fd, TCSANOW|TCSADRAIN|TCSAFLUSH, &t);
```

| 何时生效 | |
|----------|--|
| `TCSANOW` | 立即 |
| `TCSADRAIN` | 输出排空后（改输出标志优先） |
| `TCSAFLUSH` | 排空输出并丢弃未读输入 |

字段：`c_iflag` / `c_oflag` / `c_cflag` / `c_lflag` / `c_cc[]`。

---

## 62.3 输入模式（高频）

| 模式 | |
|------|--|
| **规范**（`ICANON`） | 按行；行编辑；shell 默认 |
| **非规范** | 关 ICANON；`VMIN`+`VTIME` 控 `read` |
| **原始 Raw** | 非规范 + 关信号/回显/输出加工等 |
| Cooked | 口语=规范 |

VMIN/VTIME 四组合：MIN>0 等够字节；MIN=0 可轮询/短超时（TIME 单位 0.1s）。

Demo：[`code/`](./code/)（关 ECHO 读一行再恢复）

---

## 62.4–62.6 标志 · 控制字符 · 行控制

`c_lflag`：`ICANON` · `ECHO` · `ISIG`…  
`ICRNL` / `ONLCR`：`\r`↔`\n` 转换。  
`c_cc`：`VINTR` Ctrl+C→SIGINT；`VSUSP` Ctrl+Z→SIGTSTP；`VEOF` Ctrl+D…  
`tcflush` / `tcflow` / `tcsendbreak`。

---

## 62.7 检测

`isatty(fd)` · `ttyname(fd)` — 区分终端输出与重定向。

---

## 62.8–62.9 会话 · 控制终端 · 信号

会话最多一控制终端；**前台进程组**收 Ctrl+C/Z。  
后台读 → `SIGTTIN`；后台写 → `SIGTTOU`（作业控制基础）。  

SIGINT / SIGQUIT / SIGTSTP / SIGTTIN / SIGTTOU。

---

## 陷阱

1. 改属性后未恢复  
2. 改输出用错 TCSANOW  
3. VMIN/VTIME 搞混  
4. 关 ISIG 后 Ctrl+C 无效  
5. 后台读写被暂停  
6. 对非 tty 调 tcgetattr  

---

## 背诵卡

| # | 要点 |
|---|------|
| 1 | ICANON=规范；关则非规范 |
| 2 | VMIN/VTIME 定非规范 read |
| 3 | 改完必须恢复 termios |
| 4 | Ctrl+C→前台组 SIGINT |
| 5 | 后台读写 → TTIN/TTOU |
| 6 | PTY 细节见 Ch64 |

---

## 参考

- Kerrisk · TLPI Ch62  
- `man 3 termios` · `man 3 isatty` · `man 7 termio`
