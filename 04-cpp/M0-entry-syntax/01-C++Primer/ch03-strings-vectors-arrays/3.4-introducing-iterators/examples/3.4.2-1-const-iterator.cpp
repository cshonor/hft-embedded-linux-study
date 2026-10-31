// 3.4.2-1  begin / end / cbegin / cend：返回类型、权限、非成员版
//
// 编译：clang++ -std=c++17 -O2 -Wall 3.4.2-1-const-iterator.cpp -o cit && ./cit

#include <iostream>
#include <iterator>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

template <class A, class B>
static bool same() { return std::is_same<A, B>::value; }

struct Tick { double price; int qty; };

int main() {
    std::vector<int> v{1, 2, 3};

    std::cout << "=== ① 三种调用各返回什么（type_traits 实测）===\n";
    std::cout << "  v.begin()  是 const_iterator? "
              << same<decltype(v.begin()), std::vector<int>::const_iterator>() << "   → iterator\n";
    std::cout << "  v.cbegin() 是 const_iterator? "
              << same<decltype(v.cbegin()), std::vector<int>::const_iterator>() << "   → const_iterator\n";
    const std::vector<int> cv{1, 2, 3};
    std::cout << "  const vector 的 begin() 是 const_iterator? "
              << same<decltype(cv.begin()), std::vector<int>::const_iterator>()
              << "   → 重载按容器 const 性选\n";
    std::cout << "  ※ 所以：函数参数写成 const vector<T>& 就自动获得只读保证，\n"
              << "     cbegin() 的用武之地是【容器本身非 const、但你只想读】。\n";

    std::cout << "\n=== ② iterator 与 const_iterator 是两个类型，且单向转换 ===\n";
    using It  = std::vector<int>::iterator;
    using CIt = std::vector<int>::const_iterator;
    std::cout << "  同一类型?                      " << same<It, CIt>() << "   ← 不同类型\n";
    std::cout << "  iterator → const_iterator ?    "
              << std::is_convertible<It, CIt>::value << "   ← 可以（加 const 安全）\n";
    std::cout << "  const_iterator → iterator ?    "
              << std::is_convertible<CIt, It>::value << "   ← 不行（标准不提供去 const）\n";

    std::cout << "\n=== ③ *it 的类型决定能不能改 ===\n";
    std::cout << "  decltype(*v.begin())  是 int&?        " << same<decltype(*v.begin()), int&>() << "\n";
    std::cout << "  decltype(*v.cbegin()) 是 const int&?  " << same<decltype(*v.cbegin()), const int&>() << "\n";
    std::cout << "  // *v.cbegin() = 10;   ← 取消注释会编译错误\n";
    std::cout << "  ※ decltype(*it) 是【引用】——所以 auto x = *it 会拷贝一份，\n"
              << "     要避免拷贝必须写 auto& / const auto&\n";

    std::cout << "\n=== ④ 空容器：begin() == end() ===\n";
    std::vector<int> e;
    std::cout << "  e.begin()==e.end()? " << (e.begin() == e.end())
              << "   (e.empty()=" << e.empty() << ", end()-begin()=" << (e.end() - e.begin()) << ")\n";
    std::cout << "  ※ 遍历循环天然正确（一次都不进），不需要先判空\n";

    std::cout << "\n=== ⑤ 非成员版 std::begin/end (C++11)、std::cbegin/cend (C++14) ===\n";
    std::cout << "  *std::begin(v) = " << *std::begin(v)
              << "   std::end(v)-std::begin(v) = " << (std::end(v) - std::begin(v)) << "\n";
    std::cout << "  std::cbegin(v) 是 const_iterator? "
              << same<decltype(std::cbegin(v)), std::vector<int>::const_iterator>() << "\n";
    int arr[3] = {7, 8, 9};
    std::cout << "  C 数组: *std::begin(arr) = " << *std::begin(arr)
              << "   长度 = " << (std::end(arr) - std::begin(arr)) << "\n";
    std::cout << "  ※ arr.begin() 【不存在】（C 数组没有成员函数）→ 模板里一律用非成员版\n";

    std::cout << "\n=== ⑥ std::as_const（C++17）===\n";
    const auto& cvref = std::as_const(v);
    std::cout << "  as_const(v).begin() 是 const_iterator? "
              << same<decltype(cvref.begin()), std::vector<int>::const_iterator>() << "\n";
    std::cout << "  ※ 比 cbegin 更通用：作用于变量本身，不限于容器\n";

    std::cout << "\n=== ⑦ 遍历只读元素要写 const auto&（别写 auto）===\n";
    std::vector<Tick> ticks{{100.5, 10}, {200.5, -3}};
    std::cout << "  sizeof(Tick) = " << sizeof(Tick) << " 字节\n";
    double sum = 0;
    for (const auto& x : ticks) if (x.qty > 0) sum += x.price;   // 零拷贝
    std::cout << "  for (const auto& x : ticks) → sum = " << sum << "（无拷贝）\n";
    std::cout << "  for (auto x : ticks)          → 每轮拷贝 " << sizeof(Tick) << " 字节，白白浪费\n";

    std::cout << "\n=== ⑧ string 也有完整迭代器 ===\n";
    std::string s = "abc";
    std::cout << "  *s.begin()=" << *s.begin() << "  长度=" << (s.end() - s.begin())
              << "  sizeof(string::iterator)=" << sizeof(std::string::iterator) << "\n";
    return 0;
}
