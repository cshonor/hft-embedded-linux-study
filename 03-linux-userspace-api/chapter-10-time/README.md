# TLPI 第 10 章 — Time

**优先级**：见 [OUTLINE](../OUTLINE.md) · [CHAPTER-MAP](../CHAPTER-MAP.md)  

---

## 小节目录

- [10.3 要点梳理](./notes/10.3-section-10-3.md)

---

## 章节定位


（待填 · 读 Kerrisk Ch 10）

→ 全书：[../README.md](../README.md) · 对照：[../CHAPTER-MAP.md](../CHAPTER-MAP.md)

---


---

## 1. 本章目标




---

## 2. 核心 API / syscall




---

## 4. C 示例摘要




---

## 5. Rust 对照（`std` / `libc` / crate）




---

## 6. 常见坑与面试点




---

## 7. 背诵卡


| # | 要点 |
|---|------|
| 1 | |

---


---

## 参考


- Kerrisk, *The Linux Programming Interface*, Chapter 10



---

## 代码示例

```c
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* Ch10 时间 — time/gettimeofday/clock_gettime/nanosleep。
 * 演示多种获取时间的方式 + 精度差异。
 * 编译: gcc -o ch10_demo ch10_demo.c */

int main(void) {
    /* time(): 秒级精度 */
    time_t t = time(NULL);
    printf("time():       %ld (%s)", (long)t, ctime(&t));

    /* gettimeofday(): 微秒精度 */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    printf("gettimeofday: %ld.%06ld\n", (long)tv.tv_sec, (long)tv.tv_usec);

    /* clock_gettime(CLOCK_MONOTONIC): 纳秒精度，不受系统时间调整影响 */
    struct timespec ts1, ts2;
    clock_gettime(CLOCK_MONOTONIC, &ts1);

    nanosleep(&(struct timespec){0, 100 * 1000 * 1000}, NULL);  /* 100ms */

    clock_gettime(CLOCK_MONOTONIC, &ts2);
    long delta_ns = (ts2.tv_sec - ts1.tv_sec) * 1000000000L
                  + (ts2.tv_nsec - ts1.tv_nsec);
    printf("nanosleep(100ms) actual: %ld ns (%.2f ms)\n",
           delta_ns, delta_ns / 1000000.0);
    return 0;
}

```

---

## 参考

- [OUTLINE](../OUTLINE.md)
- 原始笔记：[notes.md.bak](./notes.md.bak)
