#include "test2A.h" // must be used because PersistPtr<A> is used in class B
#include "test2B.h"

PersistImplement(B); // use just once, here for 3 classes
PersistImplement(C); 
PersistImplement(D); 

// implementation of methods for class B
void B::prt(PersistVptr<B> pb,const char *txtEnd){
      pb.prt("pb=","");
      next.prt("next=","");
      aPtr.prt("aPtr=","");
      arr.prt("arr=",txtEnd);
      cout.flush(); 
}

B::B(){ 
    PersistConstructor;
    next.setNull(); aPtr.setNull(); arr.setNull();
}
