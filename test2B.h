#include "factory.h"

class A;

class B {
PersistVclass(B);
    PersistPtr<A> aPtr;
    PersistPtr<A> arr;
    PersistVptr<B> next;
public:
    B();
    B(PersistPtr<A>& a, PersistPtr<A>& aa){next.setNull(); aPtr=a; arr=aa;}
    ~B(){        // is really not needed
         if(next.getIndex())next.delObj(); 
         if(aPtr.getIndex())aPtr.delObj(); 
         if(arr.getIndex())arr.delObj();
    }
    PersistVptr<B> nxt(){return next;}
    PersistPtr<A> aObj(){return aPtr;}
    PersistPtr<A> aArr(){return arr;}
    void prt(PersistVptr<B> pb,const char *txtEnd);
    virtual void prtVal(){cout<<"just plain B\n";}
    void add(PersistVptr<B>& pb){next=pb;}
};

class C : public B {
PersistVclass(C);
    int c;
public:
    C(){PersistConstructor; c=0;};
    C(PersistPtr<A>& a, PersistPtr<A>& aa, int i) : B(a,aa){c=i;}
    virtual void prtVal(){cout<<"c="<<c<<"\n";}
};

class D : public B {
PersistVclass(D);
    int d1,d2;
public:
    D(){PersistConstructor; d1=d2=0;};
    D(PersistPtr<A>& a, PersistPtr<A>& aa, int i) : B(a,aa){d1=i; d2=i+1;}
    virtual void prtVal(){cout<<"d="<<d1<<" "<<d2<<"\n";}
};

#include "pointer.cpp" // always include when PersistPtr<> is used
