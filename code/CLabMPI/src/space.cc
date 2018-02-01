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
// File: space.cc
// Desc: BDD based representation of a configuration space
// Auth: Rune M. Jensen
// Date: 7/21/04
//////////////////////////////////////////////////////////////////////////

#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <iostream>
#include <string>
#include <queue>

#include "set.hpp"
#include "common.hpp"
#include "cp.hpp"
#include "symbols.hpp"
#include "layout.hpp"
#include "space.hpp"
#include "clab.hpp"
using namespace std;



//////////////////////////////////////////////////////////////////////////
// Space constructor
//////////////////////////////////////////////////////////////////////////



//IN
// cp    : CP data structure (cp is assumed to be type checked)
// layout : Definition of BDD variable mapping 
//OUT
// This constructor function builds the "space" BDD vector in the Space struct. RuleSol[i] is a BDD representing 
// the configuration space of rule i in the cp file. It is a precondition that the cp representation first has been 
// type checked. If sematically illegal expressions are given to the constructor, they will not be catched.   
Space::Space(Layout& layout) {

  validDom = bddtrue;
  // compute variable domain constraints
  for (int i = 0; i < layout.var.size(); i++)
    if (layout.type[layout.var[i].typeNo].type != tl_bool)
      {
	vector<bdd> var = mkVar(layout.var[i].BDDvar);
	vector<bdd> domSize = mkVal(layout.type[layout.var[i].typeNo].domSize);
	validDom &= lessThan(var,domSize);	
      }

}





//////////////////////////////////////////////////////////////////////////
// Space member functions
//////////////////////////////////////////////////////////////////////////



/////////// Aux. structures for method cm_dynamic ////////////////////////

// comparison operator
struct BDDcmp {
  bool operator()(const bdd a, const bdd b) {
    return (bdd_nodecount(a) < bdd_nodecount(b));
  }
};


/////////// Aux. structures for method cm_ascending //////////////////////

// priority queue node with comparison function
struct PQbdd {
  bdd b;
  int size;
  PQbdd(bdd bc) { b = bc; size = bdd_nodecount(b); }
  friend bool operator<(const PQbdd& b1, const PQbdd& b2);
};


bool operator<(const PQbdd& b1, const PQbdd& b2) {
  return(b1.size > b2.size);
}



bdd Space::compileRules(CP& cp, Symbols& symbols, Layout& layout, CompileMethod method,void (*error) (int,string)) {

  vector<bdd> sol;   // vector of BDDs representing the configuration solution space of each
                     // rule. Variable domain constraints and operator definition comstraints are 
                     // included in these expressions  

  // compute BDD for each rule 
  for (int i = 0; i < cp.rule.size(); i++) 
    {


      Bval val = expr2bddVec(cp.rule[i].ex,layout,symbols.enumVars,error);
  //    cout<<i<<"-------------------------"<<endl;
//if(cp.rule[i].ex->type==e_impl)
//{
//CPexpr* p;
//p=cp.rule[i].ex->right;

//cout<<p->write()<<endl;

//cout<<cp.rule[i].ex->left->left->left->write()<<endl;//print


//}

//cout<<cp.rule[i].ex->write()<<endl;
//cout<<i<<"-------------------------"<<endl;
//cout<<"val.b.size()="<<val.b.size()<<endl;
//bdd_printtable(val.b[0]);
//bdd_printtable(val.b[1]);
//bdd chen=integer2bool(val.b);
//bdd_printtable(chen);

      bdd expr = integer2bool(val.b) & val.defCon & validDom;
      
      if (expr == bddfalse) 
	{
	  string res;
	  res += "Rule line ";
	  if (cp.rule[i].lineStart >= cp.rule[i].lineEnd)
	    res += strOf(cp.rule[i].lineEnd);
	  else
	    res += strOf(cp.rule[i].lineStart) + " - " + strOf(cp.rule[i].lineEnd);     
	  res += ": rule is unsatisfiable\n";
	  error(0,res);
	}
      sol.push_back(expr);
    }
//cout<<"sol.size()="<<sol.size()<<endl;
//cout<<"sol[0]"<<endl;
//bdd_printtable(sol[0]);
//cout<<"sol[1]"<<endl;
//bdd_printtable(sol[1]);
  switch (method) {

    // Method cm_static
    // - compile rules as they appear in description
  case cm_static:
    {      
      bdd res = bddtrue;
      for (int i = 0; i < sol.size(); i++)	  
	res &= sol[i];
	
      return res;
    }
    break;

    // Method cm_dynamic
    // - make a dynamic priority list after bdd size of rules in "space". 
    // - continue conjoining the two top elements and inserting the result until one element is left
  case cm_dynamic:
    {      
      priority_queue<PQbdd> pq;

      for (int i=0; i < sol.size(); i++)
	pq.push(PQbdd(sol[i]));

      while (pq.size() > 1) {
	bdd a = (pq.top()).b;
	pq.pop();
	bdd b = (pq.top()).b;
	pq.pop();
	pq.push(a & b);
      }
      return((pq.top()).b);
    }
    break;


    // Method cm_ascending
    // -sort rules according to size and then conjoin in ascending order
  case cm_ascending:
    {
      sort(sol.begin(),sol.end(),BDDcmp());      
      
      bdd res = bddtrue;
      for (int i = 0; i < sol.size(); i++)
	res &= sol[i];
      
      return res;
    }
    break;

  default:
    error(4,"space.cc : Space::compileRules : case not covered\n");
    break;   
  }  
}





