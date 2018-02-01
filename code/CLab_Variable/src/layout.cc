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
// File: layout.cc
// Desc: Defines the order and representation of 
//       CP variables in the BDD variable set  
// Auth: Rune M. Jensen
// Date: 10/3/04
//////////////////////////////////////////////////////////////////////////

#include <vector>
#include <set>
#include <map>
#include <string>
#include <fstream>
#include <iostream>
#include "layout.hpp"
#include "common.hpp"
#include "cp.hpp"
using namespace std;



//////////////////////////////////////////////////////////////////////////
// Aux function
//////////////////////////////////////////////////////////////////////////


// ex
//  
//     nextBDDvar
//      | 
//      3 4 5 6
//      | | |     
// res  2 1 0       thus, bits are encoded in binary with enumeration from right to left
vector<int> BooleanVector(int nextBDDvar,int varNum) {
  vector<int> res;
  for (int i = varNum - 1; i >= 0 ; i--)
    res.push_back(nextBDDvar + i);
  return res;
}




//////////////////////////////////////////////////////////////////////////
// TypeLayout member functions
//////////////////////////////////////////////////////////////////////////

string TypeLayout::write() {
  
  string res;
  
  switch (type) {

    case tl_enum:
      res += "enum type: " + typeName + "\n";
      res += "BDDvarNum = " + strOf(BDDvarNum) + "\n";
      res += "domSize = " + strOf(domSize) + "\n";
      res += "elem2no: ";
      for (map<string,int>::iterator i = elem2no.begin(); i != elem2no.end(); ++i)
	res + " " + i->first + "->" + strOf(i->second);
      res += "\n";
      res += "no2elem: ";
      for (int i = 0; i < no2elem.size(); i++)
	res += " " + strOf(i) + "->" + no2elem[i];
      res += "\n";
      break;

    case tl_rng:
      res += "rng type: " + typeName + "\n";
      res += "BDDvarNum = " + strOf(BDDvarNum) + "\n";
      res += "domSize = " + strOf(domSize) + "\n";
      res += "start = " + strOf(start) + "\n";
      res += "end = " + strOf(end) + "\n";
      break;

    case tl_bool:
      res += "bool type: " + typeName + "\n";
      res += "BDDvarNum = " + strOf(BDDvarNum) + "\n";
      res += "domSize = " + strOf(domSize) + "\n";
      break;
  }
  return res;
}



string TypeLayout::writeEnc(void (*error) (int,string)) {
  
  string res;
  
  switch (type) {

    case tl_enum:
   // res += "Enum type: " + typeName + "\n";
   // res += "Domain size: " + strOf(domSize) + "\n";
   // res += "Number of BDD vars in encoding: " + strOf(BDDvarNum) + "\n";
   // res += "Encoding:";
      
      for (int i = 0; i < no2elem.size(); i++)
	res += " " + no2elem[i] + ":" + strOf(i);
      
      break;

    case tl_rng:
      res += "Range type: " + typeName + "\n";
      res += "Domain size: " + strOf(domSize) + "\n";
      res += "Number of BDD vars in encoding: " + strOf(BDDvarNum) + "\n";
      res += "Range start: " + strOf(start) + "\n";
      res += "Range end: " + strOf(end) + "\n";
      res += "Encoding: enc = val - start \n";
      break;

    case tl_bool:
      res += "Bool type: " + typeName + "\n";
      res += "Domain size: " + strOf(domSize) + "\n";
      res += "Number of BDD vars in encoding: " + strOf(BDDvarNum) + "\n";
      res += "Encoding: false 0, true 1 \n";
      break;

  default:
    error(4,"layout.cc : TypeLayout::write() : switch case not covered\n");
    break;
  }

  return res;
}





//////////////////////////////////////////////////////////////////////////
// VarLayout member functions
//////////////////////////////////////////////////////////////////////////

string VarLayout::write() {
  
  string res;
  res += "typeNo = " + strOf(typeNo) + "\n";
  res += "varName = " + varName + "\n";
  res += "BDDvar = ";
  for (int i = 0; i < BDDvar.size(); i++)
    res += " " + strOf(i) + ":" + strOf(BDDvar[i]);
  res += "\n";
  
  return res;
}

//20170307
string VarLayout::writeEnc() {
  
  string res;
  res += varName + ":[";
  for (int i = BDDvar.size() - 1; i >= 0 ; i--)
    res += "" + strOf(BDDvar[i]);
    res+="]";
  return res;
}




//////////////////////////////////////////////////////////////////////////
// layout constructor
//////////////////////////////////////////////////////////////////////////


