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
// File  : clab.hpp
// Desc. : CLab header file.  
// Author: Rune M. Jensen, ITU
// Date  : 8/2/04
//////////////////////////////////////////////////////////////////////////

#ifndef CLABHPP
#define CLABHPP

#include <bdd.h>
#include <string>
#include <set>
#include <map>
#include<vector>
using namespace std;
const int INITBDDNODES       = 1000000;
const int INITBDDCACHE       =  100000;
const int INITBDDMAXINCREASE = 5000000;

struct CP;
struct Symbols;
struct Layout;
struct Space;
struct ValidAsnData;
struct Bval;


//////////////////////////////////////////////////////////////////////////
// Expression 
//////////////////////////////////////////////////////////////////////////


class Expr {

public:
  Expr();
  Expr(const Expr& e);
  Expr(int v);
  Expr(std::string s);
  Expr(char* s);
  Expr& operator=(const Expr& e);
  ~Expr();
  std::string write();
  friend Expr operator-(const Expr& e);     
  friend Expr operator!(const Expr& e); 
  friend Expr operator>>(const Expr& l, const Expr& r);  
  friend Expr operator||(const Expr& l, const Expr& r);
  friend Expr operator&&(const Expr& l, const Expr& r);
  friend Expr operator<=(const Expr& l, const Expr& r);  
  friend Expr operator>=(const Expr& l, const Expr& r);
  friend Expr operator<(const Expr& l, const Expr& r);
  friend Expr operator>(const Expr& l, const Expr& r);
  friend Expr operator!=(const Expr& l, const Expr& r);
  friend Expr operator==(const Expr& l, const Expr& r);
  friend Expr operator-(const Expr& l, const Expr& r);
  friend Expr operator+(const Expr& l, const Expr& r);
  friend Expr operator%(const Expr& l, const Expr& r);
  friend Expr operator/(const Expr& l, const Expr& r);
  friend Expr operator*(const Expr& l, const Expr& r);
  friend Bval expr2bddVec(Expr* e, Layout& layout, std::set<std::string>& enumVars,void (*error) (int,std::string));
 
private:
  friend class Symbols;
  enum ExprType {et_val, et_id, et_neg, et_not, et_impl, et_or, et_and, et_lte, et_gte, 
	      et_lt, et_gt, et_ne, et_eq, et_minus, et_plus, et_mod, et_div, et_times};
  ExprType type;
  int         val;
  int         ref;
  std::string id;
  Expr*       left;
  Expr*       right;
  Expr(ExprType t,const Expr& l,const Expr& r);
  Expr(ExprType t,const Expr& e);
};

  



//////////////////////////////////////////////////////////////////////////
// Configuration Problem Representation (CPR) 
//////////////////////////////////////////////////////////////////////////


enum CompileMethod {cm_static,cm_dynamic,cm_ascending};


class CPR {

public:
  CPR(std::string cpFileName);
  bdd compileRules(CompileMethod method = cm_dynamic);
  bdd compile(Expr expr);
  std::map< std::string, std::set<std::string> > validAssignments(bdd sol);
  ~CPR(); 
  std::string writeBDDencoding();
  void dump(std::string dumpFilename, bdd b);
  void setErrorFunc( void (*errorFunc) (int,std::string) );
  std::string write();
  int getBddVarNum();
  void reorder(int method);
  vector<string>  getDirectedGraph();
  string getValuEncoding();
  void outputWeight(string path);
  void dumpTop_k(std::string dumpFilename, bdd b, int top_k);
  
 
private:
  CP* cpP;
  Symbols* symbolsP;
  Layout* layoutP;
  Space* spaceP;
  ValidAsnData* vadP;
  void (*error) (int,std::string);
};


#endif  
