// 3.3.2-2  「initializer_list 匹配失败 → 回退」这条反直觉规则 + 窄化检查
//
// 编译：g++ -std=c++17 -O2 -Wall 3.3.2-2-init-fallback.cpp -o fb && ./fb
//
// 结论速记：
//   vector<int>    v{3}   -> size=1   （int 能从 3 构造，列表生效）
//   vector<string> v{3}   -> size=3   （string 不能从 3 构造 → 回退成 v(3)！）
//   vector<Level>  v{100} -> size=100 （聚合类型 → 回退）
//   vector<Explic> v{100} -> 编译错误  （回退后要默认构造，没有）

#include <iostream>
#include <string>
#include <vector>

struct Level  { double bid{0}, ask{0}; };   // 聚合类型：无 int 构造函数
struct Explic { explicit Explic(int) {} };  // explicit 单参：不参与隐式转换

int main() {
    std::cout << "=== 1. 列表生效 vs 回退：只取决于元素类型能否从该值构造 ===\n";

    std::vector<int> i3{3};
    std::cout << "vector<int>    v{3}   size=" << i3.size()
              << "   <- 列表生效（int 能构造）\n";

    std::vector<double> d3{3};
    std::cout << "vector<double> v{3}   size=" << d3.size()
              << "   <- 列表生效\n";

    std::vector<std::string> s3{3};
    std::cout << "vector<string> v{3}   size=" << s3.size()
              << "   v[0]=\"" << s3[0] << "\"   <- 回退！不是 1 个元素\n";

    std::vector<Level> l100{100};
    std::cout << "vector<Level>  v{100} size=" << l100.size()
              << "  v[0].bid=" << l100[0].bid
              << " v[99].ask=" << l100[99].ask << "   <- 回退成 100 个默认对象\n";

    std::cout << "\n=== 2. 回退后仍能带初值：v(3, \"x\") 语义 ===\n";
    std::vector<std::string> sx{3, "x"};
    std::cout << "vector<string> v{3,\"x\"} size=" << sx.size()
              << "  v[0]=\"" << sx[0] << "\" v[2]=\"" << sx[2] << "\"\n";
    std::cout << "  （3 不能构造 string → 整列表匹配失败 → 回退成 v(3, \"x\")）\n";

    std::cout << "\n=== 3. 回退但无默认构造 → 硬编译错误 ===\n";
    std::cout << "  vector<Explic> v{100};  // 取消注释后报错：\n";
    std::cout << "  error: no matching constructor for initialization of 'Explic'\n";
    std::cout << "  （回退到 v(100) 要默认构造 100 个，而 Explic 只有 explicit Explic(int)）\n";
    // std::vector<Explic> bad{100};   // ← 取消注释即编译失败

    std::cout << "\n=== 4. 窄化：花括号拒绝，圆括号静默截断 ===\n";
    std::vector<int> paren(1, 2.5);            // 只 warning，值被截断
    std::cout << "vector<int> v(1, 2.5)  size=" << paren.size()
              << "  v[0]=" << paren[0] << "   <- 2.5 静默变 2（仅 -Wliteral-conversion）\n";
    std::vector<double> ok{1, 2.5};            // double→double 不窄化
    std::cout << "vector<double> v{1,2.5} size=" << ok.size() << "   合法\n";
    std::cout << "  vector<int> v{1, 2.5}; // 取消注释后报错：\n";
    std::cout << "  error: type 'double' cannot be narrowed to 'int' in initializer list\n";
    // std::vector<int> narrowed{1, 2.5};      // ← 取消注释即编译失败

    std::cout << "\n=== 5. 工程结论 ===\n";
    std::cout << "  表达「个数」→ 一律用圆括号 v(n) / v(n, value)\n";
    std::cout << "  表达「列出具体元素」→ 用花括号 v{a, b, c}\n";
    std::cout << "  初始化完立刻 assert(v.size() == expected) 锁死长度\n";

    return 0;
}
