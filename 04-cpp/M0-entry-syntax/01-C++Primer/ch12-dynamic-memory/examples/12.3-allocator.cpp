// 12.3 动态数组与 allocator —— 可跑例子
// 自己写一个 allocator 挂到 std::vector 上，看它到底被调用几次
//
// 编译运行：
//   clang++ -std=c++20 12.3-allocator.cpp -o /tmp/demo && /tmp/demo

#include <cstddef>
#include <cstdio>
#include <new>
#include <vector>

static long g_alloc = 0;
static long g_dealloc = 0;

template <typename T>
struct CountingAllocator {
    using value_type = T;

    CountingAllocator() = default;
    template <typename U>
    CountingAllocator(const CountingAllocator<U> &) {}

    T *allocate(std::size_t n) {
        ++g_alloc;
        std::printf("      [allocate]   %2zu 个 = %4zu 字节\n", n, n * sizeof(T));
        return static_cast<T *>(::operator new(n * sizeof(T)));
    }
    void deallocate(T *p, std::size_t n) {
        ++g_dealloc;
        std::printf("      [deallocate] %2zu 个 = %4zu 字节\n", n, n * sizeof(T));
        ::operator delete(p);
    }
    template <typename U>
    bool operator==(const CountingAllocator<U> &) const { return true; }
};

int main() {
    std::printf("--- 1. 默认 allocator：push_back 5 次 ---\n");
    {
        std::vector<int> v;
        for (int i = 0; i < 5; i++) {
            v.push_back(i);
            std::printf("  push_back(%d) -> size=%zu capacity=%zu\n",
                        i, v.size(), v.capacity());
        }
    }
    std::printf("  (默认 allocator 不打印，看不到它偷偷分配了几次)\n\n");

    std::printf("--- 2. 换成 CountingAllocator：同样 push_back 5 次 ---\n");
    g_alloc = g_dealloc = 0;
    {
        std::vector<int, CountingAllocator<int>> v;
        for (int i = 0; i < 5; i++) {
            v.push_back(i);
            std::printf("  push_back(%d) -> size=%zu capacity=%zu\n",
                        i, v.size(), v.capacity());
        }
    }
    std::printf("  >>> allocate 次数 = %ld\n\n", g_alloc);

    std::printf("--- 3. 先 reserve(5) 再 push_back ---\n");
    g_alloc = g_dealloc = 0;
    {
        std::vector<int, CountingAllocator<int>> v;
        v.reserve(5);
        for (int i = 0; i < 5; i++) v.push_back(i);
    }
    std::printf("  >>> allocate 次数 = %ld\n", g_alloc);
    std::printf("\n  ^ 扩容的代价 = 分配新内存 + 把旧元素全部搬过去 + 释放旧的\n");
    std::printf("    reserve 把多次分配压成 1 次 —— 这就是 HFT 预先分配、\n");
    std::printf("    避免在关键路径上调用 malloc 的原因\n");
    return 0;
}
