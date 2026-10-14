// 3.5.6-1-layout-and-decay.cpp
// 编译：g++ -std=c++20 -O1 -S -masm=intel 3.5.6-1-layout-and-decay.cpp
// 实测环境：Compiler Explorer / g132（gcc 13.2.0, x86-64 linux, -O1 -S -masm=intel）
//
// 目的：汇编级验证「内建数组完全继承 C 语义」——
//   作用域内 sizeof 不退化（16） / 传参后退化（8） / 多维步长（24、12）
//
// 预期汇编（gcc 13.2 实测）：
//   size_in_scope:  mov eax, 16 ; ret
//   size_param:    mov eax, 8  ; ret     <- 形参已被改写为 int*
//   size_multi:    mov eax, 24 ; ret
//   size_multi_row:mov eax, 12 ; ret
//   diff_multi:    mov eax, 12 ; ret     <- 行间距 = sizeof(int[3])
//   diff_whole:    mov eax, 4  ; ret
//
// 编译告警（正是退化的自证）：
//   warning: 'sizeof' on array function parameter 'a' will return size of 'int*'
//           [-Wsizeof-array-argument]

#include <cstddef>

int a[4] = {1, 2, 3, 4};   // 连续存储，无边界检查
int m[2][3] = {};          // 二维：m 的类型是 int[2][3]，不是 int(*)[2][3]
int (*pa)[4] = &a;         // 指向「整个数组」的指针（不退化的两种场景之一）

// 不退化：作用域内 sizeof 拿到整个数组的字节数
std::size_t size_in_scope() { return sizeof(a); }

// 退化：形参 int a[4] 被改写为 int*，sizeof 拿到的是指针大小
std::size_t size_param(int a[4]) { return sizeof(a); }

// 多维数组：sizeof(m) = 6 个 int；sizeof(m[0]) = 一行 3 个 int
std::size_t size_multi() { return sizeof(m); }
std::size_t size_multi_row() { return sizeof(m[0]); }

// 多维退化后 m 是 int(*)[3]，指针算术的步长是「一整行」而不是「一个元素」
std::ptrdiff_t diff_multi() {
    return reinterpret_cast<char*>(&m[1]) - reinterpret_cast<char*>(&m[0]);
}

std::ptrdiff_t diff_whole() {
    return reinterpret_cast<char*>(&a[1]) - reinterpret_cast<char*>(&a[0]);
}

// a[i][j] 在 C++23 起是 deprecated 的逗号表达式（见笔记正文），
// 此处仍写二维下标形式 —— 与 C 完全一致。
int get_2d(int i, int j) { return m[i][j]; }