bdd Space::compile(Expr* e, Symbols& symbols, Layout& layout,void (*error) (int,string)) {

  symbols.typeCheck(e,error);
  Bval val = expr2bddVec(e,layout,symbols.enumVars,error); 
	  
  return integer2bool(val.b) & val.defCon & validDom;
}

vector<string>  Space::getDirectedGraph(CP& cp)
{
	vector<string> vc;

	for (int i = 0; i < cp.rule.size(); i++) 
	{
	//cout<<cp.rule[i].ex->write2()<<endl;
	vc.push_back(cp.rule[i].ex->write2());
	}
	return vc;



}








//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//
// Expression to BDD
//
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Auxiliary functions
//////////////////////////////////////////////////////////////////////////


//IN
// x   : BDD vector in 2-complement format to extend 
// k   : number of bits to add to the vector
//OUT
// the BDD vector extended with k bits
// ex. 
// 00011, k=2 -> 0000011
// 10011, k=2 -> 1110011 (recall we use 2-complement format)
// in general
// abcde, k=2 -> aaabcde
vector<bdd> extend(const vector<bdd>& x, int k) {
  
  vector<bdd> res(x.size());

  for (int i = 0; i < x.size(); i++)
    res[i] = x[i];

  bdd last = x[x.size()-1];
  for (int i = 0; i < k; i++)
    res.push_back(last);

  return res;
}



//IN
// x : BDD vector in 2-complement format to truncate
//OUT
// the BDD vector in 2-complement format with redundant bits removed
// Truncation mainly serves the purpose of keeping BDD vectors dense
// in intermediate computations to avoid unecessary computations, in 
// particularly when computing multiplcation.
//
// ex.
// 0000011 -> 011
// 1110011 -> 10011
// In general
// aaabcde -> abcde
vector<bdd> truncate(const vector<bdd>& x) {

  vector<bdd> res = x; 

  while ( (res.size() > 2) && (res[res.size()-1] == res[res.size()-2]) )
    res.pop_back();
  
  return res;
}


//IN
// x,y : two 2-complement bdd vector references 
//OUT
// extending the shortst one of the vectors
// such that the resulting vectors have the same size   
pair< vector<bdd>, vector<bdd> >   mkSameSize(const vector<bdd>& x, const vector<bdd>& y) {
  
  vector<bdd> rx = x;
  vector<bdd> ry = y;

  if (rx.size() > ry.size())
    ry = extend(ry,rx.size() - ry.size());
  else if (ry.size() > rx.size())
    rx = extend(rx,ry.size() - rx.size());
  
  pair< vector<bdd>, vector<bdd> > res(rx,ry);

  return res;
}




//////////////////////////////////////////////////////////////////////////
// Boolean and arithmetic functions
// and constructors
//////////////////////////////////////////////////////////////////////////


