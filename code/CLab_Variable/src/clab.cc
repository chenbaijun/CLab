/*========================================================================
               Copyright (C) 2004 by Rune M. Jensen
                            All rights reserved

    Permission is hereby granted, without written agreement and without
    license or royalty fees, to use, reproduce, prepare derivative
    works, distribute, and display this software and its documentation
    for NONCOMMERCIAL RESEARCH AND EDUCATIONAL PURPOSES, provided 
    that (1) the above copyright notice and the following two paragraphs 
    appear in all copies of the source code and (2) redistributions, 
    including without limitation binaries, reproduce these notices in 
    the supporting documentation. 

    IN NO EVENT SHALL RUNE M. JENSEN, OR DISTRIBUTORS OF THIS SOFTWARE 
    BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, 
    OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE 
    AND ITS DOCUMENTATION, EVEN IF THE AUTHORS OR ANY OF THE ABOVE 
    PARTIES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

    RUNE M. JENSEN SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
    BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
    FITNESS FOR A PARTICULAR PURPOSE. THE SOFTWARE PROVIDED HEREUNDER IS
    ON AN "AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO
    OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
    MODIFICATIONS.
========================================================================*/

//////////////////////////////////////////////////////////////////////////
// File  : clab.cc
// Desc. : CLab implementation
// Author: Rune M. Jensen, ITU
// Date  : 7/30/04
//////////////////////////////////////////////////////////////////////////

#include <bdd.h>
#include <string>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "common.hpp"
#include "cp.hpp"
#include "layout.hpp"
#include "space.hpp"
#include "dump.hpp"
#include "clab.hpp"

using namespace std;



//////////////////////////////////////////////////////////////////////////
// Default error function
//////////////////////////////////////////////////////////////////////////

void clabErrorDefault(int type,string errMsg) {

  switch (type) {
    
  case 0: // warning
    cout << "CLab warning: \n";
    cout << errMsg;
    break;
    
  case 1: // parse error
    cout << "CLab parse error: \n";
    cout << errMsg;
    exit(1);
    break;

  case 2: // type check error
    cout << "CLab type check error: \n";
    cout << errMsg;
    exit(1);
    break;

  case 3: // system error
    cout << "CLab system call error: \n";
    cout << errMsg;
    exit(1);
    break;

  case 4: // clab internal error
    cout << "CLab internal error: \n";
    cout << errMsg;
    exit(1);
    break;
    
  default:
    cout << "clab.cc : clabErrorDefault : switch case not covered\n";
    exit(1);
    break;
  }
}


  
//////////////////////////////////////////////////////////////////////////
// Expr constructors
//////////////////////////////////////////////////////////////////////////

enum ExprType {et_val, et_id, et_neg, et_not, et_impl, et_or, et_and, et_lte, et_gte, 
	       et_lt, et_gt, et_ne, et_eq, et_minus, et_plus, et_mod, et_div, et_times};



Expr::Expr() {
  left = NULL;
  right = NULL;
}

Expr::Expr(const Expr& e) {
  type = e.type;
  val = e.val;
  id = e.id;
  left = e.left;
  if (left) left->ref++;   
  right = e.right;
  if (right) right->ref++;
}

Expr::Expr(ExprType t,const Expr& l,const Expr& r) {
  type = t;
  left = new Expr(l);
  left->ref = 1;
  right = new Expr(r);
  right->ref = 1;
}

Expr::Expr(ExprType t,const Expr& e) {
  type = t;
  left = new Expr(e);
  left->ref = 1;
  right = NULL;
}

Expr::Expr(int v) {
    type = et_val; 
    val = v;
    left = NULL;
    right = NULL;
}

Expr::Expr(char* s) {
  type = et_id; 
  id = s;
  left = NULL;
  right = NULL;
}

Expr::Expr(string s) {
  type = et_id; 
  id = s;
  left = NULL;
  right = NULL;
}


 
//////////////////////////////////////////////////////////////////////////
// Expr destructor
//////////////////////////////////////////////////////////////////////////

Expr::~Expr() {

  if (left) 
    if (left->ref == 1)
      delete left;
    else
      left->ref--;

  if (right) 
    if (right->ref == 1)
      delete right;  
    else
      right->ref--;
}


//////////////////////////////////////////////////////////////////////////
// Expr member functions
//////////////////////////////////////////////////////////////////////////

Expr& Expr::operator=(const Expr& e) {
  
  if (this != &e) // do nothing if self-assignment
    {
      if (left) 
	if (left->ref == 1)
	  delete left;
	else
	  left->ref--;
      
      if (right) 
	if (right->ref == 1)
	  delete right;  
	else
	  right->ref--;

      type = e.type;
      val = e.val;
      id = e.id;
      left = e.left;
      if (left) left->ref++;   
      right = e.right;
      if (right) right->ref++;
    }
}


