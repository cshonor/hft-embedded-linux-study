# TLPI 第 49 章 — Memory Mappings

**优先级**：🔴（文件 IO / 分配 / IPC 交汇）  
**前置**：[Ch48 SysV 共享内存](../chapter-48-sysv-shared-memory/README.md)  
**后置**：[Ch50 虚拟内存操作](../chapter-50-virtual-memory/README.md) · [Ch51 POSIX IPC](../chapter-51-posix-ipc-intro/README.md)

---

## 小节目录

- [49.1 四大组合](notes/49.1-overview.md)
- [49.2 –49.3 `mmap` / `munmap`](notes/49.2-creating-a-mapping-mmap.md)
- [49.4 文件映射](notes/49.10-the-map-fixed-flag.md)
- [49.5 `msync`](notes/49.5-synchronizing-a-mapped-region-msync.md)
- [49.6 –49.8 Flags · `mremap`](notes/49.6-additional-mmap-flags.md)

---

## 章节目标


`mmap`/`munmap`/`msync`；四大组合；PRIVATE vs SHARED；SIGBUS；匿名映射；与 read/SysV shm 对比。

---


---

## 横向对比


| | read/write | mmap 文件 |
|--|------------|-----------|
| 拷贝 | 用户↔页缓存两次 | 直接碰页缓存，少一次 |
| 代价 | 系统调用 | 缺页；小顺序 IO 未必更快 |

| | 共享匿名 mmap | SysV shm | 共享文件 mmap |
|--|---------------|----------|---------------|
| 进程 | 仅亲缘 | 任意本机 | 任意本机（同文件） |
| 持久 | 末 munmap 毁 | 内核持久+RMID | 可回盘 |

共享区同样：**勿存裸指针，用 offset**。

---


---

## 陷阱清单


1. `offset` 未页对齐 → 失败  
2. SHARED 越过 EOF → SIGBUS  
3. PRIVATE 改文件「不生效」  
4. munmap ≠ msync  
5. 匿名 SHARED 不能给无关进程  
6. prot > open 权限  
7. 共享区裸指针  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | PRIVATE=COW 不回盘；SHARED=可见可回盘 |
| 2 | 四组合：私匿/私文/共匿/共文 |
| 3 | fork 继承；exec 全毁 |
| 4 | EOF 越界 SHARED → SIGBUS |
| 5 | 落盘用 msync；匿名 fd=-1 |
| 6 | mmap 大文件随机访问常更省拷贝 |
| 7 | mmap 只画区不搬数据，访问才缺页填页；设备独占靠驱动锁（非 mmap 能力） |

---


---

## 参考


- Kerrisk · TLPI Ch49（非「第 15 章」误标）  
- `man 2 mmap` · `munmap` · `msync` · `mremap`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/* Ch49 内存映射 — mmap/munmap/msync/mremap。
 * mmap 可以映射文件/匿名内存, 父子进程可共享。
 * 编译: gcc -o ch49_demo ch49_demo.c */

int main(void) {
    /* === 匿名共享映射 (父子进程共享) === */
    int *shared = mmap(NULL, sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);
    if (shared == MAP_FAILED) { perror("mmap"); return 1; }

    *shared = 0;
    printf("Initial value: %d\n", *shared);

    pid_t pid = fork();
    if (pid == 0) {
        /* 子进程: 修改共享映射 */
        for (int i = 0; i < 5; i++) {
            (*shared)++;
            printf("Child: value=%d\n", *shared);
            usleep(100000);
        }
        _exit(0);
    }

    /* 父进程: 也能看到变化 */
    for (int i = 0; i < 5; i++) {
        usleep(100000);
        printf("Parent sees: value=%d\n", *shared);
    }
    waitpid(pid, NULL, 0);

    printf("Final value: %d\n", *shared);

    /* === 文件映射 === */
    int fd = open("/tmp/ch49_mmap.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "Hello, mmap!", 12);

    char *fmap = mmap(NULL, 12, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (fmap != MAP_FAILED) {
        /* 通过内存修改文件内容 */
        fmap[0] = 'J';  /* Hello -> Jello */
        msync(fmap, 12, MS_SYNC);  /* 同步到文件 */
        printf("File content after mmap modify: %.12s\n", fmap);
        munmap(fmap, 12);
    }
    close(fd);
    remove("/tmp/ch49_mmap.txt");

    munmap(shared, sizeof(int));
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
