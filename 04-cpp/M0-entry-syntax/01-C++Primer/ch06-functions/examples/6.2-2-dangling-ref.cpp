// 6.2 悬垂引用：返回局部对象的引用
//
//   clang++ -std=c++17 -O0 -Wall 6.2-2-dangling-ref.cpp -o d && ./d
//
// 预期：
//   编译期：warning: reference to stack memory associated with local variable 'x' returned
//           [-Wreturn-stack-address]
//   运行期：第一次读"碰巧"是 42，第二次读就被 printf 复用栈帧改写成垃圾值
//   → "我试了一下能打印出 42" 完全不能证明它安全

#include <cstdio>

int& bad() {
    int x = 42;
    return x;          // ❌ 返回局部变量的引用
}

int main() {
    int& r = bad();
    printf("  第一次读 r = %d   (看起来能用)\n", r);
    printf("  第二次读 r = %d   (已被栈帧复用改写)\n", r);
    return 0;
}
