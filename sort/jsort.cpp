ALL THIS IS A DEAD END ROAD _ MERGE CANNOT WORK IN-PLACE WITH AN ARRAY
	// =======================================================================
// Jiri's fast merge sort
// Author: Jiri Soukup, Copyright (c) Code Farms Inc. 2012
// =======================================================================
// Algorithm: Inteligent yet simple merge which takes advantage of
//    natural runs in the data - both increasing and decreasing.
//    It works 'in place', and makes no function calls exept for
//    calling a compare function identical to the one used in qsort()
// Note: This is the simple form of the algorithm without selective merging.
// =======================================================================

void jsort(int *v, int n){  // version that sorts a plain array of integers
	int i,i1,i2,i3,k,k1,k2,mid,pass,rev,first,vv;
    for(first=1; ; first=0){ // repeated merge runs, break off when one single run
		for(i=pass=0; i<n; pass++){ // run will alternate between 0 and 1
			if(pass>1)run=0;
			if(pass==0)i1=i; else i2=i;
			rev=0; // ??? missingn the case of == in the beginning
			if(first && i<n-1){
				if(v[i]>v[i+1])rev=1;
			}

			// detect the next run
			if(rev==0){ // normal case, ascending run
				for(i++; i<n ;i++){if(v[i]<v[i])break;}
				if(run==0)i2=i-1; else i3=i-1;
			}
			else { // special case, descending run, can happen only on the first pass
				for(i++; i<n ;i++){if(v[i]>v[i])break;}
				if(run==0)i2=i-1; else i3=i-1;
				mid=(i2-i1-1)/2; // reverse the run
				for(k=0; k<=mid; k++){vv=v[i1+k]; v[i1+k]=v[i2-k]; v[i2-k]=vv;} // swap
			}
			
			if(pass==0)continue;
			// merge the runs between (i1 ... i2) and (i2+1 ... i3)
			for(k1=i1, k2=i2+1; ; ){
				if(v[k1]<=v[k2]){
					k1++; 
					if(k1>i2)break; else continue;
				}
				else {
				    vv=v[k1]; v[k1]=v[k2]; v[k2]=vv; // swap
					k1++; k2++;
					if(k1>i2 || k2>i3)break; else continue;
				}


