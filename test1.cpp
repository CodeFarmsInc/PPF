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
//     Link list of B's, each having a pointer to A and an array of A's.
//
//            B  ->   B   ->   B   -> ...
//           | |     | |      | |
//           V V     V V      V V
//           A A     A A      A A
//             A       A        A
//             A       A        A
//             A                A
//             A                 
//
// The program generates 30 B's; the array sizes are 100, 200, ... ,3000.
// If you check sizes of files a.ppf and b.ppf after the run, you will see
// that there is only a very small overhead in storing the data.
// For Win98 version of the program, sizeof(A)=4, sizeof(B)=12,
// total number of A's stored=46,530, number of B's=30.
// For A: exact data size=186,120 B, actual disk used=186,168 B
// For B: exact data size=360 B, actual disk used=432.
//
// Syntax: test 1 ... generates the test data and tests it
//         test 2 ... loads the data from the disk in the read mode and tests it
//         test 3 ... loads the data from the disk in the R/W mode and tests it
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
#include "test1A.h"
#include "test1B.h"

#define STEP 100
#define LIMIT 3000

int main(int argc,char **argv)
{
    long i,n,m,count,createNew,rd;
    PersistPtr<B> root,pb;
    PersistPtr<A> pa,Pa;

    if(argc<2 || argc>2)createNew= -1;
    else if(!strcmp("1",argv[1])){createNew=1; rd=0;}
    else if(!strcmp("2",argv[1])){createNew=0; rd=1;}
    else if(!strcmp("3",argv[1])){createNew=0; rd=0;}
    else                         createNew= -1;
    if(createNew==(-1)){
  cout<<"SYNTAX: test n // n=1 creates data, n=2 Rmode,n=3 RWmode\n";
        return 2;
    }
    

    if(createNew){
#ifdef DOS
        system("del *.ppf");
#else // UNIX
        system("rm *.ppf");
#endif // DOS
    }

    // start the two pagers
    A::startPager(50,3,1000,rd);
    B::startPager(50,2,200,rd);
    PersistStart;

    // simple testing of the '!=' operator
    pa=new A(11);
    Pa=pa;
    if(Pa!=pa){
        cout << Pa.getIndex()<<" "<<pa.getIndex()<<" ... do not compare\n";
    }

    if(createNew){  // create new test data
        root=new B; 
        root.setRoot();
        for(pb=root, n=STEP; n<=LIMIT; n=n+STEP){

            // test closing & restarting pager with different parameters
            if(n==LIMIT/2){
                A::closePager();
                A::startPager(32,7,1000,rd);
            }

            pa=new A(n);
            Pa.newArr(n);
            for(i=0; i<n; i++)Pa[i].set(i);
            pb=new B(pb,pa,Pa);
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
                cout << Pa[i].get() << (char*)" "; Pa[i].prt("\n"); cout.flush();
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
    return 0;
};
