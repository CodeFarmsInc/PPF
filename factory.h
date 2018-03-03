// ***********************************************************************
//                       Code Farms Inc.
//          7214 Jock Trail, Richmond, Ontario, Canada, K0A 2Z0
//          tel 613-838-4829           htpp://www.codefarms.com
// 
//                   Copyright (C) 1997-2014
//        This software is subject to copyright protection under
//        the laws of Canada, United States, and other countries.
// ************************************************************************

#ifndef PPF_FACTORY_INCLUDE
#define PPF_FACTORY_INCLUDE

// ************************************************************************
// IMPORTANT:
//
// In the basic version, the overall size of the data is limited to 2^31,
// the largest positive number that can be expressed as 'long'.
// The size of PersistPtr is only 32 bits even when running in 
// a 64-bit environment, which can make your objects up to 50% smaller.
//
// If your overall data size is over 2^31, then you must compile
// under 64 bits, and uncomment the following #define statement:
// ************************************************************************
// UNCOMMENT THE NEXT LINE FOR TOTAL DATA LARGER THAN 2^^31
// #define LARGE_DATA_SET
// ************************************************************************

#ifdef LARGE_DATA_SET
typedef long long pLong;
typedef unsigned long long upLong;
#else
typedef long pLong;
typedef unsigned long upLong;
#endif


// --------------- specify the operating system and the compiler --------
// For VS++ define MICROSOFT and DOS
// For Linux define LINUX and UNIX and GNU
#define MICROSOFT
#define DOS
// #define LINUX
// #define UNIX
// #define GNU
// ----------------------------------------------------------------------
// #define short int wchar_t;

#include <iostream>
using namespace std;
#ifdef MICROSOFT
#endif
#include <stdio.h>

#define null 0L

class PersistHeader;
class PersistPager;
class PersistVoid;
template<class T> class PersistPtr;

#define NAME(A) #A

typedef void (*hiddenType)(void*);



// ----------------------------------------------------------------------
// PersistHeader is stored at the beginning of the file for normal
// classes, but it is not stored for the PersistString class.
// Also, only the value of 'current' is used for PersistString
// ----------------------------------------------------------------------
class PersistHeader {
public:
    pLong current; 
    pLong freeObj;
    pLong freeArr;
    pLong root;
    PersistHeader(pLong firstObj);
    void prt(char *label){
        cout<<label<<"PersistHeader current="<<current<<" freeObj="<<
                  freeObj<<" freeArr="<<freeArr<<" root="<<root<<"\n";
    }
};

class PersistFactory {
friend class PersistString;
public:
    static int useCache;
private:
    static size_t cacheBufSz;
    static char* cacheBuf;
    static int cannotIncreaseCacheBuf;
    int tableInd;  // index into persistent type table, 'table'
    pLong objSz;
    PersistHeader *info;
    PersistPager *pPager;
    pLong firstObj; // address of the first object, after the PersistHeader

    pLong lastInd;  // last pointer-index pair for a fast identification
    void* lastPtr;
    int lastTableInd;

    char *classN;  // name of the class using this factory (for error messages)
    static char *filePath; // path to the directory where to store the data
    static int tableSz;    // allocated size of the table
    static int allocTable(int sz); // allocate or re-allocate the class table
public:
    static void warmDiskCache(bool create){if(create==false)useCache=1;}
    static void preloadPages(bool create){ if(create==false)useCache=2;}
    static void removeCacheBuf();
    static void fillCache(char *fileName);
    PersistFactory(int *cIndex,pLong objSize,void *validObj,pLong pageSize,
        pLong pagesInMem, pLong totalSize,char* className,int needsUpd,
        hiddenType hn,int rd); // when valiObj=NULL no object initialization
    ~PersistFactory();
    void startPager(); 
    void closePager();
    void* getPtr(pLong ind); // ind = byte address within the pager file
    void* newObj();   // allocation of memory for T::new()
    pLong newArr(pLong size); // equivalent of new T[], returns index
    pLong adjArray(pLong oldInd,pLong oldSz,pLong newSz); // special new T[], returns index
    void freeObj(pLong ind); // single object to the free list, or release it
    void freeArr(pLong ind); // array to the free list, or release it
    pLong getInd(void *ptr);
    void setRoot(pLong rt);
    pLong getRoot();
    static void synchClasses();
    static void prtTable(); // debugging print
    static void debugObj(char *label,void *v,int sz);

