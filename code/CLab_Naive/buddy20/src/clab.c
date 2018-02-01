/***********************************************************
 * File: clab.c 
 * Desc: misc structures used by the clab bdd 
 *       functions in bddop.c 
 * Auth: Rune M. Jensen, ITU
 * Date: 23/04/04
 **********************************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "clab.h"


/***********************************************************
 *              
 * Aux. data structures and functions for
 * 
 * void bdd_extractvalues(BDD b, int lastCSPvar)
 * int bdd_recognize(BDD b, int pattern)
 *               
 **********************************************************/

int*  bddVar2cspVar;         /* mapping from bdd var number to associated csp var number */
                             /* (must be provided by the user of the bdd library)        */			     
int   cspVarNum;             /* number of CSP variables */
int   completeFrom;          /* from this CSP variable and on, any assignment is possible */
int** unmatchedPatterns;     /* unmatchedPatterns[i] is an array of non-negative numbers 
                                representing assignment patterns of CSP variable i that still
                                has not been recognized */    
int*  unmatchedPatternsSize; /* unmatchedPatternsSize[i] number of patterns left of CSP variable i */   
int*  domStart;              /* domStart[i] number of the BDD variable where the encoding of 
                                CSP variable i starts */        


int val2pattern(int val, int dom) {

  int v;
  int pat;
  int i;
  int bitNum;
 
  bitNum = (int) ceil(log( (double) dom ) / log(2.0)); 

  v = val;
  pat = 0;
  for (i = 0; i < bitNum; i++) 
    {
      pat <<= 1;
      pat |= v & 1;
      v >>= 1;
    }
  
  return pat;
}



int pattern2val(int pat, int dom) {
  
  int val;
  int p;
  int i;
  int bitNum;
  
  bitNum = (int) ceil(log( (double) dom ) / log(2.0)); 

  p = pat;
  val = 0;
  for (i = 0; i < bitNum; i++) 
    {
      val <<= 1;
      val |= p & 1;
      p >>= 1;
    }
 
  return val;
}



/*
IN
 i : number of CSP variable
 j : index of pattern to remove (must be valid)
OUT
 side effect
 pattern of CSP variable i with index j is removed from
 array
*/
void removePattern(int i, int j) {
  
  unmatchedPatterns[i][j] = unmatchedPatterns[i][unmatchedPatternsSize[i]-1];
  unmatchedPatternsSize[i]--;
}



/*
IN
 dom       : array of domain sizes for each CSP var
 cspVarNum : number of CSP variables
Side effect 
 allocates unmatchedPatterns. CSP values are assumed 
 to be encoded in binary with the most significant bit first 
 in the variable ordering. Since the bits of an assignemt should 
 be possible to decode using the usual p % 2, p >> 1 procedure
 the ordering of the bits in an encoding must be reversed
*/
void newUnmatchedPatterns(int* dom, int cspVarNum) {
  
  int i,j;
  
  unmatchedPatternsSize = malloc(cspVarNum*sizeof(int));
  unmatchedPatterns = malloc(cspVarNum*sizeof(int*));  
    
    for (i = 0; i < cspVarNum; i++) 
      {
	unmatchedPatternsSize[i] = dom[i];
	unmatchedPatterns[i] = malloc(dom[i]*sizeof(int));
	for (j = 0; j < dom[i]; j++)  
	  unmatchedPatterns[i][j] = val2pattern(j,dom[i]);      
    }  
}	
	

void deleteUnmatchedPatterns(int cspVarNum) {
  
  int i;
  free(unmatchedPatternsSize);
  for (i = 0; i < cspVarNum; i++)
    free(unmatchedPatterns[i]);
  free(unmatchedPatterns);
}
    

/* 
IN 
 s : start of CSP variable range 
 e : end of CSP variable range 
OUT
 side effect
 sets unmatchedPatterns empty betwen 
 and including s and e.

 Aggregate analysis:
 if (s == e)  
  --> O(times called from extractValues) = O(|BDD of configuration space|)

 for (i = s; i <= e; i++)
   {
     dsUnion(s,i);
     unmatchedPatternsSize[i] = 0;
   }
 --> 0(#CSPvars^2)
*/	
void mkComplete(int s, int e) {
  
  int i;

  if (s == e)
    {
      unmatchedPatternsSize[e] = 0;
      if (completeFrom == e + 1) completeFrom = e;
    }
  else if (s < e)
    {
      if (dsFind(s) != dsFind(e))
	for (i = s; i <= e; i++)
	  {
	    dsUnion(s,i);
	    unmatchedPatternsSize[i] = 0;
	  }
      if (completeFrom <= e + 1 && completeFrom > s)
	completeFrom = s;
    }
}








/***********************************************************
 *              
 * Disjoint Set implementation Kingston p. 202               
 *               
 **********************************************************/

/*

 The set of elements is assumes to be a range of integers
 [0..elemnum-1]. The elements are initialized to be singleton
 sets. The elements are stored in a vector of dsNodes. Parent
 is the index of the parent element of a node. A node with 
 parent = -1 is a root node. Notice that the arguments to Find 
 and Union are element values and not pointers to elements as
 in Kingston's implementation.

 Use:
 1) define dsNode array with dsConstruct 
 2) make each element a singleton with with dsReset 
 3) call dsFind, dsUnion on a set not larger than the 
    allocated size of dsElem
 4) free memory with dsDelete
 

*/ 

typedef struct s_dsNode {
  int parent;
  int size;
} dsNode;



/* 
 globals must be initialized by the user before accessing 
 functions. We want to avoid frequent allocation and 
 deallocation of memory
*/
dsNode* dsElem;   /* array of element nodes */ 
int dsElemSize;   /* array size */ 

  

void dsConstruct(int elemNum) {
  dsElem = malloc(elemNum*sizeof(dsNode));
  dsElemSize = elemNum;
}


void dsDelete(void) {
  dsElemSize = 0;
  free(dsElem);
}


void dsReset(void) {

  int i;
  
  for (i = 0; i < dsElemSize; i++) {
    dsElem[i].parent = -1; 
    dsElem[i].size = 1;
  }
}


int dsFind(int x) {
  
  int y,z,tmp;

  y = x;
  while ( dsElem[y].parent > -1 )
    y = dsElem[y].parent;
  
  z = x;
  while ( dsElem[z].parent > -1 ) {
    tmp = dsElem[z].parent;
    dsElem[z].parent = y;
    z = tmp;
  }

  return y;
}



void dsUnion(int x, int y) {

  int setx, sety;
 
  setx = dsFind(x);
  sety = dsFind(y);
  if (setx != sety)
    {
      if ( dsElem[setx].size <= dsElem[sety].size ) {
	dsElem[setx].parent = sety;
	dsElem[sety].size += dsElem[setx].size;
      }
      else {
	dsElem[sety].parent = setx;
	dsElem[setx].size += dsElem[sety].size;
      }
    }
}


void dsPrint(void) {
  int i;

  printf("index parent size\n");
  for(i = 0; i < dsElemSize; i++) 
    printf("%i  %i  %i\n",i,dsElem[i].parent,dsElem[i].size);
}
    
    
    
  

