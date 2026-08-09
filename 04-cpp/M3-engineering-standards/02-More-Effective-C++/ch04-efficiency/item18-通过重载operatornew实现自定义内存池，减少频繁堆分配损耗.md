# 条款 18：通过重载 operator new 实现自定义内存池，减少频繁堆分配损耗

## 本节讲什么

> 待补充详细笔记（错误案例、原理、正确写法、代码示例）。

## 示例

```cpp
class Pool {
    char slab[4096];
public:
    void *allocate(std::size_t n);
    void deallocate(void *p);
};
```

---

## 代码自测

**题目 1：** 自定义 `operator new` 实现内存池的核心思路是什么？
```cpp
class Widget {
    static char pool[sizeof(Widget) * 100];
    static bool used[100];
public:
    static void* operator new(size_t s) {
        for (int i = 0; i < 100; i++)
            if (!used[i]) { used[i] = true; return &pool[i]; }
        throw std::bad_alloc();
    }
    static void operator delete(void* p) {
        used[(char*)p - pool] = false;
    }
};
```

<details>
<summary>参考答案</summary>

核心思路：预分配固定大小的内存块，用标记数组管理使用状态。`operator new` 在池中找空闲块（O(n) 或用 free-list O(1)），`operator delete` 标记为空闲。优点：无系统调用、分配/释放 O(1)、缓存友好。缺点：固定大小（只能分配 Widget 大小的对象）、固定容量。生产级实现用 free-list 链表管理空闲块，查找 O(1)。

</details>
