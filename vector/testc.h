#include "stdlib.h"
#include "string.h"
#include "..\factory.h"
#include "vector.h"

class C {
PersistClass(C);
    vector<D> vec;
public:
    C():vec(){}
    void set(int i,D *d){vec.set(i,d);}
    int append(D d,int i){return vec.append(d,i);}
    D& get(int i){return vec.get(i);}
    int size(){return vec.size();}
    void prtVect(char *label);
    
};

#include "..\pointer.cpp" // always use when PersistPtr or PersistVptr is used
