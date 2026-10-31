// 3.3.2-1  圆括号 () vs 花括号 {} 初始化：全部写法对照
//
// 编译：g++ -std=c++17 -O2 -Wall 3.3.2-1-brace-vs-paren.cpp -o brace && ./brace
//
// 核心规则：写 v{...} 时编译器【先试 initializer_list<T>】，
//          列表里有元素转不成 T → 回退到圆括号语义 v(n) / v(n, value)。

#include <iostream>
#include <string>
#include <vector>
#include <cassert>

struct Level { double bid{0}, ask{0}; };   // 聚合类型：不能从 int 构造

int main() {
    std::cout << "=== 1. 基本对照（元素类型 int，能从 int 构造） ===\n";

    std::vector<int> a(10);
    std::cout << "vector<int> v(10)      size=" << a.size() << "  v[0]=" << a[0] << "\n";

    std::vector<int> b{10};
    std::cout << "vector<int> v{10}      size=" << b.size() << "  v[0]=" << b[0] << "\n";

    std::vector<int> c(10, 5);
    std::cout << "vector<int> v(10,5)    size=" << c.size() << "  v[0]=" << c[0]
              << " v[9]=" << c[9] << "\n";

    std::vector<int> d{10, 5};
    std::cout << "vector<int> v{10,5}    size=" << d.size() << "  v[0]=" << d[0]
              << " v[1]=" << d[1] << "   <- 不是 10 个 5！\n";

    std::cout << "\n=== 2. 空容器：三种等价写法，别加圆括号 ===\n";
    std::vector<int> e1;
    std::vector<int> e2{};
    std::vector<int> e3 = {};
    std::cout << "v; / v{}; / v = {};    size=" << e1.size() << " "
              << e2.size() << " " << e3.size() << "\n";
    std::cout << "vector<int> v();       → 【函数声明】，不是变量（most vexing parse）\n";
    // 下面这行一旦取消注释就会报错：v 是函数，不能 .size()
    // std::vector<int> f();

    std::cout << "\n=== 3. 类类型元素：() 是 N 个默认构造对象 ===\n";
    std::vector<Level> lv1(100);
    std::cout << "vector<Level> v(100)   size=" << lv1.size()
              << "  v[0].bid=" << lv1[0].bid << "\n";
    std::vector<std::string> sv(3);
    std::cout << "vector<string> v(3)    size=" << sv.size() << "  v[0]=\""
              << sv[0] << "\"\n";

    std::cout << "\n=== 4. 嵌套 vector（矩阵） ===\n";
    std::vector<std::vector<int>> m1(3, std::vector<int>(4));
    std::cout << "m(3, vector<int>(4))   size=" << m1.size()
              << " 行0列数=" << m1[0].size() << "\n";
    std::vector<std::vector<int>> m2{3, std::vector<int>(4)};
    std::cout << "m{3, vector<int>(4)}   size=" << m2.size()
              << " 行0列数=" << m2[0].size() << "   <- 恰好回退成同样结果\n";

    std::cout << "\n=== 5. HFT 用法：8 合约 × 20 档，用圆括号 + assert 锁长度 ===\n";
    std::vector<std::vector<Level>> book(8, std::vector<Level>(20));
    assert(book.size() == 8 && book[0].size() == 20);
    std::cout << "book(8, Level(20))     size=" << book.size()
              << " 每行列数=" << book[0].size() << "  assert 通过\n";

    return 0;
}
