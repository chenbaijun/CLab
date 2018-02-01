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
// File  : symbols.cc
// Desc. : CP symbol data 
// Author: Rune M. Jensen, ITU
// Date  : 8/2/04
//////////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <set>

#include "symbols.hpp"
#include "set.hpp"
#include "cp.hpp"
#include "clab.hpp"

using namespace std;

//////////////////////////////////////////////////////////////////////////
// Symbols init function (constructor)
//////////////////////////////////////////////////////////////////////////


// In 
//  cp : cp internal representation
// Out
//  compiles information of declared variables simulataneously checking the format 
//  of type and rule declarations
//
//   Format rules:
//   1) types: sets must be non-empty, ranges must be non-empty,
//      type names must be unique
//   2) variables: types must be declared, variable names must be unique 
Symbols::Symbols(CP& cp,void (*error) (int,string)) {

  // fill and check type info
  for (int i=0; i < cp.type.size(); i++)
    {
      if (setMember(allTypes,cp.type[i].id)) {
	string res;
	res += "Type declaration error line ";
	if (cp.type[i].lineStart >= cp.type[i].lineEnd)
	  res += strOf(cp.type[i].lineEnd);
	else
	  res += strOf(cp.type[i].lineStart) + "-" + strOf(cp.type[i].lineEnd);
	res += ": re-declaration of type " + cp.type[i].id + "\n";
	error(2,res);
      }

      allTypes.insert(cp.type[i].id);
      switch (cp.type[i].type) {
      case td_enum:
	{
	  if (cp.type[i].elements.size() < 2) {
	    string res;
	    res += "Type declaration error line ";
	    if (cp.type[i].lineStart >= cp.type[i].lineEnd)
	      res += strOf(cp.type[i].lineEnd);
	    else
	      res += strOf(cp.type[i].lineStart) + "-" + strOf(cp.type[i].lineEnd);
	    res += ": enumration type " + cp.type[i].id 
	      + " has less than 2 elements.";
	    res += " Please encode expressions on variables of this type with constants\n";
	    error(2,res);
	  }    
	  enumTypes.insert(cp.type[i].id);
	  
	  // general check
	  set<string> enumSet;
	  for (list<string>::iterator it = cp.type[i].elements.begin(); it != cp.type[i].elements.end(); ++it) 
	    {
	      if (setMember(enumSet,*it)) {
		string res;
		res += "Enumeration element declaration error line ";
		if (cp.type[i].lineStart >= cp.type[i].lineEnd)
		  res += strOf(cp.type[i].lineEnd);
		else
		  res += strOf(cp.type[i].lineStart) + "-" + strOf(cp.type[i].lineEnd); 
		res += ": re-declaration of element " + *it + "\n";
		error(2,res);
	      }
	      enumSet.insert(*it);
	    }
	  
	  allEnums = setUnion(allEnums,enumSet);
	  enumTypeElems[cp.type[i].id] = setOf(cp.type[i].elements);
	}
	break;

      case td_rng:
	if ( (cp.type[i].end - cp.type[i].start + 1) < 2) {
	  string res;
	  res += "Type declaration error line ";
	  if (cp.type[i].lineStart >= cp.type[i].lineEnd)
	    res += strOf(cp.type[i].lineEnd);
	  else
	    res += strOf(cp.type[i].lineStart) + "-" + strOf(cp.type[i].lineEnd); 
	  res += ": range type " + cp.type[i].id + " has less than 2 elements.";
	  res += " Please encode expressions on variables of this type with constants\n";
	  error(2,res);
	}    
	rngTypes.insert(cp.type[i].id);
	break;

      default:
	error(4,"symbols.cc : typeCheck : switch case not covered\n");
	break;
      }
    }



  // fill and check var info
  for (int i=0; i < cp.var.size(); i++)
    {
      // general check
      for (list<string>::iterator it = cp.var[i].varIds.begin(); it != cp.var[i].varIds.end(); ++it) 
	{
	  if (setMember(allVars,*it)) {
	    string res;
	    res += "Variable declaration error line ";
	    if (cp.var[i].lineStart >= cp.var[i].lineEnd)
	      res += strOf(cp.var[i].lineEnd);
	    else
	      res += strOf(cp.var[i].lineStart) + "-" + strOf(cp.var[i].lineEnd); 
	    res += ": re-declaration of variable " + *it + "\n";
	    error(2,res);
	  }
	  
	  if (setMember(allEnums,*it)) {
	    string res;
	    res += "Variable declaration error line ";
	    if (cp.var[i].lineStart >= cp.var[i].lineEnd)
	      res += strOf(cp.var[i].lineEnd);
	    else
	      res += strOf(cp.var[i].lineStart) + "-" + strOf(cp.var[i].lineEnd);
	    res += ": variable name clash with enumeration element " + *it + "\n";
	    error(2,res);
	    }

	  allVars.insert(*it);
	}


      /// here
      // type specific checks
      switch (cp.var[i].vartype.type) {
      case vt_id:
	if (!setMember(allTypes,cp.var[i].vartype.id)) {
	  string res;
	  res += "Variable declaration error line ";
	  if (cp.var[i].lineStart >= cp.var[i].lineEnd)
	    res += strOf(cp.var[i].lineEnd);
	  else
	    res += strOf(cp.var[i].lineStart) + "-" + strOf(cp.var[i].lineEnd);
	  res += ": the type " + cp.var[i].vartype.id + " has not been declared\n";
	  error(2,res);
	}

	if (setMember(enumTypes,cp.var[i].vartype.id))
	  { 
	    // variables are of enumeration type
	    enumVars = setUnion(enumVars,setOf(cp.var[i].varIds));
	    //map variable names to their enumration type name
	    for (list<string>::iterator it = cp.var[i].varIds.begin(); it != cp.var[i].varIds.end(); ++it)	   
	      enumVarType[*it] = cp.var[i].vartype.id;
	  }
	else
	  // variables are of range type 
	  rngVars = setUnion(rngVars,setOf(cp.var[i].varIds));
        break;

      case vt_bool:
	  boolVars = setUnion(boolVars,setOf(cp.var[i].varIds));
	  break;
	  
      default:
	error(4,"symbols.cc : typeCheck : switch case not covered\n");
	break;
      }
    }   
}





