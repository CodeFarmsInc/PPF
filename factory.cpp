// ************************************************************************
//                       Code Farms Inc.
//          7214 Jock Trail, Richmond, Ontario, Canada, K0A 2Z0
//          tel 613-838-4829           htpp://www.codefarms.com
// 
//                   Copyright (C) 1997-2013
//        This software is subject to copyright protection under
//        the laws of Canada, United States, and other countries.
// ************************************************************************

// ----------------------------------------------------------------------
// IMPORTANT:
// In the basic version, the overall size of the data is limited to 2^31,
// the largest positive number expressed as 'long'. The size of PersistPtr
// is only 32 bits even when running in a 64-bit environment.
//
// If your overall data size is over 2^31, then you must compile
// under 64 bits, and uncomment the following statement in the beginning 
// of the file factory.h:
// #define LARGE_DATA_SET
//                                      Jiri Soukup, April 13, 2013
// ----------------------------------------------------------------------
// PersistFactory class reduces the control of the Persistent Pager
// to only four function calls:
//
// (1) The constructor creates one instance of the Pager; typically
//     one instance is used for each application class:
//
//     PersistFactory(long objSize,void *validObj,long pageSize,
//        long pagesInMem,long totalSize,char* className,int rd);
//
//     For example:
//
//         class A {
//             ....
//         }a;
//         PersistFactory pf(sizeof(A), &a, 1024, 5, 100000, "A",
//                                         &(A::PersistInfo),int rd);
//
//     will create a Pager for storing objects of type A, where
//     one page will be 1024 bytes, not more than 5 pages in memory,
//     estimated size of the overall data 100000 bytes. This pager
//     will work with file A.PPF.
//     COMMENTS:
//     - Note that one valid object of this class must be provided.
//     This object is used for initialization of the pager space to
//     valid C++ objects, including the hidden virt.function pointers.
//     - The given page size will be rounded down to accomodate objects
//     of this type without overlapping the boundary.
//     - The Pager adjusts its internal data structure to any data size.
//     However, if you provide a good initial estimate, repeated reallocation
//     of internal arrays is avoided.
//     - The page size and the number of pages are file independent.
//     After you close the Pager, another program can access the data
//     using different page specification.
//     - The constructor itself only records the parameters for the Pager,
//     and does not create any internal data structure. Also, it does not
//     open the disk file. All this happens automatically on the first
//     call to getPtr(). If your application does not access any objects
//     of this class, there is only a few bytes penalty for using the Pager.
//     - rd=1 when read-only is requested for the class, rd=0 means readOrWrite.
//       rd=2 read/write with shadow pages (more memory used but
//       faster IO in situations when some data is only read and not modified).

// (2) The destructor moves all pages, which are presently in memory,
//     to the disk, closes the disk file, and destroys all the internal
//     data structures of this Pager. If you want to close the pager,
//     but plan to use it later within the same program, do not destroy
//     it. Use function closePager() instead.
//
// (3) Function closePager() moves all pages which are currently in memory
//     to the disk, and closes the file. It also releases the memory
//     used for the pages, and the internal data structures.
//     If you call getPtr() after the Pager was closed, it automatically
//     re-opens it using the same page specifications.
//
// (4) Function void *getPtr(long ind) is the main function for accessing
//     the data. Each object including arrays is identified by the long
//     integer, which is like an index into the virtual array provided
//     by the pager. For example, if the object size is 24 bytes,
//     ind=3 corresponds to the disk byte address 72. When you call this
//     function, the Pager makes sure the corresponding page is loaded
//     in memory, and then it returns the pointer to the object within
//     the page. THIS POINTER MUST BE USED WITH EXTREME CAUTION,
//     usually immediately without being stored. The next call to getPtr()
//     may release the page, and move the object back to the disk.
//
// Author: Jiri Soukup, Dec.11/97
// ----------------------------------------------------------------------
//       For additional Design Notes - see the bottom of this file.

static int testMode=0; // level of debugging prints, 0=no prints
// --------------- general includes that apply to all environments ------
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef NOT_WIN_NT
#include <dsetup.h>
#endif // NOT_WIN_NT

#define PERSIST_SOURCE
#include "factory.h"

// -------------- includes specific to individual environments ----------
#ifdef __VMS
#include <unixio.h>
#include <types.h>
#include <stat.h>
#include <file.h>
#endif

#ifdef BORLAND
#include <sys\stat.h>
#include <fcntl.h>
#include <io.h>
#endif

#ifdef MICROSOFT
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <io.h>
#endif

#ifdef UNIX
#include <fcntl.h>
#ifndef MAC
#include <sys/stat.h>
#include <sys/types.h>
#endif
#ifdef SUN
#ifndef MAC
#include <sys/uio.h>
#endif
#endif
#endif
#ifdef HP
#include <unistd.h>
#endif

#ifdef SABERCPLUS
#include <sysent.h>
#endif

// --------------- disk access functions in different environments ------
#ifdef GNU
extern "C" {
    int open(const char *,int,...);
#ifdef LINUX
    int creat(const char *,unsigned int);
#else // LINUX
    int creat(const char *,pLong unsigned int);
#endif // LINUX
    int close(int);
    int read(int,char *,int);
    int write(int,char *,int);
    pLong lseek(int,pLong,int);
    int ftruncate(int,off_t);
}
#endif

#ifndef MAC
#ifndef HP
#ifndef __VMS
#ifdef SUN
#ifndef SABERCPLUS
extern "C" {
    int close(int);
    int read(int,char *,int);
    int write(int,char *,int);
    pLong lseek(int,pLong,int);
#ifndef IBM
#ifndef SUN2_1
#ifndef DECPLUS
    int open(char *,int,int);
#endif
#endif
#endif
}
#endif
#endif
#endif
#endif
#endif

// ---------------- stuff for PersistVoid class ------------------------
int PersistVoid::pvPageSz=0;
int PersistVoid::pvNumPages=0;
int PersistVoid::pvTotSz=0;

void PersistVoid::pagerReset(pLong pageSz,pLong numPages,pLong totSz){
    upLong oldT,newT;

    if(pvPageSz>0)closePager();
    if(pvPageSz<pageSz)pvPageSz=pageSz;
    oldT=pvPageSz*pvNumPages;
    newT=pageSz*numPages;
    if(newT<oldT)newT=oldT;
    pvNumPages=(newT+pvPageSz-1)/pvPageSz; // round up the division
    if(pvTotSz<totSz)pvTotSz=totSz;
    PersistVoid::startPager(pvPageSz,pvNumPages,pvTotSz,0);
}

PersistImplement(PersistVoid);

// ---------------- classes used only within the pager ------------------
// ---- Note that PersistHeader definition is now in factory.h (Aug.24, 3014)

class PersistPage {
friend class PersistPager;
public:
    PersistPage(pLong pageNum,pLong pageSz,pLong objSize,void *oneObj,
                                            PersistPage *masterPage);
    ~PersistPage();
    void debugPrt(PersistPager *pp);
private:
    PersistPage *next; // doubly linked list for each pager
    PersistPage *prev;
    pLong pageNo;       // pages are numbered
    char *where;       // where in memory
    char *shadow;      // shadow of the page "where"
};

// ----------------------------------------------------------------------
// MaxNumPag is only an estimate, and if it proves not to be sufficient
// during the program run, array 'pageArr' reallocates as needed.
// A good estimate helps the performance of the software though,
// because it eliminates the reallocation.
// 
// Originally, the constructor did not allocate any internal data,
// and did not even open the file until the first access. Unfortunately,
// this does not work, because before(!) accessing the data, we must
// already know whether the data is already there, how big, whether
// there is anything on the free lists, etc.
//
// The doubly linked list of pages also serves as a stack; its tail is
// always the page which has been unused for the pLongest time.
//
// A pointer to an object must always be even. For this reason, the bit=1
// in 'pageArr' is used to mark pages in which the virtual function pointers
// have been updated to the current environment. Functions ptrPart and updPart
// provide access to the two values. Classes that do not use inheritance
// do not need this update (light-weight classes). Member 'needsUpdate'
// keeps the information about the class (needsUpdate==0 marks a light-weight
// class).
// ----------------------------------------------------------------------
class PersistPager {
friend class PersistFactory;
public:
    void* getPtr(pLong ind); // ind=object address within the pager file
    PersistPager(pLong objSize,void *validObj,pLong pageSize,pLong pagesInMem,
          pLong totalSize,char* className,PersistHeader *ph,pLong firstObject,
          int needsUpd,hiddenType hn,pLong *freeStrings,int rd);
    ~PersistPager();
    void startPager();
    void closePager();
    void debugPages(); // printout of the internal data structures
    void debugCheck(); // automatic crosscheck, no printout if OK
    pLong getPageSz(){return pageSz;}
    int getNumPages(){return numPages;}
private:
    static void setPath(char *path){filePath=path;}
    void markUpdated(PersistPage *pp);
    PersistPage *ptrPart(pLong k){ // pointer information stored in pageArr[]
                     return (PersistPage*)((pLong)(pageArr[k]) & ptrMask);}
    pLong updPart(pLong k){         // update information stored in pageArr[]
                     return (pLong)(pageArr[k]) & updMask;}
    hiddenType myNew;
    void vfUpdate(PersistPage *pp,int numBytesUsed);
    PersistPage *getMasterPage(); // get master page to initialize from
    void truncateFile(); // possibly truncate the unused end
    void debugInfo(char *label);
    void debugHeader(char *label);
    void loadPagerHeader();
    void loadStringHeader();
    void dumpPagerHeader();
    void pageToDisk(PersistPage *pp);
    void pageToMemory(PersistPage *pp);
    void growArr(pLong pageNo); // grow by multiple of 2 to fit the given pageNo
    void sortPages();
    PersistPage* getPage(pLong pageNo); // get new/old page, assign it pageNo
    void remove(PersistPage *pp);
    void insertAsHead(PersistPage *pp);
    pLong cmp(PersistPage*p1,PersistPage *p2); // compare two page numbers
    void loadAllPages(); // accelerator in case all pages will be used

