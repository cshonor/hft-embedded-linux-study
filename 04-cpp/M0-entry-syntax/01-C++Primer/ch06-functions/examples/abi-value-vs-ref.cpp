// 配套 6.1-1：三种形参写法的汇编对照（Apple Silicon / clang）
//
//   clang++ -std=c++17 -O0 -S abi-value-vs-ref.cpp -o - | grep -A4 "^__Z2f"
//
// 预期：
//   fv(int)            传值    → 直接 ldr w8, [sp, #12]
//   fr(const int&)     传引用  → ldr x8,[sp,#8] ; ldr w8,[x8]   ← 与下面完全相同
//   fp(const int*)     传指针  → ldr x8,[sp,#8] ; ldr w8,[x8]
// 即：引用在机器层就是"自动解引用的指针"，零开销指的是省拷贝，不是省寻址。

int fv(int x) { return x + 1; }            // 传值
int fr(const int& x) { return x + 1; }     // 传引用
int fp(const int* x) { return *x + 1; }    // 传指针
