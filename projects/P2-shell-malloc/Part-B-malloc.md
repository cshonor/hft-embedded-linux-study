# Part B — 自制 malloc/free 实现指南

> 从零实现一个堆分配器。先写最简单的隐式链表，再逐步升级到分离适配。每一步都能跑测试。

## 你在做什么

`malloc` 的本质是**管理一块大内存**（从 OS 申请来的），把它切成小块分给用户。`free` 是把用完的小块还回来，跟相邻空闲块合并。

```
堆内存布局（通过 sbrk 向 OS 扩展）：

低地址                                                     高地址
┌──────┬───────────────────────────────────────────┬──────┐
│ 头块  │  block1  block2  block3  ...  blockN      │ brk  │
│(哨兵) │  [H|payload|P] [H|payload|P] ...           │      │
└──────┴───────────────────────────────────────────┴──────┘
         ↑ 每个块 = Header + Payload + (Footer)
         ↑ H = 块大小 + 分配位    P = 块大小 + 分配位（边界标记）
```

你的任务就是管理这些 block：分配时找一个够大的空闲块切出去，释放时标记为空闲并跟邻居合并。

## 文件结构

```
P2-shell-malloc/
├── malloc/
│   ├── Makefile
│   ├── mm.c            ← 你的 malloc/free 实现
│   ├── mm.h            ← 接口声明
│   ├── memlib.c        ← 模拟堆内存（sbrk 封装）
│   ├── memlib.h
│   └── test/
│       ├── stree.c      ← 正确性测试
│       ├── stress.c     ← 压力测试
│       └── throughput.c ← 吞吐量测试
```

## 接口

```c
// mm.h — 你的实现必须匹配这些签名（跟 libc 一样）
void *mymalloc(size_t size);
void  myfree(void *ptr);
void *myrealloc(void *ptr, size_t newsize);
void *mycalloc(size_t nmemb, size_t size);
```

## 关键约束

| 约束 | 值 | 为什么 |
|------|------|--------|
| 对齐 | 8 字节（或 16） | malloc 返回的地址必须对齐，否则 SIMD/原子操作会崩 |
| 最小块大小 | 16 字节 | Header(4) + Footer(4) + 最小 payload(8) |
| size 编码 | 右移 3 位存 | 8 字节对齐 → 低 3 位永远是 0 → 可以用来存标志位 |

---

## Phase 1：隐式空闲链表（1-2 小时）

**目标：** 最简单的 malloc 能跑通基础测试。不做合并，不做优化，先让它工作。

### 数据结构

整个堆就是一条**隐式链表**——每个 block 的 Header 里有 size，顺着 size 走就能遍历到下一个 block。"隐式"是因为没有 next/prev 指针，靠 size 算出下一个块的位置。

```
Header 结构（4 字节，复用 CSAPP 的编码方式）：

  31                           3  2  1  0
 ┌──────────────────────────────┬──┬──┬──┐
 │       block size (字节)       │ 0│ 0│ A│
 └──────────────────────────────┴──┴──┴──┘
                                  ↑
                          A = 1 已分配, A = 0 空闲

注意：size 包含 Header + Footer，是整个块的大小
因为 8 字节对齐，size 的低 3 位永远是 0
所以低位可以用来存分配标志
```

### 核心代码

