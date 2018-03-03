#include "factory.h"

class A {
PersistClass(A);
   int a;
public:
    void set(int i){a=i;}
    int get(){return a;}
    A(int i=0){a=i;}
    void prt(const char *txtEnd);
};

#include "pointer.cpp" // always include when PersistPtr<> is used
