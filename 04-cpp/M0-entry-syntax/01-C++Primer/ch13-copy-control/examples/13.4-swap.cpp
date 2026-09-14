// 13.4 其他应用：swap —— 可跑例子
// 对比「通用 std::swap（三次移动）」和「成员 swap（直接换指针）」
//
// 编译运行：
//   clang++ -std=c++20 13.4-swap.cpp -o /tmp/demo && /tmp/demo

#include <cstddef>
#include <cstdio>
#include <utility>

struct Buffer {
    char *data;
    std::size_t size;

    static int moves, copies;

    Buffer(std::size_t n) : data(new char[n]), size(n) {}
    Buffer(const Buffer &o) : data(new char[o.size]), size(o.size) { ++copies; }
    Buffer(Buffer &&o) noexcept : data(o.data), size(o.size) {
        ++moves;
        o.data = nullptr;
        o.size = 0;
    }
    Buffer &operator=(Buffer &&o) noexcept {
        ++moves;
        if (this != &o) {
            delete[] data;
            data = o.data;
            size = o.size;
            o.data = nullptr;
            o.size = 0;
        }
        return *this;
    }
    ~Buffer() { delete[] data; }

    void swap(Buffer &o) noexcept {              // 成员 swap：只换指针和大小
        std::swap(data, o.data);
        std::swap(size, o.size);
    }
};
int Buffer::moves = 0;
int Buffer::copies = 0;

void swap(Buffer &a, Buffer &b) noexcept {       // 非成员 swap，转发给成员版
    a.swap(b);
}

int main() {
    std::printf("--- 1. std::swap（通用模板版）---\n");
    {
        Buffer a(1024), b(2048);
        Buffer::moves = Buffer::copies = 0;
        std::swap(a, b);
        std::printf("    移动次数 = %d   拷贝次数 = %d\n", Buffer::moves, Buffer::copies);
        std::printf("    ^ 通用版的做法：1 次移动构造 + 2 次移动赋值 = 3 次「搬」\n");
    }

    std::printf("\n--- 2. 成员 swap（直接换指针）---\n");
    {
        Buffer a(1024), b(2048);
        Buffer::moves = Buffer::copies = 0;
        a.swap(b);
        std::printf("    移动次数 = %d   拷贝次数 = %d\n", Buffer::moves, Buffer::copies);
        std::printf("    ^ 0 次 —— 只交换了两个指针和两个整数\n");
    }

    std::printf("\n--- 3. 标准惯用法：让泛型代码自动选中高效版本 ---\n");
    {
        Buffer a(1024), b(2048);
        Buffer::moves = Buffer::copies = 0;
        using std::swap;        // 引入通用版作为兜底
        swap(a, b);             // ADL 会优先选中我们自己的非成员 swap
        std::printf("    移动次数 = %d   拷贝次数 = %d\n", Buffer::moves, Buffer::copies);
        std::printf("    ^ 仍是 0 次 —— 这就是「using std::swap; swap(a,b);」的写法意义\n");
    }

    std::printf("\n  要点：swap 在 HFT 里常用在「热切换」—— 准备一份新的数据结构，\n");
    std::printf("  准备好后一次 swap 换上去，全程只动几个指针，没有停顿。\n");
    return 0;
}
