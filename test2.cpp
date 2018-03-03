// ************************************************************************
//                       Code Farms Inc.
//          7214 Jock Trail, Richmond, Ontario, Canada, K0A 2Z0
//          tel 613-838-4829           htpp://www.codefarms.com
// 
//                     Copyright (C) 1997
//        This software is subject to copyright protection under
//        the laws of Canada, United States, and other countries.
// ************************************************************************

// --------------------------------------------------------------------
// Data structure:
//     Link list of B's which are alternately C and D, each having a pointer
//     to A and an array of A's.
//
//            C  ->   D   ->   C   -> ...
//           | |     | |      | |
//           V V     V V      V V
//           A A     A A      A A
//             A       A        A
//             A       A        A
//             A                A
//             A                 
//
// Both C and D are derived from B, and the program generates 30 of them.
// The array sizes are 100, 200, ... ,3000.
// If you check sizes of files a.ppf and b.ppf after the run, you will see
// that there is only a very small overhead in storing the data.
// For Win95 version of the program, sizeof(A)=4, sizeof(B)=12,
// total number of A's stored=46,530, number of B's=30.
// For A: exact data size=186,120 B, actual disk used=186,168 B
// For B: exact data size=360 B, actual disk used=432.
//
// Syntax: test 1 ... generates the test data and tests it
//         test 2 ... loads the data from the disk and tests it
//
// NOTES:
// PersistPtr<> objects must be allocated with new(), not automatically.
// The syntax of destroying objects is:
//     ap->free();
//     Ap->freeArray();
// You can T::closePager() and later T::startPager() even for selected
// individual classes.
// --------------------------------------------------------------------

#include "string.h"
#include "stdlib.h"
#include "test2A.h"
#include "test2B.h"

#define STEP 100
#define LIMIT 3000

int main(int argc,char **argv)
{
    long i,n,m,count,createNew,cd;
    PersistVptr<D> pd;
    PersistVptr<C> pc;
    PersistVptr<B> root,pb,pbNew;
    PersistPtr<A> pa,Pa;

    if(argc<2 || argc>2)createNew= -1;
    else if(!strcmp("1",argv[1]))createNew=1;
    else if(!strcmp("2",argv[1]))createNew=0;
    else                         createNew= -1;
    if(createNew==(-1)){
        cout<<"SYNTAX: test 1 ... to create data,    test 2 ... use old data\n";
        return 2;
    }

    if(createNew){
#ifdef DOS
        system("del *.ppf");
#else  // UNIX
        system("rm *.ppf");
#endif // DOS
    }

    // start the four pagers
    A::startPager(50,3,1000,0);
    B::startPager(50,2,200,0);
    C::startPager(50,2,200,0);
    D::startPager(50,2,200,0);
    PersistStart;

    // simple testing of the '!=' operator
    pa=new A(11);
    Pa=pa;
    if(Pa!=pa){
        cout << Pa.getIndex()<<" "<<pa.getIndex()<<" ... do not compare\n";
    }

    if(createNew){  // create new test data
        pb=new B;
        for(n=STEP, cd=0; n<=LIMIT; n=n+STEP, cd=1-cd, pb=pbNew){
            pa=new A(n);
            Pa.newArr(n);
            for(i=0; i<n; i++)Pa[i].set(i);
            if(cd==0){
                pc=new C(pa,Pa,n);
                pbNew.CAST(pc,C,B);
            }
            else {
                pd=new D(pa,Pa,n);
                pbNew.CAST(pd,D,B);
            }
            pb->add(pbNew);
            if(n==STEP){
                root=pb; 
                root.setRoot();
            }
        }
    }
    else { // get older data from the disk
        root.getRoot();
    }

    // check the data, it should be the same as when created
    count=LIMIT/STEP; // correct number of B objects, m will count errors
    for(pb=root->nxt(), m=0; pb.getIndex()>0; pb=pb->nxt(), count--){ 
        n=pb->aObj()->get();
        Pa=pb->aArr();
        for(i=0;i<n;i++){
            if(Pa[i].get()!=i){
                cout << Pa[i].get() << " "; Pa[i].prt("\n"); cout.flush();
                m++;
            }
        }
    }
    if(m>0 || count!=0){
        cout << "errors: unmatched=" << m << " count=" << count << "\n";
        cout.flush();
    }
    else   {cout << "No errors\n"; cout.flush();}
    
    A::closePager();
    B::closePager();
    C::closePager();
    D::closePager();
    return 0;
};