//IN
// x,y : 2-complement vectors 
//OUT 
// a bdd representing the truth-value of x < y
//
// algorithm
//
// (x_n & ! y_n) -> true,
//  (!x_n & y_n)  -> false, 
//    (y_n-1 & !x_n-1) -> true,
//     (!y_n-1 & x_n-1) -> false,
//      ...
//       (y_0 & !x_0) -> true, false
//
bdd lessThan(const vector<bdd>& x, const vector<bdd>& y) {
  
  pair< vector<bdd>, vector<bdd> > r = mkSameSize(x,y);
  vector<bdd>& rx = r.first;
  vector<bdd>& ry = r.second;

  bdd res = ry[0] & !rx[0];
  for (int i = 1; i < rx.size() - 1; i++) {
    res = bdd_ite(!ry[i] & rx[i],bddfalse,res);
    res = bdd_ite(ry[i] & !rx[i],bddtrue,res);
  }
  
  res = bdd_ite(!rx[rx.size() - 1] & ry[rx.size() - 1],bddfalse,res);
  res = bdd_ite(rx[rx.size() - 1] & !ry[rx.size() - 1],bddtrue,res);
  
  return res;
}


//IN
// x,y : BDD vectors in 2-complement format
//OUT
// BDD for x == y
bdd equal(const vector<bdd>& x, const vector<bdd>& y) {

  pair< vector<bdd>, vector<bdd> > r = mkSameSize(x,y);
  vector<bdd> rx = r.first;
  vector<bdd> ry = r.second;
  
  bdd res = bddtrue;
  for (int i = 0; i < rx.size(); i++)
    res &= bdd_biimp(rx[i],ry[i]);
  return res; 
}




//IN
// x,y : BDD vectors in 2-complement format
//OUT
// a BDD vector z = x + y in 2-complement format
//
// algorithm c_i : carry i 
//           z_i : bit i in result
//
// c_0 = false
//
// z_0 = x_0 xor y_0 xor c_0
// c_1 = (x_0 & y_0) | (x_0 xor y_0) & c_0
//  ...
// z_n = x_n xor y_n xor c_n
// c_n+1 = (x_n & y_n) | (x_n xor y_n) & c_n
// z_n+1 = c_n+1
//
vector<bdd> add(const vector<bdd>& x, const vector<bdd>& y) {
  
  pair< vector<bdd>, vector<bdd> > r = mkSameSize(x,y);
  vector<bdd> rx = r.first;
  vector<bdd> ry = r.second;

  bdd sameSign = bdd_biimp(rx[rx.size()-1],ry[ry.size()-1]);

  vector<bdd> z(rx.size() + 1);
  
  bdd c = bddfalse; 
  for (int i = 0; i < rx.size(); i++)
    {
      z[i] = rx[i] ^ ry[i] ^ c;
      c = (rx[i] & ry[i]) | ((rx[i] ^ ry[i]) & c);
    }

  // overflow bit should only be set if the two vectors have same sign
  // otherwise it should be discarded by extending the format
  z[rx.size()] = bdd_ite(sameSign,c,z[rx.size()-1]);

  return truncate(z);
}


//IN
// x : bdd vector in 2-complement format
//OUT
// a bdd vector containing the negated value of x
//
// Algorithm
// usual 2-complement negation algorithm
// 1) logically negate each bit
// 2) add 1 to the result
//
// 011 -> 100 + 001 -> 0-101
//  00 -> 11 + 01 -> 1-00
// 100 -> 011 + 001 -> 0-100
// 101 -> 010 + 001 -> 011
vector<bdd> neg(const vector<bdd>& x) {
  
  vector<bdd> r = x;
  for (int i = 0; i < r.size(); i++)
    r[i] = !r[i];

  return add(r,mkVal(1));
}




//IN
// val : any integer
//OUT
// 2 complement rep of val
vector<bdd> mkVal(int val) {
  
  vector<bdd> res;

  // generate |val| in the usual way
  int remainder = abs(val);
  int i = 0;
  do {
    if (remainder % 2)
      res.push_back(bddtrue);
    else
      res.push_back(bddfalse);
    remainder /= 2;
  }  while (remainder);
  res.push_back(bddfalse);

  if (val < 0)
    return neg(res);
  else
    return res;
}



//IN
// x : 2 complement vector
//OUT
// BDD for x != 0 
bdd integer2bool(const vector<bdd>& x) {
  return !equal(x,mkVal(0));
}



vector<bdd> bool2integer(bdd x) {

  vector<bdd> res;

  res.push_back(x);
  res.push_back(bddfalse);

  return res;
}


//////////////////////////////////////////////////////////////////////////
// arithmetic sub functions
//////////////////////////////////////////////////////////////////////////