    PersistHeader *pHeader; // outside structure where the header is stored
    pLong *freeStr;   // outside structure for free strings, NULL for classes
    pLong objSz;      // size of one object in bytes
    void *oneObj;    // valid object of the type for which this pager is used
    pLong pageSz;     // size of one page in bytes, multiple of objSize
    pLong numPages;   // number of active pages actually used
    pLong maxActive;  // maximum number of pages in memory at the same time 
    pLong maxNumPag;  // estimated maximum number of pages, not critical
    pLong fileFill;   // number of bytes currently used in the file
    char *fileName;  // className.ppf
    int fileHandle;  // handle to the file to be paged
    int newFile;     // =1 when new problem, =0 when using old file
    PersistPage **pageArr;  // array indexed by pageNo, NULL=not active
    PersistPage *tail; //  tail of the stack for all active pages
    pLong firstObj;    // starting address of the first object
    static pLong ptrMask;
    static pLong updMask;
    static char *filePath;
    int needsUpdate; // 1=needs to update vf pointers, 0=does not (light-weight)
    int IOperm; // 0=read only, 1=read or write; 2=use shadow pages
};
pLong PersistPager::ptrMask=(pLong)(-2);
pLong PersistPager::updMask=(pLong)(1);
char* PersistPager::filePath=(char*)"";

// ----------------------------------------------------------------------
//                     IMPLEMENTATION: PersistHeader 
// ----------------------------------------------------------------------
PersistHeader::PersistHeader(pLong firstObj){freeObj=freeArr=root=0; 
                  current=firstObj;}

// ----------------------------------------------------------------------
//                     IMPLEMENTATION: PersistFactory 
// ----------------------------------------------------------------------
// 'table' is an array of pointers; the index into this array is a persistent
// type id. Note that each instance of PersistentFactory knows the name
// of the class it is serving, and the index it has in this table
// FORMAT of file 'classes.ppf':
//    number of persistent classes
//    class name
//    ...
//    class name
// When reading data from disk, 'table' is set up so that the order of the
// classes is the same. If any class is missing, there will be NULL entry
// in the 'table'. New classes will be at the end of the 'table'.
// WARNING: File 'classes.ppf' must be removed before the first run.
// ----------------------------------------------------------------------
typedef PersistFactory* PersistFactoryPtr;
typedef char* charPtr;

int PersistFactory::useCache=0;
size_t PersistFactory::cacheBufSz=0;
char* PersistFactory::cacheBuf=NULL;
int PersistFactory::cannotIncreaseCacheBuf=0;

char* PersistFactory::filePath=(char*)"";
PersistFactory** PersistFactory::table=NULL;
int PersistFactory::tableSz=0;
int PersistFactory::classCount=0;
int PersistFactory::newOperatorCall=0;


// ----------------------------------------------------------------------
// If this file does not exit, bypass. 
// If it exists, open it, read it into a dummy buffer cacheBuf, and close it.
// In most cases, this will keep the entire file in the cache of the disk
// and will allow a very fast random access to its content.
// ----------------------------------------------------------------------
void PersistFactory::fillCache(char *fileName){
    struct stat stat_buf; int i,fh,newF;

    fh=PersistFactory::myFileOpen(fileName,0,&newF); // newF is not used 
    if(fh<0) return; // when creating data, this always happens

    int rc=stat(fileName, &stat_buf);
    size_t fileSz= rc==0 ? stat_buf.st_size : -1;
    if(cacheBufSz<fileSz+8 && !cannotIncreaseCacheBuf){
	if(cacheBuf==NULL)delete[] cacheBuf;
	cacheBufSz=fileSz+8;
	cacheBuf=new char[cacheBufSz];
	while(cacheBuf==NULL){
            cannotIncreaseCacheBuf=1;
	    if(cacheBufSz<=8){
		printf("Warning: filling cache failed ");
		printf("for file=%s, performance decreased\n",fileName);
		close(fh);
		return;
	    }
	    cacheBufSz=cacheBufSz/2;
	    cacheBuf=new char[cacheBufSz];
	}
    }

    // read the file into cacheBufSz, which may be smaller than the file
    for(i=0; i<fileSz; i=i+cacheBufSz){
	read(fh,cacheBuf,cacheBufSz);
    }
    close(fh);
}

// ----------------------------------------------------------------------
// If the selected number of pages in memory is not large enough to hold
// all pages, call fillCache() instead.
// Main idea: Call getPtr(index) with index equal to the disk index of
// all pages. In case of index=0 use 8, because index 0 is normally treated 
// as NULL pointer.
// ----------------------------------------------------------------------
void PersistPager::loadAllPages(){
    struct stat stat_buf; size_t index,numPagesNeeded;

    int rc=stat(fileName, &stat_buf);
    size_t fileSz= rc==0 ? stat_buf.st_size : -1;
    numPagesNeeded=(fileSz+pageSz-1)/pageSz;
    if(numPagesNeeded>maxActive){
        printf("warning: cannot preload pages for=%s\n",fileName);
	printf("   fileSz=%u pageSz=%u numPagesNeeded=%u maxActive=%u\n",
	           fileSz,pageSz,numPagesNeeded,maxActive);
	// all we can do is to use the simple method
        PersistFactory::fillCache(fileName);
        return;
    }

    // touching the beginning of all pages
    for(index=0; index<fileSz; index=index+pageSz){
	if(index==0)getPtr(sizeof(long long));
	else        getPtr(index);
    }
}

void PersistFactory::removeCacheBuf(){
    if(cacheBuf){
        delete[] cacheBuf;
	cacheBufSz=0;
    }
}

void PersistFactory::setPath(char *path){
    int n=strlen(path);
    if(n==0)return; 
    filePath=new char[n+2];
    strcpy(filePath,path);
    for(int i=0; i<n; i++){
	if(filePath[i]=='/')filePath[i]='\\';
    }
    if(filePath[n-1]!='\\')strcat(filePath,"\\");
    PersistPager::setPath(filePath);
}

void PersistFactory::setRoot(pLong rt){info->root=rt;}

pLong PersistFactory::getRoot(){return info->root;}

// Debugging printout - table of all active factories
void PersistFactory::prtTable(){
    int i,k;
    if(classCount<=0){cout<<"no class registered as persistent\n"; return;}
    for(i=0; i<classCount; i++){
        cout<<"PersistFactory::table["<<i<<"]="<<
              (pLong)(table[i])<<" class="<< table[i]->classN;
        k=table[i]->tableInd;
        if(i!=k)cout<<" error: tableInd="<<k;
        cout<<"\n";
    }
}

// debugging printout of one object as an array of integers
void PersistFactory::debugObj(char *label,void *v,int sz){
    int *ip; int i;
    char* p=(char*)v;
    cout << label <<":";
    for(i=0; i<sz; i=i+sizeof(int)){
        ip=(int*)(&p[i]);
        cout << *ip << " ";
    }
    cout << "\n"; cout.flush();
}

void PersistFactory::freeArr(pLong ind){
    pLong* next=(pLong*)(getPtr(ind));
    *next=info->freeArr;
    info->freeArr=ind;
}
    
// ------------------------------------------------------------------------
// This function must be called exactly once for each registered class.
// Through cIndex, it sets T::PersistIndex for these classes, and it counts
// how many classes there are, and sets the table[] of pointers to all 
// PersistentFactories in use.
//   rd=0 read/write; rd=1 read only, rd=2 read/write with shadow pages
// ------------------------------------------------------------------------
PersistFactory::PersistFactory(int *cIndex,pLong objSize,void *validObj,
                       pLong pageSize,pLong pagesInMem,pLong totalSize,
                       char* className,int needsUpd,hiddenType hn,int rd){
    char buff[80]; static int bSz=80;
    FILE *cFile; // classes file
    int tSz;

    objSz=objSize;
    lastInd=0;
    lastPtr=NULL;
    classN=new char[strlen(className)+1];
    if(classN)strcpy(classN,className); 
    else {
        cout<<"error allocating internal class name\n"; cout.flush(); return;
    }
    firstObj=((sizeof(PersistHeader)+objSz-1)/objSz)*objSz;
    info=new PersistHeader(firstObj);
    if(!info){
        cout<<"error: unable to allocate PersistHeaer\n"; cout.flush(); return;
    }
    pPager=new PersistPager(objSize,validObj,pageSize,pagesInMem,
                     totalSize,className,info,firstObj,needsUpd,hn,NULL,rd);
    if(!pPager){
        cout<<"error: unable to allocate the Pager\n";cout.flush(); return;}
    pPager->startPager();

    // Add this factory to the 'table' of all factories (classes)
    if(!table){
        cFile=fopen("classes.ppf","r");
        if(cFile){
            if(fgets(buff,bSz,cFile)) sscanf(buff,"%d",&tSz);
            fclose(cFile);
            if(tSz>0)tSz=2*tSz; // temporarily for both old and new classes
        }
        else tSz=40; // built-in initial guess
        if(allocTable(tSz)) exit(1);
    }
    if(classCount>=tableSz){
        if(allocTable(2*tableSz)) exit(1);
    }
    table[classCount]=this;
    *cIndex=classCount++;
}

// ----------------------------------------------------------------------
// Re-allocate the class table to a larger size, copy the old table in it.
// Returns: 0=normal, 1=allocation problem
// ----------------------------------------------------------------------
int PersistFactory::allocTable(int sz){
    PersistFactoryPtr *newTable;
    int i;

    newTable=new PersistFactoryPtr[sz];
    if(!newTable){
        cout<<"problem to allocate internal class table, sz="
                               << sz << "\n"; cout.flush();
        return 1;
    }
    for(i=0; i<tableSz; i++)newTable[i]=table[i];
    if(table)delete[] table;
    table=newTable;
    tableSz=sz;
    return 0;
}