```c
// mm.c — Phase 1: 隐式空闲链表

#include "mm.h"
#include "memlib.h"
#include <string.h>

// 基本常量
#define WSIZE       4               // 字大小（Header/Footer）
#define DSIZE       8               // 双字大小
#define CHUNKSIZE   (1 << 12)       // 扩展堆的默认大小 (4096)

#define MAX(x, y)   ((x) > (y) ? (x) : (y))

// 把 size 和 alloc 标志打包成一个字
#define PACK(size, alloc)  ((size) | (alloc))

// 读写一个字（在地址 p 处）
#define GET(p)       (*(unsigned int *)(p))
#define PUT(p, val)  (*(unsigned int *)(p) = (val))

// 从打包的值中读出 size 和 alloc
#define GET_SIZE(p)   (GET(p) & ~0x7)
#define GET_ALLOC(p)  (GET(p) & 0x1)

// 给定 block 的 payload 指针（就是 malloc 返回给用户的指针）
// 计算各种地址
#define HDRP(bp)  ((char *)(bp) - WSIZE)                    // Header 地址
#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // Footer 地址

// 下一个 / 上一个 block 的 payload 指针
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

// 全局变量：指向第一个 block 的 payload
static char *heap_listp;

// 初始化堆
int mm_init(void) {
    // 创建初始堆：4 个字 [padding][prologue_header][prologue_footer][epilogue_header]
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0);                          // padding（对齐用）
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // prologue header（永不分陪的哨兵块）
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     // epilogue header（堆尾标记，size=0, alloc=1）
    heap_listp += (2*WSIZE);  // 指向 prologue 的 payload

    // 扩展堆，给点初始空间
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

// 向 OS 申请更多内存
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    // 保证双字对齐：words 是奇数就加 1
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // 新块的 Header 和 Footer
    PUT(HDRP(bp), PACK(size, 0));          // 空闲块
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));  // 新的 epilogue header

    return coalesce(bp);  // 尝试跟前面的空闲块合并
}

// mymalloc
void *mymalloc(size_t size) {
    size_t asize;       // 对齐后的 block 大小
    size_t extendsize;  // 找不到够大的块时要扩展的大小
    char *bp;

    if (size == 0)
        return NULL;

    // 计算需要的 block 大小（加上 Header+Footer，对齐到 8）
    if (size <= DSIZE)
        asize = 2 * DSIZE;  // 最小块 = Header(4) + Footer(4) + payload(8) = 16
    else
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);

    // 在隐式链表里找第一个够大的空闲块
    bp = find_fit(asize);
    if (bp != NULL) {
        place(bp, asize);   // 切分（如果太大就分裂）
        return bp;
    }

    // 找不到：扩展堆
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

// 首次适配：从头到尾找第一个 >= asize 的空闲块
static void *find_fit(size_t asize) {
    char *bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) {  // 遇到 epilogue(size=0) 停止
        if (!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp)) >= asize)
            return bp;
        bp = NEXT_BLKP(bp);
    }
    return NULL;  // 没找到
}

// 在空闲块 bp 中放入 asize 大小的分配，剩余部分如果够大就分裂
static void place(void *bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));

    if ((csize - asize) >= (2 * DSIZE)) {
        // 分裂：前半部分分配，后半部分空闲
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    } else {
        // 不够分裂，整块分配
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

// myfree
void myfree(void *bp) {
    if (bp == NULL) return;
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));  // 清除分配位
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);                   // 合并相邻空闲块
}

// 合并：检查前后块是否空闲，合并
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {
        // 前后都分配了，不合并
    } else if (prev_alloc && !next_alloc) {
        // 后面空闲，合并后面
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {
        // 前面空闲，合并前面
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {
        // 前后都空闲，合并三块
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}
```

### 为什么需要边界标记（Footer）

释放一个块时，要检查**前一个块**是否空闲。但只看 Header 只能往后走，怎么往前看？

答案：Footer。每个块的 Footer 里也存了 size+alloc，而 Footer 的地址可以从当前块算出来：`FTRP(PREV_BLKP(bp))`。有了 Footer，释放时四个方向都能看到。

### 测试

```c
// test/basic.c
#include "mm.h"
#include <stdio.h>
#include <string.h>

int main() {
    mm_init();

    char *a = mymalloc(100);
    char *b = mymalloc(200);
    char *c = mymalloc(100);

    strcpy(a, "hello");
    strcpy(b, "world");

    printf("a = %s\n", a);   // hello
    printf("b = %s\n", b);   // world

    myfree(b);
    char *d = mymalloc(50);   // 应该复用 b 的空间
    printf("d reused b? %s\n", d < c ? "likely yes" : "no");

    return 0;
}
```

### 常见坑

| 坑 | 原因 | 解决 |
|----|------|------|
| mymalloc 返回的地址不对齐 | 没算好 Header 偏移 | 确保 payload 指针 = Header 地址 + WSIZE，且整体 8 对齐 |
| 释放后数据损坏 | coalesce 写错了 Footer 位置 | 画图！每次 PUT 确认写到哪个地址 |
| 死循环 | find_fit 没检查 size=0 | epilogue 的 size=0，循环条件是 `size > 0` |
| sbrk 失败 | 申请太大 | memlib 有上限，默认 20MB |

