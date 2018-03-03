#include "stdlib.h"
#include "string.h"
#include "..\factory.h"
#include "vector.h"
#include "testA.h"
#include "testE.h"

void E::prtVect(char *label){
    int i,n; 
    cout << label << "\n";
    n=vec.size();
    for(i=0; i<n; i++)get(i).prt(i);
    cout.flush();
};

PersistImplement(E); // use just once, typically in B.cpp