    char *getClassN(){return classN;}
    int getTableInd(){return tableInd;}
    static int newOperatorCall; // 1 when updating vf pointers from disk
    static PersistFactory** table; // table of active classes, always same order
    static int classCount; // used part of the table
    static void setPath(char *path);
    static int myFileOpen(char *fileName,int wFlag,int *newF); 
// -------------------------------------------------------------
// THE FOLLOWING PART IS STILL PART OF THE PersistFactory DEFINITION
// THESE ARE FUNCTIONS WHICH ARE NOT INTENDED TO BE INLINE, BUT BECAUSE
// VisualStudio 2010 COMPILER IS NOT ABLE TO PROCESS THEM FROM A SEPARATE *.CPP
// FILE, THIS IS THE ONLY WAY TO CODE THEM.
// ----------------------------------------------------------------------
}; // End of definition of PersistFactory


template<class T> class PersistPtr {
friend class PersistFactory;
public:
    PersistPtr(){} // must be empty - used for initialization of v.f. pointers
    PersistPtr(T* realPtr);
    PersistPtr(int i){index=0; 
                       if(i)cout<<"error in use of PersistPtr(int i)\n";}
    PersistPtr(pLong i){index=0;
                       if(i)cout<<"error in use of PersistPtr(pLong i)\n";}
    PersistPtr(const PersistPtr& rhs){index=rhs.index;} // copy PersistPtr
    ~PersistPtr(){} // deleting a pointer should not delete the object

    PersistPtr& operator=(const PersistPtr& rhs){index=rhs.index; return *this;}
    PersistPtr& operator=(pLong rhs){index=rhs; return *this;}
    PersistPtr& operator=(int rhs){index=rhs; return *this;}
    PersistPtr& operator=(T* realPtr){
        index=T::PersistStore->getInd((void*)realPtr); return *this;}
    inline operator int() { return index; } // cast operator
    int operator==(const PersistPtr& rhs){ int i; 
                                  if(index==rhs.index)i=1; else i=0; return i;}
    int operator!=(const PersistPtr& rhs){ int i; 
                                  if(index==rhs.index)i=0; else i=1; return i;}
    int operator==(const pLong rhs){ return (index==rhs);}
    int operator!=(const pLong rhs){ return (index!=rhs);}
    int operator==(const int rhs){ return (index==(pLong)rhs);}
    int operator!=(const int rhs){ return (index!=(pLong)rhs);}
    T* operator->() const{ return getPtr(index);}
    T& operator*() const{return *(getPtr(index));}
    T& operator[](pLong incr){return *(getPtr(index+incr*sizeof(T)));}
    operator int () const{return index;}
    pLong getIndex(){return index;} 
    void setNull(){index=0;}
    void getRoot(); // get root of this class
    void setRoot(); // set root of this class

    PersistPtr<T>& newArr(pLong size){    // equivalent of T::new[]()
        index=T::PersistStore->newArr(size); return *this;
    }
    PersistPtr<T>& adjArray(pLong oldInd,pLong oldSz,pLong newSz); // like newArr()
    void delObj(); // equivalent of T::delete()
    void delArr(); // equivalent of T::delete[]()
    // for debugging:
    void prt(const char* before,const char* after){ cout<<before<<" "<<
       getStore()->getClassN()<<":index="<<index; cout.flush();
       if(index)cout<<" ptr="<<(upLong)(getPtr()); 
       cout<<" "<<after; cout.flush();
    }
private:
    T* getPtr() const{ return getPtr(index);}
    T* getPtr(pLong ind) const{ if(T::PersistStore)
             return (T*)(T::PersistStore->getPtr(ind)); else return NULL; }
    pLong index; // byte address within the pager file

    // ---- the only difference between PersistPtr<> and PersistVptr<> ----
    PersistFactory *getStore(){return T::PersistStore;}
};

