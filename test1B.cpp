#include "test1A.h" // must be used because PersistPtr<A> is used in class B
#include "test1B.h"

PersistImplement(B); // use just once, typically in b.cpp

// implementation of methods for class B
void B::prt(PersistPtr<B> pb,const char *txtEnd){
      pb.prt("pb=","");
      next.prt("next=","");
      aPtr.prt("aPtr=","");
      arr.prt("aPtr=",txtEnd);
      cout.flush(); 
}
