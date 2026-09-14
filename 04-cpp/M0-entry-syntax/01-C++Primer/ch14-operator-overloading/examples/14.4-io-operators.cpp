// 14.4 输入输出运算符 —— operator>> 解析行情行，operator<< 打印订单
// 实测：clang++ -std=c++20 -Wall 14.4-io-operators.cpp -o /tmp/demo && /tmp/demo

#include <iostream>
#include <sstream>
#include <string>

// 一笔成交：SYMBOL PRICE QTY，如 "AAPL 189.52 300"
struct Trade {
    std::string symbol;
    double      price = 0;
    long        qty   = 0;
};

// operator>> 返回 istream& —— 让 (iss >> t) 本身可以作为条件判断：
// "读到了且没坏" 为 true，EOF/格式错误为 false
std::istream& operator>>(std::istream& is, Trade& t) {
    is >> t.symbol >> t.price >> t.qty;
    if (t.qty <= 0) is.setstate(std::ios::failbit);   // 业务校验也走流状态
    return is;
}

std::ostream& operator<<(std::ostream& os, const Trade& t) {
    return os << t.symbol << " @ " << t.price << " x " << t.qty;
}

int main() {
    // 模拟一段行情回放数据（真实场景来自文件/网络）
    const char* feed = "AAPL 189.52 300\n"
                       "MSFT 415.10 -5\n"      // 负数量 -> failbit
                       "GOOG 141.80 120\n";

    std::istringstream iss(feed);
    Trade t;
    long lineno = 0;
    while (iss >> t) {                    // operator>> 的返回值直接当条件
        ++lineno;
        std::cout << "第 " << lineno << " 笔: " << t << "\n";
    }
    if (iss.fail())
        printf("流进入 fail 状态 —— 坏数据行被识别并终止循环\n");
    printf("有效成交 %ld 笔\n", lineno);

    // 两个关键点：
    // 1. 返回流的引用 -> 链式调用 + while(iss >> t) 判定，这是 IO 惯用法的根基
    // 2. 语法上 14.1 说过：左操作数是流，operator 必须是非成员
    return 0;
}