// ----------------------------------------------------------------------
PersistFactory::~PersistFactory(){ 
     if(pPager){pPager->closePager(); delete pPager;}
     if(info)delete info;
}
// ----------------------------------------------------------------------
// If there is no file 'classes.ppf', do nothing, and accept the 'table'
// as it is. 
// If the file is there, reorganize the table so that the old class indexes
// apply. If any old classes are missing, use NULL for their position in
// the table. All new classes must be listed with new indexes - after the
// old table. When finished, close the file, and overwrite it with the 
// new information.
// NOTE: This function does not have to be very efficient, it is performed
//   only once during the program run.
// ----------------------------------------------------------------------
void PersistFactory::synchClasses(){
    FILE *cFile,*tFile;
    char buff[80], name[80]; static int bSz=80;
    char **newName; // new 'table', even classes not used must have a name
    char *p;
    int i,k,n;
    PersistFactory *pf;
    static int firstTime=1;
    
    if(!firstTime)return;
    firstTime=0;

    printf(
  "Persistent Pointer Factory Ver.3.8, Copyright(C) 2014 Code Farms Inc.\n");
    // create a temporary table of class names
    if(tableSz<=0 || classCount<=0)return;
    cFile=fopen("classes.ppf","r");
    if(cFile){
        fgets(buff,bSz,cFile); 
        sscanf(buff,"%d",&n);
    }
    else n=0;
    // make sure the table is large enough to take both old and new classes
    if(tableSz<classCount+n){
        if(allocTable(classCount+n)) exit(1);
    }

    newName=new charPtr[tableSz];
    if(!newName){
        cout<<"error: cannot allocate newName[="<<tableSz<<"]\n"; cout.flush();
        exit(2);
    }

    if(cFile){
        for(i=0; i<n; i++){
            fgets(buff,bSz,cFile); sscanf(buff,"%s",name);
            newName[i]=new char[sizeof(name)+1];
            if(!newName[i]){
                cout<<"error: cannot allocate newName[]="<<name<<"\n";
                exit(3);
            }
            strcpy(newName[i],name);

            for(k=i; k<classCount; k++){
                if(!strcmp(name,table[k]->classN))break;
            }
            if(k==i)continue;       // same order of classes
            else if(k>=classCount){ // class not used any more
                table[classCount]=table[i]; 
                table[i]=NULL;
                classCount++;
            }
            else { // old class used in a different order
                pf=table[k]; table[k]=table[i]; table[i]=pf;
                table[k]->tableInd=k;
            }
        }
        fclose(cFile);
    }
    // record the remaining names, reset tableInd to correct values
    for(i=n; i<classCount; i++){
        p=table[i]->classN;
        newName[i]=new char[strlen(p)+1];
        if(!newName[i]){
            cout<<"error allocating newName[]="<<name<<"\n",name; cout.flush();
            exit(4);
        }
        strcpy(newName[i],p);
    }

    // internal table is complete, print it into 'classes.ppf'
    cFile=fopen("classes.ppf","w");
    if(!cFile){
        cout<<"warning: unable to write new file: classes.ppf\n";
        return;
    }
    fprintf(cFile,"%d\n",classCount); fflush(cFile);
    for(k=0; k<classCount; k++){
        fprintf(cFile,"%s\n",newName[k]); fflush(cFile);
        if(table[k])table[k]->tableInd=k;
    }
    fclose(cFile);

    // destroy the temporary table of class names
    for(k=0; k<classCount; k++){ delete[] newName[k];}
    delete[] newName;
}
// ----------------------------------------------------------------------
void PersistFactory::startPager(){pPager->startPager();}
// ----------------------------------------------------------------------
void* PersistFactory::getPtr(pLong ind){ return pPager->getPtr(ind);}
// ----------------------------------------------------------------------
void PersistFactory::closePager(){ pPager->closePager(); }
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// Allocate an array of objects of the type with which this PersistFactory 
// is associated, and return the index for the beginning.
// The function allocates (size+1) entries, entry 0 contains the size of
// the array, index for entry 1 is returned as the beginning of the array.
// The code must use getPtr() and not just byte offsets, because even with
// the offset of 1 the two objects may be on different pages.
// ----------------------------------------------------------------------
pLong PersistFactory::newArr(pLong size){ 
    pLong k,*last,next,*p;  void *v;

    for(k=info->freeArr, last=NULL; k>0; ){
        p=(pLong*)getPtr(k-objSz);
        if((*p)>=size && (*p)<2*size)break;
        last=(pLong*)(getPtr(k)); k=(*last);
    }
    if(k==0){ // new array is needed
        k=info->current+objSz; info->current=k+size*objSz;
        p=(pLong*)(getPtr(k-objSz)); *p=size;
        // the array needs no initialization, the page is already initialized
    }
    else { // array located at k can be reused, no initialization needed
        p=(pLong*)(getPtr(k)); next=(*p);
        if(last){ *last=next; }
        else { info->freeArr=next; }
    }
    v=getPtr(k); // all this may not be eventually needed JS
    lastInd=k; lastPtr=v;
    return k;
}

// ----------------------------------------------------------------------
// newObj allocates one object for T::new()
// ----------------------------------------------------------------------
void* PersistFactory::newObj(){ void *v; pLong *vv;
    if(info->freeObj>0){
        v=getPtr(info->freeObj);
        lastInd=info->freeObj;
        vv=(pLong*)v; info->freeObj=(*vv);
    }
    else {
        v=getPtr(info->current); 
        lastInd=info->current;
        info->current=info->current+objSz;
    }
    lastPtr=v;
    lastTableInd=tableInd;
    return v;
} 

// ----------------------------------------------------------------------
// free() moves a single object to the free list or, if the object is at the
// very end of the free disk space, decreases the high-water-mark instead.
// ----------------------------------------------------------------------
void PersistFactory::freeObj(pLong ind){
    pLong i=ind;
    void* v=getPtr(i);
    if(i+objSz==info->current)info->current=i;
    else { pLong* s=(pLong*)v; *s=info->freeObj; info->freeObj=i;}
}

// ----------------------------------------------------------------------
// If the array starting at oldIndex and of size oldSize is at the end
// of the disk file, which is a common situation, we do not need to
// reallocate the array, only to shorten or lengthen it, without copying
// the data. The function returns the new value of 'current', 
// essentilly the index for the end of the data. 
// If the given last array cannot be adjusted, the return is NULL.
// ----------------------------------------------------------------------
pLong PersistFactory::adjArray(pLong oldInd,pLong oldSz,pLong newSz){ 
    pLong k,*p;

    if(oldInd+oldSz*objSz!=info->current) return 0;

    // we have an array at the end of the data
    k=oldInd;
    k=k+(newSz*objSz); // works both for decreasing/increasing the array
    info->current=k;
    p=(pLong*)(getPtr(oldInd-objSz)); *p=newSz;
    getPtr(k-objSz); // force to load all the pages
    return k;
}

// ----------------------------------------------------------------------
// This is only an auxilliary function to recover index for the recently
// allocated pointer. It should not be used by the application itself,
// because as soon as another object is allocated, the stored values are
// replaced, and to find the index for a general pointer would be CPU
// intensive.
// ----------------------------------------------------------------------
pLong PersistFactory::getInd(void *ptr){ 
    if(ptr==lastPtr && lastTableInd==tableInd)return lastInd; 
    cout<<"error: keeping raw pointer="<<(pLong)ptr<<" for too pLong\n";
    return 0;
}

// ----------------------------------------------------------------------
//                     IMPLEMENTATION: PersistPage 
// ----------------------------------------------------------------------
// Allocate the memory for this page
// objSize = size of one object
// oneObj  = pointer to any valid object of this class.
// These two variables are used to initialize pages to arrays of valid objects.
// ----------------------------------------------------------------------
PersistPage::PersistPage(pLong pageNum,pLong pageSz,pLong objSize,void *oneObj,
                         PersistPage *masterPage){
    char *p,*s; pLong i,k;

    next=prev=NULL; 
    pageNo=pageNum;
    where=new char[pageSz];
    shadow=NULL;
    if(!where){
        cout<<"error: allocating page, size="<<pageSz<<"\n"; cout.flush();
        return;
    }

    // In order to initialize a page, we can either fill it with copies
    // of a valid object of this type, or copy another page which is
    // already in memory (which is faster). We only have to be careful
    // not to copy the first page (pageNo=0), because its beginning has
    // PersistHeader, and not valid objects.

    if(masterPage){ // copy all bytes from the master page
        for(k=0, p=where, s=masterPage->where; k<pageSz; k++,p++,s++){*p=(*s);}
    }
    else {          // repeat copying the given valid object
        s=(char*)oneObj; // copy this object over the entire page
        for(k=0, p=where; k<pageSz; k=k+objSize, p=p+objSize){
            for(i=0; i<objSize; i++) *(p+i)=s[i];
        }
    }
}
    
// ----------------------------------------------------------------------
// Check the page for being properly disconnected, and destroy the page
// ----------------------------------------------------------------------
PersistPage::~PersistPage(){
    if(next||prev){
        cout<<"error: destroying page which is still in a list\n";
    }
    if(where)delete[] where;
}

// ----------------------------------------------------------------------
// Debugging print of one page, when testMode>1, it prints also its content
// ----------------------------------------------------------------------
void PersistPage::debugPrt(PersistPager *pp){
    pLong k,sz,*p,pageSz;
    printf("pager=%ld pageNo=%ld where=%ld next=%ld prev=%ld\n",
                                             pp,pageNo,where,next,prev);

    if(testMode<=1)return; // otherwise detailed printout
    sz=sizeof(pLong);
    pageSz=pp->getPageSz();
    for(k=0, p=(pLong*)where; k<pageSz; k=k+sz, p++){
        printf("%ld ",*p);
    }
    printf("\n");
}

// ----------------------------------------------------------------------
// When marking a page as updated, we cannot just add 1 to pageArr[] - this is 
// a pointer, and when adding 1 may change the pointer by 4.
// ----------------------------------------------------------------------
void PersistPager::markUpdated(PersistPage *pp){
    pLong k=(pLong)(pageArr[pp->pageNo]);
    pageArr[pp->pageNo]=(PersistPage*)(k | 1);
}

// ----------------------------------------------------------------------
//                     IMPLEMENTATION: PersistPager
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
// return < 0 when p1 will be next on the list (p1 is before p2)
// ----------------------------------------------------------------------
inline pLong PersistPager::cmp(PersistPage *p1,PersistPage *p2){
    if(p1 && !p2)return -1;
    if(!p1 && p2)return 1;
    return p1->pageNo - p2->pageNo;
}