// IN
//  cp: type checked CP representation
// OUT
//  structure holding the organisation of 
//  the BDD variables
//  Conventions:
//   BDD variables are sorted according to the order
//   CP variables appear in the variable declaration list.
//   An obvious extension is to sort variables according to
//   a file containing a variable order
Layout::Layout(CP& cp,void (*error) (int,string)) {


  // make the bool type
  type.push_back(TypeLayout(tl_bool,"bool",2));
  typeName2no["bool"] = type.size() - 1; 


  ////////////////////////////////
  // A) fill the type encoding 
  //    structure
  ////////////////////////////////
  for (int i=0; i < cp.type.size(); i++)
    switch (cp.type[i].type) {

    case td_enum:
      {
	// define encoding of elements
	map<string,int> elem2no;
	vector<string> no2elem;
	for (list<string>::iterator e = cp.type[i].elements.begin(); e != cp.type[i].elements.end(); ++e)
	  {
	    no2elem.push_back(*e);
	    elem2no[*e] = no2elem.size() - 1;
	  }
	type.push_back(TypeLayout(tl_enum,
		       cp.type[i].id,
		       no2elem.size(),
		       elem2no,no2elem));
	typeName2no[cp.type[i].id] = type.size() - 1;
      }
      break;
      
    case td_rng:
	type.push_back(TypeLayout(tl_rng, 
				  cp.type[i].id,
				  cp.type[i].end - cp.type[i].start + 1,
				  cp.type[i].start,
				  cp.type[i].end));
	typeName2no[cp.type[i].id] = type.size() - 1;	
	break;
	
      default:
	error(4,"layout.cc : layout::layout : switch case not covered\n");
	break;
    }


  ////////////////////////////////
  // B) fill the var encoding 
  //    structure (assign
  //    BDD vars to each SCP var
  ////////////////////////////////
  int nextBDDvar = 0;  // first BDD variable not part of some SCP var encoding
  for (int i = 0; i < cp.var.size(); i++)
    {
      // find type encoding no
      int tlNo;
      if (cp.var[i].vartype.type == vt_bool)
	 tlNo = typeName2no["bool"];
      else 
	 tlNo = typeName2no[cp.var[i].vartype.id];
      
      // assign BDD vars
      for (list<string>::iterator v = cp.var[i].varIds.begin(); v != cp.var[i].varIds.end(); ++v)
	{
	  var.push_back(VarLayout(tlNo,*v,BooleanVector(nextBDDvar,type[tlNo].BDDvarNum)));
	  nextBDDvar += type[tlNo].BDDvarNum;
	  varName2no[*v] = var.size() - 1;
	}
	
    }

  bddVarNum = nextBDDvar;
  
}



//////////////////////////////////////////////////////////////////////////
// layout member functions
//////////////////////////////////////////////////////////////////////////


string Layout::write() {

  string res;
  res += "bddVarNum = " + strOf(bddVarNum) + "\n";
  res += "typeName2no = ";
  for (map<string,int>::iterator i = typeName2no.begin(); i != typeName2no.end(); ++i)
    res += " " + i->first + "->" + strOf(i->second);
  res += "\n";
  res += "\n";
  for (int i=0; i < type.size(); i++) {
    res += "type[" + strOf(i) + "] =\n";
    res += type[i].write();
    res += "\n";
  }

  res += "\n";
  
  res += "varName2no = ";
  for (map<string,int>::iterator i = varName2no.begin(); i != varName2no.end(); ++i)
    res += " " + i->first + "->" + strOf(i->second);
  res += "\n";
  res += "\n";
  for (int i=0; i < var.size(); i++) {
    res += "var[" + strOf(i) + "] =\n";
    res += var[i].write();
    res += "\n";
  }
  return res;
}





string Layout::writeEnc(void (*error) (int,string)) {

  string res;

  res += "\n";
  res += "Binary encoding of types\n";
  res += "========================\n\n";
  for (int i=1; i < type.size(); i++) {
    res = res+ var[i-1].writeEnc()+"="+type[i].writeEnc(error);
    res += "\n";
  }

  res += "\n";

  res += "BDD variable encoding of CP variables\n";
  res += "=====================================\n\n";
  for (int i=0; i < var.size(); i++) {
    res += var[i].writeEnc();
    res += "\n";
  }
  res += "\n";
  res += "Total number of BDD variables in encoding: " + strOf(bddVarNum) + "\n";
  res += "\n";
  return res;
}



//////////////////////////////////////////////////////////////////////////
//
// ValidAsnData constructor
//
//////////////////////////////////////////////////////////////////////////

ValidAsnData::ValidAsnData(Layout& layout) {
  
  cpVarNum = layout.var.size();
  dom = new int[cpVarNum];
  domStart = new int[cpVarNum];

  bddVar2cpVar = new int[layout.bddVarNum];
  valExist = new int*[cpVarNum];
  
  for (int i = 0; i < cpVarNum; i++)
    {
      dom[i] = layout.type[layout.var[i].typeNo].domSize;
      valExist[i] = new int[layout.type[layout.var[i].typeNo].domSize];
      for (int j = 0; j < layout.type[layout.var[i].typeNo].domSize; j++)
	valExist[i][j] = 0;
      domStart[i] = layout.var[i].BDDvar[layout.var[i].BDDvar.size()-1];      
      for (int j = 0; j < layout.var[i].BDDvar.size(); j++)
	bddVar2cpVar[layout.var[i].BDDvar[j]] = i;
    }
}


//////////////////////////////////////////////////////////////////////////
//
// ValidAsnData destructor
//
//////////////////////////////////////////////////////////////////////////

ValidAsnData::~ValidAsnData() {
  
  delete dom;
  delete domStart;
  delete bddVar2cpVar;
  for (int i = 0; i < cpVarNum; i++)
    delete valExist[i];
  delete valExist;
}




//////////////////////////////////////////////////////////////////////////
//
// ValidAsnData member functions
//
//////////////////////////////////////////////////////////////////////////


string ValidAsnData::write(Layout& layout) {

  string res;
  res += "cpVarNum=" + strOf(cpVarNum) + "\n";
  res += "dom=";
  for (int i = 0; i < cpVarNum; i++)
    res += " " + strOf(i) + "->" + strOf(dom[i]);
  res += "\n";

  res += "domStart=";
  for (int i = 0; i < cpVarNum; i++)
    res += " " + strOf(i) + "->" + strOf(domStart[i]);
  res += "\n";

  res += "bddVar2cpVar=";
  for (int i = 0; i < layout.bddVarNum; i++)
    res += " " + strOf(i) + "->" + strOf(bddVar2cpVar[i]);
  res += "\n";
  res += "valExist=";
  for (int i = 0; i < cpVarNum; i++)
    {
      res += "\n";
      res += i + ":";
      for (int j = 0; j < dom[i]; j++)
	res += " " + strOf(j) + "->" + strOf(valExist[i][j]);
    }
  res += "\n";
  return res;
}
  