vector<bdd> shiftLeft(const vector<bdd>& x, int k) {
  
  vector<bdd> res(x.size() + k);

  for (int i = 0; i < k; i++)
    res[i] = bddfalse;

  for (int i = 0; i < x.size(); i++)
    res[i+k] = x[i];

  return res;
}



//IN
// x : BDD vector in 2-complement format
//OUT
// BDD vector in 2-complement format representing 
// the absolute value of x
vector<bdd> abs(const vector<bdd>& x) {
  
  bdd xPos = lessThan(mkVal(0),x);

  vector<bdd> nx = neg(x);
  
  pair< vector<bdd>, vector<bdd> > xs = mkSameSize(x,nx);
  
  vector<bdd> res(xs.first.size());
  for (int i = 0; i < xs.first.size(); i++) 
    res[i] = bdd_ite(xPos,xs.first[i],xs.second[i]);

  return truncate(res);
}
    


//IN 
// x,y: BDD vectors in 2-complement format
//
//OUT
// BDD vector on 2-complement form of x * y
//
// Algorithm
// 1) check sign of x and y
// 2) convert x and y to absolute values
// 3) compute shiftleft values of y 
// 4) sum shift left values in result 
// 5) compute sign of result
vector<bdd> mul(const vector<bdd>& x, const vector<bdd>& y) {

  // 1)
  bdd xPos = lessThan(mkVal(0),x);
  bdd yPos = lessThan(mkVal(0),y);

  // 2)
  vector<bdd> xa = abs(x);  
  vector<bdd> ya = abs(y);

  // 3)
  vector< vector<bdd> > sl(xa.size());
  for (int i = 0; i < xa.size(); i++)
    sl[i] = shiftLeft(ya,i);

  // 4)
  vector<bdd> sum = mkVal(0);
  for (int i = 0; i < xa.size(); i++)
    {
      vector<bdd> nextSum;
      vector<bdd> incSum = add(sum,sl[i]);
      pair< vector<bdd>, vector<bdd> > p = mkSameSize(incSum,sum);
      nextSum.resize(p.first.size());
      for (int j = 0; j < p.first.size(); j++)
	nextSum[j] = bdd_ite(xa[i],p.first[j],p.second[j]);
      sum = truncate(nextSum); 
    }

  // 5)
  vector<bdd> negated = neg(sum);
  pair< vector<bdd>, vector<bdd> > p = mkSameSize(sum,negated);
  
  vector<bdd> res(p.first.size());
  bdd resPos = bdd_biimp(xPos,yPos);
  for (int i = 0; i < p.first.size(); i++)
    res[i] = bdd_ite(resPos,p.first[i],p.second[i]);
  
  return truncate(res);
}






vector<bdd> div(const vector<bdd>& x,const vector<bdd>& y) {

  // 1) Check sign
  bdd xPos = lessThan(mkVal(0),x);
  bdd yPos = lessThan(mkVal(0),y);

  vector<bdd> xa = abs(x);  
  vector<bdd> ya = abs(y);

  // 2) make sl vectors
  vector< vector<bdd> > sl(xa.size());
  for (int i = 0; i < xa.size(); i++)
    sl[i] = shiftLeft(ya,i);


  // 3) compute z vector
  vector<bdd> r = xa;
  vector<bdd> z(xa.size());
  
  for (int i = xa.size() - 1; i >= 0; i--)
    {
      vector<bdd> sub = add(r,neg(sl[i]));
      bdd subNotNeg = !lessThan(sub,mkVal(0));
      
      z[i] = subNotNeg;

      pair< vector<bdd>, vector<bdd> > p = mkSameSize(sub,r);
      r.resize(p.first.size());
      for (int j = 0; j < p.first.size(); j++)
	r[j] = bdd_ite(subNotNeg,p.first[j],p.second[j]);
      r = truncate(r);
    }

  // 4) glue sign back 
  z = truncate(z);
  vector<bdd> negated = neg(z);
  pair< vector<bdd>, vector<bdd> > p = mkSameSize(z,negated);
  
  vector<bdd> res(p.first.size());
  bdd resPos = bdd_biimp(xPos,yPos);
  for (int i = 0; i < p.first.size(); i++)
    res[i] = bdd_ite(resPos,p.first[i],p.second[i]);
  
  return truncate(res);
}


