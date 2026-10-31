// 1.3-1  注释的三个隐藏陷阱 —— 不能嵌套 / 反斜杠吃掉下一行 / 注释是"一个空格"不是"删除"
//
// 编译运行：clang++ -std=c++17 -O2 -Wall 1.3-1-comment-traps.cpp -o cmt && ./cmt
// 复现陷阱一：clang++ -std=c++17 1.3-nested.cpp   （见文件末尾注释里的代码片段）
//
// 核心结论（clang 23.1.0 实测）：
//   ① /* */ 不能嵌套：第一个 */ 就关掉整个注释，clang 给 warning: '/*' within block comment
//   ② // 末尾的反斜杠 \ 会把下一行也吃掉 —— 因为【行拼接（翻译阶段 2）先于注释处理（阶段 3）】
//   ③ 注释被替换成"一个空格"，不是被删除：a/**/b 是 "a b" 两个 token，不会粘成 ab
//   ④ C++17 移除了三字符词（trigraph）：??/ 在 C++11 下等于反斜杠，在 C++17 下是普通字符

#include <iostream>

int main() {
    // ---- 陷阱 ③ 正面演示：注释 = 一个空格 ----
    const char* s = "he"/* 这里的注释不参与拼接 */"llo";
    std::cout << "字符串字面量仍会拼接: " << s << "  sizeof=" << sizeof("he"/*x*/"llo") << "\n";

    int a = 1, b = 2;
    int sum = a /*加号左边*/ + /*加号右边*/ b;
    std::cout << "a /*c*/ + /*c*/ b = " << sum << "\n";

    // 字符串里的 // 和 /* 是普通字符，不触发注释
    std::cout << "价格 // 这个不是注释\n";
    std::cout << "路径 /* 这个也不是注释 */\n";

    // ---- 陷阱 ③ 反例（取消注释会编译失败）----
    // int ab = a/**/b;
    //   → error: expected ';' at end of declaration
    //   因为展开后是 `int ab = a b ;`，a 和 b 是两个独立 token

    // ---- 陷阱 ② 反例（取消注释后，下一行的声明会消失）----
    // // 这行末尾有个反斜杠 \
    // int x = 42;
    // std::cout << x;
    //   → error: use of undeclared identifier 'x'

    return 0;
}

// ---- 陷阱 ① 复现（存成单独文件 nest.cpp 编译）----
//   #include <iostream>
//   /* 外层注释开始
//    * [内层用斜杠+星号开头]
//    * 外层注释还没结束
//    */
//   int main() { std::cout << "ok\n"; return 0; }
//
//   实测报错：
//     nest.cpp:4:4: warning: '/*' within block comment [-Wcomment]
//     nest.cpp:5:4: error: a type specifier is required for all declarations
//     nest.cpp:5:28: error: expected ';' after top level declarator
//   说明：第一个 "斜杠星号 + 星号斜杠" 组合就把外层注释关掉了，
//         "外层注释还没结束" 那一行被当成代码解析。
//
// ---- 陷阱 ④ trigraph 复现（存成 tg.cpp）----
//   #include <iostream>
//   int main() {
//       // 注释末尾的 ??/ 在 C++11 下等于反斜杠 ??/
//       std::cout << "这行被吃了吗\n";
//       return 0;
//   }
//
//   clang++ -std=c++11 tg.cpp -o tg && ./tg   →  什么都不输出（下一行被吃掉）
//   clang++ -std=c++17 tg.cpp -o tg && ./tg   →  输出「这行被吃了吗」
//   （C++17 起 trigraph 已从语言里移除）
//
// ---- 安全做法：整段禁用代码用预处理指令，不受注释规则限制 ----
//   #if 0
//   /* 这里的注释不会被误关闭 */
//   void old_code() { }
//   #endif
