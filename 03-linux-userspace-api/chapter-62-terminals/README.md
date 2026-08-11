# TLPI 第 62 章 — Terminals

**优先级**：🔴（shell/作业控制/交互程序）  
**前置**：[Ch61 Socket Advanced](../chapter-61-sockets-advanced/notes.md)  
**后置**：[Ch63 Alternative I/O](../chapter-63-alternative-io/notes.md) · [Ch64 PTY](../chapter-64-pseudoterminals/notes.md)

---

## 小节目录

- [62.1 类型](./notes/62.1-types.md)
- [62.2 `termios`](./notes/62.2-termios.md)
- [62.3 输入模式（高频）](./notes/62.3-mode.md)
- [62.4 –62.6 标志 · 控制字符 · 行控制](./notes/62.4-flag.md)
- [62.7 检测](./notes/62.7-detection.md)
- [62.8 –62.9 会话 · 控制终端 · 信号](./notes/62.8-signal-terminal.md)

---

## 章节目标


终端类型；`tcgetattr`/`tcsetattr`；规范/非规范/原始；标志与 `c_cc`；会话/控制终端/前台组；终端相关信号。

---


---

## 陷阱


1. 改属性后未恢复  
2. 改输出用错 TCSANOW  
3. VMIN/VTIME 搞混  
4. 关 ISIG 后 Ctrl+C 无效  
5. 后台读写被暂停  
6. 对非 tty 调 tcgetattr  

---


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


---

## 参考


- Kerrisk · TLPI Ch62  
- `man 3 termios` · `man 3 isatty` · `man 7 termio`


---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
