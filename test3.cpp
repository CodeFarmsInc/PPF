
// ************************************************************************
//                       Code Farms Inc.
//          7214 Jock Trail, Richmond, Ontario, Canada, K0A 2Z0
// 
//                     Copyright (C) 1997
//        This software is subject to copyright protection under
//        the laws of Canada, United States, and other countries.
// ************************************************************************

// --------------------------------------------------------------------
// Data structure:
//     Singly-linked list of A's, each having and integer id and two strings:
//       name, address
//     Head of the list is and empty A.
//
// The program generates many A's, but uses again and again a small set
// of pairs name/address
// --------------------------------------------------------------------

#include "string.h"
#include "stdlib.h"
#include "test3.h"

#define LIMIT 1000

int main(int argc,char **argv)
{
    long createNew,changeName0;
    PersistPtr<A> head,tail,pa;
    int i,k,m,rd;
    const char *names[]={"J.Brown","D.Wilkes-Barre","F.Zelikovitz","H.Doe"};
    const char* name0 =  "H.Adams";
    const char *addresses[]={
        "Box 566, Lower Hutt, Ontario, K0A 2F4",
        "45 Alta Vista, apt.17, Toronto, Ontario, M5A 3F3, Canada",
        "CVUT, Praha, Czech Republic",
        "P.O.Box 13567, Miami, FL 34012, U.S.A."
    };
    int num=4;

    if(argc<2 || argc>2)createNew= -1;
    else if(!strcmp("1",argv[1])){createNew=1; changeName0=0; rd=0;}
    else if(!strcmp("2",argv[1])){createNew=0; changeName0=0; rd=1;}
    else if(!strcmp("3",argv[1])){createNew=0; changeName0=1; rd=0;}
    else if(!strcmp("4",argv[1])){createNew=0; changeName0=0; rd=1;
                                                    names[0]=name0;}
    else                         createNew= -1;
    if(createNew==(-1)){
        cout<<"SYNTAX: test n // n=1 create, n=2 Rmode, n=3 changes WRmode\n";
        return 2;
    }

    if(createNew>0){
#ifdef DOS
        system("del *.ppf");
#else   // UNIX
        system("rm *.ppf");
#endif // DOS
    }

    A::startPagerObj(64,200,30000,rd); // start the pager for class A
    PersistString::startPagerObj(64,128,60000,rd); // opening pager for strings
    PersistStart; // needed to synchronize classes

    if(createNew>0){  // create new test data
        for(i=k=0, tail.setNull(); i<LIMIT; i++, k++, tail=pa ){
            if(k>=num)k=0;
            pa=new A(tail,names[k],addresses[k],i);
            if(i==0){
                head=pa;
                head.setRoot();
            }
        }
    }
    else { // get older data from the disk
        head.getRoot();
    }

    // check the data, it should be the same as when created
    for(pa=head, i=k=m=0; pa!=0; pa=pa->next(), i++, k++){
        if(k>=num)k=0;
        if(strcmp(names[k],pa->name()) || strcmp(addresses[k],pa->address())
                                                         || i!=pa->id()){
            m++;
        }
    }
    if(m>0) {cout << "errors: unmatched=" << m << "\n"; cout.flush();}
    else    {cout << "No errors\n"; cout.flush();}

    // test free lists, remove all names for k=0, and then replace them 
    // (in a seaparte loop) by a new name of the same size - there should
    // be no increase in the file size.
    if(createNew==0 && changeName0==1){
        for(pa=head, k=0; pa!=0; pa=pa->next(), k++){
            if(k>=num)k=0;
            if(k==0){
                pa->delName();
            }
        }
        for(pa=head; pa!=0; pa=pa->next()){
            if(pa->name()==0){
                pa->addName((char*)name0);
            }
        }
    }

    // check thoroughly the validity of all strings
    PersistString::debugFree(0); // check free lists of strings
    // traverse all strings, and check them
    for(pa=head; pa!=0; pa=pa->next()){
        pa->debugStrings();
    } 

    
    A::closePager();
    PersistString::closePager();
    return 0;
};
PersistImplement(A);
