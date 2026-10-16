// 7.6 类的静态成员 —— static const 整型成员为什么能当数组大小
// 实测：clang++ -std=c++20 -Wall 7.6-static-const-array.cpp -o /tmp/demo && /tmp/demo
//
// 想看「去掉 static 会怎样」：把下面 Entity 里的 static 删掉再编译即可，
// 详见正文那节引的报错。

#include <cstdio>

// ① Cherno 视频里的写法：static const 整型，类内就地初始化
class Entity {
public:
    static const int size = 5;      // 全类一份，且是编译期常量表达式
    int example[size];              // 于是可以拿它当数组大小

    Entity() {
        for (int i = 0; i < size; i++) example[i] = 2;
    }
};

// C++17 起更干净的等价写法：constexpr（隐含 inline，取地址也不用补类外定义）
class Entity2 {
public:
    static constexpr int size = 5;
    int example[size];
    Entity2() { for (int i = 0; i < size; i++) example[i] = 3; }
};

int main() {
    std::printf("--- Entity（static const）---\n");
    Entity e;
    std::printf("Entity::size      = %d\n", Entity::size);
    std::printf("sizeof(Entity)    = %zu 字节 —— 就是 %zu 个 int\n",
                sizeof(Entity), sizeof(Entity) / sizeof(int));
    std::printf("example[0..4]     = ");
    for (int v : e.example) std::printf("%d ", v);
    std::printf("\n");

    // size 是「全类一份」，跟对象无关
    Entity e2;
    std::printf("&e.size  == &e2.size ? %s（两个对象访问的是同一个实体）\n",
                (&e.size == &e2.size) ? "是" : "否");

    std::printf("\n--- Entity2（static constexpr，C++17 推荐）---\n");
    Entity2 f;
    std::printf("Entity2::size     = %d\n", Entity2::size);
    std::printf("sizeof(Entity2)   = %zu 字节\n", sizeof(Entity2));
    std::printf("example[0..4]     = ");
    for (int v : f.example) std::printf("%d ", v);
    std::printf("\n");

    std::printf("\n要点:\n");
    std::printf("  数组大小必须是编译期常量表达式，static const 整型就地初始化正好满足\n");
    std::printf("  去掉 static 就成了「非静态数据成员」，连引用的资格都没有:\n");
    std::printf("      error: invalid use of non-static data member 'size'\n");
    std::printf("  example 是对象内嵌数组，跟着对象走，不需要 new / delete\n");
    return 0;
}
