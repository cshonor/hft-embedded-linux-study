// 3.4.1 / 3.4.2 / 3.4.4 —— 迭代器实测：本质、常量性、随机访问算术、失效
// 编译：clang++ -std=c++17 -O2 examples/3.4.1-1-iterators.cpp -o iter && ./iter
#include <cstdio>
#include <iterator>
#include <list>
#include <type_traits>
#include <vector>

int main() {
    printf("① 迭代器的本质（vector）：就是裸指针\n");
    std::vector<int> v{1, 2, 3, 4, 5};
    printf("  sizeof(vector<int>::iterator) = %zu    sizeof(int*) = %zu\n",
           sizeof(std::vector<int>::iterator), sizeof(int*));
    printf("  &*v.begin() == &v[0] ? %d\n", (int)(&*v.begin() == &v[0]));
    std::list<int> l{1, 2, 3};
    printf("  sizeof(list<int>::iterator)   = %zu    （双向，不是裸指针）\n",
           sizeof(std::list<int>::iterator));

    printf("② end() 是尾后，不可解引用\n");
    printf("  &v[0]=%p  &v[4]=%p  &*v.end()=%p  → end() 在最后一个之后\n",
           (void*)&v[0], (void*)&v[4], (void*)&*v.end());
    printf("  v.end() - v.begin() = %td  (== size)\n", v.end() - v.begin());

    printf("③ iterator vs const_iterator vs cbegin\n");
    std::vector<int>::iterator it = v.begin();
    std::vector<int>::const_iterator cit = v.cbegin();
    printf("  *begin()  是 int&         ? %d\n", (int)std::is_same<decltype(*it), int&>::value);
    printf("  *cbegin() 是 int&         ? %d  （const int& → 0，不可改）\n",
           (int)std::is_same<decltype(*cit), int&>::value);
    const std::vector<int>& cv = v;
    printf("  const vector 的 begin() 返回 const_iterator ? %d\n",
           (int)std::is_same<decltype(cv.begin()), std::vector<int>::const_iterator>::value);
    printf("  *cbegin() = %d\n", *cit);

    printf("④ 随机访问算术（vector 有，list 没有）\n");
    printf("  *(v.begin() + 3) = %d\n", *(v.begin() + 3));
    printf("  v.end() - v.begin() = %td，类型是 ptrdiff_t（有符号！sizeof=%zu）\n",
           v.end() - v.begin(), sizeof(std::ptrdiff_t));
    printf("  把差值存进 size_t 会让负数变成天文数字 → 用 auto 或 ptrdiff_t\n");
    // l.begin() + 1;   // ❌ error: 双向迭代器没有 operator+

    printf("⑤ 迭代器失效：push_back 扩容后旧迭代器变野指针\n");
    std::vector<int> w;
    w.reserve(2);
    w.push_back(10);
    int* old_ptr = &w[0];
    printf("  扩容前 &w[0]=%p cap=%zu\n", (void*)old_ptr, w.capacity());
    w.push_back(20);
    w.push_back(30);                       // 触发扩容
    printf("  扩容后 &w[0]=%p cap=%zu  → 旧地址 %s 新地址，旧迭代器失效\n",
           (void*)&w[0], w.capacity(), old_ptr == &w[0] ? "==" : "!=");

    printf("⑥ erase 返回下一个有效位置\n");
    std::vector<int> e{1, 2, 3, 4, 5};
    auto after = e.erase(e.begin() + 1);   // 删掉 2
    printf("  *after = %d（指向原来的 3），删除后元素:", *after);
    for (int x : e) printf(" %d", x);
    printf("\n");

    printf("⑦ 插入会让【插入点之后】的迭代器全部失效（哪怕没扩容）\n");
    std::vector<int> g;
    g.reserve(100);
    g.push_back(1); g.push_back(2); g.push_back(3);
    int* p0 = &g[0];
    size_t cap0 = g.capacity();
    g.insert(g.begin(), 0);
    printf("  插入前 cap=%zu，插入后 cap=%zu（没扩容，&g[0] %s）\n",
           cap0, g.capacity(), p0 == &g[0] ? "没变" : "变了");
    printf("  但元素全都后移了，指向 1/2/3 的旧迭代器现在指向 0/1/2 或已失效。元素:");
    for (int x : g) printf(" %d", x);
    printf("\n");
    return 0;
}
