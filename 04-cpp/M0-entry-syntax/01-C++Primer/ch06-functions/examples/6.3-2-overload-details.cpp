// 6.3-2-overload-details.cpp —— 重载的几个"边角"规则：extern "C"、默认实参、重载集传递
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 6.3-2-overload-details.cpp -o a && ./a
// 看符号（重载在二进制里是不同符号——name mangling）：
//   clang++ -std=c++17 -c 6.3-2-overload-details.cpp -o /tmp/o.o && nm -g /tmp/o.o | c++filt | grep -i "ovl"

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

// ---------- 1. extern "C" 没有重载 ----------
extern "C" void cfn(int) {}
// extern "C" void cfn(double) {}   // ❌ 见文件末尾：C 链接没有 name mangling，装不下两套符号
//   error: declaration of 'cfn' has a different language linkage / redefinition
extern "C" {
    void cfn2(int) {}
    // void cfn2(int) {}            // 同理，重复定义
}

// ---------- 2. 默认实参会制造二义性 ----------
void amb(int)          { printf("  amb(int)\n"); }
void amb(int, int = 0) { printf("  amb(int,int=0)\n"); }
// amb(1);                          // ❌ 见文件末尾：call to 'amb' is ambiguous
//   ↑ 两个候选都"能被调用"，编译器没有理由偏好谁

// 与"重载"对照：真重载靠形参类型区分，不靠默认值
void ovl(int)    { printf("  ovl(int)\n"); }
void ovl(double) { printf("  ovl(double)\n"); }

// ---------- 3. 重载集不能直接传给算法 ----------
int    sq(int x)    { return x * x; }
double sq(double x) { return x * x; }

// ---------- 4. const 重载：按对象的 const 选版本 ----------
struct S {
    int get()       { return 1; }
    int get() const { return 2; }
};

// ---------- 5. 返回值不同不算重载 ----------
// int    f2(int);
// double f2(int);          // ❌ functions that differ only in return type cannot be overloaded

int main() {
    printf("=== 6.3-2 重载的边角规则 ===\n\n");

    printf("[1] extern \"C\"：没有 name mangling，所以不能重载\n");
    cfn(1);
    cfn2(2);
    printf("    ✅ 同名 C 函数只能有一个签名（nm 里就是裸符号 _cfn / _cfn2）\n");
    printf("    C++ 重载靠 mangling：ovl(int) 和 ovl(double) 是两个不同符号\n\n");

    printf("[2] 默认实参制造二义性\n");
    // amb(1);              // ❌ ambiguous（取消注释可看编译器原话）
    amb(1, 2);              // ✅ 两个实参，只有 amb(int,int) 匹配
    ovl(1);                 // ✅ 真重载：按实参类型挑
    ovl(1.5);
    printf("    教训：能给同一个调用匹配上的多个重载，只要有默认实参就很容易撞车\n\n");

    printf("[3] 把重载集传给算法：必须「指定」其中一个\n");
    std::vector<int> v{1, 2, 3};
    std::vector<int> out(3);
    // std::transform(v.begin(), v.end(), out.begin(), sq);          // ❌ 不知道取哪个 sq
    std::transform(v.begin(), v.end(), out.begin(),
                   static_cast<int (*)(int)>(sq));                   // ✅ 显式指定
    // 或者用 lambda 包一层（现代写法，还能顺便内联）：
    std::transform(v.begin(), v.end(), out.begin(),
                   [](int x) { return sq(x); });                     // ✅ 推荐
    printf("    transform 结果：%d %d %d\n", out[0], out[1], out[2]);
    printf("    重载集不是「一个值」，它是一个名字；要当值传必须先定型\n\n");

    printf("[4] const 重载：按对象的常量性分派（编译期决定）\n");
    S s;
    const S cs;
    printf("    S::get()       非 const 对象 -> %d\n", s.get());
    printf("    S::get() const const  对象 -> %d\n", cs.get());
    printf("    这就是容器 operator[] 有两份的原因（可读版 / 可写版）\n\n");

    printf("[5] 只有返回值不同 —— 不构成重载（编译错误，见文件末注释）\n");
    return 0;
}

// ---------------------------------------------------------------------------
// 取消注释可复现的错误：
//
// (a) extern "C" void cfn(double) {}
//     error: declaration of 'cfn' has a different language linkage
//     或（同一语言链接下）error: redefinition of 'cfn'
//
// (b) amb(1);
//     error: call to 'amb' is ambiguous
//
// (c) std::transform(..., sq);
//     error: no matching function for call to 'transform' ...
//     note: candidate template ignored: couldn't infer template argument '_UnaryOperation'
//     （根本原因：sq 是重载集，无法推导模板实参）
//
// (d) double f2(int); 与 int f2(int); 并存
//     error: functions that differ only in their return type cannot be overloaded
