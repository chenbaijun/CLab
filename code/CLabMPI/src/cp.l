Number                  [0-9]+
Id                      [a-zA-Z_][0-9A-Za-z_]*'?

%{
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


/************************************************************************* 
 * File  : cp.l
 * Desc. : lex file for the Constraint Problem 
 *         declaration language 
 * Author: Rune M. Jensen, ITU 
 * Date  : 07/17/04, 12/10/11 
 ************************************************************************/

#include "y.tab.h"

int rmjlineno = 1;
int yyLastSemicolon;
int yyCurSemicolon = 0;

%}

%option caseless

%%
"//".*                  /* ignore comments, (rest of line from //) */ ;
[\t ]+                  /* ignore whitespace */ ;
\n                      { rmjlineno++; }
"type"                  { return(TYPE); }
"variable"              { return(VARIABLE); }
"rule"                  { return(RULE); }
"bool"                  { return(BOOL); }
">>"                    { return(IMPL); }
"%"                     { return('%'); }
"/"                     { return('/'); }
"&&"                    { return(AND); }
"||"                    { return(OR); }
"!"                     { return('!'); }
"=="                    { return(EQ); }
"!="                    { return(NE); }
">"                     { return('>'); }
"<"                     { return('<'); }
">="                    { return(GTE); }
"<="                    { return(LTE); }
"+"                     { return('+'); }
"-"                     { return('-'); }
"*"                     { return('*'); }
","                     { return(','); }
"."                     { return('.'); }
";"                     { yyLastSemicolon = yyCurSemicolon; yyCurSemicolon = rmjlineno; return(';'); }
"("                     { return('('); }
")"                     { return(')'); }
"{"                     { return('{'); }
"}"                     { return('}'); }
"["                     { return('['); }
"]"                     { return(']'); }
{Id}                    { return(ID); }
{Number}                { return(NUMBER); }
%%