// ----------------------------------------------------------------------
// Update virtual function pointers for all objects 
// actually used in this page, so that they fit to the existing
// environment. This is performed by invoking
// a special new() operator over every object on this page.
//
// Several things must be considered:
// (a) needsUpdate defines whether this class uses inheritance and therefore
//     needs any updates.
// (b) pageNo=0 starts with a header; for this page, the update must not
//     involve the first few objects (the size of the header).
// (c) Typically, the last page is not full, and we update only its used
//     part as defined by 'numBytesUsed'.
// (d) When creating a new page, the PersistPage constructor sets all its
//     memory space to valid objects. This means that if only a part of
//     the page is loaded from disk, the remaining space always contains
//     valid objects.
// ----------------------------------------------------------------------
void PersistPager::vfUpdate(PersistPage *pp,int numBytesUsed){
    int i,istart,sz; void *v;


    if(updPart(pp->pageNo)==1)return; // page already updated
    if(pp->pageNo==0)istart=firstObj; else istart=0;

    // Block any initialization by all default constructors.
    PersistFactory::newOperatorCall=1; 

    for(i=istart; i<numBytesUsed; i=i+objSz){
        v= &(pp->where[i]);
        (*myNew)(v);
    }

    // Reset back the initialization flag
    PersistFactory::newOperatorCall=0; 

    markUpdated(pp);
}

// ----------------------------------------------------------------------
// If the original disk file was reduced during the run of the pager,
// truncate the unused end. Note that the size is rounded up to the
// number of entire pages.
// ----------------------------------------------------------------------
void PersistPager::truncateFile(){
    // first get the size of the true data in bytes
    pLong newSize=pHeader->current;
    // then round it up to the entire pages (in bytes)
    newSize=((newSize+pageSz-1)/pageSz)*pageSz; // round up
    if(fileFill<=newSize)return; // no truncation needed
#ifdef DOS
    int i=chsize(fileHandle,newSize); // truncate the file
#else
    off_t sz=newSize;
    int i=ftruncate(fileHandle,sz); // truncate the file
#endif
    if(i!=0)cout<<"warning: data reduced but unable to truncate file="<<
                                                 fileName<<"\n";
}

// ----------------------------------------------------------------------
// Temporarily, the ring is turned into a set of singly linked, NULL ending
// sub-lists, each partially sorted; 'next' is used for these lists.
// Pointers 'prev' are used to keep track of the beginnings of all sublists.
// The sub-arrays are merged in successive passes. The algorithm is of 
// order O(n*logn), and no recursive functions are used.
// ----------------------------------------------------------------------
void PersistPager::sortPages(){
    PersistPage *np,*pp,*p1,*p2,*nxt,*last,*lst,*list;

    if(tail==NULL || tail->next==tail)return;
    
    // change to singly-linked, NULL ending list
    for(np=list=tail->next; np; ){
        pp=np;
        if(np==tail)np=NULL; else np=np->next;
        pp->next=np;
        pp->prev=NULL;
    }

    //  detect sorted sublists, link their beginings by 'prev'
    for(np=list->next, pp=list, pp->prev=NULL; np->next; np=nxt){ 
        nxt=np->next;
        if(cmp(np,nxt)<=0)continue;
        pp->prev=nxt; pp=nxt; pp->prev=NULL; np->next=NULL;
    }
    
    while(list->prev){ // repeat merging sublists
        for(np=list, pp=np->prev, lst=NULL; np || pp; ){
            // two sublists start at np,pp; one can be empty
            // lst is the beginning of the last already merged sublist
            // nxt will be the beginning of the new merged sublist
            // p1, p2 traverse the two sublists as they merge
            if(cmp(np,pp)<=0){nxt=np; p1=np->next; p2=pp;}
            else             {nxt=pp; p1=np; p2=pp->next;}

            // 'last' will be the last item of the newly merged sublist
            for(last=nxt ;p1||p2; ){
                if(cmp(p1,p2)<=0){last->next=p1; last=p1; p1=p1->next;}
                else             {last->next=p2; last=p2; p2=p2->next;}
            }
            last->next=NULL;

            if(lst)lst->prev=nxt; else list=nxt;
            lst=nxt;

            if(!pp)break;
            np=pp->prev; if(!np)break;
            pp=np->prev;
        }
        nxt->prev=NULL;
    }

    // the list is sorted, return it to the normal, doubly linked ring
    for(np=list, lst=NULL; np!=NULL; np=np->next){ np->prev=lst; lst=np; }
    lst->next=list; list->prev=lst; tail=lst;
}

// ----------------------------------------------------------------------
// Debugging printout of the global pager information
// ----------------------------------------------------------------------
void PersistPager::debugInfo(char *label){
    printf("%s current=%ld freeObj=%ld freeArr=%ld root=%ld\n", label,
        pHeader->current, pHeader->freeObj, pHeader->freeArr, pHeader->root);
}

// ----------------------------------------------------------------------
// Remove page pp from the stack/list
// ----------------------------------------------------------------------
void PersistPager::remove(PersistPage *pp){
    PersistPage *p1,*p2;

    if(!pp){cout<<"error: attempt to remove NULL page\n"; cout.flush(); return;}
    p1=pp->prev;
    p2=pp->next;
    if(!p1 || !p2){cout<<"error: removing isolated page\n";cout.flush();return;}
    if(p1==pp) {tail=NULL;}
    else       { p1->next=p2; p2->prev=p1; if(tail==pp)tail=p1; }
    pp->next=pp->prev=NULL;
}

// ----------------------------------------------------------------------
// Insert page pp as the head of the stack
// ----------------------------------------------------------------------
void PersistPager::insertAsHead(PersistPage *pp){
    PersistPage* h;

    if(!pp){cout<<"error: attempt to remove NULL page\n"; cout.flush(); return;}
    if((pp->next) || (pp->prev)){
      cout<<"inserting page which is not disconnected\n"; cout.flush(); return;}
    if(!tail){tail=pp; pp->next=pp->prev=pp;}
    else {h=tail->next; tail->next=pp; pp->next=h; h->prev=pp; pp->prev=tail;}
}

// ----------------------------------------------------------------------
// Find any existing page with pageNo != 0, as a master for initializing
// new pages. It cannot be the first page (pageNo=0) because this page
// has an irregular header in the beginning (PersistHeader).
// Return NULL if the page is not found.
// ----------------------------------------------------------------------
PersistPage* PersistPager::getMasterPage(){
    PersistPage *pp;
   
    if(!tail)return NULL;
    for(pp=tail->next; pp; ){
        if(pp->pageNo>0)break;
        if(pp==tail)pp=NULL; else pp=pp->next;
    }
    return pp;
}

// ----------------------------------------------------------------------
// If there are fewer than maxNumPag pages active, create a new page.
// If there is not, go to the page stack, remove the page which has
// been unused for the pLongest time, move its content to disk, and
// re-assign its pageNo (both within the PersistPage class, and in pageArr).
// ----------------------------------------------------------------------
PersistPage* PersistPager::getPage(pLong pageNum){
    PersistPage *pp,*mp; pLong k;

    if(numPages<maxActive){ // create a new page, add it
        if(pageArr[pageNum]){
            cout<<"error: page="<<pageNum<<" already exists\n"; cout.flush();
        }
        mp=getMasterPage();
        pp=new PersistPage(pageNum,pageSz,objSz,oneObj,mp);
        if(!pp){cout<<"error; unable allocate PersistPage\n"; cout.flush();}
        pageArr[pageNum]=pp;
        markUpdated(pp);
        insertAsHead(pp);
        numPages++;
        if(!newFile)pageToMemory(pp); 
    }
    else {  // get the page from the tail of the stack
        pp=tail; tail=pp->prev; // not necessary to remove & insertAsHead
        pageToDisk(pp); // move this page to the disk
        pageArr[pp->pageNo]=(PersistPage*)(updPart(pp->pageNo));
        pp->pageNo=pageNum;
        k=updPart(pageNum);
        pageArr[pageNum]=pp; if(k)markUpdated(pp);
        pageToMemory(pp);
    }
    if(testMode)pp->debugPrt(this);
    return pp;
}

// ----------------------------------------------------------------------
// For the given address in the pager file, make sure the relevant page is
// loaded into memory, and then return the pointer to the object.
// ----------------------------------------------------------------------
void* PersistPager::getPtr(pLong address){
    pLong pageNo,offset; PersistPage *page;

    if(IOperm==0 && address>pHeader->current){
        printf("error file=%s is RD_ONLY, access beyond the EOF\n",fileName);
        printf("file size=%ld, accessing address=%ld\n",
                                                   pHeader->current,address);
        return NULL;
    }

    if(!address){
        cout<<"error: pager dereferencing NULL address\n"; cout.flush();
        exit(1);
    }
    if(!pageArr){
          cout<<"error: data access without openning pager\n"; cout.flush();
          return NULL;
    }
    pageNo=address/pageSz;
    if(pageNo>=maxNumPag) growArr(pageNo);
    if(pageNo>=maxNumPag){
        cout<<"error counting pages, should never happen\n"; cout.flush();
    }
    page=ptrPart(pageNo);
    if(page){
        if(page->prev!=tail){ // FIX Oct.28/00
            remove(page); 
            insertAsHead(page);
        }
    }
    else {       // another page re-assigned
        page=getPage(pageNo); // another page re-assigned
    }
    if(!page){printf("error: cannot assign a new page\n"); return NULL;}
    offset=address - pageNo*pageSz;
    if(offset<0 || offset>=pageSz){
         cout<<"error in calculating offset\n"; cout.flush();
    }
    return (page->where + offset);
}

// ----------------------------------------------------------------------
// Grow the size of pageArr by the multiple of two to fit the given pageNo
// Bypass, if no need to increase the size - you can call it all the time.
// ----------------------------------------------------------------------
void PersistPager::growArr(pLong pageNo){
    PersistPage **newPgArr;

    pLong oldNumPag=maxNumPag;
    for( ; maxNumPag<=pageNo; maxNumPag=2*maxNumPag)continue;
    if(oldNumPag<maxNumPag){
        newPgArr=new PersistPage* [maxNumPag];
        if(newPgArr){
            for(pLong i=0; i<maxNumPag; i++){
                if(i<oldNumPag) newPgArr[i]=pageArr[i];
                else            newPgArr[i]=NULL;
            }
            delete[] pageArr;
            pageArr=newPgArr;
        }
        else printf("error: unable to increase array to size=%ld\n",maxNumPag);
    }
}

