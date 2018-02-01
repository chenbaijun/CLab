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
// File  : dump.cc
// Desc. : routines for printing a BDD representing
//         configuration space
// Author: Rune M. Jensen, ITU 
// Date  : 7/21/04
//////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <set>
#include <map>
#include <string>
#include <vector>
#include "layout.hpp"
#include "dump.hpp"
#include "common.hpp"
#include <iomanip>
using namespace std;



//////////////////////////////////////////////////////////////////////////
// aux. functions
//////////////////////////////////////////////////////////////////////////


// reads one variable assignment
// of the bdd_printset format 
int parse(FILE* setFile,int* as,int asLen,void (*error) (int,string)) {
  int varNo,value;
  char c;
  
  //check if file T or False
  c = getc(setFile);
  
  if ( c == 'F' || c == EOF )
    return 0;
  else 
    if ( c == 'T' )
      {
	// set all variables to -1 (any value)
	for (int i = 0; i < asLen; i++)
	  as[i] = -1;
	return 1;
      }
    else
      if ( c == '<' ) 
	{  
	  // there is an assignment to read

	  // set all variables to -1 (any value)
	  for (int i = 0; i < asLen; i++)
	    as[i] = -1;
	  
	  // read assignment 	  
	  while(c != '>') {
	    
	    // pre: pointer is at first digit
	    // in var. assignment
	    if (fscanf(setFile,"%d:%d",&varNo,&value) != 2)
	      error(4,"CLab internal error : dump.cc : parse : scan error\n");
	    as[varNo] = value;
	    // post: pointer is just after last difit
	    // in var. assignment 
	    c = getc(setFile);
	  }
	  return 1;
	}
}

	
  

// Aux. function for allSatDump::print 

void mkSpace(ofstream &out,int n) {
  int i;
  for (i=0;i<n;i++)
    out << " ";
}


int max(int a,int b) {
  return a > b ? a : b;
} 


int bin2intStr(int* as,vector<int>& var,char* buf) {
  
  int res = 0;
  for (int i= 0; i < var.size(); i++)
    if (as[var[i]] < 0) 
      return 0;
    else
      res += as[var[i]]*int(pow(double(2),double(i)));
  sprintf(buf,"%d",res);
  return 1;
}
  







char *rawRep(int *as,vector<int>& var,char *buf) {
 
  strcpy(buf,"");
  for (int i = var.size() - 1; i >= 0; i--)
    switch (as[var[i]]) {
    case 0 : 
      strcat(buf,"0");
      break;
    case 1 :
      strcat(buf,"1");
      break;
    default:
      strcat(buf,"*");
      break;
    }
  return buf;
}
    




