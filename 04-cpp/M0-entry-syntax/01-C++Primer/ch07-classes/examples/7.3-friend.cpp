// 7.3 友元 —— 明确授权某个函数/类访问私有成员
// 实测：clang++ -std=c++20 -Wall 7.3-friend.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>

class Account;   // 前向声明

// 审计员：不是成员函数，却被授权访问 Account 的私有成员
class Auditor {
public:
    void audit(const Account& a);
};

class Account {
    friend void print(const Account& a);   // 友元函数
    friend class Auditor;                  // 友元类
public:
    explicit Account(long bal) : balance_(bal) {}
private:
    long balance_;                         // 外部不可见，friend 除外
};

void print(const Account& a) {
    // 普通函数却摸得到私有成员 —— friend 授权的结果
    printf("[print] 余额 = %ld\n", a.balance_);
}

void Auditor::audit(const Account& a) {
    printf("[audit] 余额 = %ld（友元类的成员函数）\n", a.balance_);
}

int main() {
    Account acct(987654);
    print(acct);        // 友元函数：直接调用，不是 acct.print()
    Auditor au;
    au.audit(acct);     // 友元类

    // 友元的三条铁律（都不是双向/传递的）：
    // 1. 友元关系不传递：A 是 B 的友元、B 是 C 的友元 => A 不是 C 的友元
    // 2. 友元关系不互惠：Account 没有授权 Auditor，Auditor 的私有对 Account 不可见
    // 3. 友元声明不继承：派生类不会自动继承基类的 friend
    printf("\n友元 = 精确到函数/类的开洞，比 public 整扇门小得多。\n");
    return 0;
}
