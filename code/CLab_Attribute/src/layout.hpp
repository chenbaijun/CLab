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
// File: layout.hpp
// Desc: Defines the order and representation of 
//       CP variables in the BDD variable set  
// Auth: Rune M. Jensen
// Date: 7/21/04
//////////////////////////////////////////////////////////////////////////

#ifndef LAYOUTHPP
#define LAYOUTHPP

#include <vector>
#include <set>
#include <map>
#include <fstream>
#include <iostream>
#include <string>
#include "common.hpp"
#include "cp.hpp"
using namespace std;


enum TypeLayoutType {tl_enum, tl_rng, tl_bool};  

//////////////////////////////////////////////////////////////////////////
//
// type, typeName and domSize are always instatiated.
//
// Conventions
// 
// Type:
// tl_enum: the numerical encoding of the elements of 
//          the enumeration is stored in elem2no and 
//          no2elem 
// 
// tl_rng:  the boundaries of the range is stored in 
//          start and end
// 
// tl_bool: no further info
//
// 
//   BDDvar[i] is the ith bit in a binary 
//   encoding of the variable value. Thus, if BDD var 0,..,4
//   encode a 4 bit number, we have  
//   BDDvar[3] = 0
//   BDDvar[2] = 1
//   BDDvar[1] = 2
//   BDDvar[0] = 3
//   That is writing BDD variable assignments from left to right will produce
//   binary numbers in ordinary format 
//////////////////////////////////////////////////////////////////////////
struct TypeLayout {
  TypeLayoutType type;
  string typeName;
  int BDDvarNum;           // number of BDD variables necessary  to encode the type
  int domSize;             // domain size of the type (all encodings are in the range [0,..,domSize-1])
  int start;               // start and end of range, if type is a range type          
  int end;
  map<string,int> elem2no; // encoding no of the elements of an enumeration type 
  vector<string> no2elem;  // 
  TypeLayout(TypeLayoutType t, string v, int d) // bool constructor
  { type = t; typeName = v; domSize = d; BDDvarNum = booleanVarNum(d); }  
  TypeLayout(TypeLayoutType t, string v, int d, int s, int e) // rng constructor  
  { type = t; typeName = v; domSize = d; BDDvarNum = booleanVarNum(d); start = s; end = e; }
  TypeLayout(TypeLayoutType t, string v, int d, map<string,int> en, vector<string>& ne) // enum constructor
  { type = t; typeName = v; domSize = d; BDDvarNum = booleanVarNum(d); elem2no = en; no2elem = ne; }
  string write();
  string writeEnc(void (*error) (int,string));
};




struct VarLayout {
  int typeNo;         // index of the layout of the variable type  
  string varName;     // name of variable
  vector<int> BDDvar; // vector of BDD variables encoding the CSP variable (see convention above)
  VarLayout(int t, string vn, vector<int> vb)
  { typeNo = t; varName = vn; BDDvar = vb; }
  string write();       
  string writeEnc();       
};




struct Layout {
  int bddVarNum;     // number of BDD variables in encoding
  map<string,int>    typeName2no;
  vector<TypeLayout> type;
  map<string,int>    varName2no;
  vector<VarLayout>  var;
  Layout(CP& cp,void (*error) (int,string));
  string write();
  string writeEnc(void (*error) (int,string));
};




//////////////////////////////////////////////////////////////////////////
// structure holding arguments
// for bdd_extractvalues
//////////////////////////////////////////////////////////////////////////

struct ValidAsnData {
  int   cpVarNum;
  int*  dom;
  int*  bddVar2cpVar;
  int*  domStart;
  int** valExist;
  ValidAsnData(Layout& layout);
  ~ValidAsnData();
  string write(Layout& layout);
};

#endif