//IN
// x, y : BDD vectors in 2-complement format
//OUT
// BDD vector in 2-complement format representing x DIV y
//
// 
// algorithm
//
// r_i  : remainder for z_i, that is the remainder we get after computing bit z_i of the result
// sl_i : y shifted i left
// r_n+1 = x
//
// r is the remainder of the  computation |x| div |y|.
// if (x < 0) & (r <> 0) then
//    remainder = y - r
// else 
//    remainder = r
vector<bdd> mod(const vector<bdd>& x, const vector<bdd>& y) {

  vector<bdd> xa = abs(x);  
  vector<bdd> ya = abs(y);

  // 1) make sl vectors
  vector< vector<bdd> > sl(xa.size());
  for (int i = 0; i < xa.size(); i++)
    sl[i] = shiftLeft(ya,i);


  // 2) compute r vector (same operations as for div, just with no z vector)
  vector<bdd> r = xa;
  
  for (int i = xa.size() - 1; i >= 0; i--)
    {
      vector<bdd> sub = add(r,neg(sl[i]));
      bdd subNotNeg = !lessThan(sub,mkVal(0));
      
      pair< vector<bdd>, vector<bdd> > p = mkSameSize(sub,r);
      r.resize(p.first.size());
      for (int j = 0; j < p.first.size(); j++)
	r[j] = bdd_ite(subNotNeg,p.first[j],p.second[j]);
      r = truncate(r);
    }

  // 3) compute remainder if (x < 0) & (r <> 0)  

  bdd xNeg = lessThan(x,mkVal(0));
  bdd rNotZero = !equal(r,mkVal(0));

  // adjustedRemainder
  vector<bdd> ar = add(neg(r),ya);

  pair< vector<bdd>, vector<bdd> > p = mkSameSize(ar,r);
  
  vector<bdd> res(p.first.size());
  for (int i = 0; i < p.first.size(); i++)
    res[i] = bdd_ite(xNeg & rNotZero,p.first[i],p.second[i]);
  
  return truncate(res);
}




//////////////////////////////////////////////////////////////////////////
// expression leaf operations
//////////////////////////////////////////////////////////////////////////




//IN
// BDDvar : BDD variables of variable BDDvar[0] is the BDD representing bit[0]
//          in the binary representation of the variable
//OUT
// 2 complement representation of var 
vector<bdd> mkVar(const vector<int>& BDDvar) {
  
  vector<bdd> var(BDDvar.size());

  for (int i = 0; i < BDDvar.size(); i++)
    var[i] = bdd_ithvar(BDDvar[i]);
  var.push_back(bddfalse);

  return var;
}




//////////////////////////////////////////////////////////////////////////
//
// CP Expression to BDD 
//  main function
//
//////////////////////////////////////////////////////////////////////////