string Expr::write() {
  
  switch (type) {

  case et_val:
    return strOf(val);
    break;

  case et_id:
    return id;
    break;
    
  case et_neg:
    return "-(" + left->write() + ")";
    break;

  case et_not:
    return "!(" + left->write() + ")";
    break;

  case et_impl:
    return "(" + left->write() + ") >> (" + right->write() + ")";
    break;

  case et_or:
    return "(" + left->write() + ") || (" + right->write() + ")";
    break;

  case et_and:
    return "(" + left->write() + ") && (" + right->write() + ")";
    break;

  case et_lte:
    return "(" + left->write() + ") <= (" + right->write() + ")";
    break;

  case et_gte:
    return "(" + left->write() + ") >= (" + right->write() + ")";
    break;

  case et_lt:
    return "(" + left->write() + ") < (" + right->write() + ")";
    break;

  case et_gt:
    return "(" + left->write() + ") > (" + right->write() + ")";
    break;

  case et_ne:
    return "(" + left->write() + ") != (" + right->write() + ")";
    break;

  case et_eq:
    return "(" + left->write() + ") == (" + right->write() + ")";
    break;

  case et_minus:
    return "(" + left->write() + ") - (" + right->write() + ")";
    break;

  case et_plus:
    return "(" + left->write() + ") + (" + right->write() + ")";
    break;

  case et_mod:
    return "(" + left->write() + ") % (" + right->write() + ")";
    break;

  case et_div:
    return "(" + left->write() + ") / (" + right->write() + ")";
    break;

  case et_times:
    return "(" + left->write() + ") * (" + right->write() + ")";
    break;
  }
}



//////////////////////////////////////////////////////////////////////////
// Expr aux. functions
//////////////////////////////////////////////////////////////////////////


Expr operator-(const Expr& e) {
  return Expr(Expr::et_neg,e); 
}  

Expr operator!(const Expr& e) {
  return Expr(Expr::et_not,e); 
}  

Expr operator>>(const Expr& l, const Expr& r) {
  return Expr(Expr::et_impl,l,r);
}

Expr operator||(const Expr& l, const Expr& r) {
  return Expr(Expr::et_or,l,r);
}

Expr operator&&(const Expr& l, const Expr& r) {
  return Expr(Expr::et_and,l,r);
}
Expr operator<=(const Expr& l, const Expr& r) {
  return Expr(Expr::et_lte,l,r);
}

Expr operator>=(const Expr& l, const Expr& r) {
  return Expr(Expr::et_gte,l,r);
}

Expr operator<(const Expr& l, const Expr& r) {
  return Expr(Expr::et_gt,l,r);
}

Expr operator>(const Expr& l, const Expr& r) {
  return Expr(Expr::et_lt,l,r);
}

Expr operator!=(const Expr& l, const Expr& r) {
  return Expr(Expr::et_ne,l,r);
}

Expr operator==(const Expr& l, const Expr& r) {
  return Expr(Expr::et_eq,l,r);
}

Expr operator-(const Expr& l, const Expr& r) {
  return Expr(Expr::et_minus,l,r);
}

Expr operator+(const Expr& l, const Expr& r) {
  return Expr(Expr::et_plus,l,r);
}

Expr operator%(const Expr& l, const Expr& r) {
  return Expr(Expr::et_mod,l,r);
}

Expr operator/(const Expr& l, const Expr& r) {
  return Expr(Expr::et_div,l,r);
}

Expr operator*(const Expr& l, const Expr& r) {
  return Expr(Expr::et_times,l,r);
}












//////////////////////////////////////////////////////////////////////////
// CPR constructor
//////////////////////////////////////////////////////////////////////////

CPR::CPR(string cpFileName) {
  
  extern FILE *yyin;                     // defined cp.y
  extern CP* cpp;                        // defined cp.y
  extern void (*clabError) (int,string); // defined cp.y
  extern int yylineno;                   // defined cp.l
  extern int yyCurSemicolon;             // defined cp.l
  
  // set error function
  error = clabErrorDefault;   


  // setup yacc/lex for parsing
  yyin = fopen(cpFileName.c_str(),"r");
  if (yyin == NULL) 
    error(3,"Cannot open \"" + cpFileName + "\"\n");
  cpp = NULL;
  clabError = error;
  yylineno = 1;
  yyCurSemicolon = 0;

 
  if (yyparse()); 
  fclose(yyin);

  
  // grab internal representation from yyac
  cpP = cpp;
  
  // compile symbol information and check format
  symbolsP = new Symbols(*cpP,error);
  
  // type check each rule
  for (int i = 0; i < cpP->rule.size(); i++)
    symbolsP->typeCheck(cpP->rule[i].ex,
			cpP->rule[i].lineStart,
			cpP->rule[i].lineEnd,error);

  // make BDD layout
  layoutP = new Layout(*cpP,error);

  // build valid assignment data structure for BuDDy
  vadP = new ValidAsnData(*layoutP);

  // make sure BuDDy is running with sufficient variables
  if (bdd_isrunning())
    {
      // extend num of BDD variables if necessary
      if (bdd_varnum() < layoutP->bddVarNum) 
	bdd_setvarnum(layoutP->bddVarNum);	
    }
  else
    {
      // initialize BuDDy
      bdd_init(INITBDDNODES,INITBDDCACHE);
      bdd_setvarnum(layoutP->bddVarNum);
      bdd_setmaxincrease(INITBDDMAXINCREASE);
    }

  // init space
  spaceP = new Space(*layoutP);

}

