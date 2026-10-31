// 1.4-1  控制流 —— for 变量作用域（含 C 对照）、cin 失败后的值、流状态位、悬空 else
//
// 编译：clang++ -std=c++17 -O2 -Wall 1.4-1-control-flow.cpp -o cf && ./cf
// 喂数据：printf '1 2 3 x\n' | ./cf
// 对照 C：clang -x c -std=c99 cf.c    （见文件末尾说明）
//
// 核心结论（clang 23.1.0 实测）：
//   ① for 里声明的变量，C99/C11/C17 和 C++ 一样【都只在循环内可见】，
//      只有 C90 模式下 clang 作为扩展放行（并告警），i 会泄漏到外层
//   ② cin >> v 失败后 v 的值取决于【失败发生在哪个阶段】：
//        - 类型不匹配（sentry 成功、num_get 失败）→ v 被置 0
//        - 纯 EOF（sentry 就失败）              → v 保持上一次的值
//   ③ 流失败后不 clear() 就继续读，后续读取全部直接失败（哪怕数据还在）
//   ④ while(1) / while(true) / for(;;) / for(;1;) 在 -O2 下汇编【逐条相同】
//   ⑤ 悬空 else 配最近的 if；clang -Wall 会告警 -Wdangling-else

#include <iostream>
#include <string>

// ---- ⑤ 悬空 else：else 配的是【内层】if ----
// -Wall 下会告警：add explicit braces to avoid dangling else [-Wdangling-else]
static int dangling(int a, int b) {
    int r = 0;
    if (a > 0)
        if (b > 0) r = 1;
    else                      // ← 这个 else 属于内层 if (b > 0)
        r = 2;
    return r;
}

int main() {
    // ---- ① for 变量作用域 ----
    for (int i = 0; i < 3; ++i) { /* ... */ }
    // std::cout << i;        // ← 取消注释会报错：use of undeclared identifier 'i'
    // 要在循环后继续用 i，就得提前声明：
    int i = 0;
    for (i = 0; i < 3; ++i) { /* ... */ }
    std::cout << "循环后 i = " << i << "（提前声明才拿得到）\n";

    // ---- ②③ cin 读取失败 ----
    int sum = 0, v = -999;
    while (std::cin >> v) sum += v;
    std::cout << "sum = " << sum << "\n";
    std::cout << "循环结束时 v = " << v << "\n";
    std::cout << "good=" << std::cin.good()
              << " eof=" << std::cin.eof()
              << " fail=" << std::cin.fail()
              << " bad=" << std::cin.bad() << "\n";
    // 实测（输入 1 2 3 x）：sum=6, v=0, good=0 eof=0 fail=1 bad=0
    // 实测（输入 1 2 3 + EOF）：sum=6, v=3, good=0 eof=1 fail=1 bad=0

    // ---- ③ 不 clear() 就继续读 ----
    std::string s;
    std::cin >> s;                                    // 流已经 fail，这次直接失败
    std::cout << "未 clear 直接读 → " << (std::cin.fail() ? "读取失败" : s) << "\n";
    std::cin.clear();                                 // 清掉 failbit
    std::cin >> s;                                    // 卡住的那个 token 还在缓冲区
    std::cout << "clear 后读到 → " << s << "\n";

    // ---- ⑤ 悬空 else 实测 ----
    std::cout << "dangling(1,0)=" << dangling(1, 0)   // 2：外层真、内层假 → 走 else
              << "  dangling(1,1)=" << dangling(1, 1) // 1：两层都真
              << "  dangling(-5,0)=" << dangling(-5, 0) << "\n"; // 0：外层假，整个跳过

    return 0;
}

/*
 * ---- C 对照：for 变量作用域（存成 cf.c）----
 *   #include <stdio.h>
 *   int main(void) {
 *       for (int i = 0; i < 3; i++) { }
 *       printf("i=%d\n", i);
 *       return 0;
 *   }
 *
 *   clang -std=c90 cf.c  →  warning: GCC does not allow variable declarations
 *                            in for loop initializers before C99 [-Wgcc-compat]
 *                            编译放行（clang 扩展），运行输出 i=3
 *   clang -std=c99 cf.c  →  error: use of undeclared identifier 'i'
 *   clang -std=c11 cf.c  →  error: use of undeclared identifier 'i'
 *   clang -std=c17 cf.c  →  error: use of undeclared identifier 'i'
 *   clang++ -std=c++17   →  error: use of undeclared identifier 'i'
 *
 *   结论：C99 起 C 和 C++ 在这一点上【行为一致】，
 *        "C 会泄漏到外层"只在 C90（且编译器放行扩展）时成立。
 */
