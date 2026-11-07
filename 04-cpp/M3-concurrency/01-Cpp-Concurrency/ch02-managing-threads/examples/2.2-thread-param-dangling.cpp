// demo1: 传局部 buffer 的指针 → 线程读到已销毁栈内存（悬垂）
// 为了把"线程在 buf 销毁后才用指针"这件事变成确定性时序，
// f 里先睡 100ms 再用指针 —— 等价于 slide 例子里
// "新线程此刻才执行 const char* → std::string 转换"的放大版。
//
// 编译运行（必挂 ASan，否则时序碰巧正常看不出问题）：
//   clang++ -std=c++17 -g -O1 -fsanitize=address -pthread 2.2-thread-param-dangling.cpp -o /tmp/d1
//   ASAN_OPTIONS=detect_stack_use_after_return=1 /tmp/d1
// 预期：AddressSanitizer: stack-use-after-scope，访问落在 main 栈帧的 buf 上
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

void f(int i, const char* s) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printf("i=%d s=%s\n", i, s);   // 此时 buf 已销毁 → stack-use-after-return
}

void oops() {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%d", 42);
    std::thread t(f, 3, buf);   // 拷贝进内部存储的只是 char* 指针
    t.detach();                 // buf 随 oops() 返回立即销毁
}

int main() {
    oops();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return 0;
}
