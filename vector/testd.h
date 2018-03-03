#include "stdlib.h"
#include "string.h"
#include "..\factory.h"

#include "testA.h"

class D {
PersistClass(D);
    PersistPtr<A> a;
public:
    D(int xx,int yy){a=new A(xx,yy);}
    D(){a=NULL;}
    D& operator=(D d){a=d.a; return *this;}
    void set(int xx, int yy){a=new A(xx,yy);}
    void prt(int i){a->prt(i);}
};

#include "..\pointer.cpp" // always use when PersistPtr or PersistVptr is used
