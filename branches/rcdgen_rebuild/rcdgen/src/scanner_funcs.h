/* $Id$ */

/*
 * This file is part of FreeRCT.
 * FreeRCT is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * FreeRCT is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with FreeRCT. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file scanner_funcs.h Declarations for the interface between the scanner and the parser. */

#ifndef SCANNER_FUNCS_H
#define SCANNER_FUNCS_H

extern int line; ///< Line number of the input file being scanned.

extern char *text;     ///< Temporary storage for a string.
extern int text_size;  ///< Length of #text.
extern int text_index; ///< Pointer into #text, pointing to the first free character.
void AddChar(int kar);

int yylex();   ///< Generated scanner function.
int yyparse(); ///< Generated parser function.

void yyerror(const char *message);

class Expression;
class ExpressionList;
class NameTable;
class NameRow;
class Group;
class GroupList;
class NamedValue;
class NamedValueList;

/** Structure to communicate values from the scanner to the parser. */
union YyStruct {
	int line; ///< Line number of the token.
	struct {
		int line;        ///< Line number of the token.
		long long value; ///< Number stored.
	} number;
	struct {
		int line;    ///< Line number of the token.
		char *value; ///< Characters stored.
	} chars;
	Expression *expr;         ///< %Expression to pass on.
	ExpressionList *exprlist; ///< Expression list to pass on.
	NameTable *iden_table;    ///< 2D table with identifiers to pass on.
	NameRow *iden_row;        ///< Row of identifiers to pass on.
	Group *group;             ///< %Group to pass on.
	GroupList *groups;        ///< Sequence of groups to pass on.
	NamedValue *value;        ///< A named value to pass on.
	NamedValueList *values;   ///< Sequence of named values to pass on.
};

#define YYSTYPE YyStruct

#endif
