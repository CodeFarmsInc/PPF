#include "stdlib.h"
#include "string.h"
#include "..\..\factory.h"

class A {
PersistClass(A);
    int x;
    int y;
public:
    A(int xx,int yy){x=xx; y=yy;}
    A(){x=y=0;}
    A& operator=(A a){x=a.x; y=a.y; return *this;}
    void set(int xx, int yy){x=xx; y=yy;}
    void prt(int i){cout << i << ": " << x << ", " << y << "\n";}
};

#include "..\..\pointer.cpp" // always use when PersistPtr or PersistVptr is used
