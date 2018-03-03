#include <stdio.h>
#include <stdlib.h>
#include "hypscoll.h"
#include "..\factory.h"

class Library;
class Book;


class parent_books {
    friend class class_books;
    friend class Library;
    PTR<Book> tail;
public:
    parent_books(){tail=NULL;}
};
class child_books {
    friend class class_books;
    friend class Book;
    PTR<Book> next;
child_books(){next=NULL;}
};

class Library {
PersistClass(Library);
    friend class_books;
    parent_books _books;
};

class Book {
PersistClass<Book>;
    friend class_books;
    child_books _books;

    int id;
public:
    Book(){id=0;}
    Book(int i){id=i;}
    int getID(){return id;}
    void prt(){printf("%d\n",id);}
};

// ZZ_HYPER_SINGLE_COLLECT(books,Library,Book);
#include "hyp"

int cmpF(PTR<Book> b1,PTR<Book> b2){
	if(b1->getID()<b2->getID())return -1;
	if(b1->getID()>b2->getID())return  1;
	return 0;
}

int main(){
    int i; PTR<Book> b; 
    books_iterator bit;

    Library::startPager(32,100,10000,rd);
    Book::startPager(32,100,10000,rd);
    PersistStart;
    PTR(Library) lib;

    lib=new Library();

    for(i=0; i<14; i=i+2){
	b=new Book(i); 
	books.addHead(lib,b);
	b=new Book(i+1); 
	books.addTail(lib,b);
    }

    bit.start(lib); i=0; printf("original set:\n");
    ITERATE(bit,b){
	if(b->getID()==3 || b->getID()==4){
	    printf("removed: ");
	    books.del(lib,b);
	}
        b->prt(); 
    }

    bit.start(lib); printf("after removal:\n");
    ITERATE(bit,b){
        b->prt(); 
    }

    books.sort(cmpF,lib);
    bit.start(lib); printf("sorted:\n");
    ITERATE(bit,b){

    Library::closePager();
    Book::closePager();
    return 0;
}

// include just once somewhere
PersistImplement(Library); 
PersistImplement(Book);

#include "..\pointer.cpp" // always compile when persist.pointers are used
	
