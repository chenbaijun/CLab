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
// File  : symbols.hpp
// Desc. : CP symbol data and type checking functions
// Author: Rune M. Jensen, ITU
// Date  : 8/2/04
//////////////////////////////////////////////////////////////////////////

#include <string>
#include <map>
#include <set>

#include "cp.hpp"
#include "clab.hpp"

#ifndef SYMBOLSHPP
#define SYMBOLSHPP


using namespace std;

struct Symbols {
  // type info
  set<string> allTypes;
  set<string> enumTypes;  
  map< string, set<string> > enumTypeElems;
  set<string> allEnums;
  set<string> rngTypes;  
  
  // variable info
  set<string> allVars;
  set<string> enumVars;
  map<string,string> enumVarType; // maps enum variable name to its enum type name
  set<string> rngVars;
  set<string> boolVars;
  Symbols(CP& cp,void (*error) (int,string));
  void typeCheck(CPexpr* e, int lineStart, int lineEnd,void (*error) (int,string));
  void typeCheck(Expr* e,void (*error) (int,string));
  string write();
};
 



#endif  
