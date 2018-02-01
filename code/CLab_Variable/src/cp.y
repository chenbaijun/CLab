%{
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <string>
#include <vector>
#include "common.hpp"
#include "cp.hpp"
using namespace std;

void (*clabError) (long,string);
void yyerror(char *s);
long yylex(void);
// externs
extern char *yytext;        // defined in cp.l
extern long rmjlineno;       // defined in cp.l
extern long yyLastSemicolon; // defined in cp.l
extern long yyCurSemicolon;  // defined in cp.l

// globals
CP* cpp; 
%}




%token TYPE VARIABLE RULE BOOL MOD DIV AND OR IMPL EQ NE GTE LTE ID NUMBER

%left OR
%left AND 
%left EQ NE 
%left '<' LTE '>' GTE
%left IMPL 
%left '+' '-' 
%left '/' '%' '*' 
%nonassoc '!' UMINUS 

%start cp
%%
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
    OBLIGATION TO PROVIDE MAlongENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
    MODIFICATIONS.
========================================================================*/

//////////////////////////////////////////////////////////////////////////
// File  : cp.y
// Desc. : Yacc file for the Constralong Problem 
//         declaration language.
//         Abstract Syntax described in cp.hpp
//         See also lex file cp.l
// Author: Rune M. Jensen, ITU 
// Date  : 07/17/04, 12/10/11
//////////////////////////////////////////////////////////////////////////

cp:     TYPE typedecls VARIABLE vardecls RULE ruledecls
                    { 
		      list<TypeDecl>* typeDecls = (list<TypeDecl>*) $2;
		      list<VarDecl>* varDecls = (list<VarDecl>*) $4;
		      list<RuleDecl>* ruleDecls = (list<RuleDecl>*) $6; 
		      cpp = new CP(typeDecls,varDecls,ruleDecls);
		      delete typeDecls;
		      delete varDecls;
		      delete ruleDecls;
		    }              
       | VARIABLE vardecls RULE ruledecls
                    { 
		      list<TypeDecl>* typeDecls = (list<TypeDecl>*) 0;
		      list<VarDecl>* varDecls = (list<VarDecl>*) $2;
		      list<RuleDecl>* ruleDecls = (list<RuleDecl>*) $4; 
		      cpp = new CP(typeDecls,varDecls,ruleDecls);
		      delete typeDecls;
		      delete varDecls;
		      delete ruleDecls;
		    }                 
                ;

typedecls:       
                    { $$ = (long) new list<TypeDecl>; } 
              | typedecl typedecls
	            { TypeDecl* typeDecl = (TypeDecl*) $1;
		      list<TypeDecl>* types = (list<TypeDecl>*) $2;
		      types->push_front(*typeDecl); 
		      delete typeDecl;
		      $$ = (long) types; }
                ;

typedecl:       id '[' longeger '.' '.' longeger ']' ';'
                    { TypeDecl* typeDecl = new TypeDecl(yyLastSemicolon+1,yyCurSemicolon,td_rng,(char*) $1,$3,$6,NULL);
		      $$ = (long) typeDecl; }          
              | id '{' idlst '}' ';'
                    { TypeDecl* typeDecl = new TypeDecl(yyLastSemicolon+1,yyCurSemicolon,td_enum,(char*) $1,0,0,(list<string>*) $3);
		      $$ = (long) typeDecl; }          
                ;

vardecls:       
                    { $$ = (long) new list<VarDecl>; } 
              | vardecl vardecls
	            { VarDecl* varDecl     = (VarDecl*)       $1;
		      list<VarDecl>* vars = (list<VarDecl>*) $2;
		      vars->push_front(*varDecl); 
		      delete varDecl;
		      $$ = (long) vars; }
                ;

vardecl:        vartype idlst ';'
                    { $$ = (long) new VarDecl(yyLastSemicolon+1,yyCurSemicolon,(VarType*) $1,(list<string>*) $2); }                
                ;

vartype:        BOOL 
	            { $$ = (long) new VarType(vt_bool,""); }                
              | id
	            { $$ = (long) new VarType(vt_id,(char*) $1); }                
                ;

idlst:          id
                    { list<string>* ids = new list<string>;
                      ids->push_front((char*) $1);
		      $$ = (long) ids; }          
              | id ',' idlst
                    { list<string>* ids = (list<string>*) $3;
                      ids->push_front((char*) $1);
		      $$ = (long) ids; }          
                ;

ruledecls:    
                    { list<RuleDecl>* rules = new list<RuleDecl>;
		      $$ = (long) rules; }
              | ruledecl ruledecls
		    { RuleDecl* rule = (RuleDecl*) $1; 
		      list<RuleDecl>* rules = (list<RuleDecl>*) $2;
		      rules->push_front(*rule);
		      delete rule;		      
                      $$ = (long) rules; } 
                ;

ruledecl:     exp ';'
		    { $$ = (long) new RuleDecl(yyLastSemicolon+1,yyCurSemicolon,(CPexpr*) $1); } 
                ;

exp:            number
                    { $$ = (long) new CPexpr(e_val,$1,"",NULL,NULL); }
              | id
                    { $$ = (long) new CPexpr(e_id,0,(char*) $1,NULL,NULL); }
              | '-' exp %prec UMINUS
                    { $$ = (long) new CPexpr(e_neg,0,"",(CPexpr*) $2,NULL); }
              | '!' exp
                    { $$ = (long) new CPexpr(e_not,0,"",(CPexpr*) $2,NULL); }
              | '(' exp ')'
                    { $$ = (long) new CPexpr(e_paren,0,"",(CPexpr*) $2,NULL); }
              | exp IMPL exp
                    { $$ = (long) new CPexpr(e_impl,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp OR exp
                    { $$ = (long) new CPexpr(e_or,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp AND exp
                    { $$ = (long) new CPexpr(e_and,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp LTE exp
                    { $$ = (long) new CPexpr(e_lte,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp GTE exp
                    { $$ = (long) new CPexpr(e_gte,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '<' exp
                    { $$ = (long) new CPexpr(e_lt,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '>' exp
                    { $$ = (long) new CPexpr(e_gt,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp NE exp
                    { $$ = (long) new CPexpr(e_ne,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp EQ exp
                    { $$ = (long) new CPexpr(e_eq,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '-' exp
                    { $$ = (long) new CPexpr(e_minus,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '+' exp
                    { $$ = (long) new CPexpr(e_plus,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '%' exp
                    { $$ = (long) new CPexpr(e_mod,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '/' exp
                    { $$ = (long) new CPexpr(e_div,0,"",(CPexpr*) $1,(CPexpr*) $3); }
              | exp '*' exp
                    { $$ = (long) new CPexpr(e_times,0,"",(CPexpr*) $1,(CPexpr*) $3); }
                ;

id:             ID
                    { char* s = new char[MAXNAMELENGTH];
			//keng si wo le
                     strcpy(s,yytext);
                     //  puts(s);
                      $$ = (long) s; }
                ;


longeger:        number
                    { $$ = $1; }
              | '-' number
                    { $$ = -($2); } 
                ;
                
number:         NUMBER
                    { $$ = (long) atoi(yytext);  }
                ;

%%




 
/* prototypes */



void yyerror(char *s) {
  clabError(1,string(s) + " line " + strOf(rmjlineno) + " at: " + string(yytext) + "\n");

}

