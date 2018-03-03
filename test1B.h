#include "factory.h"

class A;

class B {
PersistClass(B);
    PersistPtr<A> aPtr;
    PersistPtr<A> arr;
    PersistPtr<B> next;
public:
    B(){next.setNull(); aPtr.setNull(); arr.setNull();}
    B(PersistPtr<B>& b, PersistPtr<A>& a, PersistPtr<A>& aa){
                           b->next=this; next.setNull(); aPtr=a; arr=aa;}
    ~B(){
         if(next.getIndex())next.delObj(); 
         if(aPtr.getIndex())aPtr.delObj(); 
         if(arr.getIndex())arr.delObj();
    }
    PersistPtr<B> nxt(){return next;}
    PersistPtr<A> aObj(){return aPtr;}
    PersistPtr<A> aArr(){return arr;}
    void prt(PersistPtr<B> pb,const char *txtEnd);
};

#include "pointer.cpp" // always include when PersistPtr<> is used
