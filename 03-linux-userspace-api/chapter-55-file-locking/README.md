# TLPI 第 55 章 — File Locking

**优先级**：🔴（文件同步；单实例 pid 文件）  
**前置**：[Ch54 POSIX shm](../chapter-54-posix-shared-memory/README.md) · 文件 I/O  
**后置**：[Ch56 Sockets 导论](../chapter-56-sockets-intro/README.md)

---

## 小节目录

- [55.1 模型](notes/55.1-overview.md)
- [55.2 `flock`](notes/55.2-file-locking-with-flock.md)
- [55.3 –55.4 `fcntl` 记录锁](notes/55.3-record-locking-with-fcntl.md)
- [55.5 劝告 vs 强制（Linux）](notes/55.5-the-proc-locks-file.md)
- [55.6 –55.9 对比 · 运维 · 场景](notes/55.6-running-just-one-instance-of-a-program.md)

---

## 章节目标


`flock` / `fcntl` 记录锁；绑定对象与 close/fork；劝告 vs 强制；pid 文件；死锁与对比。

---


---

## 陷阱


1. fcntl：关「无关」fd 也放全锁  
2. flock：dup 后 close 放锁  
3. flock+fcntl 混用失效  
4. GETLK 非原子获取  
5. stdio：锁内外 `fflush`；关键路径用 `read`/`write`  
6. NFS 锁不可靠  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 默认劝告；协作才有效 |
| 2 | flock=OFD 整文件；fcntl=进程+区 |
| 3 | fcntl：任意 close → 全锁放 |
| 4 | fork 两类都不继承锁 |
| 5 | 勿混用 flock/fcntl |
| 6 | pid 文件用 F_WRLCK 单实例 |

---


---

## 参考


- Kerrisk · TLPI Ch55（非「第 14 章」误标）  
- `man 2 flock` · `fcntl` · `man 3 lockf`


---

## 代码示例

```c
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* Ch55 文件锁 — flock(BSD 整文件锁) vs fcntl(POSIX 记录锁)。
 * 演示两种文件锁的使用。
 * 编译: gcc -o ch55_demo ch55_demo.c */

#define LOCK_FILE "/tmp/ch55_lock.txt"

int main(void) {
    /* 创建测试文件 */
    int fd = open(LOCK_FILE, O_RDWR | O_CREAT, 0644);
    write(fd, "test data\n", 10);

    /* === flock: BSD 锁, 锁整个文件 === */
    printf("=== flock (BSD whole-file lock) ===\n");
    if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
        printf("Acquired exclusive flock\n");

        pid_t pid = fork();
        if (pid == 0) {
            int cfd = open(LOCK_FILE, O_RDWR);
            /* 子进程尝试获取锁, 会失败 (非阻塞) */
            if (flock(cfd, LOCK_EX | LOCK_NB) < 0)
                printf("Child: flock failed (already locked)\n");
            else
                printf("Child: got flock (unexpected)\n");
            close(cfd);
            _exit(0);
        }
        waitpid(pid, NULL, 0);
        flock(fd, LOCK_UN);  /* 释放 */
    }
    close(fd);

    /* === fcntl: POSIX 记录锁, 可锁文件的一部分 === */
    printf("\n=== fcntl (POSIX record lock) ===\n");
    fd = open(LOCK_FILE, O_RDWR);

    /* 锁定字节 0-9 */
    struct flock fl;
    fl.l_type = F_WRLCK;    /* 写锁 */
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 10;           /* 锁 10 字节 */

    if (fcntl(fd, F_SETLK, &fl) == 0) {
        printf("Locked bytes 0-9 (exclusive)\n");

        pid_t pid = fork();
        if (pid == 0) {
            int cfd = open(LOCK_FILE, O_RDWR);
            struct flock cfl = fl;
            /* 尝试锁同一区域 */
            if (fcntl(cfd, F_GETLK, &cfl) == 0) {
                if (cfl.l_type == F_UNLCK)
                    printf("Child: region unlocked\n");
                else
                    printf("Child: region locked by pid %d\n",
                           (int)cfl.l_pid);
            }
            close(cfd);
            _exit(0);
        }
        waitpid(pid, NULL, 0);

        /* 解锁 */
        fl.l_type = F_UNLCK;
        fcntl(fd, F_SETLK, &fl);
        printf("Unlocked\n");
    }

    close(fd);
    remove(LOCK_FILE);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
