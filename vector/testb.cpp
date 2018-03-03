#include "stdlib.h"
#include "string.h"
#include "..\factory.h"
#include "vector.h"
#include "testA.h"
#include "testB.h"

void B::prtVect(char *label){
    int i,n; A a; 
    cout << label << "\n";
    n=vec.size();
    for(i=0; i<n; i++)get(i).prt(i);
    cout.flush();
};

PersistImplement(B); // use just once, typically in B.cpp
