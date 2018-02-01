#include<stdlib.h>
#include <string>
#include <iostream> 
#include <clab.hpp>
#include <bdd.h>
#include <vector>
#include<fstream>
#include<time.h>
using namespace std;

int main(int argc, char *argv[])  {

  CPR shirt("shirt.cp");
  bdd solutionSpace;
  bdd_fnload("bdd.txt",solutionSpace);
  bdd  constraint = shirt.compile(Expr(argv[2]) == Expr(argv[3]));
  solutionSpace &= constraint;
  bdd_fnsave("bdd.txt",solutionSpace);
  bdd_set_weightfile_path("shirt_profile.txt");
  shirt.dumpTop_k("consistency_detect_topk_result.txt",solutionSpace,atoi(argv[1]));

  return 0; 
}







