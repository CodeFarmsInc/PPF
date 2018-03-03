#include <stdio.h>
#include "factory.h"
void foo(){
    // EXPERIMENT WITH COMMENTING/UNCOMMENTING THE NEXT THREE LINES:
    PersistString s;              // causes no addition to file pstring.ppf
    // PersistString s; s="xyz";  // causes an addition to file pstring.ppf
    // PersistString s("xyz");    // causes an addition to file pstring.ppf
};
int main(){
    PersistString::startPager(64,2,200,0); // opening pager for strings
    PersistStart; // needed to synchronize classes

    for(int i=0; i<100; i++) foo();
    PersistString::closePager();
    return 0;
}

