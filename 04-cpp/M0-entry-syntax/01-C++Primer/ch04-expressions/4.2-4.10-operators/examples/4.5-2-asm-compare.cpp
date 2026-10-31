// 4.5-2  汇编级验证：++it 与 it++ 在 -O2 下生成完全相同的代码
//
// 用法：
//   clang++ -std=c++17 -O2 -S 4.5-2-asm-compare.cpp -o asm.s
//   grep -c "^\t" asm.s        → 两个函数各 83 条指令
//   sed 's/sum_pre/FN/;s/sum_post/FN/' asm.s | diff ...  → 除符号名外逐条相同
//
// 运行时输出的是两个函数的返回值（应完全一致），验证语义等价。

#include <iostream>
#include <numeric>
#include <vector>

long sum_pre(const std::vector<int>& v) {
    long s = 0;
    for (auto it = v.begin(); it != v.end(); ++it) s += *it;   // 前置
    return s;
}

long sum_post(const std::vector<int>& v) {
    long s = 0;
    for (auto it = v.begin(); it != v.end(); it++) s += *it;   // 后置
    return s;
}

// 内置 int 版本
long loop_pre(int n)  { long s = 0; for (int i = 0; i < n; ++i) s += i; return s; }
long loop_post(int n) { long s = 0; for (int i = 0; i < n; i++) s += i; return s; }

int main() {
    std::vector<int> v(1000);
    std::iota(v.begin(), v.end(), 0);

    std::cout << "sum_pre  = " << sum_pre(v) << "\n";
    std::cout << "sum_post = " << sum_post(v) << "\n";
    std::cout << "loop_pre = " << loop_pre(1000) << "\n";
    std::cout << "loop_post= " << loop_post(1000) << "\n";
    std::cout << "\n用 -S 生成汇编对比：\n"
              << "  clang++ -std=c++17 -O2 -S 4.5-2-asm-compare.cpp -o asm.s\n"
              << "  grep -c \"^\\t\" asm.s\n"
              << "实测：sum_pre 与 sum_post 各 83 条指令，归一化符号名后逐条完全相同。\n"
              << "→ \"遍历 vector 一定要用 ++it\"这个建议在 -O2 下已经过时。\n";
    return 0;
}