// --------------------------------------------------------------------------
// Notes: operator[] does not make any sense for a VirtPtr
//        operator= to assign PersistPtr to PersistVptr would be useful
//                  but I have not figured out how to implement it yet
// --------------------------------------------------------------------------
template<class T> class PersistVptr {
friend class PersistFactory;
public:
    PersistVptr(){} // must be empty - used for initialization of v.f. pointers
    PersistVptr(T* realPtr);
    PersistVptr(int i){index=0;
                       if(i)cout<<"error in use of PersistVptr(int i)\n";}
    PersistVptr(pLong i){index=0;
                       if(i)cout<<"error in use of PersistVptr(pLong i)\n";}
    PersistVptr(const PersistVptr& rhs){
                             index=rhs.index; factoryInd=rhs.factoryInd;}
    ~PersistVptr(){} // deleting a pointer should not delete the object

    PersistVptr& operator=(const PersistVptr& rhs){
                 index=rhs.index; factoryInd=rhs.factoryInd; return *this;}
    PersistVptr& operator=(pLong rhs){
            index=rhs; factoryInd=T::PersistStore->getTableInd(); return *this;}
    PersistVptr& operator=(int rhs){
            index=rhs; factoryInd=T::PersistStore->getTableInd(); return *this;}
    PersistVptr& operator=(T* realPtr){
                 index=T::PersistStore->getInd((void*)realPtr);
                 factoryInd=T::PersistStore->getTableInd(); return *this;}
    PersistVptr& cast(void *ptr,void *from,void *to){
        PersistVptr* p=(PersistVptr*)ptr;
        index=p->index + ((pLong)to - (pLong)from);
        factoryInd=p->factoryInd; return *this;}
    int operator==(const PersistVptr& rhs){ int i; 
     if(index==rhs.index && factoryInd==rhs.factoryInd)i=1; else i=0; return i;}
    int operator!=(const PersistVptr& rhs){ int i; 
     if(index==rhs.index && factoryInd==rhs.factoryInd)i=0; else i=1; return i;}
    T* operator->() const{ return getPtr(index);}
    T& operator*() const{return *(getPtr(index));}
    pLong getIndex(){return index;} 
    int getFactInd(){return factoryInd;} 
    void setNull(){index=0;}
    void getRoot(); // get root of this class
    void setRoot(); // set root of this class

    PersistVptr<T>& newArr(pLong size){    // equivalent of T::new[]()
        index=getStore()->newArr(size); return *this;
    }
    PersistVptr<T>& adjArray(pLong oldInd,pLong oldSz,pLong newSz); // like newArr()
    void delObj(); // equivalent of T::delete()
    void delArr(); // equivalent of T::delete[]()
    // for debugging:
    void prt(const char *before,const char *after){ 
             cout<<before<<" "<<getStore()->getClassN()<<":index="<<index<<"/"
             <<factoryInd<<" "<<after; cout.flush();}
private:
    T* getPtr() const{ return getPtr(index);}
    T* getPtr (pLong ind) const;
    pLong index; // byte address within the pager file

    // ---- the only difference between PersistPtr<> and PersistVptr<> ----
    int factoryInd; // index into the 'table' of factories
    PersistFactory *getStore() const{
                                 return PersistFactory::table[factoryInd];}
};

class PersistDummy {
    int i;
};

