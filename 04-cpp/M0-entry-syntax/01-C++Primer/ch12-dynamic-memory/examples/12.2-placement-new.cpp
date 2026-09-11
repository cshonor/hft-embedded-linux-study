// 12.2 直接内存管理 —— 可跑例子
// 重点：placement new 到底是什么、为什么内存池离不开它
//
// 编译运行：
//   clang++ -std=c++20 12.2-placement-new.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <cstring>
#include <new>

struct Packet {
    int  seq;
    char payload[16];

    Packet(int s, const char *p) : seq(s) {
        std::strncpy(payload, p, sizeof(payload) - 1);
        payload[sizeof(payload) - 1] = '\0';
        std::printf("    [构造] seq=%d payload=\"%s\"\n", seq, payload);
    }
    ~Packet() {
        std::printf("    [析构] seq=%d\n", seq);
    }
};

int main() {
    // 一块自己管理的原始内存，不是 new 出来的
    alignas(Packet) unsigned char buf[sizeof(Packet)];

    std::printf("buf 地址       = %p\n", (void *)buf);
    std::printf("sizeof(Packet) = %zu\n\n", sizeof(Packet));

    std::printf("步骤 1: 在 buf 上构造对象（不申请任何内存）\n");
    Packet *p = new (buf) Packet(42, "hello");
    std::printf("    p 地址       = %p\n", (void *)p);
    std::printf("    p == buf ?   %s\n\n",
                ((void *)p == (void *)buf) ? "是 —— 对象就长在 buf 里" : "否");

    std::printf("步骤 2: 手动析构（不归还内存）\n");
    p->~Packet();
    std::printf("    buf 仍然有效，可以再构造一个\n\n");

    std::printf("步骤 3: 在同一个 buf 上构造另一个对象\n");
    Packet *q = new (buf) Packet(43, "world");
    std::printf("    q 地址       = %p\n", (void *)q);
    std::printf("    q == p ?     %s\n\n",
                ((void *)q == (void *)p) ? "是 —— 复用了同一块内存" : "否");

    q->~Packet();

    std::printf("要点:\n");
    std::printf("  new / malloc       = 要内存 + 构造\n");
    std::printf("  placement new      = 只构造，内存你自己给\n");
    std::printf("  析构可以手动调     = 构造和内存分配是两件事\n");
    std::printf("  -> 这就是内存池的原理：内存拿一次，反复构造/析构\n");
    return 0;
}
