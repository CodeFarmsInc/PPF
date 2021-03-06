// ************************************************************************
//                       Code Farms Inc.
//          7214 Jock Trail, Richmond, Ontario, Canada, K0A 2Z0
//          tel 613-838-3622           htpp://www.codefarms.com
// 
//                     Copyright (C) 2008
//        This software is subject to copyright protection under
//        the laws of Canada, United States, and other countries.
// ************************************************************************

#include "stdlib.h"
#include "string.h"
#include "..\..\factory.h"
#include "testA.h"
#include "testE.h"

typedef PersistPtr<A> * PP;

int main(int argc,char **argv) {
    int createNew, rd, n, i;
    PersistPtr<E> root,cp;
    PersistPtr<A> a; // temporary, non-persistent object
    PP ap= &a;

    if(argc<2 || argc>2)createNew= -1;
    else if(!strcmp("1",argv[1])){createNew=1; rd=0;} // new DB
    else if(!strcmp("2",argv[1])){createNew=0; rd=0;} // use old, save results
    else if(!strcmp("3",argv[1])){createNew=0; rd=1;} // use old, don't save results
    else                         createNew= -1;
    if(createNew==(-1)){
  cout<<"SYNTAX: test n // n=1 creates data, n=2 add to old data\n";
        return 2;
    }
    cout << " rd=" << rd << ", sizeof(A)=" << sizeof(A) <<
                           ", sizeof(E)=" << sizeof(E) << "\n" <<
                           " sizeof(int)=" << sizeof(int) <<
          ", sizeof(PersistPtr<A>)=" << sizeof(PersistPtr<A>) << "\n";

    if(createNew){
#ifdef DOS
        system("del *.ppf");
#else // UNIX
        system("rm *.ppf");
#endif // DOS
    }

    // start the two pagers
    E::startPager(42,3,100,rd);
    A::startPager(32,100,1000000,rd); // use same space for individual objects
    PersistStart;

    if(createNew){  // create new test data
        root=new E; 
        root.setRoot();
    }
    else { // get older data from the disk
        root.getRoot();
    }
    cp=root;

    if(createNew==1) cp->prtVect("new database");
    else {
        if(rd==1) cp->prtVect("read only DB, old data");
        else      cp->prtVect("read/write DB, old data from the disk");
    }
   
    n=cp->size();
    cout << "adding 6 entries after the original " << n << "\n"; cout.flush();
    for(i=0; i<6; i++){
        a->set(n+i,n+i);
        cp->set(n+i,&a);
    }
    cout << "appending neg.values after entry=" << n+2 << "\n"; cout.flush();
    a->set(-(n+2), -(n+2));

    if( cp->append(a,n+2) ){cout << "error in append\n"; return 1;};
    cp->prtVect("entire new vector");

    A::closePager();
    E::closePager();
    return 0;
};