#define PersistClass(T)                                                      \
public:                                                                      \
    PersistFactory* getStore(){return PersistStore;}                         \
    void* operator new(size_t size){ void* v=PersistAlloc;                   \
                                   if(!v)v=PersistStore->newObj(); return v;}\
    static void startPager(pLong pgSz,pLong maxPgs,pLong totMem,int rd){     \
        static T t; int ph=sizeof(PersistHeader);                            \
	if(pgSz<ph)pgSz=ph;                                                  \
	if(maxPgs==1){                                                       \
	    if(totMem<=pgSz){totMem=pgSz=pgSz+ph;}                           \
	    else maxPgs=2; /* all data in 1 page, or at least 2 pages needed */\
	}                                                                    \
	pgSz=((pgSz-1)/sizeof(char*)+1)*sizeof(char*);                       \
        PersistStore=new PersistFactory(&PersistIndex,                       \
                   sizeof(T),&t,pgSz,maxPgs,totMem,myName(),0,NULL,rd);      \
        if(!PersistStore)err1(myName());                                     \
    }                                                                        \
   static void startPagerObj(pLong numObjPg,pLong objInMem,pLong maxObj,int rd){\
        pLong pgSz,maxPgs,totMem,totLim,x;                                   \
	totLim=017777777777;                                                 \
	int  pgLim=010000000000;                                             \
	if(numObjPg==0) pgSz=4096;                                           \
	else {                                                               \
	    x=totLim/sizeof(T)-1;                                            \
	    if(numObjPg>x || numObjPg<=0)numObjPg=x;                         \
	    pgSz=(numObjPg+1)*sizeof(T);                                     \
            if(pgSz<=0 || pgSz>pgLim)pgSz=pgLim;                             \
	    pgSz=pgSz + 4096 - (pgSz%4096);                                  \
	}                                                                    \
	x=totLim/sizeof(T);                                                  \
	if(objInMem>x || objInMem<=0)objInMem=x;                             \
	maxPgs=((objInMem*sizeof(T)-1)/pgSz)+1;                              \
	x=totLim/sizeof(T)-1;                                                \
	if(maxObj>x)maxObj=x;                                                \
	if(maxObj<=0)totMem=0; else totMem=(1+maxObj)*sizeof(T);             \
        startPager(pgSz,maxPgs,totMem,rd);                                   \
    }                                                                        \
    static void closePager(){PersistStore->closePager();}                    \
    static PersistFactory *PersistStore;                                     \
    static int PersistIndex;                                                 \
private:                                                                     \
    static char *myName(){return (char*) NAME(T);}                           \
    static void err1(char *p){cout<<"error starting pager: "<<NAME(T)<<"\n";}\
friend class PersistFactory


#define PersistVclass(T)                                                     \
public:                                                                      \
    virtual PersistFactory* getStore(){return PersistStore;}                 \
    void* operator new(size_t size){ void* v=PersistAlloc;                   \
                                   if(!v)v=PersistStore->newObj(); return v;}\
    static void startPager(pLong pgSz,pLong maxPgs,pLong totMem,int rd){     \
        static T t; int ph=sizeof(PersistHeader);                            \
	if(pgSz<ph)pgSz=ph;                                                  \
	if(maxPgs==1){                                                       \
	    if(totMem<=pgSz){totMem=pgSz=pgSz+ph;}                           \
	    else maxPgs=2; /* all data in 1 page, or at least 2 pages needed */\
	}                                                                    \
	pgSz=((pgSz-1)/sizeof(char*)+1)*sizeof(char*);                       \
        PersistStore=new PersistFactory(&PersistIndex,                       \
                sizeof(T),&t,pgSz,maxPgs,totMem,myName(),1,T::hiddenNew,rd); \
        if(!PersistStore)err1(myName());                                     \
    }                                                                        \
   static void startPagerObj(pLong numObjPg,pLong objInMem,pLong maxObj,int rd){\
        pLong pgSz,maxPgs,totMem,totLim,x;                                   \
	totLim=017777777777;                                                 \
	int  pgLim=010000000000;                                             \
	if(numObjPg==0) pgSz=4096;                                           \
	else {                                                               \
	    x=totLim/sizeof(T)-1;                                            \
	    if(numObjPg>x || numObjPg<=0)numObjPg=x;                         \
	    pgSz=(numObjPg+1)*sizeof(T);                                     \
            if(pgSz<=0 || pgSz>pgLim)pgSz=pgLim;                             \
	    pgSz=pgSz + 4096 - (pgSz%4096);                                  \
	}                                                                    \
	x=totLim/sizeof(T);                                                  \
	if(objInMem>x || objInMem<=0)objInMem=x;                             \
	maxPgs=((objInMem*sizeof(T)-1)/pgSz)+1;                              \
	x=totLim/sizeof(T)-1;                                                \
	if(maxObj>x)maxObj=x;                                                \
	if(maxObj<=0)totMem=0; else totMem=(1+maxObj)*sizeof(T);             \
        startPager(pgSz,maxPgs,totMem,rd);                                   \
    }                                                                        \
    static void closePager(){PersistStore->closePager();}                    \
    static PersistFactory *PersistStore;                                     \
    static int PersistIndex;                                                 \
    T(PersistDummy *pd){}                                                    \