### 卡住翻哪篇笔记

| 问题 | 翻哪 |
|------|------|
| 为什么要对齐 | CSAPP 9.9.5 实现问题 |
| 边界标记怎么工作 | CSAPP 9.9.10-9.9.11 合并与边界标记 |
| 首次适配 vs 最佳适配 | CSAPP 9.9.12 分配器综合 |
| 碎片是什么 | CSAPP 9.9.4 碎片 |

---

## Phase 2：显式空闲链表（1 小时）

**目标：** 隐式链表每次 malloc 都要遍历整个堆（O(n)），太慢。显式链表把所有空闲块用 next/prev 指针串起来，只遍历空闲块。

### 数据结构变化

空闲块的 payload 前两个字存 next/prev 指针（只有空闲块才有，分配块不需要）：

```
空闲块：
┌────────┬──────┬──────┬──────────────────────┬────────┐
│ Header │ next │ prev │    (剩余空间)          │ Footer │
│  4B    │ 4B   │ 4B   │                       │  4B    │
└────────┴──────┴──────┴──────────────────────┴────────┘

分配块（next/prev 不需要）：
┌────────┬──────────────────────────┬────────┐
│ Header │       payload             │ Footer │
│  4B    │                           │  4B    │
└────────┴──────────────────────────┴────────┘
```

最小块大小变成 16 字节（Header + next + prev + Footer）。

### 核心改动

```c
// 空闲链表操作宏
#define GET_NEXT(bp)  (*(void **)(bp))       // 空闲块的 next 指针
#define GET_PREV(bp)  (*(void **)(bp + WSIZE)) // 空闲块的 prev 指针
#define SET_NEXT(bp, val)  (*(void **)(bp) = (val))
#define SET_PREV(bp, val)  (*(void **)(bp + WSIZE) = (val))

static char *free_list_head = NULL;

// 从空闲链表删除
static void remove_from_free_list(void *bp) {
    void *next = GET_NEXT(bp);
    void *prev = GET_PREV(bp);
    if (prev)
        SET_NEXT(prev, next);
    else
        free_list_head = next;
    if (next)
        SET_PREV(next, prev);
}

// 插入到空闲链表头部（LIFO 策略）
static void insert_to_free_list(void *bp) {
    SET_NEXT(bp, free_list_head);
    SET_PREV(bp, NULL);
    if (free_list_head)
        SET_PREV(free_list_head, bp);
    free_list_head = bp;
}
```

malloc 时从 `free_list_head` 开始遍历，free 时插入链表头。coalesce 合并后把合并的大块插入链表。

### 性能对比

| | 隐式链表 | 显式链表 |
|---|---------|---------|
| malloc 遍历 | 所有块（含已分配） | 只遍历空闲块 |
| free | O(1) + coalesce | O(1) + coalesce + 链表操作 |
| 内存开销 | 每 block 只 Header+Footer | 空闲块多 8B（next+prev） |

### 卡住翻哪篇笔记

- CSAPP 9.9.13 显式空闲链表

---

## Phase 3：分离适配（1-2 小时）

**目标：** 显式链表还是一条长链表。分离适配按大小分类，每类一条链表，查找更快。

### 数据结构

```c
// 大小类：2^k 到 2^(k+1)-1
// 类 0: [1, 2]
// 类 1: [3, 4]
// 类 2: [5, 8]
// 类 3: [9, 16]
// ...
// 类 k: [2^k + 1, 2^(k+1)]

#define NUM_CLASSES  20

static char *free_lists[NUM_CLASSES];  // 每个大小类一条链表

// 根据 size 算大小类
static int size_class(size_t size) {
    int class = 0;
    size_t bound = 2;
    while (class < NUM_CLASSES - 1 && size > bound) {
        class++;
        bound <<= 1;
    }
    return class;
}
```

### malloc 改动

```c
void *mymalloc(size_t size) {
    // ... 计算 asize ...

    int cls = size_class(asize);

    // 从对应大小类的链表开始找
    for (int c = cls; c < NUM_CLASSES; c++) {
        void *bp = find_fit_in_class(c, asize);
        if (bp) {
            place(bp, asize);
            return bp;
        }
    }
    // 当前类和更大的类都没有 → 扩展堆
    // ...
}
```