void printDump(Layout& layout, string fileName, bdd b, void (*error) (int,string)) {
   
  char  buf[MAXNAMELENGTH];         
  FILE* setFile;

  // open tmp outputfile for bdd_printset
  if ( (setFile = tmpfile()) == NULL )
    error(4,"CLab internal error :dump.cc : dump ::print: cannot open tmp file\n");
  

  ofstream outFile(fileName.c_str());
  if (!outFile) error(4,"CLab internal error : dump : Cannot open file \"" + string(fileName) + "\"\n");

  
  // make the set
  bdd_fprintset(setFile,b);
 	FILE *fp;
	fp=fopen("setfile.txt","w");
bdd_fprintset(fp,b);
fclose(fp);
  fflush(setFile);
  rewind(setFile);


  
  //////////////////////////////////////////////////
  // find max string length of values for each type 
  //////////////////////////////////////////////////
  vector<int> maxTypeValStr(layout.type.size(),0);
  for (int i = 0; i < layout.type.size(); i++)
    switch (layout.type[i].type) {

    case tl_enum:
      for (int j = 0; j < layout.type[i].no2elem.size(); j++)
	if (layout.type[i].no2elem[j].length() > maxTypeValStr[i])
	  maxTypeValStr[i] = layout.type[i].no2elem[j].length();
      maxTypeValStr[i] = max(maxTypeValStr[i], layout.type[i].BDDvarNum);
      break;

    case tl_rng:
      maxTypeValStr[i] = max( max(int(ceil(log10(double(abs(layout.type[i].start))))),
				  int(ceil(log10(double(abs(layout.type[i].end)))))) + 1,
			      layout.type[i].BDDvarNum );
      break;

    case tl_bool:
      maxTypeValStr[i] = 1;
      break;

    default:
      error(4,"CLab internal error : dump.cc : Dump::print : switch case not covered\n");
      break;      
    }
      
  
  
  //////////////////////////////////////////////////
  // find max string length of values for each 
  // variable 
  //////////////////////////////////////////////////
  vector<int> maxVarValStr(layout.var.size(),0);
  for (int i = 0; i < layout.var.size(); i++)
    maxVarValStr[i] = max(maxTypeValStr[layout.var[i].typeNo],layout.var[i].varName.length());
  
  
  // add column space
  for (int i = 0; i < maxVarValStr.size(); i++)
    maxVarValStr[i] += COLSPACE;

  
  ////////////////
  // write header
  ////////////////
  
  for (int i = 0; i < layout.var.size(); i++) {
      outFile << layout.var[i].varName;
      mkSpace(outFile, maxVarValStr[i] - layout.var[i].varName.size());
  }
  outFile << "\n";

  // Init assignment table
  int asLen = layout.bddVarNum;
  int* as = new int[asLen]; 

  // go through all lines in the table
  while (parse(setFile,as,asLen,error))
    {
      
      // write value of each variable (according to type)
      for (int i = 0; i < layout.var.size(); i++)
	switch (layout.type[layout.var[i].typeNo].type) {

	case tl_enum:
	  {
	    if (bin2intStr(as,layout.var[i].BDDvar,buf))
	      {
		if ( atoi(buf) < layout.type[layout.var[i].typeNo].domSize)
		  {
		    // enum const can be written by its name
		    outFile << ( layout.type[layout.var[i].typeNo].no2elem[atoi(buf)] );
		    mkSpace(outFile, maxVarValStr[i] - layout.type[layout.var[i].typeNo].no2elem[atoi(buf)].length());
		  }
		else
		  {
		    // action doesn't exist write the number
		    outFile << buf;
		    mkSpace(outFile,maxVarValStr[i] - strlen(buf));
		  }
	      }
	    else
	      {
		// the line defines a set of actions
		// we have to write the number pattern
		outFile << rawRep(as,layout.var[i].BDDvar,buf);
		mkSpace(outFile,maxVarValStr[i] - strlen(buf));
	      }
	  }
	  break;
	  

	case tl_rng:
	  {
	    if (bin2intStr(as,layout.var[i].BDDvar,buf))
	      {
		// write the number + start of range (even if out of range)
		outFile << atoi(buf) + layout.type[layout.var[i].typeNo].start;
		sprintf(buf,"%i",atoi(buf) + layout.type[layout.var[i].typeNo].start);
		mkSpace(outFile,maxVarValStr[i] - strlen(buf));
	      }
	    else
	      {
		// the line defines a set of actions
		// we have to write the number pattern
		outFile << rawRep(as,layout.var[i].BDDvar,buf);
		mkSpace(outFile,maxVarValStr[i] - strlen(buf));
	      }
	  }
	  break;      


      

	case tl_bool:
	  {
	    if (bin2intStr(as,layout.var[i].BDDvar,buf))
	      {
		// write the bool
		outFile << buf; 
		mkSpace(outFile,maxVarValStr[i] - strlen(buf));
	      }
	    else
	      {
		// the line defines a set of actions
		// we have to write the number pattern
		outFile << rawRep(as,layout.var[i].BDDvar,buf);
		mkSpace(outFile,maxVarValStr[i] - strlen(buf));
	      }
	  }
	  break;      


	default:
	  error(4,"CLab internal error : dump.cc : Dump::print : switch case not covered\n");
	  break;      
	}
      
      // next line
      outFile << "\n";

    }  
  // close outFile
  outFile.close();     
  fclose(setFile);
}

