#include<stdlib.h>
#include<string>
#include<iostream> 
#include<bdd.h>
#include<vector>
#include<fstream>
#include<time.h>
#include<omp.h>
#include<clab.hpp>
using namespace std;
bdd loadTree(char* path)
{
       bdd_init(1000, 10000);
       bdd_setvarnum(1);
       bdd tree;
       bdd_fnload(path,tree);
       return tree;
}
int main(int argc, char *argv[])  {
CPR shirt("shirt.cp");
bdd solutionSpace = shirt.compileRules(cm_dynamic);
bdd_set_weightfile_path("shirt_profile.txt");

//bdd solutionSpace=loadTree("bdd.txt");
bdd_fnsave("bdd.txt",solutionSpace);
int top_k=atoi(argv[1]);
//bdd_printset_topk(solutionSpace,top_k);
shirt.dumpTop_k("top_k.txt",solutionSpace,top_k);
return 0; 
}







