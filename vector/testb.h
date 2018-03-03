#include "stdlib.h"
#include "string.h"
#include "..\factory.h"
#include "vector.h"

class A;

class B {
PersistClass(B);
    vector<A> vec;
public:
    B():vec(){}
    void set(int i,A *a){vec.set(i,a);}
    int append(A a,int i){return vec.append(a,i);}
    A& get(int i){return vec.get(i);}
    int size(){return vec.size();}
    void prtVect(char *label);
    
};

#include "..\pointer.cpp" // always use when PersistPtr or PersistVptr is used
