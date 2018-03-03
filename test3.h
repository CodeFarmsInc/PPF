#include "factory.h"

class A {
PersistClass(A);
    PersistPtr<A> _next;
    PersistString _name;
    PersistString _address;
    int _id; // -1 when not set
public:
    A(){_next.setNull(); _name.setNull(); _address.setNull(); _id= -1;}
    A(PersistPtr<A>& tail, const char *nameStr, const char *addressStr, int id){
        _next.setNull(); 
        if(tail==null)tail=this;
        else tail->_next=this;
        _name=new PersistString((char*)nameStr);
        _address=new PersistString((char*)addressStr);
        _id=id;
    }
    ~A(){}
    void delName(){_name.delString(); _name.setNull();}
    void addName(char *nm){ 
        _name=new PersistString(nm);
    }
    PersistPtr<A> next(){return _next;}
    char* name(){if(_name==0) return NULL; return _name.getPtr();}
    char* address(){return _address.getPtr();}
    int id(){return _id;}
    void prt(){ _name.prt("_name",", "); _address.prt("_address","\n"); }
    void debugStrings(){
        if(_name.debugOne())exit(1);
        if(_address.debugOne())exit(2);
    }
};

#include "pointer.cpp" // always include when PersistPtr<> is used
