#include "xregistry.h"
#include <iostream>

class B {
public:
    B() = default;
    virtual ~B() {}
    virtual void Show() { std::cout << "this is B" << std::endl; }
};
REGISTER_SUBCLASS(B, B);

class D final : public B {
public:
    D()  = default;
    ~D() = default;
    void Show() override { std::cout << "this is D" << std::endl; }
};
REGISTER_SUBCLASS(B, D);

int main(int argc, char** argv) {
    B* b1 = Registry<B>::Create("B");
    B* b2 = Registry<B>::Create("D");
    b1->Show();
    b2->Show();
    return 0;
}
