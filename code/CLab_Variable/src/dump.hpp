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
// File  : dump.hpp
// Desc. : routines for printing a BDD representing
//         a CLab configuration space
// Author: Rune M. Jensen, ITU
// Date  : 7/21/04
//////////////////////////////////////////////////////////////////////////

#ifndef DUMPHPP
#define DUMPHPP

#include <bdd.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include "layout.hpp"
#include "common.hpp"
using namespace std;

  
// prototype 
void printDump(Layout& layout, string filename, bdd b, void (*error) (int,string));
void printDumpTop_k(Layout& layout, string filename, bdd b, void(*error) (int, string), int top_k);
void printDumpTopkMultiCore(Layout& layout, string fileName, bdd b, void(*error) (int, string), int top_k, int threadnums, int hops);
#endif  