// ----------------------------------------------------------------------
// Moving a page from disk to memory - this is a straight byte transfer
// Transfer only the data which actually is on disk - which may not be
// the entire page. The entire page in memory is already initialized as
// valid objects anyway. 
// The initialization of the memory to the array of valid objects
// is done at the time of creating the page. However, since the previous
// page could have been pageNo=0 which starts with a header, this function
// starts with initializing the first few objects.
// If this page is used for the first time within this program run,
// vfUpdate() call at the end of this function updates all objects
// that came from disk (sets their virtual function pointers).
// ----------------------------------------------------------------------
void PersistPager::pageToMemory(PersistPage *pp){
    pLong i,k,address,numBytesReadIn;
    char *p,*t,s;
    static int warningIssued=0;
    
    // In case this is an empty or partially full page, we still want
    // the entire memory space to contain valid objects. We can rely on
    // the old data from the previous page providing this, except when
    // the previous pageNo=0 started with a header.
    if(needsUpdate){
        t=(char*)oneObj; // copy this object over the entire page
        for(k=0, p=pp->where; k<firstObj; k=k+objSz, p=p+objSz){
            for(i=0; i<objSz; i++) *(p+i)=t[i];
        }
    }

    if(testMode){printf("page->mem: "); pp->debugPrt(this);}
    address=(pp->pageNo)*pageSz;
    if(fileFill>address){ // when not true: new page, not on disk yet
        if(fileHandle>=0){
            lseek(fileHandle, address, 0); // 0=offset mode from the beg.of file
            numBytesReadIn=read(fileHandle, pp->where, pageSz);
            if(numBytesReadIn<=0){
                printf("error: read pageSz=%d\n",pageSz +sizeof(int));
                return;
            }
            if(IOperm==2){
                if(!(pp->shadow) && !warningIssued)pp->shadow=new char[pageSz];
                if(pp->shadow)memcpy(pp->shadow, pp->where, pageSz);
                else  {
                   if(!warningIssued)printf(
                  "WARNING: out of memory, continueing wihout shadow pages\n");
                   warningIssued=1;
                }
            }
            if(needsUpdate==1)vfUpdate(pp,numBytesReadIn);
        }
        else printf("error: no file from which to move data to memory\n");
    }
}

// ----------------------------------------------------------------------
// Writing a page to the disk:
//     - if the page is within the size of the file, just move it in;
//     - if the page is outside of the current size, write a filler
//       and then the new page; update fileFill info.
// ----------------------------------------------------------------------
void PersistPager::pageToDisk(PersistPage *pp)
{
    pLong address;
    
    if(IOperm==0)return;
    if(IOperm==2 && pp->shadow!=NULL){
        if(memcmp(pp->where,pp->shadow,pageSz)==0)return;
    }

    if(testMode){printf("page->disk:"); pp->debugPrt(this);}
    address=(pp->pageNo)*pageSz;
    if(fileFill<address){
        lseek(fileHandle, 0L, 2); // 2=mode set to the end of file
        // create a new section on the disk
        for(;fileFill<address; fileFill=fileFill+pageSz){
            // filler pages will be initialized to any valid objects
            write(fileHandle, pp->where,pageSz);
        }
    }
    // over-writing old data on disk
    if(fileHandle>=0){
        lseek(fileHandle, address, 0); // 0=offset mode from the beg.of file
        write(fileHandle, pp->where, pageSz);
    }
    else printf("error: no file to move to the disk\n");
    if(fileFill<address+pageSz)fileFill=address+pageSz;
}

// ----------------------------------------------------------------------
// Allocate the page array, and open the file for direct access.
// If this is a new data file, no page is read in. This happens with
// the first call to getPtr(). However, if the file already contains
// some data, the PersistHeader information is read from Page 0,
// so that the Pager can continue in the use of the existing data.
// ----------------------------------------------------------------------
void PersistPager::startPager(){
    pLong numPg; int newF; 

    if(PersistFactory::useCache==1)PersistFactory::fillCache(fileName);

    if(fileHandle>=0){ 
        printf("error: startPager() called on pager which is active\n");
        return;
    }
    if(pageArr){
        printf("error: startPager() called on pager with internal data\n");
        return;
    }

    fileHandle=PersistFactory::myFileOpen(fileName,IOperm,&newF); 
    newFile=newF; // keeps this information on the pager

    if(fileHandle<0){
        printf("error: cannot open file <%s>,",fileName);
        if(IOperm==0)printf(" read-only mode requested");
        printf("\n");
        return;
    }
    // the value of newFile determines how the following functions work
    if(freeStr)loadStringHeader(); 
    else loadPagerHeader(); 

    // allocate the internal array, at least as big as for the old file
    numPg=(pHeader->current + pageSz-1)/pageSz;
    if(!newFile && numPg>maxNumPag)maxNumPag=numPg;
    pageArr=new PersistPage* [maxNumPag];
    if(!pageArr){
        printf("error: unable to allocate pageArr[%ld]\n",maxNumPag);
        return;
    }
    for(pLong i=0; i<maxNumPag; i++)pageArr[i]=NULL;
    
    if(PersistFactory::useCache==2)loadAllPages();
}

// ----------------------------------------------------------------------
// For a given fileName and the R/W flag, it opens a binary file
// and returns the handle (<0 when failure) and
// newFile which is 1 for a new file, 0 otherwise.
// Set wFlg-1 when writing is required, set it to 0 otherwise.
// ----------------------------------------------------------------------
int PersistFactory::myFileOpen(char *fileName,int wFlag,int *newF){
    int Oflag,Sflag,fh;
    *newF=0; // will record whether old/new data file is used
    if(wFlag==0)Oflag=O_RDONLY; else Oflag=O_RDWR;
    if(wFlag==0)Sflag=S_IREAD; else Sflag=S_IREAD | S_IWRITE;

#ifdef MAC
    fh=open(fileName,Oflag);
    if(fh<0 && wFlag>0){
        fh=open(fileName,O_CREAT | Oflag); *newF=1;
    }
#else
#ifdef __VMS
    fh=open(fileName,Oflag, 0); 
    if(fh<0 && wFlag>0){
        fh=open(fileName,O_CREAT | Oflag, 0); *newF=1;
    }
#else
    // fh=open(fileName,O_RDWR | O_BINARY, S_IREAD);
#ifdef DOS
    fh=open(fileName,Oflag | O_BINARY,Sflag);
    if(fh<0 && wFlag>0){
        fh=open(fileName,O_CREAT | Oflag | O_BINARY,Sflag);
        *newF=1;
    }
#else
    fh=open(fileName,Oflag,Sflag);
    if(fh<0 && wFlag>0){
        fh=open(fileName,O_CREAT | Oflag,Sflag);
        *newF=1;
    }
#endif
#endif
#endif
    return fh;
}

// ----------------------------------------------------------------------
// When newFile==1, Read the pager header which starts at the begining 
// of the first page.
// When newFile==0, initialize all the values to an empty data set.
// ----------------------------------------------------------------------
void PersistPager::loadPagerHeader(){
    if(newFile){
        pHeader->current=firstObj;
        pHeader->root=pHeader->freeObj=pHeader->freeArr=0;
        fileFill=0;
    }
    else {
        if(fileHandle>=0){
            lseek(fileHandle, 0, 0); // 0=offset mode from the beg.of file
            read(fileHandle, (char*)pHeader, sizeof(PersistHeader));
            // first get the true size of the data, in bytes
            fileFill=pHeader->current; 
            // then round it up to entire pages (again in bytes)
            fileFill=((fileFill+pageSz-1)/pageSz)*pageSz;
        }
        else printf("error: cannot read the header, file not open\n");
    }
}

// ----------------------------------------------------------------------
// Read in only the first two pLongs, and fix up the header as if remaining
// values are not used.
// When newFile==1, Read the pager header which starts at the begining 
// of the first page.
// When newFile==0, initialize all the values to an empty data set.
// ----------------------------------------------------------------------
void PersistPager::loadStringHeader(){
    pLong i,n;

    // For strings, these members of PersistHeader are unused
    pHeader->root=pHeader->freeObj=pHeader->freeArr=0;
    // only the member 'current' must be set:

    if(newFile){
        pHeader->current=firstObj;
        fileFill=0;

        freeStr[0]=pageSz;
        freeStr[1]=firstObj;
        for(i=2, n=pageSz/sizeof(pLong); i<n; i++)freeStr[i]=0;
    }
    else {
        if(fileHandle>=0){
            lseek(fileHandle, 0, 0); // 0=offset mode from the beg.of file
            read(fileHandle, (char*)freeStr, pageSz);
            if(freeStr[0]!=pageSz){
                printf("error: page size for strings must not change\n");
                printf("       old page=%d bytes, new page=%d bytes\n",
                                         freeStr[0],pageSz);
                return;
            }
            // first get the true size of the data, in bytes
            fileFill=freeStr[1]; 
            // then round it up to entire pages (again in bytes)
            fileFill=((fileFill+pageSz-1)/pageSz)*pageSz;

            pHeader->current=freeStr[1];
        }
        else printf("error: cannot read the header, file not open\n");
    }
}

// ----------------------------------------------------------------------
// Copy the Pager global state to the beginning of the disk file.
// ----------------------------------------------------------------------
void PersistPager::dumpPagerHeader(){
    if(fileHandle>=0){
        if(freeStr){
            freeStr[0]=pageSz;
            freeStr[1]=pHeader->current;
            lseek(fileHandle, 0, 0); // 0=offset mode from the beg.of file
            write(fileHandle, (char*)freeStr, pageSz);
        }
        else { // pager for normal classes
            lseek(fileHandle, 0, 0); // 0=offset mode from the beg.of file
            write(fileHandle, (char*)pHeader, sizeof(PersistHeader));
        }
    }
}

// ----------------------------------------------------------------------
// Move all active pages to the disk, close the file, clean internal data
// ----------------------------------------------------------------------
void PersistPager::closePager(){
    PersistPage *np,*pp;

    // sort the pages, in order to avoid unnecessary filling of the file
    sortPages();
    // move all pages to the disk
    for(np=tail; np; ){
        pageToDisk(np);
        if(np==tail->prev)np=NULL; else np=np->next;
    }

    // copy the Pager header directly to the beginning of the file
    // This must be done after(!) all pages move to disk.
    dumpPagerHeader();
    truncateFile(); // possibly truncate the unused end

    // initialize the pager as unused
    pHeader->current=firstObj; 
    pHeader->freeObj=pHeader->freeArr=pHeader->root=0;

    // close the file
    if(fileHandle>=0) close(fileHandle);
    fileHandle= -1;

    // destroy all pages
    for(np=tail; np; ){
        pp=np;
        if(np==tail->prev)np=NULL; else np=np->next;
        remove(pp); delete pp;
    }
    tail=NULL;

    delete[] pageArr;
    pageArr=NULL;
}