void printDumpTop_k(Layout& layout, string fileName, bdd b, void(*error) (int, string), int top_k)
{

	char  buf[MAXNAMELENGTH];
	FILE* setFile;

	// open tmp outputfile for bdd_printset
	if ((setFile = tmpfile()) == NULL)
		error(4, "CLab internal error :dump.cc : dump ::print: cannot open tmp file\n");


	ofstream outFile(fileName.c_str());
	if (!outFile) error(4, "CLab internal error : dump : Cannot open file \"" + string(fileName) + "\"\n");


	// make the set
	int* weight = bdd_fprintset_topk(setFile, b, top_k);
	// 	FILE *fp;
	//	fp=fopen("setfile.txt","w");
	// bdd_fprintset(fp,b);
	//fclose(fp);
	fflush(setFile);
	rewind(setFile);



	//////////////////////////////////////////////////
	// find max string length of values for each type
	//////////////////////////////////////////////////
	vector<int> maxTypeValStr(layout.type.size(), 0);
	for (int i = 0; i < layout.type.size(); i++)
		switch (layout.type[i].type) {

		case tl_enum:
			for (int j = 0; j < layout.type[i].no2elem.size(); j++)
			if (layout.type[i].no2elem[j].length() > maxTypeValStr[i])
				maxTypeValStr[i] = layout.type[i].no2elem[j].length();
			maxTypeValStr[i] = max(maxTypeValStr[i], layout.type[i].BDDvarNum);
			break;

		case tl_rng:
			maxTypeValStr[i] = max(max(int(ceil(log10(double(abs(layout.type[i].start))))),
				int(ceil(log10(double(abs(layout.type[i].end)))))) + 1,
				layout.type[i].BDDvarNum);
			break;

		case tl_bool:
			maxTypeValStr[i] = 1;
			break;

		default:
			error(4, "CLab internal error : dump.cc : Dump::print : switch case not covered\n");
			break;
	}



	//////////////////////////////////////////////////
	// find max string length of values for each
	// variable
	//////////////////////////////////////////////////
	vector<int> maxVarValStr(layout.var.size(), 0);
	for (int i = 0; i < layout.var.size(); i++)
		maxVarValStr[i] = max(maxTypeValStr[layout.var[i].typeNo], layout.var[i].varName.length());


	// add column space
	for (int i = 0; i < maxVarValStr.size(); i++)
		maxVarValStr[i] += COLSPACE;


	////////////////
	// write header
	////////////////
	outFile << "No#  ";
	//cout << "No#  ";
	outFile << " weight  ";
	//cout << "weight  ";
	for (int i = 0; i < layout.var.size(); i++) {
		outFile <<" " <<layout.var[i].varName;
		//cout << layout.var[i].varName<<" ";
		mkSpace(outFile, maxVarValStr[i] - layout.var[i].varName.size());
	}
	outFile << "\n";
	//cout << "\n";

	// Init assignment table
	int asLen = layout.bddVarNum;
	int* as = new int[asLen];

	// go through all lines in the table
	int count = 0;
	while (parse(setFile, as, asLen, error))
	{
		outFile << setw(3)<< count + 1 << "   | ";
		//cout << count + 1 << "   | ";
		outFile << weight[count] << "   | ";
		//cout << weight[count] << "   | ";
		count++;

		// write value of each variable (according to type)
		for (int i = 0; i < layout.var.size(); i++)
			switch (layout.type[layout.var[i].typeNo].type) {

			case tl_enum:
			{

							if (bin2intStr(as, layout.var[i].BDDvar, buf))
							{
								if (atoi(buf) < layout.type[layout.var[i].typeNo].domSize)
								{
									// enum const can be written by its name
									outFile << (layout.type[layout.var[i].typeNo].no2elem[atoi(buf)])<<" ";
									//cout << (layout.type[layout.var[i].typeNo].no2elem[atoi(buf)]);
									mkSpace(outFile, maxVarValStr[i] - layout.type[layout.var[i].typeNo].no2elem[atoi(buf)].length());
								}
								else
								{
									// action doesn't exist write the number
									outFile << buf<<" ";
									//cout << buf;
									mkSpace(outFile, maxVarValStr[i] - strlen(buf));
								}
							}
							else
							{
								// the line defines a set of actions
								// we have to write the number pattern
								outFile << rawRep(as, layout.var[i].BDDvar, buf)<<" ";
								//cout << rawRep(as, layout.var[i].BDDvar, buf);
								mkSpace(outFile, maxVarValStr[i] - strlen(buf));
							}
			}
				break;


			case tl_rng:
			{
						   if (bin2intStr(as, layout.var[i].BDDvar, buf))
						   {
							   // write the number + start of range (even if out of range)
							   outFile << atoi(buf) + layout.type[layout.var[i].typeNo].start << " ";
							   //cout << atoi(buf) + layout.type[layout.var[i].typeNo].start;
							   sprintf(buf, "%i", atoi(buf) + layout.type[layout.var[i].typeNo].start);
							   mkSpace(outFile, maxVarValStr[i] - strlen(buf));
						   }
						   else
						   {
							   // the line defines a set of actions
							   // we have to write the number pattern
							   outFile << rawRep(as, layout.var[i].BDDvar, buf) << " ";
							   //cout << rawRep(as, layout.var[i].BDDvar, buf);
							   mkSpace(outFile, maxVarValStr[i] - strlen(buf));
						   }
			}
				break;




			case tl_bool:
			{
							if (bin2intStr(as, layout.var[i].BDDvar, buf))
							{
								// write the bool
								outFile << buf << " ";
								//cout << buf;
								mkSpace(outFile, maxVarValStr[i] - strlen(buf));
							}
							else
							{
								// the line defines a set of actions
								// we have to write the number pattern
								outFile << rawRep(as, layout.var[i].BDDvar, buf) << " ";
								//cout << rawRep(as, layout.var[i].BDDvar, buf);
								mkSpace(outFile, maxVarValStr[i] - strlen(buf));
							}
			}
				break;


			default:
				error(4, "CLab internal error : dump.cc : Dump::print : switch case not covered\n");
				break;
		}

		// next line

		outFile << "\n";
		//cout << "\n";

	}
	// close outFile
	outFile.close();
	fclose(setFile);

}