### 为什么分离适配快

申请 16 字节 → 只在 class 3 的链表里找 → 链表很短 → 快。
不用从 4096 字节的大块链表开始找，避免遍历一堆不相关的大块。

这就是 glibc ptmalloc 的基本思路（加上一些优化如 fastbin/smallbin/largebin）。

### 卡住翻哪篇笔记

- CSAPP 9.9.14 分离空闲链表

---

## Phase 4：压力测试 + 调优（1 小时）

### 正确性测试

```c
// test/stress.c — 随机分配/释放，检查数据不损坏
#include "mm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NALLOC 1000
#define MAXSIZE 512

int main() {
    mm_init();

    void *ptrs[NALLOC] = {0};
    int sizes[NALLOC] = {0};

    for (int round = 0; round < 10000; round++) {
        int idx = rand() % NALLOC;

        if (ptrs[idx] == NULL) {
            // 分配
            sizes[idx] = rand() % MAXSIZE + 1;
            ptrs[idx] = mymalloc(sizes[idx]);
            assert(ptrs[idx] != NULL);

            // 写入魔数验证数据完整性
            memset(ptrs[idx], (idx & 0xFF), sizes[idx]);
        } else {
            // 验证数据没被损坏
            unsigned char *p = ptrs[idx];
            for (int i = 0; i < sizes[idx]; i++) {
                assert(p[i] == (idx & 0xFF));
            }
            // 释放
            myfree(ptrs[idx]);
            ptrs[idx] = NULL;
        }
    }

    // 释放所有
    for (int i = 0; i < NALLOC; i++)
        if (ptrs[i]) myfree(ptrs[i]);

    printf("stress test passed!\n");
    return 0;
}
```

### 常见 bug

| Bug | 症状 | 排查方法 |
|-----|------|----------|
| 合并错误 | 数据损坏 / assert 失败 | 画 block 布局图，手算每个 PUT 的地址 |
| 链表指针写错 | segfault / 死循环 | 在 remove/insert 后打印链表内容验证 |
| size 计算错 | 块重叠 | mymalloc 后打印返回地址和 block 范围 |
| 对齐错 | 偶尔崩在 SIMD 指令 | 打印返回地址 % 8 |

### 吞吐量测试

```c
// test/throughput.c — 测 ops/sec
#include <time.h>

int main() {
    mm_init();
    clock_t start = clock();

    void *ptrs[10000];
    for (int i = 0; i < 10000; i++)
        ptrs[i] = mymalloc(rand() % 256 + 1);
    for (int i = 0; i < 10000; i++)
        myfree(ptrs[i]);

    clock_t end = clock();
    double ms = (double)(end - start) * 1000 / CLOCKS_PER_SEC;
    printf("20000 ops in %.1f ms = %.0f ops/sec\n",
           ms, 20000.0 / (ms / 1000));
    return 0;
}
```

对比你的 malloc 和 glibc malloc 的吞吐量，差距是正常的——glibc 优化了 20 年。

---

## 完成检查清单

- [ ] Phase 1：隐式链表 + 合并 + 分裂，通过基础测试
- [ ] Phase 2：显式空闲链表，空闲块用 next/prev 串联
- [ ] Phase 3：分离适配，按大小类分链表
- [ ] 压力测试 10000 轮随机 alloc/free 不 assert
- [ ] 吞吐量测试跑通，记录 ops/sec
- [ ] 用 `myrealloc` 实现（提示：malloc 新块 + memcpy + free 旧块）

## 学完你应该能回答

1. malloc 返回的地址为什么必须对齐？不对齐会怎样？
2. 隐式链表怎么遍历到下一个块？不用 next 指针靠什么？
3. 释放一个块时，怎么知道前一个块是不是空闲的？为什么需要 Footer？
4. 分离适配为什么比显式链表快？大小类怎么分？
5. 碎片是什么意思？内部碎片和外部碎片有什么区别？
6. `sbrk` 和 `mmap` 向 OS 申请内存有什么区别？什么时候用哪个？
