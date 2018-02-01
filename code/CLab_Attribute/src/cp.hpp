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
// File  : cp.hpp   
// Desc. : Abstract syntax representation of the Constraint 
//         Problem declaration language defined in cp.y 
// Author: Rune M. Jensen
// Date  : 07/17/04
//////////////////////////////////////////////////////////////////////////

#ifndef CPHPP
#define CPHPP

#include <map>
#include <list>
#include <string>
#include <vector>
#include "common.hpp"
using namespace std;

//////////////////////////////////////////////////////////////////////////
// CP expression 
// 
// type         struct format
// e_val        value of integer stored in 'val'
// e_id         id string stored in 'id': id can either be a variable or enum 
//              constant               
// e_neg,e_not  numerical or Boolean negated expression stored in 'left' 
// e_paren      expression in parenthesis stored in 'left'
// e_impl
// ...          left and right expression of operation stored in 'left' and 
//              'right'
// e_times 
//////////////////////////////////////////////////////////////////////////

enum CPexprType {e_val, e_id, e_neg, e_not, e_paren, 
              e_impl, e_or, e_and, e_lte, e_gte, e_lt,
              e_gt, e_ne, e_eq, e_minus, e_plus, e_mod, e_div,
              e_times};

struct CPexpr {
  CPexprType    type;
  int           val;
  string        id;
  CPexpr*       left;
  CPexpr*       right;
  CPexpr(CPexprType t, int va, string i, CPexpr* l, CPexpr* r)
    {type = t; id = i; val = va; left = l; right = r;}
  CPexpr() {}
  ~CPexpr();
  string write();
  string write2();
};


//////////////////////////////////////////////////////////////////////////
// rule declaration 
// 
//////////////////////////////////////////////////////////////////////////

struct RuleDecl {
  int      lineStart;
  int      lineEnd;
  CPexpr*  ex;
  RuleDecl(int ls, int le,CPexpr* e)
    {lineStart = ls; lineEnd = le; ex = e;}
  RuleDecl() {}
  string write();
};



//////////////////////////////////////////////////////////////////////////
// variable type declaration 
// 
// type         struct format
// vt_bool      no additional information stored 
// vt_id        user type id stored in "id"
//////////////////////////////////////////////////////////////////////////

enum VarTypeType {vt_bool,vt_id};

struct VarType {
  VarTypeType type;
  string      id;
  VarType(VarTypeType t, string i)
    {type = t; id = i;}
  VarType() {}
  string write();
};



//////////////////////////////////////////////////////////////////////////
// variable declaration
//////////////////////////////////////////////////////////////////////////

struct VarDecl {
  int          lineStart;
  int          lineEnd;
  VarType      vartype;   
  list<string> varIds;
  VarDecl(int ls, int le, VarType* vt, list<string>* vi)
    {lineStart = ls; lineEnd = le; vartype = *vt; varIds = *vi;}
  VarDecl() {}
  string write();
};


//////////////////////////////////////////////////////////////////////////
// type declaration
// 
// type         struct format
// td_rng       type id stored in 'id'. Range limits 
//              stored in "start" and "end"
// td_enum      type id stored in 'id'. Elements of 
//              enumeration stored in "elements" 
//////////////////////////////////////////////////////////////////////////

enum TypeDeclType {td_rng,td_enum};

struct TypeDecl {
  int          lineStart;
  int          lineEnd;
  TypeDeclType type;
  string       id;
  int          start;
  int          end;
  list<string> elements;
  TypeDecl(int ls, int le, TypeDeclType t, string i, int s, int e, list<string>* elems)
    {lineStart = ls; lineEnd = le; type = t; id = i; start = s; end = e; if (elems != NULL) elements = *elems;}
  TypeDecl() {}
  string write();
};




//////////////////////////////////////////////////////////////////////////
// CP
//////////////////////////////////////////////////////////////////////////

struct CP {
  vector<TypeDecl> type;
  vector<VarDecl>  var;
  vector<RuleDecl> rule;
  CP(list<TypeDecl>* t, list<VarDecl>* v, list<RuleDecl>* r);
  CP() {}
  ~CP();
  string write();
}; 




//////////////////////////////////////////////////////////////////////////
//  function prototypes 
//////////////////////////////////////////////////////////////////////////

int yyparse();


#endif
