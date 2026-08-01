#include <iostream>

struct Base {
    virtual void show() { std::cout << "Base\n"; }
    virtual ~Base() = default;
};

struct Derive : Base {
    void show() override { std::cout << "C++ virtual: Derive\n"; }
};

int main(void)
{
    Base *p = new Derive();
    p->show();
    delete p;
    return 0;
}
