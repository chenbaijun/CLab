/***********************************************************
 * File: clab.h 
 * Desc: misc structures used by the clab bdd 
 *       functions in bddop.c 
 * Auth: Rune M. Jensen, ITU
 * Date: 23/04/04
 **********************************************************/

#ifndef _CLAB_H
#define _CLAB_H


/***********************************************************
 *              
 * Aux. data structures and functions for
 * 
 * void bdd_extractvalues(BDD b, int lastCSPvar)
 * int bdd_recognize(BDD b, int pattern)
 *               
 **********************************************************/


#ifdef __cplusplus
extern "C" {
#endif
  
  
  extern int*  bddVar2cspVar;         /* mapping from bdd var number to associated csp var number */
                                      /* must be allocated by the user */
  extern int   cspVarNum;             /* number of CSP variables */
  extern int   completeFrom;          /* from this CSP variable and on, any assignment is possible */
  extern int** unmatchedPatterns;     /* unmatchedPatterns[i] is an array of non-negative numbers 
                                         representing assignment patterns of CSP variable i that still
                                         has not been recognized */    
  extern int*  unmatchedPatternsSize; /* unmatchedPatternsSize[i] number of patterns left of CSP variable i */   
  extern int*  domStart;              /* domStart[i] number of the BDD variable where the encoding of 
                                         CSP variable i starts */        



  /*==== Prototypes ========================================*/
  int val2pattern(int val, int dom);
  int pattern2val(int pat, int dom);

  void newUnmatchedPatterns(int* dom, int cspVarNum);
  void deleteUnmatchedPatterns(int cspVarNum);
  void removePattern(int i, int j);
  void mkComplete(int s, int e);






#ifdef __cplusplus
}
#endif





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
 in Kingston's implementation

 Use:
 1) define dsNode array with dsConstruct 
 2) make each element a singleton with with dsReset 
 3) call dsFind, dsUnion on a set not larger than the 
    allocated size of dsElem
 4) free memory with dsDelete

*/ 


/*==== Prototypes ========================================*/

#ifdef __cplusplus
extern "C" {
#endif

void dsConstruct(int elemNum);
void dsDelete(void);
void dsReset(void);
int dsFind(int x);
void dsUnion(int x, int y);
void dsPrint(void);

#ifdef __cplusplus
}
#endif

 
#endif 
