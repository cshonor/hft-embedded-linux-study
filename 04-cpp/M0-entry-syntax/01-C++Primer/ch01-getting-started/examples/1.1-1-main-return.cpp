// 1.1-1  main 与程序入口 —— 返回类型、退出码截断、main 的三条禁令
//
// 编译：clang++ -std=c++17 -O2 -Wall 1.1-1-main-return.cpp -o m && ./m
// 看退出码：./m ; echo "exit=$?"
// 看汇编：clang++ -std=c++17 -O2 -S 1.1-1-main-return.cpp -o m.s
//
// 核心结论（全部在本机 clang 23.1.0 / arm64-apple-darwin 实测）：
//   ① main 必须返回 int（void main / long main 直接编译错误）
//   ② 不写 return 的 main 与 return 0 的 main，汇编【完全相同】：mov w0, #0 ; ret
//   ③ 退出码只有【低 8 位】传出：return 256 → echo $? 得到 0；return -1 → 255
//   ④ 截断发生在 OS 层，不是编译器：return 256 的汇编是 mov w0, #256
//   ⑤ main 不能在程序里被"使用"（调用 / 取地址），-pedantic-errors 下会报错

#include <iostream>

// 下面三种写法都是非法的，取消注释会看到对应报错：
//
//   void main() { }
//     → error: 'main' must return 'int'
//
//   long main() { return 0; }
//     → error: 'main' must return 'int'
//
//   int Main() { return 0; }          // 大小写敏感！
//     → 编译能过，链接报错：
//       Undefined symbols for architecture arm64:
//         "_main", referenced from: implicit entry/start for main executable
//
//   int main() { if (0) main(); return 0; }        // 在表达式里引用 main
//     → 默认只是 clang 扩展（不报错）；加 -pedantic-errors 后：
//       error: referring to 'main' within an expression is a Clang extension [-Wmain]
//
//   int main() { auto p = &main; (void)p; return 0; }
//     → 同上，取地址也属于"使用 main"

int main(int argc, char** argv) {
    std::cout << "argc = " << argc << "（程序名自身也算一个参数）\n";
    for (int i = 0; i < argc; ++i)
        std::cout << "  argv[" << i << "] = " << argv[i] << "\n";

    // 演示退出码：argc==1 时返回 256，否则返回 125
    // 在 shell 里分别跑： ./m ; echo $?   →  0（256 的低 8 位）
    //                    ./m x ; echo $?  →  125
    if (argc == 1) return 256;   // 实际退出码是 0，不是 256
    return 125;                  // 在 0–125 区间内，能原样传出
}