//IN
// e      : CP expression
// layout : BDD variable layout and type information
//OUT
// vector of BDDs b_n, b_n-1, ..., b_0  representing the result as a 
// binary number in dynamic size 2-complement form. That is b_n always
// denote the sign bit.  
Bval expr2bddVec(CPexpr* e, Layout& layout, set<string>& enumVars,void (*error) (int,string)) {

  switch (e->type) {


  case e_val: 
    return Bval(mkVal(e->val),bddtrue);
    break;


  case e_id: // enumeration variables and enumeration constants are caught in the eq, ne,
             // in, and notin expression. Thus, we can assume ids to be range or bool variables
    {
      VarLayout vl = layout.var[layout.varName2no[e->id]];
      TypeLayout tl = layout.type[vl.typeNo];

      switch (tl.type) {
      case tl_bool: 
	return Bval(mkVar(vl.BDDvar),bddtrue);
	break;
      
      case tl_rng: 
	{
	  vector<bdd> var = mkVar(vl.BDDvar);
	  vector<bdd> start = mkVal(tl.start);
	  return Bval(add(var,start),bddtrue);
	}
	break;
	
      default:
	error(4,"space.cc : expr2bddVec(CPexpr) : 1 : switch case not covered\n");
	break;
      }
    }
    break;

  case e_ne: 
  case e_eq: // if lval or rval is an enum type then type checker has ensured that e_ne and e_eq is 
             // a leaf (and that the corresponding rval and lval is a valid enum constant)
    {
      bool enumExpr = false;
      string var = "";
      string con = "";

      // check if lval = enum var and rval = enum constant
      if (e->left->type == e_id)
	if (setMember(enumVars,e->left->id))
	  {
	    enumExpr = true;
	    var = e->left->id;
	    con = e->right->id;
	  }

      // checking if rval = enum var and lval = enum constant
      if (e->right->type == e_id)
	if (setMember(enumVars,e->right->id))
	  {
	    enumExpr = true;
	    var = e->right->id;
	    con = e->left->id;
	  }


      // compute expression
      if (enumExpr)
	{ // 1) Base case: e_ne or e_eq are enumeration expressions 
	  VarLayout vl = layout.var[layout.varName2no[var]];
	  TypeLayout tl = layout.type[vl.typeNo];
	  
	  vector<bdd> var = mkVar(vl.BDDvar);
	  bdd res = equal(var,mkVal(tl.elem2no[con])); 
	  
	  if (e->type == e_eq)
	    return Bval(bool2integer(res),bddtrue);
	  else
	    return Bval(bool2integer(!res),bddtrue);	  
	}
      else
	{ // 2) recursive case: e_ne or e_eq are usual expressions
	  Bval left = expr2bddVec(e->left,layout,enumVars,error);
	  Bval right = expr2bddVec(e->right,layout,enumVars,error);
	  bdd res = equal(left.b,right.b); 

	  if (e->type == e_eq)
	    return Bval(bool2integer(res),left.defCon & right.defCon);
	  else
	    return Bval(bool2integer(!res),left.defCon & right.defCon);
	}            
    }

    break;


  case e_not: 
	{ 
	  Bval left = expr2bddVec(e->left,layout,enumVars,error);
	  bdd res = !integer2bool(left.b);
	  return Bval(bool2integer(res),left.defCon);
	}            
	break;
	
	


  case e_impl: 
  case e_or: 
  case e_and: 
    { 
	  Bval left = expr2bddVec(e->left,layout,enumVars,error);
	  Bval right = expr2bddVec(e->right,layout,enumVars,error);
	  bdd l = integer2bool(left.b);
	  bdd r = integer2bool(right.b);
	  bdd defCon = left.defCon & right.defCon;

	  switch (e->type) {
	    
	  case e_impl: 
	    return Bval(bool2integer(l >> r),defCon);
	    break;
	    
	  case e_or: 
	    return Bval(bool2integer(l | r),defCon);
	    break;
			
	  case e_and: 
	    return Bval(bool2integer(l & r),defCon);
	    break;
	    
	  default:
	    error(4,"space.cc : expr2bddVec(CPexpr) : 2 : switch case not covered\n");
	    break;
	  }	  
    }            
    break;
			

    
  case e_lte:   
  case e_gte: 
  case e_lt:
  case e_gt: 
    { 
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      Bval right = expr2bddVec(e->right,layout,enumVars,error);
      bdd defCon = left.defCon & right.defCon;
      
      switch (e->type) {
      case e_lte: 
	return Bval(bool2integer(!lessThan(right.b,left.b)),defCon);
	break;
	
      case e_gte: 
	return Bval(bool2integer(!lessThan(left.b,right.b)),defCon);
	break;
	
      case e_lt: 
	return Bval(bool2integer(lessThan(left.b,right.b)),defCon);
	break;
	
      case e_gt: 	
	return Bval(bool2integer(lessThan(right.b,left.b)),defCon);
	break;
		
      default:
	error(4,"space.cc : expr2bddVec(CPexpr) : 3 : switch case not covered\n");
	break;
      }	  
    }            
    break;



  case e_minus: 
  case e_plus: 
  case e_times:
    {
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      Bval right = expr2bddVec(e->right,layout,enumVars,error);
      bdd defCon = left.defCon & right.defCon;
      
      switch (e->type) {
      case e_minus: 
	{
	  vector<bdd> negated = truncate(neg(right.b));
	  return Bval(add(left.b,negated),defCon);
	}
	break;
      
      case e_plus: 
	return Bval(add(left.b,right.b),defCon);
	break;
	
      case e_times: 
	return Bval(mul(left.b,right.b),defCon);
	break;
	
      default:
	error(4,"space.cc : expr2bddVec(CPexpr) : 4 : switch case not covered\n");
	break;
      }	  
    }            
    break;
    

  case e_mod: 
  case e_div:
    {
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      Bval right = expr2bddVec(e->right,layout,enumVars,error);
      // notice defCon must ensure right side is non-zero
      vector<bdd> zero = mkVal(0);
      bdd rightNotZero = !equal(right.b,zero); 
      bdd defCon = left.defCon & right.defCon & rightNotZero;
      
      switch (e->type) {
      case e_mod: 
	return Bval(mod(left.b,right.b),defCon);
	break;
      
      case e_div: 
	return Bval(div(left.b,right.b),defCon);
	break;
		
      default:
	error(4,"space.cc : expr2bddVec(CPexpr) : 6 : switch case not covered\n");
	break;
      }	  
    }            
    break;
    
  
  case e_neg:
    {
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      return Bval(neg(left.b),left.defCon);
    }
    break;


  case e_paren:
    return expr2bddVec(e->left,layout,enumVars,error);
    break;
  }
}













