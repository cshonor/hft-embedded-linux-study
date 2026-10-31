// 1.5.1-1  头文件与实现文件分离 —— ODR、inline、include guard、增量编译
//
// 编译运行：clang++ -std=c++17 -O2 -Wall 1.5.1-1-header-separation.cpp -o hdr && ./hdr
//
// 本文件是【单文件可跑】的正面演示：把"能放头文件"和"该放 .cpp"的写法都跑一遍。
// 会失败的坏写法以注释 + 实测报错原文列出（复现步骤见文件末尾）。
//
// 核心结论（clang 23.1.0 实测）：
//   ① 非 inline 的函数【定义】放 .h，被两个 .cpp 包含 → 链接期 duplicate symbol
//   ② 改成 inline / 类内定义（隐式 inline）→ 链接通过，nm 显示 weak external（弱符号）
//   ③ 非 inline 的【变量定义】放 .h 同样 duplicate（正确写法：inline 变量 C++17，或 extern 声明）
//   ④ 缺 include guard，同一个 .cpp 里包含两次 → 编译期 redefinition of 'X'
//   ⑤ 声明/实现分离的真实收益：改实现只重编 1 个 .cpp（实测 0.05s），
//      实现写在 .h 里则每个使用者都要重编（N 个使用者 = N 次）

#include <iostream>

// ============================================================
// ❷ 可以放头文件的：inline 函数定义
// ============================================================
inline int add_ok(int a, int b) { return a + b; }        // 显式 inline

struct Adder {                                            // 类内定义 = 隐式 inline
    int a, b;
    int sum() const { return a + b; }
};

// ============================================================
// ❸ 可以放头文件的：inline 变量（C++17）
// ============================================================
inline int g_counter = 0;   // C++17 起允许多 TU 各有一份，链接器合并

// ============================================================
// ❺ 该放 .cpp 的：只把【声明】留在头文件
// ============================================================
int price_of(int qty);      // 声明（真实项目里这行在 .h，实现在 .cpp）

// 下面这个就是"实现"，真实项目里应该放在另一个 .cpp 里：
int price_of(int qty) { return qty * 100 + 5; }

// 注意 Class:: 前缀：成员函数在类外定义时必须带，否则变成普通自由函数
class Sales_item {
public:
    const char* isbn() const;        // 只声明
private:
    const char* bookNo_ = "0-201";
};
const char* Sales_item::isbn() const { return bookNo_; }   // ← 必须有 Sales_item::

int main() {
    std::cout << "inline 函数: add_ok(1,2) = " << add_ok(1, 2) << "\n";
    std::cout << "类内定义  : Adder{3,4}.sum() = " << Adder{3, 4}.sum() << "\n";
    std::cout << "inline 变量: g_counter = " << g_counter << "\n";
    std::cout << "分离实现  : price_of(2) = " << price_of(2) << "\n";

    Sales_item item;
    std::cout << "成员函数  : item.isbn() = " << item.isbn() << "\n";
    // item.bookNo_ = "x";   // ← 取消注释会报错：'bookNo_' is a private member

    return 0;
}

/*
 * ============================================================
 * ❶ 复现：非 inline 函数定义放头文件（两个 .cpp 都包含它）
 * ============================================================
 *   // bad.h
 *   #pragma once
 *   int add_bad(int a, int b) { return a + b; }
 *
 *   // u1.cpp
 *   #include "bad.h"
 *   int use1() { return add_bad(1, 2); }
 *
 *   // u2.cpp
 *   #include "bad.h"
 *   int use2() { return add_bad(3, 4); }
 *
 *   clang++ -c u1.cpp -o u1.o ; clang++ -c u2.cpp -o u2.o
 *   clang++ u1.o u2.o main.o -o app
 *
 *   实测报错：
 *     duplicate symbol 'add_bad(int, int)' in:
 *         u1.o
 *         u2.o
 *     ld: 1 duplicate symbol for architecture arm64
 *
 *   改成 inline 后 nm 的差别（符号类型）：
 *     非 inline → 0000000000000000 (__TEXT,__text) external  __Z7add_badii
 *     inline    → 000000000000001c (__TEXT,__text) weak external __Z6add_okii
 *                                                  ↑ weak：允许重复，链接器任选一份
 *
 * ============================================================
 * ❸ 复现：非 inline 变量定义放头文件
 * ============================================================
 *   // var.h
 *   #pragma once
 *   int g_counter = 0;      // 每个 .cpp 都会生成一个 g_counter
 *
 *   实测报错：duplicate symbol '_g_counter' in:  w1.o / w2.o
 *
 * ============================================================
 * ❹ 复现：缺 include guard
 * ============================================================
 *   // noguard.h
 *   struct Node { int v; };
 *
 *   // ng.cpp
 *   #include "noguard.h"
 *   #include "noguard.h"     ← 同一个 TU 里包含两次
 *   int main(){ return 0; }
 *
 *   实测报错：error: redefinition of 'Node'
 *
 * ============================================================
 * ❺ 增量编译实测（2 个使用者）
 * ============================================================
 *   实现放 .cpp ：改一次实现 → 重编 1 个文件  0.05s
 *   实现放 .h   ：改一次实现 → 重编 2 个文件  0.05s + 0.05s
 *   （真实项目里使用者是几十上百个，差距就是这个倍数）
 */
