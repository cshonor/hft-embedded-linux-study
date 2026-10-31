// 4.7 —— 条件运算符实测：短路性、公共类型、左值性、与 if 的汇编等价
// 编译：clang++ -std=c++17 -O2 examples/4.7-1-conditional.cpp -o cond && ./cond
// 汇编对照（?: 与 if-else 生成完全相同的指令）：
//   clang++ -std=c++17 -O2 -S -o - - <<'EOF' | grep -E "cmp|cneg"
//   int tern(int x){ return x>0?x:-x; }
//   int ifver(int x){ if(x>0) return x; else return -x; }
//   EOF
#include <cstdio>

int g_l = 0, g_r = 0;
int left()  { ++g_l; return 1; }
int right() { ++g_r; return 2; }

int main() {
    // ① 只求值选中的那个分支（和 && || 一样是短路的）
    int t = (true ? left() : right());
    printf("1) true  ? left() : right() -> val=%d  left calls=%d, right calls=%d\n", t, g_l, g_r);
    g_l = g_r = 0;
    int f = (false ? left() : right());
    printf("   false ? left() : right() -> val=%d  left calls=%d, right calls=%d\n", f, g_l, g_r);

    // ② 公共类型规则：两分支同类型→不提升；不同类型→usual arithmetic conversions
    printf("2) sizeof(cond ? (short)1 : (short)2) = %zu   (同类型，不提升)\n",
           sizeof(true ? (short)1 : (short)2));
    printf("   sizeof(cond ? (char)1  : (char)2 ) = %zu   (同类型，不提升)\n",
           sizeof(true ? (char)1 : (char)2));
    printf("   sizeof(cond ? (short)1 : (char)2 ) = %zu   (不同类型 → 都升到 int)\n",
           sizeof(true ? (short)1 : (char)2));
    printf("   sizeof(cond ? 'a' : 1)             = %zu   (char → int)\n",
           sizeof(true ? 'a' : 1));
    printf("   sizeof(cond ? 1 : 2.5)             = %zu   (int → double)\n",
           sizeof(true ? 1 : 2.5));
    printf("   sizeof(cond ? 1 : 2u)              = %zu   (int → unsigned)\n",
           sizeof(true ? 1 : 2u));

    // ③ C++ 独有：两分支都是同类型左值时，结果是左值，可赋值（C 不行）
    int p = 5, q = 9;
    (p > 0 ? p : q) = 99;                 // C++ 合法；C 里 error: expression is not assignable
    printf("3) (p > 0 ? p : q) = 99 -> p=%d q=%d\n", p, q);

    // ④ 右结合
    int g = 75;
    char c = g > 90 ? 'A' : g > 60 ? 'B' : 'C';   // 等价 g>90?'A':(g>60?'B':'C')
    printf("4) g=75 -> %c   (右结合)\n", c);

    // ⑤ 优先级陷阱：?: 几乎最低，与 << 混用必须加括号
    printf("5) 想打印等级：cout << (g < 60 ? \"fail\" : \"pass\") 必须带括号，\n"
           "   否则解析成 (cout << g) < 60 ? ... ，编译期就报错。\n");
    printf("   g<60?\"fail\":\"pass\" = %s\n", (g < 60 ? "fail" : "pass"));
    return 0;
}
