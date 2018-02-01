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
// File  : cp.cc   
// Desc. : CP member functions
// Author: Rune M. Jensen
// Date  : 07/17/04
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <set>
#include <map>
#include <string>
#include "common.hpp"
#include "set.hpp"
#include "cp.hpp"
using namespace std;


//////////////////////////////////////////////////////////////////////////
// CPexpr destructor                            
//////////////////////////////////////////////////////////////////////////

CPexpr::~CPexpr() {
  if (left) delete left;
  if (right) delete right;
}


//////////////////////////////////////////////////////////////////////////
// CP constructor                            
//////////////////////////////////////////////////////////////////////////

CP::CP(list<TypeDecl>* t, list<VarDecl>* v, list<RuleDecl>* r) {


  if (t != NULL) // type declaration is optional
    for (list<TypeDecl>::iterator i = (*t).begin(); i != (*t).end(); ++i)
      type.push_back(*i);
  for (list<VarDecl>::iterator i = (*v).begin(); i != (*v).end(); ++i)
    var.push_back(*i);
  for (list<RuleDecl>::iterator i = (*r).begin(); i != (*r).end(); ++i)
    rule.push_back(*i);



}



//////////////////////////////////////////////////////////////////////////
// CP destructor                            
//////////////////////////////////////////////////////////////////////////

CP::~CP() {
  for (int i=0; i < rule.size(); i++)
    delete rule[i].ex;
}
  


//////////////////////////////////////////////////////////////////////////
//           print functions                         
//////////////////////////////////////////////////////////////////////////

string CPexpr::write() {
  switch (type) {
  case e_val:
    return "val";
    break;
  case e_id:
    return id;
    break;
  case e_neg:
    return "-" + left->write(); 
    break;
  case e_not:
    return "!" + left->write(); 
    break;
  case e_paren:
    return "(" + left->write() + ")";
    break;
  case e_impl:
    return left->write() + " >> " + right->write(); 
    break;
  case e_or:
    return left->write() + " || " + right->write(); 
    break;
  case e_and:
    return left->write() + " && " + right->write(); 
    break;
  case e_lte:
    return left->write() + " <= " + right->write(); 
    break;
  case e_gte:
    return left->write() + " >= " + right->write(); 
    break;
  case e_lt:
    return left->write() + " < " + right->write(); 
    break;
  case e_gt:
    return left->write() + " > " + right->write(); 
    break;
  case e_ne:
    return left->write() + " != " + right->write(); 
    break;
  case e_eq:
    return left->write() + " == " + right->write(); 
    break;
  case e_minus:
    return left->write() + " - " + right->write(); 
    break;
  case e_plus:
    return left->write() + " + " + right->write(); 
    break;
  case e_mod:
    return left->write() + " % " + right->write(); 
    break;
  case e_div:
    return left->write() + " / " + right->write(); 
    break;
  case e_times:
    return left->write() + " * " + right->write(); 
    break;
  }
}


string VarType::write() {
  switch (type) {
  case vt_bool:
    return "bool";
    break;
  case vt_id:
    return id;
    break;
  }
}

string VarDecl::write() {
  return vartype.write() + " " + listWrite(varIds) + ";";
}
    


string TypeDecl::write() {

  switch (type) {
  case td_rng:
    return id + " : [" + strOf(start) + ".." + strOf(end) + "]";    
    break;
  case td_enum:
    return id + " : {" + listWrite(elements) + "}";
    break;
  }
}


string RuleDecl::write() {
  return ex->write() + ";";
}

string CP::write() {
  string res;
  res += "type\n";
  for (int i=0; i < type.size(); i++)
      res += "  " + type[i].write() + "\n";
  res += "\n";
  res += "variable\n";
  for (int i=0; i < var.size(); i++)
      res += "  " + var[i].write() + "\n";
  res += "\n";
  res += "rule\n";
  for (int i=0; i < rule.size(); i++)
      res += "  " + rule[i].write() + "\n";
  res += "\n";
  
  return res;
}
string CPexpr::write2() {
  switch (type) {
  case e_val:
    return "val";
    break;
  case e_id:
    return id;
    break;
  case e_neg:
    return left->write2(); 
    break;
  case e_not:
    return  left->write2(); 
    break;
  case e_paren:
    return   left->write2() ;
    break;
  case e_impl:
    return left->write2() + " :: " + right->write2(); 
    break;
  case e_or:
    return left->write2()+" "+ right->write2() ; 
    break;
  case e_and:
    return left->write2()+" "+ right->write2(); 
    break;
  case e_lte:
    return left->write() ; 
    break;
  case e_gte:
    return left->write2() ; 
    break;
  case e_lt:
    return left->write2() ; 
    break;
  case e_gt:
    return left->write2() ; 
    break;
  case e_ne:
    return left->write2() ; 
    break;
  case e_eq:
    return left->write2() ; 
    break;
  case e_minus:
    return left->write2() ; 
    break;
  case e_plus:
    return left->write2() ; 
    break;
  case e_mod:
    return left->write2() ; 
    break;
  case e_div:
    return left->write2() ; 
    break;
  case e_times:
    return left->write2() ; 
    break;
  }
}
