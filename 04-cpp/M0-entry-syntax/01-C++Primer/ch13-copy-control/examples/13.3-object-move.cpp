// 13.3 对象移动（C++11）—— 可跑例子
// 打印 move 前后源对象和目标对象的状态，看「指针搬家」到底搬了什么
//
// 编译运行：
//   clang++ -std=c++20 13.3-object-move.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <cstring>
#include <utility>
#include <vector>

struct Buffer {
    char *data;
    std::size_t size;

    Buffer(std::size_t n, const char *tag) : data(new char[n]), size(n) {
        std::strncpy(data, tag, n - 1);
        data[n - 1] = '\0';
        std::printf("    [构造]     data=%p size=%2zu \"%s\"\n",
                    (void *)data, size, data);
    }
    Buffer(const Buffer &o) : data(new char[o.size]), size(o.size) {
        std::memcpy(data, o.data, size);
        std::printf("    [拷贝构造] data=%p size=%2zu  <- 新分配一块，逐字节复制\n",
                    (void *)data, size);
    }
    Buffer(Buffer &&o) noexcept : data(o.data), size(o.size) {
        o.data = nullptr;
        o.size = 0;
        std::printf("    [移动构造] data=%p size=%2zu  <- 只搬指针，没分配\n",
                    (void *)data, size);
    }
    ~Buffer() {
        std::printf("    [析构]     data=%p size=%2zu\n", (void *)data, size);
        delete[] data;
    }
};

int main() {
    std::printf("--- 1. 拷贝：两个对象，两块内存 ---\n");
    {
        Buffer a(16, "hello");
        Buffer b = a;
        std::printf("    a.data=%p   b.data=%p  %s\n", (void *)a.data, (void *)b.data,
                    (a.data != b.data) ? "-> 地址不同，各持一份" : "-> 相同？");
    }

    std::printf("\n--- 2. 移动：指针搬家，源对象变空壳 ---\n");
    {
        Buffer a(16, "hello");
        Buffer c = std::move(a);
        std::printf("    a.data=%p  a.size=%2zu   <- 源对象被置空\n",
                    (void *)a.data, a.size);
        std::printf("    c.data=%p  c.size=%2zu   <- 目标接管了那块内存\n",
                    (void *)c.data, c.size);
        std::printf("    a 虽然空，析构依然安全（delete[] nullptr 是无害的）\n");
    }

    std::printf("\n--- 3. vector 扩容时，元素用「移动」而不是「拷贝」 ---\n");
    {
        std::vector<Buffer> v;
        v.reserve(2);
        std::printf("  push_back 第 1 个:\n");
        v.push_back(Buffer(16, "one"));
        std::printf("  push_back 第 2 个:\n");
        v.push_back(Buffer(16, "two"));
        std::printf("  push_back 第 3 个（容量满了，触发扩容，已有元素要搬去新内存）:\n");
        v.push_back(Buffer(16, "three"));
        std::printf("\n  ^ 搬家用的是 [移动构造]，因为移动构造标了 noexcept\n");
        std::printf("    把 noexcept 去掉再跑一次 —— vector 会退化成 [拷贝构造]，\n");
        std::printf("    因为它必须保证「搬一半抛异常」时旧数据还在。\n");
    }
    return 0;
}
