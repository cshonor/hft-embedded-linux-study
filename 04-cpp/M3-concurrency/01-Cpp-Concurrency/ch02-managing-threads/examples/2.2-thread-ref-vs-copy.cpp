// demo3: 对照实验 —— 默认拷贝 vs std::ref 真引用
// 注意：std::ref / std::cref 在 <functional> 里，不在 <thread>
//
// 编译运行：
//   clang++ -std=c++17 -pthread 2.2-thread-ref-vs-copy.cpp -o /tmp/d3 && /tmp/d3
// 预期：after copy-pass: copy（原值不变）/ after ref-pass: ref [modified in thread]
#include <cstdio>
#include <functional>
#include <string>
#include <thread>

void by_copy(std::string s)  { s += " [modified in thread]"; }
void by_ref(std::string& s)  { s += " [modified in thread]"; }

int main() {
    std::string a = "copy";
    std::thread t1(by_copy, a);        // 形参虽是值，但即便写成 string& 也一样是拷贝
    t1.join();
    printf("after copy-pass: %s\n", a.c_str());   // 原值不变

    std::string b = "ref";
    std::thread t2(by_ref, std::ref(b)); // 显式 reference_wrapper → 真共享
    t2.join();
    printf("after ref-pass:  %s\n", b.c_str());   // 原值被改
    return 0;
}
