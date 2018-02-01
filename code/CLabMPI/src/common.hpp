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
// File  : common.hpp   
// Desc. : shared functions and constants
// Author: Rune M. Jensen
// Date  : 7/19/04, 12/10/11
//////////////////////////////////////////////////////////////////////////

#ifndef COMMONHPP
#define COMMONHPP

#include <cmath>
#include <vector>
#include <list>
#include <set>
#include <string>
using namespace std;

const int MAXNAMELENGTH = 512;
const int COLSPACE = 1;

// prototypes
int booleanVarNum(int dom);
set<string> setOf(list<string>& l);
string      strOf(int n);
string listWrite(list<string>& l);

#endif
