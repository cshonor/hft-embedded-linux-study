// 1.6-1  书店程序 —— 可运行的 Sales_item + 分组累加（含空输入 / 乱序输入实测）
//
// 编译：clang++ -std=c++17 -O2 -Wall 1.6-1-bookstore.cpp -o bookstore
// 运行：printf '0-201-A 3 20.00\n0-201-A 2 25.00\n0-201-B 1 99.00\n' | ./bookstore
// 空输入：printf '' | ./bookstore     → 输出 "No data?!" 并返回 -1（255）
//
// 核心结论（实测）：
//   ① 分组累加依赖输入【按书号有序】，同一本书的记录必须连续
//   ② 乱序输入会漏合并：0-201-A / 0-201-B / 0-201-A 会输出三组，A 被拆成两组
//   ③ 空输入必须防御：if (cin >> total) 包住整个循环，否则对未初始化数据做累加
//   ④ 换组时忘了 total = trans 会把新书累加到旧书上
//   ⑤ 要处理乱序，得用 map / unordered_map（见 1.6-2 的性能对比）

#include <iostream>
#include <string>

class Sales_item {
public:
    Sales_item() = default;
    Sales_item(const std::string& isbn, unsigned u, double p)
        : bookNo_(isbn), units_sold_(u), revenue_(u * p) {}

    // 注意：这些是类内定义 → 隐式 inline
    const std::string& isbn()       const { return bookNo_; }
    unsigned           units_sold() const { return units_sold_; }
    double             revenue()    const { return revenue_; }
    double             avg_price()  const { return units_sold_ ? revenue_ / units_sold_ : 0.0; }

    Sales_item& operator+=(const Sales_item& rhs) {   // 类内定义也要写 operator+=，不能省
        units_sold_ += rhs.units_sold_;
        revenue_    += rhs.revenue_;
        return *this;
    }

private:                                  // private：外部碰不到，只能通过 public 方法
    std::string bookNo_;
    unsigned    units_sold_ = 0;
    double      revenue_    = 0.0;
};

// 运算符重载：定义在类外（真实项目里声明放 .h、定义放 .cpp，记得别漏 Sales_item::）
std::istream& operator>>(std::istream& in, Sales_item& s) {
    std::string isbn; unsigned u; double p;
    if (in >> isbn >> u >> p) s = Sales_item(isbn, u, p);   // 读失败时不动 s
    return in;                                               // 返回值就是流 → 可转 bool
}
std::ostream& operator<<(std::ostream& out, const Sales_item& s) {
    return out << s.isbn() << " " << s.units_sold() << " "
               << s.revenue() << " " << s.avg_price();
}

int main() {
    Sales_item total;

    // ❸ 防御：先确认第一条读成功了，再进循环
    if (std::cin >> total) {
        Sales_item trans;
        while (std::cin >> trans) {              // 读到 EOF 或类型不匹配就停
            if (total.isbn() == trans.isbn()) {
                total += trans;                  // 同一本书 → 累加
            } else {
                std::cout << total << std::endl; // 换书 → 先输出上一本
                total = trans;                   // ❹ 别忘了重置，否则会累加到旧书上
            }
        }
        std::cout << total << std::endl;         // 输出最后一组
    } else {
        std::cerr << "No data?!\n";
        return -1;                               // shell 里看到 255
    }
    return 0;
}

/*
 * ---- 实测输出 ----
 *
 * ① 有序输入：
 *   $ printf '0-201-78345-X 3 20.00\n0-201-78345-X 2 25.00\n0-201-78346-X 1 99.00\n' | ./bookstore
 *   0-201-78345-X 5 110 22
 *   0-201-78346-X 1 99 99
 *
 * ② 空输入：
 *   $ printf '' | ./bookstore ; echo "exit=$?"
 *   No data?!
 *   exit=255
 *
 * ③ 乱序输入（A / B / A）：
 *   $ printf '0-201-A 1 10.0\n0-201-B 1 20.0\n0-201-A 1 10.0\n' | ./bookstore
 *   0-201-A 1 10 10      ← A 被拆成两组
 *   0-201-B 1 20 20
 *   0-201-A 1 10 10
 *
 * 结论：这个程序是"流式分组"，只保证【相邻同组】能合并。
 *      要真正按 key 聚合，用 map/unordered_map（第 10 章的关联容器）。
 */