//////////////////////////////////////////////////////////////////////////
// type check functions                         
//////////////////////////////////////////////////////////////////////////


// IN
//  e 		   : CP expression
//  lineStart      : start line of expression
//  lineEnd	   : end line of expression
// OUT 
//  side effect: terminates with error, if expression has type errors.
//  Since boolean expressions are giving a numerical result, we only need to 
//  be concerned with leaf expressions
//     a) if a variable has enumeration type it has to be in a 
//        (var = element), (var <> element) or (var in/not {subset of elements})
//     b) all other variables must be ranges or Booleans
void Symbols::typeCheck(CPexpr* e, int lineStart, int lineEnd,void (*error) (int,string)) {

  switch (e->type) {

  case e_id:
    if (!setMember(allVars,e->id) && !setMember(allEnums,e->id)   ) {
      string res;
      res += "Rule declaration error line ";
      if (lineStart >= lineEnd)
	res += strOf(lineEnd);
      else
	res += strOf(lineStart) + " - " + strOf(lineEnd); 
      res += ": variable or enumeration constant " 
	   + e->id + " not declared\n";   
      error(2,res);
    }				   

    if (setMember(enumVars,e->id)) {
      string res;
      res += "Rule declaration error line ";
      if (lineStart >= lineEnd)
	res += strOf(lineEnd);
      else
	res += strOf(lineStart) + " - " + strOf(lineEnd); 
      res += ": illegal enumeration expression for enumeration variable " 
	   + e->id + "\n";
      error(2,res);
    }

    if (setMember(allEnums,e->id)) {
      string res;
      res += "Rule declaration error line ";
      if (lineStart >= lineEnd)
	res += strOf(lineEnd);
      else
	res += strOf(lineStart) + " - " + strOf(lineEnd);      
      res += ": illegal enumeration expression for enumeration constant " 
	  + e->id + "\n";
      error(2,res);
    }
    break;


  case e_eq:
  case e_ne:
    {
      bool enumVarLeft    = false;
      bool enumVarRight   = false;
      bool enumConstLeft  = false;
      bool enumConstRight = false;
      string var = "";
      string con = "";

      // checking left side
      if (e->left->type == e_id)
	if (!setMember(allVars,e->left->id) && !setMember(allEnums,e->left->id)   ) {
	  string res;
	  res += "Rule declaration error line ";
	  if (lineStart >= lineEnd)
	    res += strOf(lineEnd);
	  else
	    res +=  strOf(lineStart) + " - " + strOf(lineEnd);      
	  res += ": left argument " + e->left->id + " of ==/!= expression is not a declared variable or enumeration constant\n"; 
	  error(2,res);
	}
	else if (setMember(enumVars,e->left->id))
	  {
	    enumVarLeft = true;
	    var = e->left->id;
	  }
	else if (setMember(allEnums,e->left->id))
	  {
	    enumConstLeft = true;
	    con = e->left->id;
	  }

      // checking right side
      if (e->right->type == e_id)
	if (!setMember(allVars,e->right->id) && !setMember(allEnums,e->right->id)   ) {
	  string res;
	  res += "Rule declaration error line ";
	  if (lineStart >= lineEnd)
	    res += strOf(lineEnd);
	  else
	    res += strOf(lineStart) + " - " + strOf(lineEnd);      
	  res += ": right argument " + e->right->id + " of ==/!= expression is not a declared variable or enumeration constant\n"; 
	  error(2,res);
	}
	else if (setMember(enumVars,e->right->id))
	  {
	    enumVarRight = true;
	    var = e->right->id;
	  }
	else if (setMember(allEnums,e->right->id))
	  {
	    enumConstRight = true;
	    con = e->right->id;
	  }


      // if either an enum variable or const on left or right side 
      // we can check without a recursive call
      if (enumVarLeft || enumVarRight || enumConstLeft || enumConstRight)
	{
	  if (enumVarLeft && enumConstRight || enumVarRight && enumConstLeft) 
	    {
	      if (!setMember(enumTypeElems[enumVarType[var]],con)) {
		string res;
		res += "Rule declaration error line ";
		if (lineStart >= lineEnd)
		  res += strOf(lineEnd);
		else
		  res += strOf(lineStart) + " - " + strOf(lineEnd);     
		res += " in some == or != expression: the enumeration constant " + con;
		res += " is not a member of " + var + "'s enumeration type " + enumVarType[var] 
		     + " = {" + setWrite(enumTypeElems[enumVarType[var]]) + "}\n";
		error(2,res);
	      }			    
	    }
	  else
	    { // error: illegal enum expression
	      string res;
	      res += "Rule declaration error line ";
	      if (lineStart >= lineEnd)
		res += strOf(lineEnd);
	      else
		res += strOf(lineStart) + " - " + strOf(lineEnd);      
	      res += " in some illegal == or != expression involving";
	      if (var != "")  res += " enumeration variable " + var;
	      if (con != "")  res += " enumeration constant " + con;
	      res += "\n";
	      error(2,res);
	    }
	}
      else
	{ 
	  // not an enum expression make recursive check
	  typeCheck(e->left,lineStart,lineEnd,error);
	  typeCheck(e->right,lineStart,lineEnd,error);
	}
    }
    break;
    
  case e_val:
    // leaf case always ok
    break;

  case e_neg:
  case e_not:
  case e_paren:
    // not a leaf case go recursively down
    typeCheck(e->left,lineStart,lineEnd,error);
    break;
   
  case e_impl:
  case e_or:
  case e_and:
  case e_lte:
  case e_gte:
  case e_lt:
  case e_gt:
  case e_minus:
  case e_plus:
  case e_mod:
  case e_div:
  case e_times:
    // not an enum expression make recursive check
    typeCheck(e->left,lineStart,lineEnd,error);
    typeCheck(e->right,lineStart,lineEnd,error);
    break;
    
  default:
    error(4,"symbols.cc : Symbols::typeCheck(CPexpr) switch case not covered\n");
    break;
  }
}      