// ----------------------------------------------------------------------
// objSize    = size of one object in bytes
// pageSize   = size of one page in bytes, may be internally rounded down
//              in order to avoid problems on the page boundary
// pagesInMem = maximum number of pages in memory at any given time
// totalSize  = estimate of the total space for all objects of this type
// class Name = name of this class, to be used for the name of the file
// pHead      = pre-allocated page header. When opening a new file, leave
//              it intact. When opening an old file, read the header from
//              the beginning of the file. However, this is not done right
//              away, only when startPager() is called. 
// needsUpd   = 1 when update of virtual function pointers is required
// hn         = stored pointer to a function, which provides ... ?
// freeStrings = pointer to the outside array, NULL for normal classes
// rd         = 0 for read/write, =1 for read only, =2 for read/write with
//              shadow pages (more memory used but more efficient IO).
// ----------------------------------------------------------------------
PersistPager::PersistPager(pLong objSize,void *validObj,pLong pageSize,
  pLong pagesInMem,pLong totalSize,char* className,PersistHeader *ph,
  pLong firstObject,int needsUpd,hiddenType hn,pLong *freeStrings,int rd){

    if(testMode)printf("constructor PersistPager=%ld: ",this);
#ifdef NOT_WIN_NT
    if((020000000000 & GetVersion())==0){
        printf("sorry - this free binary version does not run with WinNT\n");
        return;
    }
}
#endif // NOT_WIN_NT

    if(rd==1)     IOperm=0; //read only
    else if(rd==0)IOperm=1; // read/write
    else if(rd==2)IOperm=2; // read or write with shadow pages
    else {
       printf("WARNING: rd=%d illegal, using rd=0 (plain read/write mode\n",rd);
       IOperm=1;
    }
    myNew=hn;
    needsUpdate=needsUpd;
    fileFill=0;
    pHeader=ph;
    freeStr=freeStrings;
    objSz=objSize;
    oneObj=validObj;
    firstObj=firstObject;
    if(!objSz)printf("error: wrong input for PersistPager constructor\n"); 
    else {
        // round the header up to the integer number of objects
        pageSz=(pageSize/objSize)*objSize; if(pageSz<=0)pageSz=objSz;
        if(firstObj>pageSz)pageSz=firstObj;
        maxActive=pagesInMem;
        numPages=0;
        if(totalSize<pagesInMem*pageSize) totalSize=pagesInMem*pageSize; 
        maxNumPag=(firstObj+totalSize+pageSz-1)/pageSz;
        if(maxNumPag<=0)maxNumPag=1;
        int m=strlen(filePath);
        int n=strlen(className);
        fileName=new char[m+n+5];
        if(!fileName){printf("error: unable to allocate fileName\n");}
        else { 
	    strcpy(fileName,filePath);
	    strcat(fileName,className); 
	    strcat(fileName,".ppf");
	    // replace characters '<' and '>' for template classes by '_'
	    for(n=0; fileName[n]!='\0'; n++){
		if(fileName[n]=='<' || fileName[n]=='>')fileName[n]='_';
            }
	}
    }
    pageArr=NULL;
    tail=NULL;
    fileHandle= -1;
    //  loadPagerHeader() sets up member 'fileFill'
    //  loadStringHeader() sets also freeStr
}
        
// ----------------------------------------------------------------------
// Close the pager, and clean up all the data.
// ----------------------------------------------------------------------
PersistPager::~PersistPager(){
    if(fileHandle>=0)closePager();
    if(fileName)delete[] fileName;
}
// ----------------------------------------------------------------------
// Automatic check of the internal data structure. If it does not print
// anything, everything is OK.
// ----------------------------------------------------------------------
void PersistPager::debugCheck(){
    PersistPage *pp,*pa; pLong k;
    
    if(tail) {
        for(pp=tail->next; ; pp=pp->next){
            k=pp->pageNo;
            pa=ptrPart(k);
            if(pa && pa != pp)
                printf("pageArr[%ld]=%ld page=%ld\n",k,pa,pp);
            if(pp==tail)break;
        }
    }

    for(k=0; k<maxNumPag; k++){
        pp=ptrPart(k);
        if(pp && pp->pageNo!=k)
                printf("page=%ld pageArr[%ld]=%ld\n",pp,k,ptrPart(k));
    }
}

// ----------------------------------------------------------------------
// print the internal list of pages, for debugging
// ----------------------------------------------------------------------
void PersistPager::debugPages(){
    PersistPage *np; int i;

    if(!tail){ printf("DEBUG: list of Pages is empty\n"); return;}
    printf("tail=%ld\n",tail);
    for(np=tail->next; np; ){
        np->debugPrt(this);
        if(np==tail)np=NULL; else np=np->next;
    }

    // print non-zero entries of pageArr
    for(i=0; i<maxNumPag; i++){
        if(pageArr[i])printf("pageArr[%d]=%ld %ld\n",i,ptrPart(i),updPart(i));
    }
}

// ----------------------------------------------------------------------
// print the global header in various forms
// ----------------------------------------------------------------------
void PersistPager::debugHeader(char *label){
    unsigned int i,k; char *p;

    printf("%s ",label);
    printf("addr=%ld current=%ld freeObj=%ld freeArr=%ld root=%ld\n",
     pHeader,pHeader->current,pHeader->freeObj,pHeader->freeArr,pHeader->root);
    for(i=0, p=(char *)pHeader; i<sizeof(PersistHeader); i++,p++){
        k=(unsigned int)p[i]; printf("%4d ",k);
    }
    printf("\n");
}
// ------------------- IMPLEMENTATION OF STRINGS ------------------------

void PersistString::startPager(pLong pgSz,pLong maxPgs,pLong totMem,int rd){
    static char ch='\0';
    pLong n;
    int ph=sizeof(PersistHeader);                            \

    if(pgSz<ph)pgSz=ph;
    if(maxPgs==1){ 
        if(totMem<=pgSz){totMem=pgSz=pgSz+ph;}
        else maxPgs=2; /* all data in 1 page, or at least 2 pages needed */
    } 

    pageSz=pgSz;
    n=pgSz/sizeof(pLong); 
    if(n*sizeof(pLong) !=pgSz){
        n++;
        pageSz=n*sizeof(pLong);
    }
    info=new PersistHeader(pageSz);
    spare=new char[pageSz];
    if(!info || !spare){
        cout<<"error: unable to allocate PersistHeader or spare page, sz=" <<
               pageSz << "\n";
        return;
    }
    info->current=pageSz;
    freeStr=new pLong[n];
    if(!freeStr){printf("error: unable to allocate freeStr\n"); return;}
       // one object is one character, because we want to address the pager
       // by individual characters,
       // pstring.ppf will be the name of the file,
       // 0=no virtual pointers involved, NULL=no callback function needed.
    freeSz=n;
    pPager=new PersistPager(1,&ch,pageSz,maxPgs,
                 totMem,(char*)"pstring",info,pageSz,0,NULL,freeStr,rd);
    if(!pPager){printf("error: unable to allocate the string Pager\n"); return;}
    pPager->startPager();
    diskSz=freeStr[1];
}

void PersistString::closePager(){
    PersistString::pPager->closePager();
    delete[] freeStr;
    delete[] spare;
    delete info;
}

// ----------------------------------------------------------------------
// (1) Check whether there is a free string of this length
// (2) If not, get new string from the end of the file.
// (3) Return pointer to the string (this also moves it into memory)
// Set 'index' to the new string.
//
// POSSIBLE IMPROVEMENT:
// When there is no string of exactly required length, carv a section
// from some pLong string (if available).
// ----------------------------------------------------------------------
char* PersistString::newStr(pLong sz){
    pLong *pLongPtr; void *nxtPtr; char *p;

    if(!pPager || !freeStr){
        cout << "error: not calling PersistString::startPager() before new()\n";
        return NULL;
    }
    getFreeString(sz);
    if(!index)getNewString(sz);
    if(!index)return NULL;
    setTail(index,0); // must be called after the header is set, 0=active string
    lastPtr=pPager->getPtr(index); // repeat, pager may have swapped pages
    lastInd=index;
    p=(char*)lastPtr;
    if(p) *p=0; // initialize as empty string
    return p;
}

// ----------------------------------------------------------------------
PersistString::PersistString(char  *s){
    if(s){
        pLong sz=strlen(s) +1;
        char* p=newStr(sz);
        if(p)strcpy(p,s);
    }
    else index=0;
}

// ----------------------------------------------------------------------
PersistString::PersistString(wchar_t  s[]){
    pLong i; char *p; char* r=(char*)s;
    for(i=0; s[i]!=0; i++)continue;
    i=2*(i+1); // total number of char
    p=newStr(i);
    for(i=i-1; i>=0; i--)p[i]=r[i];
}

// ----------------------------------------------------------------------
// When destroying this object, the string must be released (when at the
// end of the disk space), or added to the free list.
// Releasing the string trigger to release another string, etc.,
// but instead of coding this is a recursive function, it is a simple loop here.
// ----------------------------------------------------------------------
void PersistString::delString(){
    pLong sz,*pLongPtr; int isFree;

    if(index==0)return;
    pLongPtr=(pLong*)(pPager->getPtr(index-sizeof(pLong)));
    sz=(*pLongPtr);
    if(index+sz+pageSz == diskSz){ // last string on disk, first page has index
        diskSz=index-sizeof(pLong);
        for(;;){
            if(diskSz<=pageSz)break;
            sz=getTailSize(diskSz-1,&isFree);
            if(!isFree)break;
            if(diskSz-sz<pageSz){
                cout << "internal error when cleaning free string list\n";
                break;
            }
            releaseFree(diskSz-sz,sz);
            diskSz=diskSz-sz-sizeof(pLong);
        }
    }
    else {   // string in the middle of the space, move it to the free list
        setFree(index);
    }
    index=0;
}
// ----------------------------------------------------------------------
// Release a string from the free list. Don't worry about diskSz or other
// details. Parameter 'ind' points to the beginning of the string (after
// the header), as everywhere in this program.
// ----------------------------------------------------------------------
void PersistString::releaseFree(pLong ind,pLong sz){
    pLong next,prev,*pLongPtr,i; char *cp;

    i=sz/sizeof(pLong);

    // same page, no need to call getPtr() twice
    cp=(char*)(pPager->getPtr(ind)); pLongPtr=(pLong*)cp; prev=(*pLongPtr);
    cp=cp-sizeof(pLong);              pLongPtr=(pLong*)cp; next=(*pLongPtr);

    if(prev==0)freeStr[i]=next;
    else {
        pLongPtr=(pLong*)(pPager->getPtr(prev-sizeof(pLong))); *pLongPtr=next;
    }
    if(next){
        pLongPtr=(pLong*)(pPager->getPtr(next)); *pLongPtr=prev;
    }
}

