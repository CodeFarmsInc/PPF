/****************************************************************/
/*							      	*/
/*   Copyright (C) 1988 1989             			*/
/*	Code Farms Inc.,  All Rights Reserved.			*/
/*							  	*/
/****************************************************************/

/* Ring sorting works like this:
 * It finds sub-sections that are already sorted.
 * Then it walks over the list and picks up the first two subsections,
 * and merges them. Then it keeps walking through the list and 
 * picks up next two subsections and merges them, and so on until 
 * the end of list is reached. Then it starts again from the beginning,
 * and repeats this process until all data is sorted.
 * The advantages of this algorithm are:
 * O(log n) complexity, no recursive function used, no additional
 * storage or temporary stack needed.
 * int (*cmpF)(PTR(pType)&,PTR(pType)&); compares two objects, like for qsort()
 * The functionality is identical with DOL function orgc/lib/sortring.c from 1898. 
 *                                       Author: Jiri Soukup, 2012
 */


char* ZZrSRfun(PTR(pType) *tail,ppfSortFun cmpF){
	/* int (*cmpF)(pType*,pType*); compares two objects, like for qsort() */
    PTR(cType) h1,t1,h2,t2,p,nxt,p1,p2,first,last,start,hook1,hook2;
    int stopFlg;

    if(!tail)return(tail);
	start=tail->MRG(_,id).next;
    if(tail==start)return(tail);

    /* walk through the input, insert up/down section by section */
    for(stopFlg=0, t1=tail; !stopFlg; t1=t2){ 
		h2=t1->MRG(_,id).next;
        t2=h2;
		nxt=h2->MRG(_,id).next;
        for(p=nxt; p!=h2 ;p=nxt){
            if(p==tail)stopFlg=1;
			nxt=p->MRG(_,id).next;
	    if((*cmpF)(p,t2)>=0)t2=p;
	    else if((*cmpF)(p,h2)<=0){
                if(nxt==h2){h2=p; break;}
                else {
					t2->MRG(_,id).next=nxt; t1->MRG(_,id).next=p; p->MRG(_,id).next=h2; 
					h2=p;
				}
            }
            else break;
        }
    }
	nxt=t2->MRG(_,id).next;
    if(nxt==h2)return(t2); /* result already sorted */
    start=h2; /* section may overlap the original start */  

    /* keep sorting adjacent sublists until everything is one list */
    for(stopFlg=0; !stopFlg;){
        hook1=t2;
        /* find the first sublist */
		h1=t2->MRG(_,id).next;
		nxt=h1->MRG(_,id).next;
        for(last=h1, p=nxt; ; last=p, p=nxt){
			nxt=p->MRG(_,id).next;
	        if((*cmpF)(last,p)>0)break;
        }
        t1=last; h2=nxt;
        if(h2==h1)break; /* one sorted sublist, job finished */

        /* find the second sublist */
		h2=t1->MRG(_,id).next;
		nxt=h2->MRG(_,id).next;
        for(last=h2, p=nxt; ; last=p, p=nxt){
			nxt=p->MRG(_,id).next;
	        if((*cmpF)(last,p)>0)break;
        }
        t2=last;
        if(h1==p)stopFlg=1; /* these are the last two sublists */
        hook2=p;

		t1->MRG(_,id).next=NULL; t2->MRG(_,id).next=NULL;/* unhook both sublists */

        /* merge the two lists, new list from 'first' to 'last' */
        first=last=(char *)NULL;
        for(p1=h1, p2=h2 ;p1||p2; last=p ){
	    if(!p1)     p=p2;
	    else if(!p2)p=p1;
	    else if((*cmpF)(p1,p2)<=0)p=p1;
	    else p=p2;
    
		nxt=tail->MRG(_,id).p;
	    if(p==p1) p1=nxt; else p2=nxt;
            if(!first)first=p;
            if(last){last->MRG(_,id).next=p;}
        }

        /* hook both list back ZZinto the main list */
        if(stopFlg){last->MRG(_,id).next=first;}
		else { hook1->MRG(_,id).next=first; last->MRG(_,id).next=hook2; }
        t2=last;
    }
    return(last);
}
