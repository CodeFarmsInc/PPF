#include "test2A.h"

PersistImplement(A); // use just once, typically in a.cpp

// implementation of methods for class A
void A::prt(const char *txtEnd){
             cout << a << " " << (long)(this) << txtEnd; cout.flush();}
