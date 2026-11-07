// demo2: 修法 —— 构造 thread 时就完成 string 转换，拷贝的是完整字符串
//
// 编译运行：
//   clang++ -std=c++17 -fsanitize=address -pthread 2.2-thread-param-fixed.cpp -o /tmp/d2 && /tmp/d2
// 预期：干净输出 i=3 s=42，ASan 无报告
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>

void f(int i, const std::string& s) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    printf("i=%d s=%s\n", i, s.c_str());
}

void fine() {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%d", 42);
    std::thread t(f, 3, std::string(buf));  // 转换发生在主线程，安全
    t.detach();
}

int main() {
    fine();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    return 0;
}
