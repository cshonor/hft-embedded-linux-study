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

## 代码示例

```c
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

/* Ch62 终端 — termios API 控制终端属性。
 * 演示关闭回显 (密码输入) + 设置原始模式。
 * 编译: gcc -o ch62_demo ch62_demo.c */

int main(void) {
    /* 保存当前终端设置 */
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;

    printf("=== Echo off demo (type something, it won't show) ===\n");

    /* 关闭回显 */
    new_term.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    char password[64];
    printf("Enter password: ");
    fflush(stdout);
    if (fgets(password, sizeof(password), stdin)) {
        printf("\nYou entered: %s", password);
    }

    /* 恢复回显 */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);

    /* === 原始模式 (无缓冲, 无回显, 无特殊字符处理) === */
    printf("\n\n=== Raw mode demo (press 'q' to quit) ===\n");

    new_term.c_lflag &= ~(ICANON | ECHO);  /* 关闭规范模式 + 回显 */
    new_term.c_cc[VMIN] = 1;               /* 至少读 1 字节 */
    new_term.c_cc[VTIME] = 0;              /* 无超时 */
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    /* 在原始模式下逐字符读取 */
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        printf("Got char: 0x%02x ('%c')\n", (unsigned char)c,
               (c >= 32 && c < 127) ? c : '.');
        if (c == 'q') break;
    }

    /* 恢复原始终端设置 */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    printf("\nTerminal restored.\n");

    /* 获取终端窗口大小 */
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        printf("Terminal size: %d rows x %d cols\n",
               ws.ws_row, ws.ws_col);
    }
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
