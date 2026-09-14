// 13.2 资源管理与三五法则 —— 可跑例子
//
// 跑法：
//   ./demo        -> 正确写法（Rule of Five 写全）
//   ./demo bad    -> 错误写法（只写析构、没写拷贝构造）—— 会崩溃，这是刻意的
//
// 编译运行：
//   clang++ -std=c++20 13.2-rule-of-five.cpp -o /tmp/demo
//   /tmp/demo
//   /tmp/demo bad

#include <cstdio>
#include <cstring>
#include <utility>

struct GoodString {
    char *data;

    GoodString(const char *s) : data(new char[std::strlen(s) + 1]) {
        std::strcpy(data, s);
        std::printf("    [构造]     data=%p\n", (void *)data);
    }
    GoodString(const GoodString &o) : data(new char[std::strlen(o.data) + 1]) {
        std::strcpy(data, o.data);
        std::printf("    [拷贝构造] data=%p  <- 新分配一块\n", (void *)data);
    }
    GoodString &operator=(const GoodString &o) {
        if (this != &o) {
            char *nd = new char[std::strlen(o.data) + 1];
            std::strcpy(nd, o.data);
            delete[] data;
            data = nd;
        }
        std::printf("    [拷贝赋值] data=%p\n", (void *)data);
        return *this;
    }
    GoodString(GoodString &&o) noexcept : data(o.data) {
        o.data = nullptr;
        std::printf("    [移动构造] data=%p  <- 只是搬指针\n", (void *)data);
    }
    GoodString &operator=(GoodString &&o) noexcept {
        if (this != &o) {
            delete[] data;
            data = o.data;
            o.data = nullptr;
        }
        std::printf("    [移动赋值] data=%p\n", (void *)data);
        return *this;
    }
    ~GoodString() {
        std::printf("    [析构]     data=%p\n", (void *)data);
        delete[] data;
    }
};

// ❌ 违反三五法则：有指针成员、写了析构，却没有拷贝构造
//    编译器生成的默认拷贝构造只复制 data 指针 —— 浅拷贝
struct BadString {
    char *data;

    BadString(const char *s) : data(new char[std::strlen(s) + 1]) {
        std::strcpy(data, s);
        std::printf("    [构造] data=%p\n", (void *)data);
    }
    ~BadString() {
        std::printf("    [析构] data=%p  -> delete[]\n", (void *)data);
        delete[] data;
    }
};

template <typename S>
void demo(const char *name) {
    std::printf("--- %s ---\n", name);
    {
        S a("hello");
        std::printf("  a.data = %p\n", (void *)a.data);

        S b = a;                                  // 关键的一行
        std::printf("  b.data = %p\n", (void *)b.data);
        std::printf("  %s\n", (a.data == b.data)
                                ? "  ^ 两个对象指向同一块内存！"
                                : "  ^ 各自一块，安全");

        std::printf("  离开作用域：b 先析构，a 后析构\n");
    }
    std::printf("  （正常走到这里）\n\n");
}

int main(int argc, char **argv) {
    if (argc > 1 && std::strcmp(argv[1], "bad") == 0) {
        std::printf(">>> 错误版本：有指针成员 + 写了析构，但没写拷贝构造\n");
        std::printf(">>> 预期：同一块内存被 delete[] 两次 -> double free -> 崩溃\n\n");
        demo<BadString>("BadString");
        std::printf("如果看到这行，说明这次没崩（未定义行为，纯属侥幸）\n");
    } else {
        std::printf(">>> 正确版本：Rule of Five 写全（析构/拷贝构造/拷贝赋值/移动构造/移动赋值）\n\n");
        demo<GoodString>("GoodString");
        std::printf("想看不写会怎样？  ./demo bad\n");
    }
    return 0;
}
