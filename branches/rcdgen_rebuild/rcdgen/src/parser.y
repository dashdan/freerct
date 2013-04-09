/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

%{
#include "scanner_funcs.h"
#include "ast.h"
%}

%token PAR_OPEN PAR_CLOSE CURLY_OPEN CURLY_CLOSE
%token COLON PIPE SEMICOLON COMMA
%token<line> MINUS
%token<chars> STRING
%token<number> NUMBER
%token<chars> IDENTIFIER
%type<expr> Factor Expression
%type<exprlist> ExpressionList
%type<iden_table> IdentifierRows IdentifierTable
%type<iden_row> IdentifierRow
%type<group> Group
%type<groups> GroupList
%type<value> NamedValue
%type<values> NamedValueList

%start Program

%%

Program : GroupList {
	delete $1;
}
        ;

ExpressionList : Expression {
	$$ = new ExpressionList;
	$$->exprs.push_back($1);
}
               ;
ExpressionList : ExpressionList COMMA Expression {
	$$ = $1;
	$$->exprs.push_back($3);
}
               ;

Expression : Factor {
	$$ = $1;
}
           ;
Expression : MINUS Factor {
	$$ = new UnaryOperator($1, '-', $2);
}
           ;

Factor : STRING {
	$$ = new StringLiteral($1.line, $1.value);
}
       ;

Factor : NUMBER {
	$$ = new NumberLiteral($1.line, $1.value);
}
       ;

Factor : IDENTIFIER {
	$$ = new IdentifierLiteral($1.line, $1.value);
}
       ;

Factor : PAR_OPEN Expression PAR_CLOSE {
	$$ = $2;
}
       ;

NamedValueList : NamedValue {
	$$ = new NamedValueList;
	$$->values.push_back($1);
}
               ;

NamedValueList : NamedValueList NamedValue {
	$$ = $1;
	$$->values.push_back($2);
}
               ;

NamedValue : Group {
	$$ = new NamedValue(NULL, $1);
}
           ;

NamedValue : IDENTIFIER COLON Group {
	Name *name = new SingleName($1.line, $1.value);
	$$ = new NamedValue(name, $3);
}
           ;

NamedValue : IDENTIFIER COLON Expression SEMICOLON {
	Name *name = new SingleName($1.line, $1.value);
	Group *group = new ExpressionGroup($3);
	$$ = new NamedValue(name, group);
}
           ;

NamedValue : IdentifierTable COLON Group {
	$$ = new NamedValue($1, $3);
}
           ;

IdentifierTable : PAR_OPEN IdentifierRows PAR_CLOSE {
	$$ = $2;
}
                ;

IdentifierRows : IdentifierRow {
	$$ = new NameTable();
	$$->rows.push_back($1);
}
               ;

IdentifierRows : IdentifierRows PIPE IdentifierRow {
	$$ = $1;
	$$->rows.push_back($3);
}
               ;

IdentifierRow : IDENTIFIER {
	$$ = new NameRow();
	IdentifierLine *il = new IdentifierLine($1.line, $1.value);
	$$->identifiers.push_back(il);
}
              ;

IdentifierRow : IdentifierRow COMMA IDENTIFIER {
	$$ = $1;
	IdentifierLine *il = new IdentifierLine($3.line, $3.value);
	$$->identifiers.push_back(il);
}
              ;

Group : IDENTIFIER CURLY_OPEN NamedValueList CURLY_CLOSE {
	$$ = new NodeGroup($1.line, $1.value, NULL, $3);
}
      ;

Group : IDENTIFIER PAR_OPEN ExpressionList PAR_CLOSE CURLY_OPEN NamedValueList CURLY_CLOSE {
	$$ = new NodeGroup($1.line, $1.value, $3, $6);
}
      ;

GroupList : Group {
	$$ = new GroupList;
	$$->groups.push_back($1);
}
          ;

GroupList : GroupList Group {
	$$ = $1;
	$$->groups.push_back($2);
}
          ;

%%

