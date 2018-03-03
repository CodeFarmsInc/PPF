
// ***********************************************************************
//                       Code Farms Inc.
//          7214 Jock Trail, Richmond, Ontario, Canada, K0A 2Z0
//          tel 613-838-4829           htpp://www.codefarms.com
// 
//                   Copyright (C) 1997-2004
//        This software is subject to copyright protection under
//        the laws of Canada, United States, and other countries.
// ************************************************************************

// -------------------------------------------------------------------------
// The implementation of these functions depends on the application class T,
// and therefore must be after the definition of class A, even though we
// really want them inline. Since PersistPtr<T> is a template, this file
// must be included in every file using this pointer, typically at its end.
// -------------------------------------------------------------------------
#include "factory.h"

#ifndef PPF_POINTER_INCLUDE
#define PPF_POINTER_INCLUDE

// -------------------------------------------------------------------------
template<class T> void PersistPtr<T>::getRoot(){
    index=T::PersistStore->getRoot();
}

template<class T> void PersistPtr<T>::setRoot(){
    T::PersistStore->setRoot(index);
}

template<class T> void PersistPtr<T>::delObj(){
                 T::PersistStore->freeObj(index); index=0;}

template<class T> void PersistPtr<T>::delArr(){
                 T::PersistStore->freeArr(index); index=0;}

template<class T> PersistPtr<T>& PersistPtr<T>::adjArray(pLong oldInd,pLong oldSz,pLong newSz){ 
    index=getStore()->adjArray(oldInd,oldSz,newSz);
    return *this;
}

template<class T> PersistPtr<T>::PersistPtr(T* realPtr){
    if(realPtr) index=T::PersistStore->getInd((void*)realPtr);
    else index=0;
}
// -------------------------------------------------------------------------
template<class T> void PersistVptr<T>::getRoot(){
    PersistFactory* pf=T::PersistStore;
    index=pf->getRoot();
    factoryInd=pf->getTableInd();
}

template<class T> void PersistVptr<T>::setRoot(){
    getStore()->setRoot(index);
}

template<class T> void PersistVptr<T>::delObj(){
                                     getStore()->freeObj(index); index=0;}

template<class T> void PersistVptr<T>::delArr(){
                                     getStore()->freeArr(index); index=0;}

template<class T> T* PersistVptr<T>::getPtr(pLong ind) const{
    PersistFactory* pf=getStore();
    if(pf)return (T*)(pf->getPtr(ind)); 
    else return NULL;
}

template<class T> PersistVptr<T>& PersistVptr<T>::adjArray(pLong oldInd,pLong oldSz,pLong newSz){ 
    index=getStore()->adjArray(oldInd,oldSz,newSz);
    return *this;
}

template<class T> PersistVptr<T>::PersistVptr(T* realPtr){
    if(realPtr){
        PersistFactory* pf=realPtr->getStore();
        factoryInd=pf->getTableInd();
        index=pf->getInd((void*)realPtr);
    }
    else index=0;
}
// -------------------------------------------------------------------------


#endif // PPF_POINTER_INCLUDE