//////////////////////////////////////////////////////////////////////////
//
// CP Expression to BDD 
//  main function
//
//////////////////////////////////////////////////////////////////////////


//IN
// e      : expression to build BDD for
// layout : BDD variable layout and type information
//OUT
// vector of BDDs b_n, b_n-1, ..., b_0  representing the result as a 
// binary number in dynamic size 2-complement form. That is b_n always
// denote the sign bit.  
Bval expr2bddVec(Expr* e, Layout& layout, set<string>& enumVars,void (*error) (int,string)) {

  switch (e->type) {


  case Expr::et_val: 
    return Bval(mkVal(e->val),bddtrue);
    break;


  case Expr::et_id: // enumeration variables and enumeration constants are caught in the eq, ne,
             // in, and notin expression. Thus, we can assume ids to be range or bool variables
    {
      VarLayout vl = layout.var[layout.varName2no[e->id]];
      TypeLayout tl = layout.type[vl.typeNo];

      switch (tl.type) {
      case tl_bool: 
	return Bval(mkVar(vl.BDDvar),bddtrue);
	break;
      
      case tl_rng: 
	{
	  vector<bdd> var = mkVar(vl.BDDvar);
	  vector<bdd> start = mkVal(tl.start);
	  return Bval(add(var,start),bddtrue);
	}
	break;
	
      default:
	error(4,"space.cc : expr2bddVec(Expr) : 1 : switch case not covered\n");
	break;
      }
    }
    break;

  case Expr::et_ne: 
  case Expr::et_eq: // if lval or rval is an enum type then type checker has ensured that et_ne and et_eq is 
             // a leaf (and that the corresponding rval and lval is a valid enum constant)
    {
      bool enumExpr = false;
      string var = "";
      string con = "";

      // check if lval = enum var and rval = enum constant
      if (e->left->type == Expr::et_id)
	if (setMember(enumVars,e->left->id))
	  {
	    enumExpr = true;
	    var = e->left->id;
	    con = e->right->id;
	  }

      // checking if rval = enum var and lval = enum constant
      if (e->right->type == Expr::et_id)
	if (setMember(enumVars,e->right->id))
	  {
	    enumExpr = true;
	    var = e->right->id;
	    con = e->left->id;
	  }


      // compute expression
      if (enumExpr)
	{ // 1) Base case: et_ne or et_eq are enumeration expressions 
	  VarLayout vl = layout.var[layout.varName2no[var]];
	  TypeLayout tl = layout.type[vl.typeNo];
	  
	  vector<bdd> var = mkVar(vl.BDDvar);
	  bdd res = equal(var,mkVal(tl.elem2no[con])); 
	  
	  if (e->type == Expr::et_eq)
	    return Bval(bool2integer(res),bddtrue);
	  else
	    return Bval(bool2integer(!res),bddtrue);	  
	}
      else
	{ // 2) recursive case: et_ne or et_eq are usual expressions
	  Bval left = expr2bddVec(e->left,layout,enumVars,error);
	  Bval right = expr2bddVec(e->right,layout,enumVars,error);
	  bdd res = equal(left.b,right.b); 

	  if (e->type == Expr::et_eq)
	    return Bval(bool2integer(res),left.defCon & right.defCon);
	  else
	    return Bval(bool2integer(!res),left.defCon & right.defCon);
	}            
    }

    break;


  case Expr::et_not: 
	{ 
	  Bval left = expr2bddVec(e->left,layout,enumVars,error);
	  bdd res = !integer2bool(left.b);
	  return Bval(bool2integer(res),left.defCon);
	}            
	break;
	
	


  case Expr::et_impl: 
  case Expr::et_or: 
  case Expr::et_and: 
    { 
	  Bval left = expr2bddVec(e->left,layout,enumVars,error);
	  Bval right = expr2bddVec(e->right,layout,enumVars,error);
	  bdd l = integer2bool(left.b);
	  bdd r = integer2bool(right.b);
	  bdd defCon = left.defCon & right.defCon;

	  switch (e->type) {
	    
	  case Expr::et_impl: 
	    return Bval(bool2integer(l >> r),defCon);
	    break;
	    
	  case Expr::et_or: 
	    return Bval(bool2integer(l | r),defCon);
	    break;
			
	  case Expr::et_and: 
	    return Bval(bool2integer(l & r),defCon);
	    break;
	    
	  default:
	    error(4,"space.cc : expr2bddVec(Expr) : 2 : switch case not covered\n");
	    break;
	  }	  
    }            
    break;
			

    
  case Expr::et_lte:   
  case Expr::et_gte: 
  case Expr::et_lt:
  case Expr::et_gt: 
    { 
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      Bval right = expr2bddVec(e->right,layout,enumVars,error);
      bdd defCon = left.defCon & right.defCon;
      
      switch (e->type) {
      case Expr::et_lte: 
	return Bval(bool2integer(!lessThan(right.b,left.b)),defCon);
	break;
	
      case Expr::et_gte: 
	return Bval(bool2integer(!lessThan(left.b,right.b)),defCon);
	break;
	
      case Expr::et_lt: 
	return Bval(bool2integer(lessThan(left.b,right.b)),defCon);
	break;
	
      case Expr::et_gt: 	
	return Bval(bool2integer(lessThan(right.b,left.b)),defCon);
	break;
		
      default:
	error(4,"space.cc : expr2bddVec(Expr) : 3 : switch case not covered\n");
	break;
      }	  
    }            
    break;



  case Expr::et_minus: 
  case Expr::et_plus: 
  case Expr::et_times:
    {
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      Bval right = expr2bddVec(e->right,layout,enumVars,error);
      bdd defCon = left.defCon & right.defCon;
      
      switch (e->type) {
      case Expr::et_minus: 
	{
	  vector<bdd> negated = truncate(neg(right.b));
	  return Bval(add(left.b,negated),defCon);
	}
	break;
      
      case Expr::et_plus: 
	return Bval(add(left.b,right.b),defCon);
	break;
	
      case Expr::et_times: 
	return Bval(mul(left.b,right.b),defCon);
	break;
	
      default:
	error(4,"space.cc : expr2bddVec(Expr) : 4 : switch case not covered\n");
	break;
      }	  
    }            
    break;
    

  case Expr::et_mod: 
  case Expr::et_div:
    {
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      Bval right = expr2bddVec(e->right,layout,enumVars,error);
      // notice defCon must ensure right side is non-zero
      vector<bdd> zero = mkVal(0);
      bdd rightNotZero = !equal(right.b,zero); 
      bdd defCon = left.defCon & right.defCon & rightNotZero;
      
      switch (e->type) {
      case Expr::et_mod: 
	return Bval(mod(left.b,right.b),defCon);
	break;
      
      case Expr::et_div: 
	return Bval(div(left.b,right.b),defCon);
	break;
		
      default:
	error(4,"space.cc : expr2bddVec(Expr) : 6 : switch case not covered\n");
	break;
      }	  
    }            
    break;
    
  
  case Expr::et_neg:
    {
      Bval left = expr2bddVec(e->left,layout,enumVars,error);
      return Bval(neg(left.b),left.defCon);
    }
    break;
  }
}
















//////////////////////////////////////////////////////////////////////////
//
// Bval print function
//
//////////////////////////////////////////////////////////////////////////


string Bval::write() {

  string ret;
  ret += "b = "; 

  int res = 0;
  string str = "";
  for (int i = 0; i < b.size() - 1; i++)
    if (b[i] == bddtrue)
      {
	res += int(pow(double(2),double(i)));
	str = "1" + str;
      }
    else if (b[i] == bddfalse)
      str = "0" + str;
    else
      str = "*" + str;

  // Solve sign
    if (b[b.size() - 1] == bddtrue)
      {
	res -= int(pow(double(2),double(b.size() - 1)));
	str = "-" + str;
      }
    else if (b[b.size() - 1] == bddfalse)
      str = "+" + str;
    else
      str = "*" + str;

    
    ret += str + " (" + strOf(res) + ")\n";
    ret += "defCon = ";
    if (defCon == bddtrue)
      ret += "1";
    else if (defCon == bddfalse)
      ret += "0";
    else
      ret += "*";
    ret += "\n";    	  

    return ret;
}

