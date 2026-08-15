# TLPI 第 50 章 — Virtual Memory Operations

**优先级**：🔴（低延迟 / JIT / 大映射调优）  
**前置**：[Ch49 mmap](../chapter-49-memory-mappings/README.md)  
**后置**：[Ch51 POSIX IPC 导论](../chapter-51-posix-ipc-intro/README.md)

---

## 小节目录

- [50.1 总览](notes/50.1-changing-memory-protection-mprotect.md)
- [50.2 `mprotect`](notes/50.1-changing-memory-protection-mprotect.md)
- [50.3 `mlock` / `mlockall`](notes/50.2-memory-locking-mlock-and-mlockall.md)
- [50.4 `mincore`](notes/50.3-determining-memory-residence-mincore.md)
- [50.5 `madvise`](notes/50.5-summary.md)

---

## 章节目标


`mprotect` · `mlock*` · `mincore` · `madvise`；页对齐约束；实时锁定与 advice 语义。

---


---

## 对比速记


| 调用 | 作用 | 权限 |
|------|------|------|
| mprotect | 改 r/w/x | 通常不需 root |
| mlock* | 禁 swap | ulimit / 特权 |
| mincore | 驻留快照 | 否 |
| madvise | 访问提示 | 否 |

---


---

## 陷阱


1. 未页对齐 → `EINVAL`  
2. mprotect 越权 → `EACCES`  
3. mlock 非引用计数叠加  
4. mincore 不可靠同步  
5. `MADV_DONTNEED` + PRIVATE 丢数据（Linux）  
6. `MCL_FUTURE` 耗尽锁定额  

---


---

## 背诵卡


| # | 要点 |
|---|------|
| 1 | 四件套：protect / lock / mincore / advise |
| 2 | 页对齐；JIT 写完再 RX |
| 3 | mlock 抗 swap；ulimit；exec 解锁 |
| 4 | mincore 仅快照 |
| 5 | madvise 非强制；DONTNEED 移植坑 |
| 6 | 大量 mlock 挤内存 |

---


---

## 参考


- Kerrisk · TLPI Ch50（非「第 45 章」误标）  
- `man 2 mprotect` · `mlock` · `mincore` · `madvise`


---

## 代码示例

```c
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

/* Ch50 虚拟内存 — mprotect/madvise/mincore。
 * 演示修改内存保护属性 + 内存使用建议。
 * 编译: gcc -o ch50_demo ch50_demo.c */

int main(void) {
    long pagesize = sysconf(_SC_PAGESIZE);
    printf("Page size: %ld bytes\n", pagesize);

    /* 分配页对齐的内存 */
    char *mem = mmap(NULL, pagesize,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS,
                     -1, 0);
    if (mem == MAP_FAILED) { perror("mmap"); return 1; }

    /* 写入数据 */
    strcpy(mem, "writable data");
    printf("Wrote: %s\n", mem);

    /* mprotect: 改为只读 */
    if (mprotect(mem, pagesize, PROT_READ) == 0) {
        printf("Memory is now read-only\n");
        printf("Reading still works: %s\n", mem);
        /* 以下写入会触发 SIGSEGV:
         * mem[0] = 'X';  -- 段错误!
         */
        printf("Writing would cause SIGSEGV now\n");
    }

    /* mprotect: 改为不可读写 */
    mprotect(mem, pagesize, PROT_NONE);
    printf("Memory is now inaccessible (PROT_NONE)\n");
    /* 读或写都会触发 SIGSEGV */

    /* madvise: 给内核内存使用提示 */
    char *mem2 = mmap(NULL, pagesize * 4,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS,
                      -1, 0);
    madvise(mem2, pagesize * 4, MADV_SEQUENTIAL);  /* 顺序访问 */
    printf("madvise(MADV_SEQUENTIAL): hint for sequential access\n");

    munmap(mem, pagesize);
    munmap(mem2, pagesize * 4);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](notes)