private:                                                                     \
    virtual void * getPersistAlloc(){return PersistAlloc;}                   \
    static void hiddenNew(void *h){                                          \
        static PersistDummy* pd=NULL; if(!pd)pd=new PersistDummy;            \
                PersistAlloc=h; (void)new T(pd); PersistAlloc=NULL;}         \
    static char *myName(){return (char*) NAME(T);}                           \
    static void err1(char *p){cout<<"error startig pager: "<<NAME(T)<<"\n";} \
friend class PersistFactory

#define PersistImplement(T) \
int T::PersistIndex= -1;    \
PersistFactory* T::PersistStore=NULL

#define PersistVimplement(T) PersistImplement(T)

#define PersistStart \
{PersistVoid::pagerReset(256,2,1024);} \
PersistFactory::synchClasses(); \
PersistFactory::removeCacheBuf()

#define PersistClose PersistVoid::closePager()

#define PersistConstructor if(PersistFactory::newOperatorCall)return

#define CAST(P,FROM,TO) cast(&(P),(FROM*)PersistAny,(TO*)((FROM*)PersistAny))

class PersistString {
public:                                                                      
    // allocation
    PersistString(){index=0;}

    PersistString(pLong sz){(void)newStr(sz);}
    PersistString(char *s);
    PersistString(wchar_t s[]);  // experimental, read

    ~PersistString(){}
    void delString();

    // pager controls
    static void startPager(pLong pgSz,pLong maxPgs,pLong totMem,int rd);
    static void startPagerObj(pLong pgSz,pLong maxInMem,pLong totMem,int rd){
	pLong maxPgs;
	int totLim=017777777777;  
	int  pgLim=010000000000;

	if(pgSz==0) pgSz=4096;
	if(pgSz>pgLim || pgSz<0)pgSz=pgLim;
	pgSz=pgSz + 4096 - (pgSz%4096);

	if(maxInMem<=0)maxInMem=totLim;
	if(totMem<=0)totMem=totLim;
	maxPgs=(maxInMem-1)/pgSz+1;
        startPager(pgSz,maxPgs,totMem,rd);
    }
    static void closePager();

    // operators
    PersistString& operator=(const PersistString& rhs){
                                               index=rhs.index; return *this;}
    PersistString& operator=(pLong rhs){index=rhs; return *this;}
    PersistString& operator=(int rhs){index=rhs; return *this;}
    PersistString& operator=(PersistString* realPtr){
                                  index=realPtr->index; return *this;}
    int operator==(const pLong rhs){return index==rhs;} // for ==NULL
    int operator!=(const pLong rhs){return index!=rhs;} // for !=NULL
    int operator==(const PersistString& rhs){ int i; 
                                  if(index==rhs.index)i=1; else i=0; return i;}
    int operator!=(const PersistString& rhs){ int i; 
                                  if(index==rhs.index)i=0; else i=1; return i;}
    pLong getIndex(){return index;} 
    char* operator->() { return getPtr(index);}
    char operator[](pLong incr){return *(getPtr(index+incr));}
    void setNull(){index=0;}
    char *getPtr(pLong ind);
    char *getPtr(){return getPtr(index);}
    pLong getInd(char *s); // may be called only immediately after allocation

    // manipulating strings
    int cmp(const PersistString& s);
    int cmp(const char *s);
    size_t size();
    pLong allocSize();

    // debugging
    void prt(const char *before,const char *after){ char* p=getPtr(); if(!p)p=(char*)"NULL";
        pLong* pLongPtr=(pLong*)(p-sizeof(pLong));
        cout<<before<<" PersistString="<<p<<" index="<<index<<
                                        " sz="<<(*pLongPtr)<<after;
        cout.flush();
    }
    int debugOne();  // check a given string with the attached text
    static void debugFree(int prtFlg); // complete check of free strings 
private:                                                                     
    pLong index; // start of the actual string in the pager file

