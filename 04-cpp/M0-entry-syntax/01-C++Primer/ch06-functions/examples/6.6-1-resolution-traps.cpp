// 6.6-1-resolution-traps.cpp —— 重载决议的实际落点：模板 vs 非模板、初始化列表、nullptr、提升 vs 转换
//
// 编译运行：
//   clang++ -std=c++17 -Wall -O2 6.6-1-resolution-traps.cpp -o a && ./a
//
// 每个用例都直接跑出"到底选了谁"，比背规则好记。

#include <cstdio>
#include <initializer_list>
#include <string>
#include <cstddef>

// ---------- 1. 非模板 vs 模板：同等匹配时选非模板 ----------
void pick(int)        { printf("    -> pick(int)        【非模板】\n"); }
template <class T>
void pick(T&&)        { printf("    -> pick(T&&)        【模板，完美转发】\n"); }

// ---------- 2. 初始化列表重载：`{}` 优先匹配 initializer_list ----------
void take2(int, int)                        { printf("    -> take2(int,int)\n"); }
void take2(std::initializer_list<int>)      { printf("    -> take2(initializer_list<int>)\n"); }

// ---------- 3. nullptr / 0 / NULL 的三岔口 ----------
void which(char*)  { printf("    -> which(char*)\n"); }
void which(int)    { printf("    -> which(int)\n"); }

// ---------- 4. 整型提升（promotion）优先于整型转换（conversion） ----------
void conv(int)    { printf("    -> conv(int)\n"); }
void conv(double) { printf("    -> conv(double)\n"); }

// ---------- 5. 引用绑定：左值 → 左值引用，右值 → 右值引用 ----------
void bind(const int&) { printf("    -> bind(const int&)\n"); }
void bind(int&&)      { printf("    -> bind(int&&)\n"); }

// ---------- 6. std::string 的"能转就能匹配" ----------
void str(const std::string&) { printf("    -> str(const std::string&)\n"); }
void str(const char*)        { printf("    -> str(const char*)\n"); }

int main() {
    printf("=== 6.6-1 重载决议实际落点 ===\n\n");

    printf("[1] 非模板 vs 模板（同等匹配 → 非模板胜出）\n");
    pick(1);                    // int 实参：非模板精确匹配
    pick(1.5);                  // double：只能走模板
    pick(std::string("x"));     // string：只能走模板
    printf("    规则：可行函数里，非模板优先于模板特化（同转换等级时）\n\n");

    printf("[2] {} 与圆括号不是一回事\n");
    take2(1, 2);                // 圆括号 → (int,int)
    take2({1, 2});              // 花括号 → initializer_list
    take2({1, 2, 3});           // 只能 initializer_list（(int,int) 装不下 3 个）
    printf("    规则：列表初始化优先选 initializer_list 版本\n\n");

    printf("[3] nullptr / 0 / NULL\n");
    which(nullptr);             // nullptr_t → char*（不是整型！）
    which(0);                   // int 精确匹配
    // which(NULL);             // ⚠ 见文件末：NULL 在 clang 上是 __null，可能二义或选 int
    printf("    教训：空指针一律写 nullptr，别写 0 / NULL\n\n");

    printf("[4] 提升优先于转换\n");
    conv('a');                  // char → int 是"整型提升"，优于 char → double 的"转换"
    conv(1.5f);                 // float → double 是"浮点提升"，优于 → int 的转换
    conv(short{3});             // short → int 提升
    printf("\n");

    printf("[5] 左值走左值引用，右值走右值引用\n");
    int x = 1;
    bind(x);                    // 左值 → const int&
    bind(1);                    // 右值 → int&&
    bind(x + 1);                // 右值 → int&&
    printf("\n");

    printf("[6] 用户定义转换也算「转换级」，谁都能转就二义\n");
    str("literal");             // const char* 精确匹配（不需要转 string）
    str(std::string("s"));      // string 精确匹配
    // str(0);                  // ❌ 二义：0 既能当 const char* 又能转 string？见文件末注释
    return 0;
}

// ---------------------------------------------------------------------------
// 取消注释可复现：
//
// (a) which(NULL);
//     clang（NULL 定义为 __null）通常报：
//     error: call to 'which' is ambiguous（或静默选 int，取决于平台定义）
//     → 这就是 nullptr 存在的理由：nullptr_t 不会隐式转整型。
//
// (b) str(0);
//     error: call to 'str' is ambiguous
//     note: candidate function (const std::string&)  —— 0 可转 const char* 再构 string
//     note: candidate function (const char*)          —— 0 是空指针常量
//
// 记忆：决议三步走 —— ① 找候选（同名可见）② 找可行（实参能转换过去）
//       ③ 选最优（逐实参比较，必须每项都不差于对手且至少一项更好，否则二义）