// ----------------------------------------------------------------------
// Pick up the free string of the given size, and set 'index'.
// If there is no string of this length, set index=0;
// 
// LIST OF FREE STRINGS:
// The target to which all references in this list point is the the
// START OF THE ACTUAL STRING. In other words, the references are the
// same as the internal 'index' which gives the position of the beginning
// of the string on the disk.
// The forward reference (next) is stored
// in the previous 4 bytes, normally used for the size of the string,
// the backward reference (prev) is stored in the first 4 bytes of the string.
// The first entry into the list (its head) is in freeStr[i],
// where after some round-ups i=stringSize/sizeof(pLong).
//
//                    next(sz)->     next(sz)->    next(sz)->0
//   freeStr[i]->  0<-prev(str)    <-prev(str)   <-prev(str)
//
// Warning: When accessing/modifying all these values, use getPtr()
// in order to guarantee that the page with the relevant data is in memory.
// You can take the advantage of the fact that the entire string including
// its header is always on the same page.
//
// When allocating a new string, if the leftover of the page is smaller
// than 3+sizeof(pLong) (string of 4 char or less), it is added to the 
// string. In the same vein, if there is no free string for given i,
// we also try i+1.
// ----------------------------------------------------------------------
void PersistString::getFreeString(pLong sz){
    char *vp; pLong *pLongPtr,next,prev,i,k,size,address;

    size=roundSz(sz);
    if(size==0){index=0; return;}
    i=size/sizeof(pLong);
    if(i<2)i=2;
    for(k=0; k<=1, i<freeSz; k++, i++){
        address=freeStr[i];
        if(address>0)break;
    }
    if(address<=0){index=0; return;}

    // re-link the list of free strings, size is the useful size of the string
    vp=(char*)(pPager->getPtr(address)); // 'next' field
    pLongPtr=(pLong*)vp;                prev=(*pLongPtr);
    pLongPtr=(pLong*)(vp-sizeof(pLong)); next=(*pLongPtr);
    freeStr[i]=next;
    *pLongPtr=i*sizeof(pLong);
    if(next>0){
        vp=(char*)(pPager->getPtr(next)); // 'prev' field of the next string
        pLongPtr=(pLong*)vp;
        *pLongPtr=0;
    }
    index=address;
}

// ----------------------------------------------------------------------
// Pick up a new string from the end of the file, and set 'index'
// It all must be within the same page, which may result in moving
// some space to the free list.  In case of a failure, set index=0;
// ----------------------------------------------------------------------
void PersistString::getNewString(pLong size){
    pLong lastChar,*pLongPtr,tmp,leftOver,sz,stringToFree; 

    sz=roundSz(size);
    if(sz==0){index=0; return;}
    lastChar=diskSz+sz+sizeof(pLong)-1;
    if(diskSz/pageSz != lastChar/pageSz){ // does not fit into the current page
        tmp=(diskSz/pageSz + 1)*pageSz - diskSz;
        pLongPtr=(pLong*)(pPager->getPtr(diskSz));
        if(!pLongPtr)return;
        *pLongPtr=tmp-sizeof(pLong);
        stringToFree=diskSz+sizeof(pLong);
        diskSz=(diskSz/pageSz + 1)*pageSz;
        setFree(stringToFree);
    }

    // If the leftover of the page is too small (<3*sizeof(pLong)), just
    // make this string bigger.
    leftOver=((diskSz/pageSz)+1)*pageSz - (diskSz+sz+sizeof(pLong));
    if(leftOver < 3*sizeof(pLong))sz=sz+leftOver;
    
    pLongPtr=(pLong*)(pPager->getPtr(diskSz));
    if(pLongPtr){
        *pLongPtr=sz;
        index=diskSz+sizeof(pLong);
        diskSz=diskSz+sz+sizeof(pLong);
        info->current=diskSz;
    }
    else index=0;
}

// ----------------------------------------------------------------------
// Take a string with a valid header, and add it to the free list.
// ----------------------------------------------------------------------
void PersistString::setFree(pLong ind){
    pLong *pLongPtr,i; char *vc;

    setTail(ind,1); // must be called before changing the header, 1=free string
    vc=(char*)(pPager->getPtr(ind));
    pLongPtr=(pLong*)(vc-sizeof(pLong));
    if(!pLongPtr)return;
    i=(*pLongPtr)/sizeof(pLong);
    *pLongPtr=freeStr[i];
    pLongPtr=(pLong*)(pPager->getPtr(ind));
    *pLongPtr=0;
    freeStr[i]=ind;
}
    
// ----------------------------------------------------------------------
// When a string has a header showing its size, set the corresponding tail info.
// isFree=0 ... active string, isFree=1 ... string on the free list.
// Tricky part: Different format depending on whether the tail is 1 or 4 bytes.
// The entire string is always in one page, so we can safely work with
// pointers within that string.
// WARNING: The size of the string must be already in the leading 4 bytes
// ind = the address of the beginning of the string
// ----------------------------------------------------------------------
void PersistString::setTail(pLong ind,int isFree){
    pLong *pLongPtr,sz,isFreeFlag; unsigned char *cp,c[4];

    pLongPtr=(pLong*)c;
    *pLongPtr=0;
    c[3]=1;
    isFreeFlag=(*pLongPtr);

    pLongPtr=(pLong*)(pPager->getPtr(ind-sizeof(pLong)));
    if(!pLongPtr)return;
    sz= *pLongPtr;
    cp=(unsigned char*)pLongPtr; 
    if(sz<=shortTail){ // tail is just 1 byte
        cp=cp+sizeof(pLong)+sz-1; // position of the last byte
        *cp=(unsigned char)(sz/2 + isFree); // sz is a multiple of 4, compacted
    }
    else {             // tail is 4 bytes
        cp=cp+sz;      // position of the last 4 bytes
        pLongPtr=(pLong*)cp;
        *pLongPtr=sz/4;
        if(*pLongPtr>=isFreeFlag){
            cout<<"ERROR setTail() sz="<<sz<<" too big\n"; cout.flush();
        }
        else if(isFree) *pLongPtr=(*pLongPtr)+isFreeFlag;
    }
}

// ----------------------------------------------------------------------
// For the index of the last byte of a string, return its size and whether
// it is free.
// ----------------------------------------------------------------------
pLong PersistString::getTailSize(pLong lastByte,int *isFree){
    unsigned char *cp,k; pLong *pLongPtr;
    static unsigned char one=1; 
    static unsigned char remain=254; 

    cp=(unsigned char*)(pPager->getPtr(lastByte));
    *isFree=(*cp) & one;
    k=(*cp) & remain; // remaining part of the byte
    if(k) return (pLong)(k+k);      // 1 byte tail
    pLongPtr=(pLong*)(cp-3); // 4 byte tail
    return (*pLongPtr)/remain;
}

// ----------------------------------------------------------------------
// Calculate the space needed for the given string. Three things must be
// considered: (1) 1 or 4 byte tail, (2) must be multiple of 4.
// (3) The string is always allocated at least 8 bytes internally.
// The value returned by this function does not include the 4 leading bytes.
// ----------------------------------------------------------------------
pLong PersistString::roundSz(pLong size){
    pLong sz,n;

    if(size<shortTail)sz=size+1; else sz=size+sizeof(pLong);
    n=sz/sizeof(pLong);
    if(n*sizeof(pLong) != sz) sz=(n + 1)*sizeof(pLong);
    if(sz<2*sizeof(pLong))sz=2*sizeof(pLong);

    if(sz+sizeof(pLong)>pageSz){
        cout << "error allocating string size=" << size << 
                ", cannot fit page size=" << pageSz;
        cout.flush();
        sz=0;
    }
    return sz;
}

// ----------------------------------------------------------------------
// Hiding the call to PersistPager - the user should not see this.
// ----------------------------------------------------------------------
char*  PersistString::getPtr(pLong ind) { return (char*)(pPager->getPtr(ind));}

// ----------------------------------------------------------------------
// This works only when called before another object is allocated.
// POSSIBLE IMPROVEMENT: Instead of keeping just lastInd and lastPtr,
//    there could be a stack of pairs - but is this efficient/useful??
// ----------------------------------------------------------------------
pLong  PersistString::getInd(char *s){
    if(lastPtr==s)return lastInd;
    cout << "error: calling getInd() after allocating other strings\n";
    return 0;
}

// ----------------------------------------------------------------------
// Compare this string with string s. Both strings must be '\0' ending.
// It is not checked that the string ends properly and its size does
// not exceed one page.
// ----------------------------------------------------------------------
int PersistString::cmp(const PersistString& s){
    char *cp; 
 
    if(pPager->getNumPages()>1){
        return strcmp((char*)(pPager->getPtr(index)),
                      (char*)(pPager->getPtr(s.index)));
    }
    cp=(char*)(pPager->getPtr(index));
    strcpy(spare,cp);
    cp=(char*)(pPager->getPtr(s.index));
    return strcmp(spare,cp);
}
int PersistString::cmp(const char *s){
    return strcmp((char*)(pPager->getPtr(index)),s);
}

// ----------------------------------------------------------------------
// Equivalent of strlen() function for regular strings
// ----------------------------------------------------------------------
size_t PersistString::size(){
    char* cp=(char*)(pPager->getPtr(index));
    return strlen(cp);
}

// ----------------------------------------------------------------------
// Returns the maximum size of the string as allocated (from the user
// point of view, without the header and the tail)
// ----------------------------------------------------------------------
pLong PersistString::allocSize(){
    pLong sz;
    pLong* pLongPtr=(pLong*)(pPager->getPtr(index-sizeof(pLong)));
    sz=(*pLongPtr);
    if(sz<=shortTail)sz--; else sz=sz-4;
    return sz;
}

