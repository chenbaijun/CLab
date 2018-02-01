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
// File: space.hpp
// Desc: BDD based representation of a configuration space
// Auth: Rune M. Jensen
// Date: 7/21/04
//////////////////////////////////////////////////////////////////////////

#ifndef SPACEHPP
#define SPACEHPP

#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <iostream>
#include <string>
#include <bdd.h>

#include "common.hpp"
#include "cp.hpp"
#include "symbols.hpp"
#include "layout.hpp"
#include "clab.hpp"
using namespace std;


// BDD representation of the configuration space of a CP expression 
struct Bval {
  vector<bdd> b; // mapping from BDD variable assignments to expression values in 2-complement format 
  bdd defCon;    // constraint ensuring sub-operations are defined (e.g., no mod or div by zero) 
  Bval(const vector<bdd>& bv, bdd dc)
  { b = bv; defCon = dc; }
  string write();
};



struct Space {
  bdd validDom;      // BDD of domain constraints of binary encoding
  Space(Layout& layout); 
  bdd compileRules(CP& cp, Symbols& symbols, Layout& layout, CompileMethod method,void (*error) (int,string));
                     // Builds a BDD of each rule (if necessary) of the configuration space.  
                     // Precondition: cp is assumed to be type checked  
                     // Conjoins the BDDs of rules in "space". ComposeOpt defines the conjoin heuristic.
                     // Precondition: the BDDs in space must first have been constructed with init
  bdd compile(Expr* e, Symbols& symbols, Layout& layout,void (*error) (int,string));
  vector<string>  getDirectedGraph(CP& cp);
  friend Bval expr2bddVec(Expr* e, Layout& layout, set<string>& enumVars,void (*error) (int,string));
};



// prototype 
vector<bdd> extend(const vector<bdd>& x, int k);
vector<bdd> truncate(const vector<bdd>& x);
pair< vector<bdd>, vector<bdd> >   mkSameSize(const vector<bdd>& x, const vector<bdd>& y);
bdd lessThan(const vector<bdd>& x, const vector<bdd>& y);
bdd equal(const vector<bdd>& x, const vector<bdd>& y);
vector<bdd> add(const vector<bdd>& x, const vector<bdd>& y);
vector<bdd> neg(const vector<bdd>& x);
vector<bdd> mkVal(int val);
bdd integer2bool(const vector<bdd>& x);
vector<bdd> bool2integer(bdd x);
vector<bdd> shiftLeft(const vector<bdd>& x, int k);
vector<bdd> abs(const vector<bdd>& x);
vector<bdd> mul(const vector<bdd>& x, const vector<bdd>& y);
vector<bdd> div(const vector<bdd>& x, const vector<bdd>& y);
vector<bdd> mkVar(const vector<int>& BDDvar);
Bval expr2bddVec(CPexpr* e,Layout& layout, set<string>& enumVars,void (*error) (int,string));
Bval expr2bddVec(Expr* e, Layout& layout, set<string>& enumVars,void (*error) (int,string));







#endif
