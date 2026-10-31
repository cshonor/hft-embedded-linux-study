// 4.2.0 —— 优先级 / 结合性 / 求值顺序 实测
// 编译：clang++ -std=c++17 -O0 -Wall examples/4.2.0-1-precedence.cpp -o prec && ./prec
// -Wall 会打出三条真实警告（-Wshift-op-parentheses / -Wparentheses / -Wunsequenced），
// 它们本身就是本节最好的证据，别关掉。
#include <cstdio>

int main() {
    // ① 算术高于移位：1 << 2 + 3 是 1 << (2+3)
    int r1 = 1 << 2 + 3;                  // = 32，不是 (1<<2)+3 = 7
    printf("1) 1 << 2 + 3 = %d          (+ 高于 <<)\n", r1);

    // ② 位与低于相等：测位的经典错写
    unsigned flags = 0x02;
    int r2 = (flags & 0x01 == 0);         // 实为 flags & (0x01 == 0) = 0x02 & 0 = 0
    printf("2) flags=0x02; flags & 0x01 == 0  -> %d   (实为 flags & (0x01==0))\n", r2);
    printf("   真正想写的是 (flags & 0x01) == 0 -> %d\n", ((flags & 0x01u) == 0));

    // ③ 左结合 vs 右结合
    int a = 100, b = 30, c = 5;
    printf("3) a - b - c = %d   ((a-b)-c=%d, a-(b-c)=%d)   (- 左结合)\n",
           a - b - c, (a - b) - c, a - (b - c));
    int x = 100, y = 200, z = 300;
    x = y = z;                            // x = (y = z)，赋值右结合
    printf("4) x = y = z -> x=%d y=%d z=%d   (右结合)\n", x, y, z);

    // ④ 一元右结合，后自增高于解引用
    int u = 5;
    printf("5) -u++ = %d  (等价 -(u++)，一元右结合；之后 u=%d)\n", -(u++), u);
    int arr[3] = {10, 20, 30};
    int* p = arr;
    int v = *p++;                         // 等价 *(p++)
    printf("   *p++ -> v=%d，p 现在指向 %d\n", v, *p);

    // ⑤ & 高于 |
    printf("6) 1|4&2=%d   (1|4)&2=%d   1|(4&2)=%d   (& 高于 |)\n",
           (1 | 4 & 2), ((1 | 4) & 2), (1 | (4 & 2)));

    // ⑥ C++17 起「赋值的右操作数先于左操作数求值」，i = i++ 变良定义
    int i = 5;
    i = i++;                              // C++17 良定义（得 5）；C++11/14 是 UB
    printf("7) i = i++ (初值 5) -> i = %d   (-std=c++11 编译会报 -Wunsequenced)\n", i);

    // ⑦ 但函数实参的求值顺序 C++17 仍未指定
    int n = 0;
    auto bump = [&]() { return ++n; };
    printf("8) g(bump(), bump()) -> %d 和 %d   (顺序未指定，本编译器是左到右)\n",
           bump(), bump());
    return 0;
}