// ----------------------------------------------------------------------
// Check the validity of the given string, including:
// - position of the string within the existing disk space
// - header/tail comparision and validity
// - NULL ending string fitting the size
// Returns: 0=normal, 1=error
// ----------------------------------------------------------------------
int PersistString::debugOne(){
    int isFree,ret; char *p;
    pLong i,s,size,sz,*pLongPtr;
    // index=beginning of the actual string, header=beg. of the header,
    // size=size of the string space, including the tail, as derived from header
    // sz=size of the string space, including the tail, as derived from tail
    
    if(index==0)return 0;
    ret=0;
    p=(char*)(pPager->getPtr(index));

    // do we fit into the existing disk space
    pLongPtr=(pLong*)p;
    size= *(pLongPtr-1); 
    if(index+size>diskSz){
        cout<<"string error index="<<index<<" size="<<size;
        cout<<" is over diskSz="<<diskSz<<"\n";
        cout.flush();
        prt("string display:","\n");
        ret=1;
    }

    // compare tail and header
    sz=getTailSize(index+size-1,&isFree); // size, string without header
    if(sz!=size){
        cout<<"string error header size="<<size<<" tail size="<<sz<<"\n";
        cout.flush();
        prt("string display:","\n");
        ret=1;
    }
    if(isFree!=0){
        cout<<"string error isFree is set\n";
        cout.flush();
        prt("string display:","\n");
        ret=1;
    }

    // is the NULL within the string
    if(size<shortTail)s=size-1;
    else              s=size-4;
    if(strlen(p) >= s){
        cout<<"string error: NULL not within the allocated string strlen="<<
                                                 strlen(p)<<"\n";
        cout<<" index="<<index<<" headSz="<<size<<" tailSz="<<sz<<"\n";
        cout.flush();
        prt("string display:","\n");
        ret=1;
    }
    return ret;
}

// ----------------------------------------------------------------------
// Go through all the free strings, and check:
// - position of the string within the existing disk space
// - tail validity for the given size
// On the first error, print message and exit(1).
// size = length of the string + size of the tail (1 or 4).
// Use prtFlg=1 to print all free disk spaces, =0 for no print.
// ----------------------------------------------------------------------
void PersistString::debugFree(int prtFlg){
    pLong i,size,sz,address,*pLongPtr,lastByte,next; char *vc;
    int isFree;

    for(i=2; i<freeSz; i++){
        size=i*sizeof(pLong);
        for(address=freeStr[i]; address; address=next){
            vc=(char*)(pPager->getPtr(address));
            pLongPtr=(pLong*)(vc-sizeof(pLong)); next=(*pLongPtr);

            if(prtFlg)cout << "i=" << i << " free string=" << address <<"\n";
            if(address+size>=diskSz){
                cout<<"freeStr["<<i<<"]="<<freeStr[i]<<" size="<<size;
                cout<<" address="<<address<<"\n";
                cout<<"outside of the diskSz="<<diskSz<<"\n";
                cout<<"freeSz="<<freeSz<<"\n";
                cout.flush();
                exit(1);
            }
            if(size<shortTail)lastByte=address+size-1;
            else                     lastByte=address+size-4;
            sz=getTailSize(lastByte,&isFree);
            if(!isFree || sz!=size){
                cout<<"error in the tail of the free string:\n";
                cout<<"freeStr["<<i<<"]="<<freeStr[i]<<" size="<<size;
                cout<<" address="<<address<<"\nlastByte="<<lastByte;
                cout<<"gives: sz="<<sz<<" isFree="<<isFree<<"\n";
                cout<<"IT SHOULD BE: sz==size, isFree=1\n";
                cout<<"freeSz="<<freeSz<<"\n";
                cout.flush();
                exit(1);
            }
        }
    }
}
    
// ------------------------ DESIGN NOTES -------------------------------
// This section provides some additional details on the overall design.
// Most of this stuff really bepLong to file factory.h, but since my intention
// is to hide the Pager when the binary copy is used, I placed it here.
// 
// (1) The free list uses pLong indexes, not pointers. The end of list is -1.
// (2) Free arrays form a separate list which starts at PersistFreeA.
//     An free array is used when its size is less than 2*requiredSize.
// (3) Re-used objects and arrays do not have to be initialized.
//     Each page is initialized by repeated copying of a valid object.
//     Other pages copy one of the existing (and already initialized) pages.
//     The internal data are NOT initialized to 0.
// (4) Index is the byte address in the pager file.
// (5) The Pager itself (class PersistFactory) works with bytes and not objects,
//     and does not use temmplates. If it used templates, the compiler would
//     create an additional class for every application class - which we
//     obviously want to avoid. 
// (6) When using PersistPtr<T>, your code must avoid using pointers to T.
//     Use consistently PersistPtr<T> instead of T* ptr. The pager constantly
//     changes the locations of objects in the memory. This program provides
//     some run-time checking of this condition, but this checking is not
//     complete, and using a raw pointer can easily crash your program.
// (7) Operator delete MUST be called with the PersistPtr and not with the 
//     raw pointer.
// (8) When using pointers to base class, ClassVptr<> must be used. It
//     uses more memory.
// (9) Pointers to members (withing an object) are prohibited.
// 
// HANDLING OF VIRTUAL FUNCTION POINTERS:
// (a) PersistPage constructor initializes the memory space for all pages
//     to an array of valid objects. This guarantees that any new objects
//     are valid.
// (b) Member 'needsUpdate' marks classes which use inheritance, and need
//     updates of the virtu.fun.ptrs for subsequent program runs.
// (c) pageArr[] entries use bit=1 to mark pages which are up-to-date.
//     Functions ptrPart(), updPart(), and markUpdated() provide
//     access to this data.
// (d) When moving a new page to memory, the first few objects in the memory
//     space are always re-initialized, because the previous page could have
//     been page=0 which starts with a header.
// (e) If the page is only partially used, only the valid data moves from
//     the disk and is updated.  The remaining part of the memory space is
//     taken from the previous page, and is therefore already valid.
// (f) Since the PersistPager class is not a template, it has no access to
//     the type of the class it serves. In order to have have access to the
//     special operator new(), it must store a pointer to a generic function
//     which hides it (hiddenNew).
//
// SPECIAL OPERATOR NEW()
// This operator is used for updating the virt.function pointers.
// The organization uses a bag of programming tricks:
// (a) Operator new() is controlled by static void* T::PersistAlloc.
//     When this variable is NULL, new() allocates a new object (or picks
//     is up from the free list) as usual. When it is not NULL, the operation is
//     applied to this memory location, re-setting vf pointers for one object.
// (b) For any class declared as PersistVclass, constructor T(PersistDummy*)
//     is automatically provided. In case that PersistAlloc!=NULL, it does
//     nothing, only resets PersistAlloc back to NULL.
// (c) When the update of the vf pointers is needed during the paging
//     operation, PersistPager has to set T::PersistAlloc and then call new().
//     This service is provided by static function T::hiddenNew(void*).
// (d) The pointer to this function is passed through the constructors from
//     T to PersistFactory to PersistPager, and is stored there as 'myNew'.
//
// CLASS PersistString
// ----------------------------------
// There is a special class which provides storage and allocation of text
// strings(PersistString). This class is similar to the 'string' class
// provided by other libraries, except that PersistString makes all strings
// persistent. It also manages both memory and disk space, including free
// lists of strings of different length.
// PersistString is a simple class, not a template.
//
// Due to the irregularity of the string size, the design of this
// class must prevent fragmentation of the disk space, and must reclaim
// disk space whenever possible - on a continuous basis rather than
// interrupting occassionally for a cleanup. Any free string at the end of
// the file must be immediately released, and the file truncated.
// The truncation of the physical file is, however, perfored only when
// closing the pager.
//
// Internally, the entire string is always inside the same page.
// This implies two important limitations:
// (1) Considering the internal format of the strings (see below), no string
//     including the possible '\0' character at the end may be pLonger than
//     (pageSize - 8).
// (2) The program which creates the string file (the first call to it)
//     determines the page size, and this size cannot be changed in
//     subsequent runs. [Node the difference from the pager for regular
//     objects, which permits to change the page size in subsequent runs.]
//
// Strings always start on a 4B boundary, and have the following format:
//    header (4B), string (at least 4B), tail info (exactly 1B or 4B).
// For actively used strings:
//    header=size of string including the tail (always multiple of 4B),
//           but not including the header;
//    string=the stored text string, possibly including '\0' at the end;
// For strings on the free list:
//    header=disk address of the next free string of the same size
//    first 4B of string=disk address of the previous free string, same size
// The tail info is the same in both situations:
//    Bit 1 = 0 for an active string, =1 for a string on the free list.
//    Remaining bits > 0: The number gives the size of the string including tail
//                        divided by 4;
//    Remaining bits = 0: Previous 3 bytes store size of string including tail,
//                        divided by 4;
// When implementing this, note that when overlaying char[4] and pLong,
// the lowest bit of the pLong is in char[0].
//
// This implies that the most efficient string sizes are 7,11,15,19,...,507,
// for which the internal tail info is only 1 byte. For pLonger strings,
// the tail is always 4 bytes, with no wasted space for string sizes
// 508,512,...
//
// The first page always stores the following information:
//    bytes 0-3:   size of page
//    bytes 4-7:   size of disk
//    bytes 8-11:  free strings size=8 (including tail)
//    bytes 12-15: free strings size=12 (including tail)
//    bytes 16-19: free strings size=16 (including tail)
//    ...
//    bytes (pageSize-4 to endOfPage): free strings size=(pageSize-4) incl.tail
//                 which from the user point of view is the pLongest string
//                 allowed, pageSize - 8.
//
// We need the size info both at the beginning and at the end of the string
// for handling and re-combination of free strings. Using a shorter info
// saves lot of space for short strings - in most applications strings
// are shorter than 507 bytes.
//
// When picking up a free string of the given size, currently only the bin
// with the given size is tested. My plan is to expand this to picking up
// perhaps a slightly pLonger string (by much pLonger?) if it is available.
// 
//                                      Jiri Soukup, Nov.26/98
//
// ----------------------------------------------------------------
// WARNINGS: 
// - When class names are pLong, file names will be pLong!! What operating
//   systems can handle that?