    // allocation and paging
    static PersistPager *pPager;
    static PersistHeader *info;
    static pLong *freeStr; 
    static pLong freeSz; 
    static char *spare;
    static pLong pageSz;
    static pLong diskSz;
    static pLong lastInd;  // last pointer-index pair for a fast identification
    static void *lastPtr;
    static pLong shortTail;
    static void err1(char *p){cout<<"error starting pager for PersistString\n";}

    // internal functions
    char *newStr(pLong size);
    pLong roundSz(pLong size);
    void getFreeString(pLong size);
    void setFree(pLong ind);
    void releaseFree(pLong ind,pLong sz);
    void getNewString(pLong sz);
    void setTail(pLong ind,int isFree);
    static pLong getTailSize(pLong lastByte,int *isFree);
    void releaseFree(pLong ind,int sz);
};

// next line is a planned experiment, not active yet
template<class T> class PersistStrings : public PersistString{};

#ifdef PERSIST_SOURCE
char* PersistAny=(char*)""; // any pointer which is not NULL
void* PersistAlloc=NULL;

PersistPager* PersistString::pPager=NULL;
pLong PersistString::pageSz=0;
pLong PersistString::diskSz=0;
PersistHeader* PersistString::info=NULL;
pLong* PersistString::freeStr=NULL; 
pLong PersistString::freeSz=0; 
pLong PersistString::lastInd=0;
void* PersistString::lastPtr=NULL;
char* PersistString::spare=NULL;
pLong PersistString::shortTail=508;
#else
extern char* PersistAny;
extern void* PersistAlloc;
#endif


#ifdef PPF_LIGHT
#define PTR(T) PersistPtr<T> 
#else
#define PTR(T) PersistVptr<T>
#endif

#define STR PersistString

// ------------------------------------------------------------------
// Persistent equivalent of void*, it is a storable class,
// which is treated in the same manner as all the user-defined classes.
// This class makes it possible to implement arrays of PersistPtr or
// PersistVptr, and it is instrumental in the implementation of the
// PPF hash table. Starting and closing of the pager for this class
// is automatic, trasparent, and it is always in the R/W mode. 
//
// If you want to reset pager parameters for this class, call
//   PersistVoid pv; pv.resetPager(pLong pageSize, pLong numPages,pLong totSize);
// The new pageSize will be maximum of the old and new one,
// the numPages will be chosen to fit the maximum of (pageSize*numPages),
// and the bigger of the two totSize will be selected. In other words, 
// you cannot downsize. The purpose of this is to prevent situations where
// one function (e.g. in a hash table) would set up large pages, and later
// another function (e.g. user code) would change to smaller pages with
// a heavy impact on the performance of the hash table.
//
// BEWARE: Calling PersistVoid::startPager(..) may lead to errors, and 
// the library is not protected against it.
// ------------------------------------------------------------------
class PersistVoid {
    PersistClass(PersistVoid);
public:
    PTR(PersistVoid) vptr;

    void static pagerReset(pLong pageSz,pLong numPages,pLong totSz);
    static int isActive(){if(pvPageSz>0)return 1; else return 0;}
    void set(void* v){ vptr= *((PTR(PersistVoid)*)v); }
    void *get(){ return (void*)(&vptr); }
    PersistVoid& operator=(pLong rhs){vptr=rhs; return *this;}
private:
    static int pvPageSz;
    static int pvNumPages;
    static int pvTotSz;
};

// ------------------------------------------------------------------
// convenient macros which cast PTR(T) which is PersistPtr<T> or PersistVptr<T>
// to PersistVoid and reverse. Note that from_void() must be given the
// target class name. This is safe - if you make a mistake with the class
// the compiler will catch the error.
// ------------------------------------------------------------------
#define to_void(P) (*((PersistVoid*)(&(P))))
#define from_void(T,P) (*(PTR(T)*)(&(P)))


#endif // PPF_FACTORY_INCLUDE
