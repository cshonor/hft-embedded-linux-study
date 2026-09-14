// 12.4 实战：文本查询程序 —— 可跑例子
// 用「存活对象计数器」看裸指针漏 delete 和智能指针自动释放的区别
//
// 编译运行：
//   clang++ -std=c++20 12.4-raw-vs-smart.cpp -o /tmp/demo && /tmp/demo

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

struct Doc {
    static int alive;
    std::string title;

    Doc(const char *t) : title(t) {
        ++alive;
        std::printf("    [构造] \"%s\"  存活=%d\n", title.c_str(), alive);
    }
    ~Doc() {
        --alive;
        std::printf("    [析构] \"%s\"  存活=%d\n", title.c_str(), alive);
    }
};
int Doc::alive = 0;

void build_raw() {
    std::printf("  [裸指针版本]\n");
    Doc *a = new Doc("doc-a");
    Doc *b = new Doc("doc-b");
    Doc *c = new Doc("doc-c");
    delete a;
    delete b;
    std::printf("    （忘了 delete c —— 这是最常见的泄漏写法）\n");
}

void build_raw_early_return() {
    std::printf("  [裸指针 + 中途返回版本]\n");
    Doc *a = new Doc("early-a");
    Doc *b = new Doc("early-b");
    std::printf("    出错，提前 return\n");
    return;                       // 两个都没 delete
    (void)a; (void)b;
}

void build_smart() {
    std::printf("  [智能指针版本]\n");
    std::vector<std::unique_ptr<Doc>> docs;
    docs.push_back(std::make_unique<Doc>("doc-a"));
    docs.push_back(std::make_unique<Doc>("doc-b"));
    docs.push_back(std::make_unique<Doc>("doc-c"));
    std::printf("    （函数结束，全部自动释放）\n");
}

int main() {
    std::printf("--- 1. 裸指针：漏一个 ---\n");
    build_raw();
    std::printf("  >>> 函数返回后存活对象 = %d  <- 泄漏了\n\n", Doc::alive);

    Doc::alive = 0;
    std::printf("--- 2. 裸指针：提前 return ---\n");
    build_raw_early_return();
    std::printf("  >>> 函数返回后存活对象 = %d  <- 泄漏得更多\n\n", Doc::alive);

    Doc::alive = 0;
    std::printf("--- 3. 智能指针：不管怎么退出都干净 ---\n");
    build_smart();
    std::printf("  >>> 函数返回后存活对象 = %d  <- 干净\n", Doc::alive);

    std::printf("\n  要点：智能指针的价值不是「少打几个字」，而是「异常/提前返回也安全」。\n");
    std::printf("  裸指针要写对，得在所有出口都记得 delete —— 漏一个就是泄漏。\n");
    return 0;
}