// IN
//  e : expression to be typeChecked
// OUT 
//  side effect: terminates with error, if expression has type errors.
//  Since boolean expressions are giving a numerical result, we only need to 
//  be concerned with leaf expressions
//     a) if a variable has enumeration type it has to be in a 
//        (var = element), (var <> element) or (var in/not {subset of elements})
//     b) all other variables must be ranges or Booleans
void Symbols::typeCheck(Expr* e,void (*error) (int,string)) {

  switch (e->type) {

  case Expr::et_id:
    if (!setMember(allVars,e->id) && !setMember(allEnums,e->id)   ) {
      string res;
      res += "Variable or enumeration constant " 
	   + e->id + " not declared\n";   
      error(2,res);
    }				   

    if (setMember(enumVars,e->id)) {
      string res;
      res += "Illegal enumeration expression for enumeration variable " 
	   + e->id + "\n";
      error(2,res);
    }

    if (setMember(allEnums,e->id)) {
      string res;
      res += "Illegal enumeration expression for enumeration constant " 
	  + e->id + "\n";
      error(2,res);
    }
    break;


  case Expr::et_eq:
  case Expr::et_ne:
    {
      bool enumVarLeft    = false;
      bool enumVarRight   = false;
      bool enumConstLeft  = false;
      bool enumConstRight = false;
      string var = "";
      string con = "";

      // checking left side
      if (e->left->type == Expr::et_id)
	if (!setMember(allVars,e->left->id) && !setMember(allEnums,e->left->id)   ) {
	  string res;
	  res += "Left argument " + e->left->id + " of ==/!= expression is not a declared variable or enumeration constant\n"; 
	  error(2,res);
	}
	else if (setMember(enumVars,e->left->id))
	  {
	    enumVarLeft = true;
	    var = e->left->id;
	  }
	else if (setMember(allEnums,e->left->id))
	  {
	    enumConstLeft = true;
	    con = e->left->id;
	  }

      // checking right side
      if (e->right->type == Expr::et_id)
	if (!setMember(allVars,e->right->id) && !setMember(allEnums,e->right->id)   ) {
	  string res;
	  res += "Right argument " + e->right->id + " of ==/!= expression is not a declared variable or enumeration constant\n"; 
	  error(2,res);
	}
	else if (setMember(enumVars,e->right->id))
	  {
	    enumVarRight = true;
	    var = e->right->id;
	  }
	else if (setMember(allEnums,e->right->id))
	  {
	    enumConstRight = true;
	    con = e->right->id;
	  }


      // if either an enum variable or const on left or right side 
      // we can check without a recursive call
      if (enumVarLeft || enumVarRight || enumConstLeft || enumConstRight)
	{
	  if (enumVarLeft && enumConstRight || enumVarRight && enumConstLeft) 
	    {
	      if (!setMember(enumTypeElems[enumVarType[var]],con)) {
		string res;
		res += "In some == or != expression: the enumeration constant " + con;
		res += " is not a member of " + var + "'s enumeration type " + enumVarType[var] 
		     + " = {" + setWrite(enumTypeElems[enumVarType[var]]) + "}\n";
		error(2,res);
	      }			    
	    }
	  else
	    { // error: illegal enum esxpression
	      string res;
	      res += "Error in some == or != expression involving";
	      if (var != "")  res += " enumeration variable " + var;
	      if (con != "")  res += " enumeration constant " + con;
	      res += "\n";
	      error(2,res);
	    }
	}
      else
	{ 
	  // not an enum expression make recursive check
	  typeCheck(e->left,error);
	  typeCheck(e->right,error);
	}
    }
    break;
    
  case Expr::et_val:
    // leaf case always ok
    break;

  case Expr::et_neg:
  case Expr::et_not:
    // not a leaf case go recursively down
    typeCheck(e->left,error);
    break;
   
  case Expr::et_impl:
  case Expr::et_or:
  case Expr::et_and:
  case Expr::et_lte:
  case Expr::et_gte:
  case Expr::et_lt:
  case Expr::et_gt:
  case Expr::et_minus:
  case Expr::et_plus:
  case Expr::et_mod:
  case Expr::et_div:
  case Expr::et_times:
    // not an enum expression make recursive check
    typeCheck(e->left,error);
    typeCheck(e->right,error);
    break;
    
  default:
    error(4,"symbols.cc : Symbols::typeCheck(Expr) : switch case not covered\n");
    break;
  }
}      




string Symbols::write() {

  string res;
  res += "type info\n";
  res += "  allTypes: " + setWrite(allTypes) + "\n";
  res += "  enumTypes: " + setWrite(enumTypes) + "\n";
  res += "  enumTypeElems:\n";
  for (map< string, set<string> >::iterator it = enumTypeElems.begin(); it != enumTypeElems.end(); ++it)
    res += "   " + it->first + "->" + setWrite(it->second) + "\n";
  res += "  allEnums: " + setWrite(allEnums) + "\n";
  res += "  rngTypes: " + setWrite(rngTypes) + "\n\n";
  
  res += "variable info\n";
  res += "  allVars: " + setWrite(allVars) + "\n";
  res += "  enumVars: " + setWrite(enumVars) + "\n";
  res += "  enumVarType: maps enum variable name to its enum type name\n";
  for (map<string,string>::iterator it = enumVarType.begin(); it != enumVarType.end(); ++it)
    res += " " + it->first + "->" + it->second;
  res += "\n";
  res += "  rngVars: " + setWrite(rngVars) + "\n";
  res += "  boolVars: " + setWrite(boolVars) + "\n";
  return res;
}
