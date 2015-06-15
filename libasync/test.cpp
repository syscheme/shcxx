#include <iostream>

class A
{
public:
    A(){}
    virtual ~A(){}
    void runa(){
        std::cout<<"A: "<<this<<std::endl;
    }
};

class B {
    public:
        B() {}
        virtual ~B(){}
        void runb(){
        std::cout<<"B: "<<this<<std::endl;
    }
};

class C : public A, public B
{
public:
    C(){}
    virtual ~C(){}
    void runc()
    {
        std::cout<<"C: "<<this<<std::endl;
        runb();
        runa();
    }
};
int main(int argc, char const *argv[])
{
    (new C())->runc();
    return 0;
}