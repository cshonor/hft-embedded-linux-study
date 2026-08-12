# TLPI 第 52 章 — POSIX Message Queues

**优先级**：🔴（epoll、优先级、notify）  
**前置**：[Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/notes.md)  
**后置**：[Ch53 POSIX 信号量](../chapter-53-posix-semaphores/notes.md)

---

## 小节目录

- [52.1 特性](./notes/52.1-section-52-1.md)
- [52.2 API 要点](./notes/52.2-api.md)
- [52.3 fork / exec](./notes/52.3-fork-exec.md)
- [52.7 `mq_notify`（难点）](./notes/52.7-mqnotify.md)
- [52.8 epoll（Linux）](./notes/52.8-epoll.md)
- [52.9 限额](./notes/52.9-quotas.md)

---

## 章节目标


`mq_open`/`send`/`receive`/`close`/`unlink`；属性；优先级；`mq_notify`；epoll；vs SysV mq。

---


---

## vs System V mq


| | SysV | POSIX |
|--|------|-------|
| 句柄 | 非 fd | fd / epoll |
| 筛选 | mtype | 仅最高优先级 |
| 非阻塞 | 每次 IPC_NOWAIT | 描述符 O_NONBLOCK |
| 删除 | IPC_RMID | unlink+引用计数 |
| 通知 / 超时 | 弱 | notify + timed* |

新事件驱动优先 POSIX；要按类型挑消息仍看 SysV / 自建协议。

---


---

## 陷阱


1. receive 缓冲 < msgsize → 失败  
2. notify 一次性 + 仅空→首条  
3. notify ∥ epoll 竞争  
4. timed 用绝对时间  
5. 名不能 `/a/b`  
6. setattr 改不了容量  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 优先级收；无 type 筛选 |
| 2 | mqd_t 可 epoll（Linux） |
| 3 | unlink / 全 close 销毁 |
| 4 | notify：空→首条；一次失效 |
| 5 | maxmsg/msgsize 创建固定 |
| 6 | 常 `-lrt`；timed 绝对时钟 |

---


---

## 参考


- Kerrisk · TLPI Ch52  
- `man 3 mq_open` · `mq_notify` · `man 7 mq_overview`


---

## 代码示例

```c
#include <stdio.h>
#include <mqueue.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/* Ch52 POSIX 消息队列 — mq_open/mq_send/mq_receive/mq_notify。
 * 演示父子进程通过 POSIX 消息队列通信。
 * 编译: gcc -o ch52_demo ch52_demo.c -lrt */

#define MQ_NAME "/ch52_demo"
#define MSG_SIZE 64

int main(void) {
    struct mq_attr attr = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_msgsize = MSG_SIZE,
        .mq_curmsgs = 0
    };

    mqd_t mqd = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0644, &attr);
    if (mqd < 0) { perror("mq_open"); return 1; }

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程: 发送消息 */
        unsigned prio;
        for (int i = 1; i <= 3; i++) {
            char msg[MSG_SIZE];
            snprintf(msg, MSG_SIZE, "Message %d", i);
            mq_send(mqd, msg, strlen(msg) + 1, i);  /* 优先级 = i */
            printf("Child: sent (prio=%d) %s\n", i, msg);
        }
        _exit(0);
    }

    sleep(1);

    /* 父进程: 接收消息 (高优先级先出) */
    for (int i = 0; i < 3; i++) {
        char buf[MSG_SIZE];
        unsigned prio;
        ssize_t n = mq_receive(mqd, buf, MSG_SIZE, &prio);
        if (n > 0)
            printf("Parent: received (prio=%u) %s\n", prio, buf);
    }

    waitpid(pid, NULL, 0);
    mq_close(mqd);
    mq_unlink(MQ_NAME);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
