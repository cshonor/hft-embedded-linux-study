#include <iostream>

static void set_via_ref(int &r)
{
    r = 100;
}

int main(void)
{
    int x = 0;
    set_via_ref(x);
    std::cout << "C++ reference: x=" << x << "\n";
    return 0;
}