//////////////////////////////////////////////////////////////////////////
// CPR destructor
//////////////////////////////////////////////////////////////////////////

CPR::~CPR() {
  if (cpP) delete cpP;
  if (symbolsP) delete symbolsP;
  if (layoutP) delete layoutP;
  if (spaceP) delete spaceP;
  if (vadP) delete vadP;
}


//////////////////////////////////////////////////////////////////////////
// CPR member functions
//////////////////////////////////////////////////////////////////////////


bdd CPR::compileRules(CompileMethod method) {

  return spaceP->compileRules(*cpP,*symbolsP,*layoutP,method,error);
}



bdd CPR::compile(Expr expr) {

  return spaceP->compile(&expr,*symbolsP,*layoutP,error);
}




map< string, set<string> > CPR::validAssignments(bdd sol) {

  // call specialized extract function
  bdd_extractvalues(sol, vadP->dom, vadP->domStart, vadP->cpVarNum, 
		    vadP->bddVar2cpVar, vadP->valExist);
  
  map< string, set<string> > res;
  
  for (int i=0; i < vadP->cpVarNum; i++)
    {
      string varName = layoutP->var[i].varName;
      for (int j = 0; j < vadP->dom[i]; j++)
	if (vadP->valExist[i][j]) 
	  {
	    switch (layoutP->type[layoutP->var[i].typeNo].type) {
	    case tl_enum:
	      res[varName].insert(layoutP->type[layoutP->var[i].typeNo].no2elem[j]);
	      break;
	    case tl_rng:
	      res[varName].insert(strOf(layoutP->type[layoutP->var[i].typeNo].start + j));
	      break;
	    case tl_bool:
	      res[varName].insert(strOf(j));
	      break;
	    default:
	      error(4,"CLab internal error : clab.cc : CPR::validAssignments : switch case not covered\n");
	      break;
	    }
	  }     
    }
  return res;
}


string CPR::writeBDDencoding() {

  return layoutP->writeEnc(error);
}
  


void CPR::dump(string dumpFilename, bdd b) {

  printDump(*layoutP,dumpFilename,b,error);
}


void CPR::setErrorFunc( void (*errorFunc) (int,string) ) {

  error = errorFunc;
}


string CPR::write() {

  string res;
  res += "\n";
  res += "CP file internal representation\n"; 
  res += "===============================\n"; 
  res += "\n";
  if (cpP) res += cpP->write();
  else res += "cpP is empty";
  res += "\n";
  res += "\n";
  res += "Symbols declared in CP file\n";
  res += "===========================\n"; 
  res += "\n";
  if (symbolsP) res += symbolsP->write();
  else res += "symbolsP is empty";
  res += "\n";
  res += "\n";
  res += "BDD variable layout\n";
  res += "===================\n"; 
  res += "\n";
  if (layoutP) res += layoutP->write();
  else res += "layoutP is empty";
  res += "\n";
  res += "\n";
  res += "Valid assignment data\n";
  res += "=====================\n"; 
  res += "\n";
  if (vadP) res += vadP->write(*layoutP);
  else res += "vadP is empty";
  res += "\n";
  res += "\n";

  return res;
}

int CPR::getBddVarNum() {

  return layoutP->bddVarNum;
}
void CPR::reorder(int method){
	bdd_varblockall();
	bdd_reorder(method);
}
 vector<string>  CPR::getDirectedGraph()
{

    return spaceP->getDirectedGraph(*cpP);


}
string CPR::getValuEncoding()
{
    return layoutP->writeEnc(error);
}
void CPR::outputWeight(string path)
{

    ofstream fout(path.c_str());
    int i=0;
    for(i=0;i<cpP->type.size();i++)
    {
       fout<< cpP->type[i].id;
       fout<<" {";
       int j=0;
       for(j=0;j<cpP->type[i].elements.size();j++)
       {

           if(j==cpP->type[i].elements.size()-1)
           {

                fout<<cpP->type[i].elements.size()-j;
           }
           else
           {

               fout<<cpP->type[i].elements.size()-j<<",";
           }
       }
       if(cpP->type.size()-1==i)
       {
             fout<<"}";
       }
       else
       {
            fout<<"}"<<endl;
       }
    }
    fout.close();
}


void CPR::dumpTop_k(std::string dumpFilename, bdd b, int top_k){

	printDumpTop_k(*layoutP, dumpFilename, b, error, top_k);
}

void CPR::dumpTop_kMultiCore(std::string dumpFilename, bdd b, int top_k, int threadnums, int hops){

	printDumpTopkMultiCore(*layoutP, dumpFilename, b, error, top_k, threadnums, hops);
}

